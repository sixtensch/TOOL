#include "network.h"
#include "atomic.h"
#include "error.h"
#include "mathematics.h"
#include "temporal.h"
#include "threading.h"

#include <string.h>



#if TOOL_SOCKETS

//~ Windows
#if TOOL_WINDOWS

#include <winsock2.h>
#include <ws2tcpip.h>

// Stops a UDP socket reporting an earlier datagram's ICMP "port unreachable" as a failure of its next receive
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

//~ Unix
#else

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#endif

#endif // TOOL_SOCKETS



namespace Tool
{
	//- Addresses
	// Parsed and printed here rather than by the system, so every platform reads and writes the same text.

	static constexpr u8 AddressMappedPrefix[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };

	// An IPv4 address mapped into IPv6 is held as the IPv4 address it maps
	static void AddressUnmap(Address* address)
	{
		if (address->family == AddressFamilyIPv6 && memcmp(address->bytes, AddressMappedPrefix, sizeof(AddressMappedPrefix)) == 0)
		{
			u8 ipv4[4];
			memcpy(ipv4, address->bytes + 12, 4);
			memset(address->bytes, 0, sizeof(address->bytes));
			memcpy(address->bytes, ipv4, 4);
			address->family = AddressFamilyIPv4;
			address->scope = 0;
		}
	}

	Address AddressIPv4(u8 a, u8 b, u8 c, u8 d, u16 port)
	{
		Address address = {};
		address.family = AddressFamilyIPv4;
		address.port = port;
		address.bytes[0] = a;
		address.bytes[1] = b;
		address.bytes[2] = c;
		address.bytes[3] = d;
		return address;
	}

	Address AddressLoopback(AddressFamily family, u16 port)
	{
		if (family == AddressFamilyIPv4)
			return AddressIPv4(127, 0, 0, 1, port);

		Address address = {};
		address.family = family;
		address.port = port;
		if (family == AddressFamilyIPv6)
			address.bytes[15] = 1;
		return address;
	}

	Address AddressAny(AddressFamily family, u16 port)
	{
		Address address = {};
		address.family = family;
		address.port = port;
		return address;
	}

	b8 AddressEqual(const Address* a, const Address* b)
	{
		if (a->family != b->family || a->port != b->port || a->scope != b->scope)
			return false;
		u32 size = a->family == AddressFamilyIPv4 ? 4 : a->family == AddressFamilyIPv6 ? 16 : 0;
		return memcmp(a->bytes, b->bytes, size) == 0;
	}

	b8 AddressIsLoopback(const Address* address)
	{
		if (address->family == AddressFamilyIPv4)
			return address->bytes[0] == 127;
		if (address->family == AddressFamilyIPv6)
		{
			static constexpr u8 loopback[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
			return memcmp(address->bytes, loopback, 16) == 0;
		}
		return false;
	}

	//~ Parsing

	static b8 ParseDigits(const c8* begin, const c8* end, u32 maxDigits, u64 max, u64* outValue)
	{
		u64 count = (u64)(end - begin);
		if (count == 0 || count > maxDigits)
			return false;

		u64 value = 0;
		for (const c8* p = begin; p < end; p++)
		{
			if (*p < '0' || *p > '9')
				return false;
			value = value * 10 + (u64)(*p - '0');
		}

		if (value > max)
			return false;
		*outValue = value;
		return true;
	}

	// Four decimal parts of 0 to 255, without leading zeros, which some systems read as octal
	static b8 ParseIPv4(const c8* begin, const c8* end, u8* outBytes)
	{
		const c8* p = begin;
		for (u32 part = 0; part < 4; part++)
		{
			const c8* partEnd = p;
			while (partEnd < end && *partEnd != '.')
				partEnd++;

			u64 value;
			if (!ParseDigits(p, partEnd, 3, 255, &value) || (partEnd - p > 1 && *p == '0'))
				return false;
			outBytes[part] = (u8)value;

			if (part < 3)
			{
				if (partEnd == end)
					return false;
				p = partEnd + 1;
			}
			else if (partEnd != end)
			{
				return false;
			}
		}
		return true;
	}

	static i32 HexDigitValue(c8 c)
	{
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		return -1;
	}

	// Groups of up to four hex digits between colons, one run of zero groups written as "::", and the last two groups
	// optionally as an IPv4 address
	static b8 ParseIPv6(const c8* begin, const c8* end, u8* outBytes)
	{
		u16 groups[8] = {};
		u32 count = 0;
		i32 gap = -1; // Where "::" stands, in groups

		const c8* p = begin;
		if (p < end && *p == ':')
		{
			if (p + 1 >= end || p[1] != ':')
				return false;
			gap = 0;
			p += 2;
		}

		while (p < end)
		{
			const c8* segmentEnd = p;
			b8 dotted = false;
			while (segmentEnd < end && *segmentEnd != ':')
			{
				dotted = dotted || *segmentEnd == '.';
				segmentEnd++;
			}

			if (dotted)
			{
				u8 ipv4[4];
				if (segmentEnd != end || count > 6 || !ParseIPv4(p, end, ipv4))
					return false;
				groups[count++] = (u16)(ipv4[0] << 8 | ipv4[1]);
				groups[count++] = (u16)(ipv4[2] << 8 | ipv4[3]);
				p = end;
				break;
			}

			u64 length = (u64)(segmentEnd - p);
			if (length == 0 || length > 4 || count == 8)
				return false;

			u32 value = 0;
			for (const c8* digit = p; digit < segmentEnd; digit++)
			{
				i32 nibble = HexDigitValue(*digit);
				if (nibble < 0)
					return false;
				value = value << 4 | (u32)nibble;
			}
			groups[count++] = (u16)value;

			p = segmentEnd;
			if (p == end)
				break;

			// A colon, or two for the gap
			p++;
			if (p < end && *p == ':')
			{
				if (gap >= 0)
					return false;
				gap = (i32)count;
				p++;
			}
			else if (p == end)
			{
				return false;
			}
		}

		if (gap < 0 ? count != 8 : count > 7)
			return false;

		u16 expanded[8] = {};
		if (gap < 0)
		{
			memcpy(expanded, groups, sizeof(groups));
		}
		else
		{
			u32 tail = count - (u32)gap;
			memcpy(expanded, groups, (u64)gap * sizeof(u16));
			memcpy(expanded + 8 - tail, groups + gap, tail * sizeof(u16));
		}

		for (u32 i = 0; i < 8; i++)
		{
			outBytes[i * 2] = (u8)(expanded[i] >> 8);
			outBytes[i * 2 + 1] = (u8)expanded[i];
		}
		return true;
	}

	// An IPv6 address with an optional "%scope"
	static b8 ParseIPv6Scoped(const c8* begin, const c8* end, Address* outAddress)
	{
		const c8* percent = begin;
		while (percent < end && *percent != '%')
			percent++;

		if (!ParseIPv6(begin, percent, outAddress->bytes))
			return false;

		if (percent < end)
		{
			u64 scope;
			if (!ParseDigits(percent + 1, end, 10, U32_MAX, &scope))
				return false;
			outAddress->scope = (u32)scope;
		}

		outAddress->family = AddressFamilyIPv6;
		return true;
	}

	static b8 ParsePort(const c8* begin, const c8* end, u16* outPort)
	{
		u64 port;
		if (!ParseDigits(begin, end, 5, 65535, &port))
			return false;
		*outPort = (u16)port;
		return true;
	}

	static b8 AddressParseInto(const c8* begin, const c8* end, Address* address)
	{
		if (begin == end)
			return false;

		if (*begin == '[')
		{
			const c8* close = begin + 1;
			while (close < end && *close != ']')
				close++;
			if (close == end || !ParseIPv6Scoped(begin + 1, close, address))
				return false;

			const c8* after = close + 1;
			if (after == end)
				return true;
			return *after == ':' && ParsePort(after + 1, end, &address->port);
		}

		const c8* colon = nullptr;
		u32 colons = 0;
		for (const c8* p = begin; p < end; p++)
		{
			if (*p == ':')
			{
				colon = colons == 0 ? p : colon;
				colons++;
			}
		}

		if (colons == 0)
		{
			address->family = AddressFamilyIPv4;
			return ParseIPv4(begin, end, address->bytes);
		}

		if (colons == 1)
		{
			address->family = AddressFamilyIPv4;
			return ParseIPv4(begin, colon, address->bytes) && ParsePort(colon + 1, end, &address->port);
		}

		return ParseIPv6Scoped(begin, end, address);
	}

	b8 AddressParse(s8 text, Address* outAddress)
	{
		Address address = {};
		if (!AddressParseInto(text.str, text.str + text.size, &address))
		{
			*outAddress = {};
			return TOOL_FAIL("Not an IP address: '%.*s'", (i32)TOOL_MIN(text.size, (u64)64), text.str);
		}

		AddressUnmap(&address);
		*outAddress = address;
		return true;
	}

	//~ Printing

	struct TextWriter
	{
		c8* text;
		u32 capacity;
		u32 length;
		b8 overflow;
	};

	static void TextWriteC8(TextWriter* writer, c8 c)
	{
		if (writer->length + 1 < writer->capacity)
			writer->text[writer->length++] = c;
		else
			writer->overflow = true;
	}

	static void TextWriteDecimal(TextWriter* writer, u32 value)
	{
		c8 digits[10];
		u32 count = 0;
		do
		{
			digits[count++] = (c8)('0' + value % 10);
			value /= 10;
		} while (value != 0);

		while (count > 0)
			TextWriteC8(writer, digits[--count]);
	}

	// Lowercase, without leading zeros
	static void TextWriteHex(TextWriter* writer, u32 value)
	{
		static constexpr c8 hex[] = "0123456789abcdef";
		b8 started = false;
		for (i32 shift = 12; shift >= 0; shift -= 4)
		{
			u32 nibble = value >> shift & 0xF;
			if (nibble != 0 || started || shift == 0)
			{
				TextWriteC8(writer, hex[nibble]);
				started = true;
			}
		}
	}

	u32 AddressFormat(const Address* address, c8* outText, u32 capacity)
	{
		TextWriter writer = { outText, capacity, 0, capacity == 0 };

		if (address->family == AddressFamilyIPv4)
		{
			for (u32 i = 0; i < 4; i++)
			{
				if (i != 0)
					TextWriteC8(&writer, '.');
				TextWriteDecimal(&writer, address->bytes[i]);
			}

			if (address->port != 0)
			{
				TextWriteC8(&writer, ':');
				TextWriteDecimal(&writer, address->port);
			}
		}
		else if (address->family == AddressFamilyIPv6)
		{
			u16 groups[8];
			for (u32 i = 0; i < 8; i++)
				groups[i] = (u16)(address->bytes[i * 2] << 8 | address->bytes[i * 2 + 1]);

			// The longest run of two or more zero groups becomes "::", the first of equal runs
			i32 gapStart = -1;
			i32 gapLength = 1;
			for (i32 i = 0; i < 8;)
			{
				if (groups[i] != 0)
				{
					i++;
					continue;
				}

				i32 start = i;
				while (i < 8 && groups[i] == 0)
					i++;
				if (i - start > gapLength)
				{
					gapStart = start;
					gapLength = i - start;
				}
			}

			if (address->port != 0)
				TextWriteC8(&writer, '[');

			for (i32 i = 0; i < 8; i++)
			{
				if (i == gapStart)
				{
					TextWriteC8(&writer, ':');
					TextWriteC8(&writer, ':');
					i += gapLength - 1;
					continue;
				}

				if (i != 0 && i != gapStart + gapLength)
					TextWriteC8(&writer, ':');
				TextWriteHex(&writer, groups[i]);
			}

			if (address->scope != 0)
			{
				TextWriteC8(&writer, '%');
				TextWriteDecimal(&writer, address->scope);
			}

			if (address->port != 0)
			{
				TextWriteC8(&writer, ']');
				TextWriteC8(&writer, ':');
				TextWriteDecimal(&writer, address->port);
			}
		}

		if (writer.overflow)
		{
			if (capacity != 0)
				outText[0] = '\0';
			return 0;
		}

		outText[writer.length] = '\0';
		return writer.length;
	}



#if TOOL_SOCKETS

	//- Native layer
	// The few places Windows and Unix sockets differ, so what's built on them is written once

#if TOOL_WINDOWS

	typedef SOCKET NativeSocket;

	typedef WSAPOLLFD NativePoll;
	static constexpr i16 NativePollRead = POLLRDNORM;
	static constexpr i16 NativePollWrite = POLLWRNORM;

	static i32 NativeError()
	{
		return WSAGetLastError();
	}

	static b8 NativeFail(i32 code)
	{
		return TOOL_FAIL_WINDOWS_CODE(code);
	}

	static b8 NativeErrorWouldBlock(i32 code)
	{
		return code == WSAEWOULDBLOCK;
	}

	static b8 NativeErrorInterrupted(i32 code)
	{
		return code == WSAEINTR;
	}

	static b8 NativeErrorNotConnected(i32 code)
	{
		return code == WSAENOTCONN;
	}

	// A connection that went away before it was taken from a listener's queue
	static b8 NativeErrorAcceptAborted(i32 code)
	{
		return code == WSAECONNRESET;
	}

	static void NativeClose(NativeSocket socket)
	{
		closesocket(socket);
	}

	// Winsock is started once, the first time anything needs it, and left running
	static AtomicU32 netStarted;

	static b8 NetStart()
	{
		if (AtomicU32Load(&netStarted) != 0)
			return true;

		WSADATA data;
		i32 result = WSAStartup(MAKEWORD(2, 2), &data);
		if (result != 0)
			return NativeFail(result);

		AtomicU32Store(&netStarted, 1);
		return true;
	}

	static b8 NativeOptionSet(NativeSocket socket, i32 level, i32 name, i32 value)
	{
		if (setsockopt(socket, level, name, (const char*)&value, sizeof(value)) != 0)
			return NativeFail(NativeError());
		return true;
	}

	static b8 NativeNonBlocking(NativeSocket socket)
	{
		u_long enable = 1;
		if (ioctlsocket(socket, FIONBIO, &enable) != 0)
			return NativeFail(NativeError());
		return true;
	}

	static b8 NativeOpen(i32 family, i32 type, i32 protocol, NativeSocket* outSocket)
	{
		if (!NetStart())
			return false;

		NativeSocket socket = WSASocketW(family, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
		if (socket == INVALID_SOCKET)
			return NativeFail(NativeError());

		if (!NativeNonBlocking(socket))
		{
			NativeClose(socket);
			return false;
		}

		*outSocket = socket;
		return true;
	}

	static b8 NativeAccept(NativeSocket listener, NativeSocket* outSocket, sockaddr_storage* outAddress)
	{
		i32 length = sizeof(*outAddress);
		NativeSocket socket = accept(listener, (sockaddr*)outAddress, &length);
		if (socket == INVALID_SOCKET)
			return false;

		// Taken from the listener already, but said again rather than relied on
		if (!NativeNonBlocking(socket))
		{
			NativeClose(socket);
			return false;
		}

		*outSocket = socket;
		return true;
	}

	static i64 NativeSend(NativeSocket socket, const void* data, u64 size)
	{
		return send(socket, (const char*)data, (i32)TOOL_MIN(size, (u64)I32_MAX), 0);
	}

	static i64 NativeReceive(NativeSocket socket, void* destination, u64 capacity)
	{
		return recv(socket, (char*)destination, (i32)TOOL_MIN(capacity, (u64)I32_MAX), 0);
	}

	static i32 NativePollWait(NativePoll* polls, u32 count, i32 timeout)
	{
		return WSAPoll(polls, count, timeout);
	}

#else

	typedef i32 NativeSocket;

	typedef pollfd NativePoll;
	static constexpr i16 NativePollRead = POLLIN;
	static constexpr i16 NativePollWrite = POLLOUT;

	static i32 NativeError()
	{
		return errno;
	}

	static b8 NativeFail(i32 code)
	{
		errno = code;
		return TOOL_FAIL_ERRNO();
	}

	static b8 NativeErrorWouldBlock(i32 code)
	{
		return code == EAGAIN || code == EWOULDBLOCK || code == EINPROGRESS;
	}

	static b8 NativeErrorInterrupted(i32 code)
	{
		return code == EINTR;
	}

	static b8 NativeErrorNotConnected(i32 code)
	{
		return code == ENOTCONN;
	}

	static b8 NativeErrorAcceptAborted(i32 code)
	{
		return code == ECONNABORTED;
	}

	static void NativeClose(NativeSocket socket)
	{
		close(socket);
	}

	static b8 NetStart()
	{
		return true;
	}

	static b8 NativeOptionSet(NativeSocket socket, i32 level, i32 name, i32 value)
	{
		if (setsockopt(socket, level, name, &value, sizeof(value)) != 0)
			return NativeFail(NativeError());
		return true;
	}

	// Non-blocking, not inherited by child processes, and on Apple, not raising SIGPIPE when sending to a closed
	// connection, which Linux avoids per send instead
	static b8 NativePrepare(NativeSocket socket)
	{
#if TOOL_APPLE
		i32 flags = fcntl(socket, F_GETFL, 0);
		if (flags < 0 || fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0 || fcntl(socket, F_SETFD, FD_CLOEXEC) < 0)
			return NativeFail(NativeError());
		return NativeOptionSet(socket, SOL_SOCKET, SO_NOSIGPIPE, 1);
#else
		(void)socket;
		return true;
#endif
	}

	static b8 NativeOpen(i32 family, i32 type, i32 protocol, NativeSocket* outSocket)
	{
#if TOOL_APPLE
		NativeSocket socket = ::socket(family, type, protocol);
#else
		NativeSocket socket = ::socket(family, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol);
#endif
		if (socket < 0)
			return NativeFail(NativeError());

		if (!NativePrepare(socket))
		{
			NativeClose(socket);
			return false;
		}

		*outSocket = socket;
		return true;
	}

	static b8 NativeAccept(NativeSocket listener, NativeSocket* outSocket, sockaddr_storage* outAddress)
	{
		socklen_t length = sizeof(*outAddress);
		NativeSocket socket;
		do
		{
#if TOOL_APPLE
			socket = accept(listener, (sockaddr*)outAddress, &length);
#else
			socket = accept4(listener, (sockaddr*)outAddress, &length, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
		} while (socket < 0 && errno == EINTR);

		if (socket < 0)
			return false;

		if (!NativePrepare(socket))
		{
			NativeClose(socket);
			return false;
		}

		*outSocket = socket;
		return true;
	}

#if TOOL_LINUX
	static constexpr i32 NativeSendFlags = MSG_NOSIGNAL;
#else
	static constexpr i32 NativeSendFlags = 0;
#endif

	static i64 NativeSend(NativeSocket socket, const void* data, u64 size)
	{
		i64 sent;
		do
			sent = send(socket, data, TOOL_MIN(size, (u64)I32_MAX), NativeSendFlags);
		while (sent < 0 && errno == EINTR);
		return sent;
	}

	static i64 NativeReceive(NativeSocket socket, void* destination, u64 capacity)
	{
		i64 received;
		do
			received = recv(socket, destination, TOOL_MIN(capacity, (u64)I32_MAX), 0);
		while (received < 0 && errno == EINTR);
		return received;
	}

	static i32 NativePollWait(NativePoll* polls, u32 count, i32 timeout)
	{
		return poll(polls, count, timeout);
	}

#endif

	//- Sockets

	// A Socket is its native handle plus one, so 0 is none, with the top bit set for IPv6 sockets
	static constexpr u64 SocketIPv6 = 1ull << 63;

	static Socket SocketMake(NativeSocket native, b8 ipv6)
	{
		return ((u64)native + 1) | (ipv6 ? SocketIPv6 : 0);
	}

	static NativeSocket SocketNative(Socket socket)
	{
		return (NativeSocket)((socket & ~SocketIPv6) - 1);
	}

	static b8 SocketIsIPv6(Socket socket)
	{
		return (socket & SocketIPv6) != 0;
	}

	static i32 NativeFamily(AddressFamily family)
	{
		return family == AddressFamilyIPv6 ? AF_INET6 : AF_INET;
	}

	// Fills a system address for a socket of the given family, mapping an IPv4 address into an IPv6 socket's. 0 for
	// an IPv6 address and an IPv4 socket, which can't reach it.
	static socklen_t AddressToNative(const Address* address, b8 ipv6Socket, sockaddr_storage* outNative)
	{
		memset(outNative, 0, sizeof(*outNative));

		if (address->family == AddressFamilyIPv4 && !ipv6Socket)
		{
			sockaddr_in* native = (sockaddr_in*)outNative;
			native->sin_family = AF_INET;
			native->sin_port = htons(address->port);
			memcpy(&native->sin_addr, address->bytes, 4);
			return sizeof(sockaddr_in);
		}

		if (address->family == AddressFamilyNone || (address->family == AddressFamilyIPv6 && !ipv6Socket))
			return 0;

		sockaddr_in6* native = (sockaddr_in6*)outNative;
		native->sin6_family = AF_INET6;
		native->sin6_port = htons(address->port);
		if (address->family == AddressFamilyIPv4)
		{
			memcpy(&native->sin6_addr, AddressMappedPrefix, sizeof(AddressMappedPrefix));
			memcpy((u8*)&native->sin6_addr + 12, address->bytes, 4);
		}
		else
		{
			memcpy(&native->sin6_addr, address->bytes, 16);
			native->sin6_scope_id = address->scope;
		}
		return sizeof(sockaddr_in6);
	}

	static void AddressFromNative(const sockaddr* native, Address* outAddress)
	{
		*outAddress = {};
		if (native->sa_family == AF_INET)
		{
			const sockaddr_in* ipv4 = (const sockaddr_in*)native;
			outAddress->family = AddressFamilyIPv4;
			outAddress->port = ntohs(ipv4->sin_port);
			memcpy(outAddress->bytes, &ipv4->sin_addr, 4);
		}
		else if (native->sa_family == AF_INET6)
		{
			const sockaddr_in6* ipv6 = (const sockaddr_in6*)native;
			outAddress->family = AddressFamilyIPv6;
			outAddress->port = ntohs(ipv6->sin6_port);
			outAddress->scope = ipv6->sin6_scope_id;
			memcpy(outAddress->bytes, &ipv6->sin6_addr, 16);
			AddressUnmap(outAddress);
		}
	}

	static b8 AddressUsable(const Address* address)
	{
		if (address->family != AddressFamilyIPv4 && address->family != AddressFamilyIPv6)
			return TOOL_FAIL("The address has no family.");
		return true;
	}

	// Opens a socket for an address's family, taking IPv4 too when it's IPv6
	static b8 SocketOpen(const Address* address, b8 stream, NativeSocket* outSocket)
	{
		i32 type = stream ? SOCK_STREAM : SOCK_DGRAM;
		i32 protocol = stream ? IPPROTO_TCP : IPPROTO_UDP;
		if (!NativeOpen(NativeFamily(address->family), type, protocol, outSocket))
			return false;

		// Best effort: a system without IPv4 keeps its IPv6 socket to itself
		if (address->family == AddressFamilyIPv6)
			NativeOptionSet(*outSocket, IPPROTO_IPV6, IPV6_V6ONLY, 0);

		return true;
	}

	static b8 SocketBind(NativeSocket socket, const Address* address)
	{
		sockaddr_storage native;
		socklen_t length = AddressToNative(address, address->family == AddressFamilyIPv6, &native);
		if (bind(socket, (const sockaddr*)&native, length) != 0)
			return NativeFail(NativeError());
		return true;
	}

	// A send or receive told the socket isn't connected: either its connection is still being made, or it failed
	static b8 SocketConnecting(NativeSocket socket)
	{
		i32 error = 0;
		socklen_t length = sizeof(error);
		if (getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*)&error, &length) != 0)
			return NativeFail(NativeError());
		if (error != 0)
			return NativeFail(error);
		return true;
	}

	void SocketClose(Socket socket)
	{
		if (socket != 0)
			NativeClose(SocketNative(socket));
	}

	b8 SocketLocalAddress(Socket socket, Address* outAddress)
	{
		sockaddr_storage native = {};
		socklen_t length = sizeof(native);
		if (getsockname(SocketNative(socket), (sockaddr*)&native, &length) != 0)
		{
			*outAddress = {};
			return NativeFail(NativeError());
		}

		AddressFromNative((const sockaddr*)&native, outAddress);
		return true;
	}

	u32 SocketsPoll(SocketPoll* polls, u32 count, u32 timeout)
	{
		if (count > TOOL_SOCKET_POLL_MAX)
		{
			TOOL_FAIL("Can't poll %u sockets at once, only %u.", count, (u32)TOOL_SOCKET_POLL_MAX);
			return 0;
		}

		// Only the sockets there are, as Windows may not skip an invalid one
		NativePoll natives[TOOL_SOCKET_POLL_MAX];
		u16 sources[TOOL_SOCKET_POLL_MAX]; // Which poll each native one is
		u32 live = 0;
		for (u32 i = 0; i < count; i++)
		{
			polls[i].ready = 0;
			if (polls[i].socket == 0)
				continue;

			natives[live] = {};
			natives[live].fd = SocketNative(polls[i].socket);
			natives[live].events = (i16)(((polls[i].wait & SocketPollFlagsRead) ? NativePollRead : 0) | ((polls[i].wait & SocketPollFlagsWrite) ? NativePollWrite : 0));
			sources[live] = (u16)i;
			live++;
		}

		// Windows refuses to poll nothing, so a wait on nothing is a sleep everywhere
		if (live == 0)
		{
			ThreadSleep(timeout);
			return 0;
		}

		Timepoint start = TimepointNow();
		i32 result;
		while (true)
		{
			i64 waited = NanosecondsSince(start) / 1000000;
			i32 remaining = (i32)TOOL_MIN((i64)timeout - TOOL_MIN(waited, (i64)timeout), (i64)I32_MAX);
			result = NativePollWait(natives, live, remaining);
			if (result >= 0 || !NativeErrorInterrupted(NativeError()))
				break;
		}

		if (result < 0)
		{
			NativeFail(NativeError());
			return 0;
		}

		u32 ready = 0;
		for (u32 i = 0; i < live; i++)
		{
			SocketPoll* poll = &polls[sources[i]];
			i16 events = natives[i].revents;
			u32 flags = 0;
			if (events & NativePollRead)
				flags |= SocketPollFlagsRead;
			if (events & NativePollWrite)
				flags |= SocketPollFlagsWrite;

			// Over, failed or invalid: ready for whatever it waits for, so the next call finds out which
			if (events & (POLLHUP | POLLERR | POLLNVAL))
				flags |= poll->wait;

			poll->ready = flags & poll->wait;
			ready += poll->ready != 0 ? 1 : 0;
		}
		return ready;
	}

	//~ TCP

	b8 TcpListen(Socket* outSocket, const Address* address, u32 backlog)
	{
		*outSocket = 0;
		if (!AddressUsable(address))
			return false;

		NativeSocket socket;
		if (!SocketOpen(address, true, &socket))
			return false;

		// Windows lets another socket bind over one with SO_REUSEADDR, so it gets exclusive use instead. Elsewhere the
		// option only lets a listener come back while its last connections still linger.
#if TOOL_WINDOWS
		b8 prepared = NativeOptionSet(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, 1);
#else
		b8 prepared = NativeOptionSet(socket, SOL_SOCKET, SO_REUSEADDR, 1);
#endif

		if (prepared && SocketBind(socket, address))
		{
			if (listen(socket, backlog != 0 ? (i32)TOOL_MIN(backlog, (u32)I32_MAX) : SOMAXCONN) != 0)
				prepared = NativeFail(NativeError());
		}
		else
		{
			prepared = false;
		}

		if (!prepared)
		{
			NativeClose(socket);
			return false;
		}

		*outSocket = SocketMake(socket, address->family == AddressFamilyIPv6);
		return true;
	}

	b8 TcpAccept(Socket listener, Socket* outSocket, Address* outAddress)
	{
		*outSocket = 0;
		if (outAddress != nullptr)
			*outAddress = {};

		sockaddr_storage native = {};
		NativeSocket socket;
		if (!NativeAccept(SocketNative(listener), &socket, &native))
		{
			i32 error = NativeError();
			if (NativeErrorWouldBlock(error) || NativeErrorAcceptAborted(error))
				return false;
			return NativeFail(error);
		}

		NativeOptionSet(socket, IPPROTO_TCP, TCP_NODELAY, 1);

		*outSocket = SocketMake(socket, SocketIsIPv6(listener));
		if (outAddress != nullptr)
			AddressFromNative((const sockaddr*)&native, outAddress);
		return true;
	}

	b8 TcpConnect(Socket* outSocket, const Address* address, u32 timeout)
	{
		*outSocket = 0;
		if (!AddressUsable(address))
			return false;

		NativeSocket socket;
		if (!SocketOpen(address, true, &socket))
			return false;

		NativeOptionSet(socket, IPPROTO_TCP, TCP_NODELAY, 1);

		sockaddr_storage native;
		socklen_t length = AddressToNative(address, address->family == AddressFamilyIPv6, &native);
		if (connect(socket, (const sockaddr*)&native, length) != 0)
		{
			i32 error = NativeError();
			if (!NativeErrorWouldBlock(error) && !NativeErrorInterrupted(error))
			{
				NativeClose(socket);
				return NativeFail(error);
			}

			if (timeout != 0)
			{
				SocketPoll poll = { SocketMake(socket, false), SocketPollFlagsWrite };
				if (SocketsPoll(&poll, 1, timeout) == 0)
				{
					NativeClose(socket);
					c8 text[TOOL_ADDRESS_TEXT_CAPACITY];
					AddressFormat(address, text, sizeof(text));
					return TOOL_FAIL("Connecting to %s took longer than %u ms.", text, timeout);
				}

				if (!SocketConnecting(socket))
				{
					NativeClose(socket);
					return false;
				}
			}
		}

		*outSocket = SocketMake(socket, address->family == AddressFamilyIPv6);
		return true;
	}

	b8 TcpSend(Socket socket, const void* data, u64 size, u64* outSent)
	{
		*outSent = 0;
		if (size == 0)
			return true;

		i64 sent = NativeSend(SocketNative(socket), data, size);
		if (sent >= 0)
		{
			*outSent = (u64)sent;
			return true;
		}

		i32 error = NativeError();
		if (NativeErrorWouldBlock(error))
			return true;
		if (NativeErrorNotConnected(error))
			return SocketConnecting(SocketNative(socket));
		return NativeFail(error);
	}

	b8 TcpReceive(Socket socket, void* destination, u64 capacity, u64* outReceived)
	{
		*outReceived = 0;
		if (capacity == 0)
			return true;

		i64 received = NativeReceive(SocketNative(socket), destination, capacity);
		if (received > 0)
		{
			*outReceived = (u64)received;
			return true;
		}

		// Closed by the other end
		if (received == 0)
			return false;

		i32 error = NativeError();
		if (NativeErrorWouldBlock(error))
			return true;
		if (NativeErrorNotConnected(error))
			return SocketConnecting(SocketNative(socket));
		return NativeFail(error);
	}

	b8 TcpShutdown(Socket socket)
	{
#if TOOL_WINDOWS
		i32 result = shutdown(SocketNative(socket), SD_SEND);
#else
		i32 result = shutdown(SocketNative(socket), SHUT_WR);
#endif
		if (result != 0)
			return NativeFail(NativeError());
		return true;
	}

	//~ UDP

	b8 UdpOpen(Socket* outSocket, const Address* address, u32 flags)
	{
		*outSocket = 0;
		if (!AddressUsable(address))
			return false;

		NativeSocket socket;
		if (!SocketOpen(address, false, &socket))
			return false;

		b8 prepared = true;
		if (flags & UdpFlagsBroadcast)
			prepared = NativeOptionSet(socket, SOL_SOCKET, SO_BROADCAST, 1);

#if TOOL_WINDOWS
		if (prepared)
		{
			BOOL report = FALSE;
			DWORD bytes = 0;
			if (WSAIoctl(socket, SIO_UDP_CONNRESET, &report, sizeof(report), nullptr, 0, &bytes, nullptr, nullptr) != 0)
				prepared = NativeFail(NativeError());
		}
#endif

		if (!prepared || !SocketBind(socket, address))
		{
			NativeClose(socket);
			return false;
		}

		*outSocket = SocketMake(socket, address->family == AddressFamilyIPv6);
		return true;
	}

	b8 UdpSend(Socket socket, const Address* to, const void* data, u64 size)
	{
		if (size > (u64)I32_MAX)
			return TOOL_FAIL("A datagram of %llu bytes is too large.", (unsigned long long)size);

		sockaddr_storage native;
		socklen_t length = AddressToNative(to, SocketIsIPv6(socket), &native);
		if (length == 0)
			return TOOL_FAIL("An IPv4 socket can't send to an IPv6 address.");

		i64 sent;
		do
		{
#if TOOL_WINDOWS
			sent = sendto(SocketNative(socket), (const char*)data, (i32)size, 0, (const sockaddr*)&native, length);
#else
			sent = sendto(SocketNative(socket), data, size, NativeSendFlags, (const sockaddr*)&native, length);
#endif
		} while (sent < 0 && NativeErrorInterrupted(NativeError()));

		if (sent < 0)
			return NativeFail(NativeError());
		if ((u64)sent != size)
			return TOOL_FAIL("Only %lld of a datagram's %llu bytes were sent.", (long long)sent, (unsigned long long)size);
		return true;
	}

	b8 UdpReceive(Socket socket, void* destination, u64 capacity, u64* outSize, Address* outFrom)
	{
		*outSize = 0;
		if (outFrom != nullptr)
			*outFrom = {};

		sockaddr_storage native = {};
		socklen_t length = sizeof(native);
		i64 received;
		do
		{
#if TOOL_WINDOWS
			received = recvfrom(SocketNative(socket), (char*)destination, (i32)TOOL_MIN(capacity, (u64)I32_MAX), 0, (sockaddr*)&native, &length);
#else
			received = recvfrom(SocketNative(socket), destination, TOOL_MIN(capacity, (u64)I32_MAX), 0, (sockaddr*)&native, &length);
#endif
		} while (received < 0 && NativeErrorInterrupted(NativeError()));

		if (received < 0)
		{
			i32 error = NativeError();
#if TOOL_WINDOWS
			// Cut short, with what fit kept
			if (error == WSAEMSGSIZE)
				received = (i64)TOOL_MIN(capacity, (u64)I32_MAX);
			else
#endif
				return NativeErrorWouldBlock(error) ? false : NativeFail(error);
		}

		*outSize = (u64)received;
		if (outFrom != nullptr)
			AddressFromNative((const sockaddr*)&native, outFrom);
		return true;
	}

	static b8 UdpMembership(Socket socket, const Address* group, b8 join)
	{
		b8 ipv6 = group->family == AddressFamilyIPv6;
		if (group->family == AddressFamilyNone || ipv6 != SocketIsIPv6(socket))
			return TOOL_FAIL("A multicast group must be of the socket's family.");

		i32 result;
		if (ipv6)
		{
			ipv6_mreq request = {};
			memcpy(&request.ipv6mr_multiaddr, group->bytes, 16);
			request.ipv6mr_interface = group->scope;
			result = setsockopt(SocketNative(socket), IPPROTO_IPV6, join ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP, (const char*)&request, sizeof(request));
		}
		else
		{
			ip_mreq request = {};
			memcpy(&request.imr_multiaddr, group->bytes, 4);
			request.imr_interface.s_addr = htonl(INADDR_ANY);
			result = setsockopt(SocketNative(socket), IPPROTO_IP, join ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP, (const char*)&request, sizeof(request));
		}

		if (result != 0)
			return NativeFail(NativeError());
		return true;
	}

	b8 UdpJoin(Socket socket, const Address* group)
	{
		return UdpMembership(socket, group, true);
	}

	b8 UdpLeave(Socket socket, const Address* group)
	{
		return UdpMembership(socket, group, false);
	}

	//~ Names

	u32 AddressResolve(const c8* host, u16 port, Address* outAddresses, u32 capacity, AddressFamily family)
	{
		if (!NetStart())
			return 0;

		i32 nativeFamily = family == AddressFamilyIPv4 ? AF_INET : family == AddressFamilyIPv6 ? AF_INET6 : AF_UNSPEC;
		u32 count = 0;

#if TOOL_WINDOWS
		// The wide form, as the narrow one reads names in the system's code page rather than UTF-8
		wchar_t wide[1024];
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, host, -1, wide, 1024) == 0)
		{
			TOOL_FAIL_WINDOWS();
			return 0;
		}

		ADDRINFOW hints = {};
		hints.ai_family = nativeFamily;
		hints.ai_socktype = SOCK_STREAM; // One answer per address, rather than one per kind of socket

		ADDRINFOW* results = nullptr;
		i32 error = GetAddrInfoW(wide, nullptr, &hints, &results);
		if (error != 0)
		{
			TOOL_FAIL_WINDOWS_CODE(error);
			return 0;
		}

		for (ADDRINFOW* result = results; result != nullptr && count < capacity; result = result->ai_next)
#else
		addrinfo hints = {};
		hints.ai_family = nativeFamily;
		hints.ai_socktype = SOCK_STREAM; // One answer per address, rather than one per kind of socket

		addrinfo* results = nullptr;
		i32 error = getaddrinfo(host, nullptr, &hints, &results);
		if (error != 0)
		{
			if (error == EAI_SYSTEM)
				TOOL_FAIL_ERRNO();
			else
				TOOL_FAIL("Couldn't look up %s: %s", host, gai_strerror(error));
			return 0;
		}

		for (addrinfo* result = results; result != nullptr && count < capacity; result = result->ai_next)
#endif
		{
			Address address;
			AddressFromNative(result->ai_addr, &address);
			if (address.family == AddressFamilyNone)
				continue;

			address.port = port;
			outAddresses[count++] = address;
		}

#if TOOL_WINDOWS
		FreeAddrInfoW(results);
#else
		freeaddrinfo(results);
#endif

		if (count == 0)
			TOOL_FAIL("No addresses were found for %s.", host);
		return count;
	}

#else

	//- No sockets

	static b8 NetUnavailable()
	{
		return TOOL_FAIL("This platform has no sockets.");
	}

	u32 AddressResolve(const c8*, u16, Address*, u32, AddressFamily)
	{
		NetUnavailable();
		return 0;
	}

	void SocketClose(Socket)
	{
	}

	b8 SocketLocalAddress(Socket, Address* outAddress)
	{
		*outAddress = {};
		return NetUnavailable();
	}

	u32 SocketsPoll(SocketPoll* polls, u32 count, u32)
	{
		for (u32 i = 0; i < count; i++)
			polls[i].ready = 0;
		NetUnavailable();
		return 0;
	}

	b8 TcpListen(Socket* outSocket, const Address*, u32)
	{
		*outSocket = 0;
		return NetUnavailable();
	}

	b8 TcpAccept(Socket, Socket* outSocket, Address* outAddress)
	{
		*outSocket = 0;
		if (outAddress != nullptr)
			*outAddress = {};
		return NetUnavailable();
	}

	b8 TcpConnect(Socket* outSocket, const Address*, u32)
	{
		*outSocket = 0;
		return NetUnavailable();
	}

	b8 TcpSend(Socket, const void*, u64, u64* outSent)
	{
		*outSent = 0;
		return NetUnavailable();
	}

	b8 TcpReceive(Socket, void*, u64, u64* outReceived)
	{
		*outReceived = 0;
		return NetUnavailable();
	}

	b8 TcpShutdown(Socket)
	{
		return NetUnavailable();
	}

	b8 UdpOpen(Socket* outSocket, const Address*, u32)
	{
		*outSocket = 0;
		return NetUnavailable();
	}

	b8 UdpSend(Socket, const Address*, const void*, u64)
	{
		return NetUnavailable();
	}

	b8 UdpReceive(Socket, void*, u64, u64* outSize, Address* outFrom)
	{
		*outSize = 0;
		if (outFrom != nullptr)
			*outFrom = {};
		return NetUnavailable();
	}

	b8 UdpJoin(Socket, const Address*)
	{
		return NetUnavailable();
	}

	b8 UdpLeave(Socket, const Address*)
	{
		return NetUnavailable();
	}

#endif // TOOL_SOCKETS
} //namespace Tool
