#ifndef _TOOL_NETWORK_H
#define _TOOL_NETWORK_H

#include "basics.h"
#include "text.h"



//~ Networking
//
// IPv4/IPv6 addresses, name lookup, and non-blocking TCP and UDP sockets. Sends and receives do what they can at once;
// SocketsPoll waits on many sockets, so one thread serves many connections from its own loop. Failures return false
// and are recorded (error.h).
//
// Addresses are parsed and printed by TOOL, identically everywhere. Everything else needs TOOL_SOCKETS (not the web).
//
// Socket listener;
// Address address = AddressLoopback(AddressFamilyIPv4, 0);
// if (TcpListen(&listener, &address))
// {
//     SocketPoll poll = { listener, SocketPollFlagsRead };
//     if (SocketsPoll(&poll, 1, 1000) != 0) // Each tick, with the connections' polls too
//     {
//         Socket connection;
//         while (TcpAccept(listener, &connection))
//             Serve(connection);
//     }
//     SocketClose(listener);
// }



//~ Definitions

// Longest AddressFormat output with terminator: "[ffff:...:ffff%4294967295]:65535"
#define TOOL_ADDRESS_TEXT_CAPACITY 64

// Most sockets per SocketsPoll call
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

	// Zeroed is none. IPv4-mapped IPv6 (::ffff:1.2.3.4) is always held as IPv4.
	struct Address
	{
		AddressFamily family;
		u16 port;
		u32 scope;    // IPv6 interface index, 0 for none
		u8 bytes[16]; // Network order. IPv4 uses the first 4
	};

	// 0 for none
	typedef u64 Socket;

	enum SocketPollFlags
	{
		SocketPollFlagsRead = 1 << 0,  // Data, a connection to accept, or the connection's end
		SocketPollFlagsWrite = 1 << 1, // Room to send, or TcpConnect's connection made or failed
	};

	struct SocketPoll
	{
		Socket socket;
		u32 wait;  // SocketPollFlags
		u32 ready; // SocketPollFlags, set by SocketsPoll
	};

	enum UdpFlags
	{
		UdpFlagsBroadcast = 1 << 0, // May send to broadcast addresses
	};



	//- Functions

	//~ Addresses

	Address AddressIPv4(u8 a, u8 b, u8 c, u8 d, u16 port);
	Address AddressLoopback(AddressFamily family, u16 port); // 127.0.0.1 or ::1
	Address AddressAny(AddressFamily family, u16 port);      // 0.0.0.0 or ::

	// "1.2.3.4", "1.2.3.4:80", "::1", "[::1]:80", "fe80::1%3", "[fe80::1%3]:80". Numeric only; port 0 if absent.
	b8 AddressParse(s8 text, Address* outAddress);

	// Shortest form (RFC 5952), port omitted when 0. Returns the length, 0 if it doesn't fit.
	u32 AddressFormat(const Address* address, c8* outText, u32 capacity);

	b8 AddressEqual(const Address* a, const Address* b); // Port and scope included
	b8 AddressIsLoopback(const Address* address);

	//~ Names

	// Blocks until the system answers. Fills up to 'capacity' addresses with 'port', preferred first; returns the
	// count, 0 on failure. AddressFamilyNone takes both families.
	u32 AddressResolve(const c8* host, u16 port, Address* outAddresses, u32 capacity, AddressFamily family = AddressFamilyNone);

	//~ Sockets

	void SocketClose(Socket socket);
	b8 SocketLocalAddress(Socket socket, Address* outAddress); // Tells the port picked for port 0

	// Waits up to 'timeout' ms for any socket to be ready, then sets every 'ready'. Returns the ready count, 0 on
	// timeout or failure. Skips sockets of 0.
	u32 SocketsPoll(SocketPoll* polls, u32 count, u32 timeout);

	//~ TCP
	// No Nagle: sends go out at once. A listener at :: takes IPv4 too where the system allows.

	b8 TcpListen(Socket* outSocket, const Address* address, u32 backlog = 0); // Port 0 picks one; backlog 0 = system default
	b8 TcpAccept(Socket listener, Socket* outSocket, Address* outAddress = nullptr); // False if none waiting

	// Waits up to 'timeout' ms. With 0, returns once under way: poll for write, and a failure shows on the next
	// send or receive.
	b8 TcpConnect(Socket* outSocket, const Address* address, u32 timeout);

	// Partial sends and empty receives are normal; poll before retrying. False once the connection is over (a close by
	// the other end isn't recorded).
	b8 TcpSend(Socket socket, const void* data, u64 size, u64* outSent);
	b8 TcpReceive(Socket socket, void* destination, u64 capacity, u64* outReceived);

	b8 TcpShutdown(Socket socket); // Ends sending; the other end reads end of stream. Receiving continues

	//~ UDP
	// A socket at :: takes IPv4 too where the system allows.

	b8 UdpOpen(Socket* outSocket, const Address* address, u32 flags = 0); // Port 0 picks one. 'flags': UdpFlags
	b8 UdpSend(Socket socket, const Address* to, const void* data, u64 size); // Whole datagram or false

	// False if none waiting. A datagram longer than 'capacity' is truncated.
	b8 UdpReceive(Socket socket, void* destination, u64 capacity, u64* outSize, Address* outFrom = nullptr);

	// Multicast through the default interface. The group's family must be the socket's.
	b8 UdpJoin(Socket socket, const Address* group);
	b8 UdpLeave(Socket socket, const Address* group);
} //namespace Tool

#endif //_TOOL_NETWORK_H
