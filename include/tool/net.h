#ifndef _TOOL_NET_H
#define _TOOL_NET_H

#include "basics.h"
#include "text.h"



//~ Networking
//
// IP addresses, looking up names, and TCP and UDP sockets. Sockets never block: sends and receives do what they can
// at once, and SocketsPoll waits, up to a timeout, until any of several sockets is ready, so one thread can serve
// many connections from its own loop. Functions that can fail return false and record why in the thread's error
// record (error.h).
//
// Addresses are parsed and printed by TOOL itself, the same everywhere. Everything else needs TOOL_SOCKETS: on the
// web, where pages have no sockets, opening one or looking up a name fails.
//
// Socket listen = 0;
// Address address = AddressLoopback(AddressFamilyIPv4, 0);
// if (TcpListen(&listen, &address))
// {
//     SocketPoll poll = { listen, SocketPollFlagsRead };
//     if (SocketsPoll(&poll, 1, 1000) != 0) // Each tick, say, with polls for the connections too
//     {
//         Socket connection;
//         while (TcpAccept(listen, &connection))
//             Serve(connection);
//     }
//     SocketClose(listen);
// }



//~ Definitions

// Room for any address as AddressFormat writes it, terminator included: "[ffff:...:ffff%4294967295]:65535"
#define TOOL_ADDRESS_TEXT_CAPACITY 64

// The most sockets one SocketsPoll can wait on, as it waits from a list on the stack
#define TOOL_SOCKET_POLL_MAX 256



namespace Tool
{
	//- Types

	enum AddressFamily
	{
		AddressFamilyNone, // No address
		AddressFamilyIPv4, // 4 bytes
		AddressFamilyIPv6, // 16 bytes
	};

	// An IP address and a port. Zeroed is none. An IPv4 address mapped into IPv6 (::ffff:1.2.3.4), as a socket taking
	// both reports one, is always held as the IPv4 address it maps.
	struct Address
	{
		AddressFamily family;
		u16 port;
		u32 scope;    // The interface an IPv6 link-local address is on. 0 for none, and for IPv4
		u8 bytes[16]; // In network order. IPv4 uses the first 4
	};

	// An open socket. 0 for none.
	typedef u64 Socket;

	enum SocketPollFlags
	{
		SocketPollFlagsRead = 1 << 0,  // Something to receive or accept, or the connection is over, which receiving tells
		SocketPollFlagsWrite = 1 << 1, // Room to send, or the connection TcpConnect began is made, or failed
	};

	// A socket for SocketsPoll: what to wait for, and what it's ready for after
	struct SocketPoll
	{
		Socket socket;
		u32 wait;  // SocketPollFlags
		u32 ready; // SocketPollFlags, set by SocketsPoll
	};

	enum UdpFlags
	{
		UdpFlagsBroadcast = 1 << 0, // Can send to broadcast addresses
	};



	//- Functions

	//~ Addresses

	Address AddressIPv4(u8 a, u8 b, u8 c, u8 d, u16 port); // a.b.c.d
	Address AddressLoopback(AddressFamily family, u16 port); // 127.0.0.1 or ::1, reachable from this machine only
	Address AddressAny(AddressFamily family, u16 port);      // 0.0.0.0 or ::, to listen on every interface

	// Reads "1.2.3.4", "1.2.3.4:80", "::1", "[::1]:80", "fe80::1%3" or "[fe80::1%3]:80", with the scope as a number.
	// Names aren't looked up; that's AddressResolve. The port is 0 when the text gives none.
	b8 AddressParse(s8 text, Address* outAddress);

	// Writes "1.2.3.4:80" or "[::1]:80", without the port when it's 0, and IPv6 in its shortest form (RFC 5952).
	// Returns the length, or 0 if it doesn't fit in 'capacity' with its terminator.
	u32 AddressFormat(const Address* address, c8* outText, u32 capacity);

	b8 AddressEqual(const Address* a, const Address* b); // Ports and scopes too
	b8 AddressIsLoopback(const Address* address);

	//~ Names

	// Looks up a host name, or reads a numeric address, waiting for the system's answer however long it takes, so
	// keep it off threads that mustn't stall. Fills up to 'capacity' addresses, each with 'port', most preferred first,
	// and returns how many. AddressFamilyNone takes either family. 0 if none were found, which is recorded.
	u32 AddressResolve(const c8* host, u16 port, Address* outAddresses, u32 capacity, AddressFamily family = AddressFamilyNone);

	//~ Sockets

	void SocketClose(Socket socket);
	b8 SocketLocalAddress(Socket socket, Address* outAddress); // Where it's bound, with the port picked for port 0

	// Waits up to 'timeout' milliseconds until any socket is ready for what it waits for, then sets every 'ready'.
	// Returns how many are ready: 0 on timeout, or on failure, which is recorded. Sockets of 0 are skipped. At most
	// TOOL_SOCKET_POLL_MAX at a time.
	u32 SocketsPoll(SocketPoll* polls, u32 count, u32 timeout);

	//~ TCP
	// A stream of bytes each way. Sends go out at once rather than wait to be gathered with later ones (no Nagle).
	// Sockets listening at :: take IPv4 connections too, where the system allows it.

	// Listens for connections at 'address'. Port 0 picks a free one. A 'backlog' of 0 is the system's default.
	b8 TcpListen(Socket* outSocket, const Address* address, u32 backlog = 0);

	// Takes a connection waiting at a listener. False when none is waiting, or on failure, which is recorded.
	b8 TcpAccept(Socket listener, Socket* outSocket, Address* outAddress = nullptr);

	// Connects to 'address', waiting up to 'timeout' milliseconds for it. With a timeout of 0 it returns as soon as
	// the connection is under way: the socket polls ready to write once it's made or has failed, and a failed one
	// fails its next send or receive.
	b8 TcpConnect(Socket* outSocket, const Address* address, u32 timeout);

	// Sends what fits now. Less than 'size' sent, maybe nothing, means the rest waits for room: poll to write. False
	// once the connection is over, which is recorded.
	b8 TcpSend(Socket socket, const void* data, u64 size, u64* outSent);

	// Receives what has arrived, up to 'capacity'. Nothing received means nothing has yet. False once the connection
	// is over: closed by the other end, or failed, which is recorded.
	b8 TcpReceive(Socket socket, void* destination, u64 capacity, u64* outReceived);

	// Stops sending: the other end receives what was sent, then the end of the stream. Receiving goes on.
	b8 TcpShutdown(Socket socket);

	//~ UDP
	// Datagrams, each arriving whole or not at all, in any order. Sockets bound at :: take IPv4 too, where the system
	// allows it.

	// Opens a socket for datagrams at 'address'. Port 0 picks a free one. 'flags' are UdpFlags.
	b8 UdpOpen(Socket* outSocket, const Address* address, u32 flags = 0);

	// Sends one datagram, whole. False if it can't go, now or at all, which is recorded.
	b8 UdpSend(Socket socket, const Address* to, const void* data, u64 size);

	// Takes one waiting datagram. False when none is waiting, or on failure, which is recorded. One longer than
	// 'capacity' is cut short, and 'outSize' is what was kept.
	b8 UdpReceive(Socket socket, void* destination, u64 capacity, u64* outSize, Address* outFrom = nullptr);

	// Receives a multicast group's datagrams too, through the system's default interface, or until left. The group's
	// family must be the socket's.
	b8 UdpJoin(Socket socket, const Address* group);
	b8 UdpLeave(Socket socket, const Address* group);
} //namespace Tool

#endif //_TOOL_NET_H
