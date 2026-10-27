#include "tool.h"
#include "test.h"
#include <stdio.h>
#include <string.h>

using namespace Tool;



//~ Addresses

// Parses 'text', checks its family and port, and that it prints as 'printed'
static void CheckAddress(const c8* text, AddressFamily family, u16 port, const c8* printed)
{
	Address address;
	TOOL_ASSERT(AddressParse(S8(text), &address), "%s", text);
	TOOL_ASSERT(address.family == family, "%s", text);
	TOOL_ASSERT(address.port == port, "%s", text);

	c8 out[TOOL_ADDRESS_TEXT_CAPACITY];
	u32 length = AddressFormat(&address, out, sizeof(out));
	TOOL_ASSERT(length == strlen(printed) && strcmp(out, printed) == 0, "%s printed as %s, not %s", text, out, printed);

	// What's printed reads back as the same address
	Address again;
	TOOL_ASSERT(AddressParse(S8(out), &again) && AddressEqual(&address, &again), "%s", out);
}

static void CheckNotAddress(const c8* text)
{
	Address address;
	TOOL_ASSERT(!AddressParse(S8(text), &address), "%s", text);
	TOOL_ASSERT(address.family == AddressFamilyNone, "%s", text);
}

static void TestAddresses()
{
	CheckAddress("1.2.3.4", AddressFamilyIPv4, 0, "1.2.3.4");
	CheckAddress("0.0.0.0", AddressFamilyIPv4, 0, "0.0.0.0");
	CheckAddress("255.255.255.255:65535", AddressFamilyIPv4, 65535, "255.255.255.255:65535");
	CheckAddress("127.0.0.1:080", AddressFamilyIPv4, 80, "127.0.0.1:80");

	CheckNotAddress("");
	CheckNotAddress("1.2.3");
	CheckNotAddress("1.2.3.4.5");
	CheckNotAddress("256.1.1.1");
	CheckNotAddress("01.2.3.4");
	CheckNotAddress("1..2.3");
	CheckNotAddress("1.2.3.4:");
	CheckNotAddress("1.2.3.4:65536");
	CheckNotAddress("1.2.3.4:-1");
	CheckNotAddress(":80");
	CheckNotAddress("a.b.c.d");
	CheckNotAddress("localhost");

	// Shortest forms (RFC 5952): lowercase, no leading zeros, the longest run of zeros as "::", the first of equals
	CheckAddress("::", AddressFamilyIPv6, 0, "::");
	CheckAddress("::1", AddressFamilyIPv6, 0, "::1");
	CheckAddress("1::", AddressFamilyIPv6, 0, "1::");
	CheckAddress("[::1]:80", AddressFamilyIPv6, 80, "[::1]:80");
	CheckAddress("2001:0DB8:0000:0000:0000:0000:0000:0001", AddressFamilyIPv6, 0, "2001:db8::1");
	CheckAddress("2001:db8:0:0:1:0:0:1", AddressFamilyIPv6, 0, "2001:db8::1:0:0:1");
	CheckAddress("2001:db8:0:0:1:0:0:0", AddressFamilyIPv6, 0, "2001:db8:0:0:1::");
	CheckAddress("2001:db8:0:1:1:1:1:1", AddressFamilyIPv6, 0, "2001:db8:0:1:1:1:1:1");
	CheckAddress("1:2:3:4:5:6:7:8", AddressFamilyIPv6, 0, "1:2:3:4:5:6:7:8");
	CheckAddress("fe80::1%3", AddressFamilyIPv6, 0, "fe80::1%3");
	CheckAddress("[fe80::1%3]:443", AddressFamilyIPv6, 443, "[fe80::1%3]:443");
	CheckAddress("64:ff9b::1.2.3.4", AddressFamilyIPv6, 0, "64:ff9b::102:304");

	// IPv4 mapped into IPv6 is held as IPv4
	CheckAddress("::ffff:1.2.3.4", AddressFamilyIPv4, 0, "1.2.3.4");
	CheckAddress("[::ffff:102:304]:80", AddressFamilyIPv4, 80, "1.2.3.4:80");

	CheckNotAddress(":::");
	CheckNotAddress(":1");
	CheckNotAddress("1:");
	CheckNotAddress("1:::2");
	CheckNotAddress("1::2::3");
	CheckNotAddress("1:2:3:4:5:6:7");
	CheckNotAddress("1:2:3:4:5:6:7:8:9");
	CheckNotAddress("1:2:3:4:5:6:7::8");
	CheckNotAddress("12345::");
	CheckNotAddress("g::1");
	CheckNotAddress("::1%");
	CheckNotAddress("::1%x");
	CheckNotAddress("[::1");
	CheckNotAddress("[::1]80");
	CheckNotAddress("[::1]:");
	CheckNotAddress("[1.2.3.4]:80");
	CheckNotAddress("::1.2.3");
	CheckNotAddress("1.2.3.4::");

	// Every pattern of zero and non-zero groups prints, and reads back, as itself
	for (u32 mask = 0; mask < 256; mask++)
	{
		Address address = {};
		address.family = AddressFamilyIPv6;
		address.port = (u16)(mask * 251);
		for (u32 group = 0; group < 8; group++)
			address.bytes[group * 2 + 1] = (mask >> group & 1) ? (u8)(group + 1) : 0;

		Address parsed;
		c8 text[TOOL_ADDRESS_TEXT_CAPACITY];
		TOOL_ASSERT(AddressFormat(&address, text, sizeof(text)) != 0);
		TOOL_ASSERT(AddressParse(S8(text), &parsed) && AddressEqual(&address, &parsed), "%s", text);
	}

	// The longest text there is fits the capacity, and a buffer too small takes nothing
	Address longest = {};
	longest.family = AddressFamilyIPv6;
	longest.port = 65535;
	longest.scope = U32_MAX;
	memset(longest.bytes, 0xFF, sizeof(longest.bytes));
	c8 text[TOOL_ADDRESS_TEXT_CAPACITY];
	u32 length = AddressFormat(&longest, text, sizeof(text));
	TOOL_ASSERT(length != 0 && strcmp(text, "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff%4294967295]:65535") == 0, "%s", text);
	TOOL_ASSERT(AddressFormat(&longest, text, length) == 0 && text[0] == '\0');
	TOOL_ASSERT(AddressFormat(&longest, text, length + 1) == length);

	Address a = AddressIPv4(10, 0, 0, 1, 80);
	Address b = AddressIPv4(10, 0, 0, 1, 81);
	TOOL_ASSERT(!AddressEqual(&a, &b));
	b.port = 80;
	TOOL_ASSERT(AddressEqual(&a, &b));

	Address loopback4 = AddressLoopback(AddressFamilyIPv4, 0);
	Address loopback6 = AddressLoopback(AddressFamilyIPv6, 0);
	Address elsewhere = AddressIPv4(128, 0, 0, 1, 0);
	Address any6 = AddressAny(AddressFamilyIPv6, 0);
	TOOL_ASSERT(AddressIsLoopback(&loopback4) && AddressIsLoopback(&loopback6));
	TOOL_ASSERT(!AddressIsLoopback(&elsewhere) && !AddressIsLoopback(&any6));
}



#if TOOL_SOCKETS

//~ Names

static void TestResolve()
{
	Address addresses[16];
	u32 count = AddressResolve("localhost", 8080, addresses, 16);
	TOOL_ASSERT(count != 0, "%s", ErrorMessage());
	for (u32 i = 0; i < count; i++)
		TOOL_ASSERT(AddressIsLoopback(&addresses[i]) && addresses[i].port == 8080);

	count = AddressResolve("127.0.0.1", 1, addresses, 16);
	Address expected = AddressIPv4(127, 0, 0, 1, 1);
	TOOL_ASSERT(count == 1 && AddressEqual(&addresses[0], &expected));

	count = AddressResolve("::1", 2, addresses, 16, AddressFamilyIPv6);
	TOOL_ASSERT(count == 1 && addresses[0].family == AddressFamilyIPv6 && addresses[0].port == 2);

	// Asking for one family of a name that only has the other finds nothing
	TOOL_ASSERT(AddressResolve("127.0.0.1", 0, addresses, 16, AddressFamilyIPv6) == 0);
}

//~ TCP

static u8 Pattern(u64 i, u32 seed)
{
	return (u8)(i * 131 + (i >> 11) + seed);
}

// Moves 'size' bytes each way at once between two connected sockets, through their buffers filling up
static void Exchange(Socket a, Socket b, u64 size)
{
	static u8 buffer[64 * 1024];
	u64 sent[2] = {};
	u64 received[2] = {};
	Socket sockets[2] = { a, b };

	while (received[0] < size || received[1] < size)
	{
		SocketPoll polls[2];
		for (u32 side = 0; side < 2; side++)
		{
			polls[side].socket = sockets[side];
			polls[side].wait = (received[side] < size ? SocketPollFlagsRead : 0) | (sent[side] < size ? SocketPollFlagsWrite : 0);
		}
		TOOL_ASSERT(SocketsPoll(polls, 2, 5000) != 0, "Stalled: %s", ErrorMessage());

		for (u32 side = 0; side < 2; side++)
		{
			if (polls[side].ready & SocketPollFlagsWrite)
			{
				u64 chunk = TOOL_MIN(size - sent[side], (u64)sizeof(buffer));
				for (u64 i = 0; i < chunk; i++)
					buffer[i] = Pattern(sent[side] + i, side);

				u64 done;
				TOOL_ASSERT(TcpSend(sockets[side], buffer, chunk, &done), "%s", ErrorMessage());
				sent[side] += done;
			}

			if (polls[side].ready & SocketPollFlagsRead)
			{
				u64 done;
				TOOL_ASSERT(TcpReceive(sockets[side], buffer, sizeof(buffer), &done), "%s", ErrorMessage());
				for (u64 i = 0; i < done; i++)
					TOOL_ASSERT(buffer[i] == Pattern(received[side] + i, 1 - side), "Byte %llu", (unsigned long long)(received[side] + i));
				received[side] += done;
			}
		}
	}
}

// Receives until the connection is over, expecting nothing more on it
static void ExpectEnd(Socket socket)
{
	u8 byte;
	u64 received = 0;
	while (true)
	{
		SocketPoll poll = { socket, SocketPollFlagsRead };
		TOOL_ASSERT(SocketsPoll(&poll, 1, 5000) == 1);
		if (!TcpReceive(socket, &byte, 1, &received))
			break;
		TOOL_ASSERT(received == 0);
	}
}

static void TestTcp(AddressFamily family)
{
	Address address = AddressLoopback(family, 0);
	Socket listener;
	TOOL_ASSERT(TcpListen(&listener, &address), "%s", ErrorMessage());

	Address bound;
	TOOL_ASSERT(SocketLocalAddress(listener, &bound) && bound.family == family && bound.port != 0);

	// Nobody waiting yet
	Socket server;
	TOOL_ASSERT(!TcpAccept(listener, &server) && server == 0);

	Socket client;
	TOOL_ASSERT(TcpConnect(&client, &bound, 5000), "%s", ErrorMessage());

	SocketPoll poll = { listener, SocketPollFlagsRead };
	TOOL_ASSERT(SocketsPoll(&poll, 1, 5000) == 1 && poll.ready == SocketPollFlagsRead);

	Address peer;
	TOOL_ASSERT(TcpAccept(listener, &server, &peer), "%s", ErrorMessage());
	TOOL_ASSERT(peer.family == family && AddressIsLoopback(&peer));

	Address clientAddress;
	TOOL_ASSERT(SocketLocalAddress(client, &clientAddress) && AddressEqual(&clientAddress, &peer));

	Exchange(client, server, 8ull << 20);

	// Finishing one way leaves the other open
	TOOL_ASSERT(TcpShutdown(client));
	ExpectEnd(server);
	u64 sent;
	u8 byte = 7;
	TOOL_ASSERT(TcpSend(server, &byte, 1, &sent) && sent == 1);
	u64 received = 0;
	while (received == 0)
	{
		SocketPoll wait = { client, SocketPollFlagsRead };
		TOOL_ASSERT(SocketsPoll(&wait, 1, 5000) == 1);
		TOOL_ASSERT(TcpReceive(client, &byte, 1, &received));
	}
	TOOL_ASSERT(byte == 7);

	SocketClose(server);
	ExpectEnd(client);

	SocketClose(client);
	SocketClose(listener);
}

static void TestTcpConnectLater(AddressFamily family)
{
	Address address = AddressLoopback(family, 0);
	Socket listener;
	TOOL_ASSERT(TcpListen(&listener, &address));
	Address bound;
	TOOL_ASSERT(SocketLocalAddress(listener, &bound));

	// Under way at once, made once it polls ready to write
	Socket client;
	TOOL_ASSERT(TcpConnect(&client, &bound, 0), "%s", ErrorMessage());
	SocketPoll poll = { client, SocketPollFlagsWrite };
	TOOL_ASSERT(SocketsPoll(&poll, 1, 5000) == 1);

	u64 sent;
	u8 byte = 42;
	TOOL_ASSERT(TcpSend(client, &byte, 1, &sent) && sent == 1, "%s", ErrorMessage());

	SocketPoll accept = { listener, SocketPollFlagsRead };
	TOOL_ASSERT(SocketsPoll(&accept, 1, 5000) == 1);
	Socket server;
	TOOL_ASSERT(TcpAccept(listener, &server));
	SocketClose(server);
	SocketClose(client);
	SocketClose(listener);

	// Nothing listens at the port any more, so connecting fails: at once with a timeout, or on the first receive
	// after polling ready without one. Windows retries a refused connection for about two seconds first.
	Socket refused;
	TOOL_ASSERT(!TcpConnect(&refused, &bound, 10000) && refused == 0);

	TOOL_ASSERT(TcpConnect(&refused, &bound, 0), "%s", ErrorMessage());
	SocketPoll failed = { refused, SocketPollFlagsWrite | SocketPollFlagsRead };
	TOOL_ASSERT(SocketsPoll(&failed, 1, 10000) == 1);
	u64 received;
	TOOL_ASSERT(!TcpReceive(refused, &byte, 1, &received));
	SocketClose(refused);
}

//~ UDP

static void TestUdp(AddressFamily family)
{
	Address address = AddressLoopback(family, 0);
	Socket a;
	Socket b;
	TOOL_ASSERT(UdpOpen(&a, &address), "%s", ErrorMessage());
	TOOL_ASSERT(UdpOpen(&b, &address), "%s", ErrorMessage());

	Address addressA;
	Address addressB;
	TOOL_ASSERT(SocketLocalAddress(a, &addressA) && SocketLocalAddress(b, &addressB));
	TOOL_ASSERT(addressA.port != 0 && addressA.port != addressB.port);

	u8 buffer[2048];
	u64 size;
	Address from;
	TOOL_ASSERT(!UdpReceive(a, buffer, sizeof(buffer), &size, &from) && size == 0);

	for (u32 i = 0; i < 16; i++)
	{
		u8 datagram[1000];
		u64 length = 1 + i * 61;
		for (u64 j = 0; j < length; j++)
			datagram[j] = Pattern(j, i);
		TOOL_ASSERT(UdpSend(a, &addressB, datagram, length), "%s", ErrorMessage());

		SocketPoll poll = { b, SocketPollFlagsRead };
		TOOL_ASSERT(SocketsPoll(&poll, 1, 5000) == 1);
		TOOL_ASSERT(UdpReceive(b, buffer, sizeof(buffer), &size, &from), "%s", ErrorMessage());
		TOOL_ASSERT(size == length && memcmp(buffer, datagram, length) == 0 && AddressEqual(&from, &addressA));
	}

	// A datagram larger than the buffer is cut short
	u8 large[500];
	memset(large, 9, sizeof(large));
	TOOL_ASSERT(UdpSend(b, &addressA, large, sizeof(large)));
	SocketPoll poll = { a, SocketPollFlagsRead };
	TOOL_ASSERT(SocketsPoll(&poll, 1, 5000) == 1);
	TOOL_ASSERT(UdpReceive(a, buffer, 10, &size, &from) && size == 10 && buffer[9] == 9);
	TOOL_ASSERT(!UdpReceive(a, buffer, sizeof(buffer), &size));

	// The other family can't be sent to from an IPv4 socket
	if (family == AddressFamilyIPv4)
	{
		Address ipv6 = AddressLoopback(AddressFamilyIPv6, addressB.port);
		TOOL_ASSERT(!UdpSend(a, &ipv6, large, 1));
	}

	// Joining and leaving a group
	Address group;
	TOOL_ASSERT(AddressParse(S8(family == AddressFamilyIPv4 ? "239.255.42.99" : "ff02::4299"), &group));
	TOOL_ASSERT(UdpJoin(a, &group), "%s", ErrorMessage());
	TOOL_ASSERT(UdpLeave(a, &group), "%s", ErrorMessage());

	SocketClose(a);
	SocketClose(b);
}

#if TOOL_LINUX
// A socket at :: takes IPv4 too. Only on Linux here, where listening on every interface asks nobody.
static void TestDualStack()
{
	Address any = AddressAny(AddressFamilyIPv6, 0);
	Socket listener;
	TOOL_ASSERT(TcpListen(&listener, &any), "%s", ErrorMessage());
	Address bound;
	TOOL_ASSERT(SocketLocalAddress(listener, &bound));

	Address ipv4 = AddressLoopback(AddressFamilyIPv4, bound.port);
	Socket client;
	TOOL_ASSERT(TcpConnect(&client, &ipv4, 5000), "%s", ErrorMessage());
	SocketPoll poll = { listener, SocketPollFlagsRead };
	TOOL_ASSERT(SocketsPoll(&poll, 1, 5000) == 1);
	Socket server;
	Address peer;
	TOOL_ASSERT(TcpAccept(listener, &server, &peer) && peer.family == AddressFamilyIPv4 && AddressIsLoopback(&peer));
	Exchange(client, server, 1 << 16);
	SocketClose(server);
	SocketClose(client);
	SocketClose(listener);

	// And an IPv6 UDP socket at :: sends to and receives from IPv4
	Socket udp6;
	Socket udp4;
	Address loopback4 = AddressLoopback(AddressFamilyIPv4, 0);
	TOOL_ASSERT(UdpOpen(&udp6, &any) && UdpOpen(&udp4, &loopback4));
	Address address4;
	Address address6;
	TOOL_ASSERT(SocketLocalAddress(udp4, &address4) && SocketLocalAddress(udp6, &address6));

	u8 byte = 5;
	TOOL_ASSERT(UdpSend(udp6, &address4, &byte, 1), "%s", ErrorMessage());
	SocketPoll wait = { udp4, SocketPollFlagsRead };
	TOOL_ASSERT(SocketsPoll(&wait, 1, 5000) == 1);
	u64 size;
	Address from;
	TOOL_ASSERT(UdpReceive(udp4, &byte, 1, &size, &from) && size == 1 && from.family == AddressFamilyIPv4 && from.port == address6.port);
	SocketClose(udp4);
	SocketClose(udp6);
}
#endif

static void TestPoll()
{
	// Nothing to wait on waits out the timeout
	SocketPoll none[2] = {};
	Timepoint start = TimepointNow();
	TOOL_ASSERT(SocketsPoll(none, 2, 20) == 0);
	TOOL_ASSERT(NanosecondsSince(start) >= 15 * 1000000ll);

	SocketPoll many[TOOL_SOCKET_POLL_MAX + 1] = {};
	TOOL_ASSERT(SocketsPoll(many, TOOL_SOCKET_POLL_MAX + 1, 0) == 0);
}

#endif // TOOL_SOCKETS

void TestNet()
{
	TestAddresses();

#if TOOL_SOCKETS
	TestResolve();
	TestPoll();
	TestTcp(AddressFamilyIPv4);
	TestTcp(AddressFamilyIPv6);
	TestTcpConnectLater(AddressFamilyIPv4);
	TestTcpConnectLater(AddressFamilyIPv6);
	TestUdp(AddressFamilyIPv4);
	TestUdp(AddressFamilyIPv6);
#if TOOL_LINUX
	TestDualStack();
#endif
#endif

	printf("Net: passed\n");
}
