#include "http.h"
#include "error.h"
#include "random.h"
#include "temporal.h"

#include <string.h>

#if TOOL_WEB
#include <emscripten.h>
#endif



namespace Tool
{
	//- Text

	static i32 HexValue(c8 c)
	{
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		return -1;
	}

	// Decimal digits only, no sign or prefix
	static b8 ParseDecimal(s8 text, u64 max, u64* outValue)
	{
		if (text.size == 0 || text.size > 19)
			return false;

		u64 value = 0;
		for (u64 i = 0; i < text.size; i++)
		{
			if (!C8IsDigit(text.str[i]))
				return false;
			value = value * 10 + (u64)(text.str[i] - '0');
		}

		if (value > max)
			return false;
		*outValue = value;
		return true;
	}

	// RFC 9110 token characters, as in header names and methods
	static b8 IsTokenChar(c8 c)
	{
		if (C8IsAlphaNumeric(c))
			return true;
		switch (c)
		{
		case '!': case '#': case '$': case '%': case '&': case '\'': case '*': case '+':
		case '-': case '.': case '^': case '_': case '`': case '|': case '~':
			return true;
		}
		return false;
	}

	// True if the comma-separated 'list' holds 'token', ignoring case
	static b8 ListHasToken(s8 list, s8 token)
	{
		s8 item;
		while (list.size > 0)
		{
			S8Cut(list, S8(","), &item, &list);
			if (S8EqualIgnoreCase(S8Trim(item), token))
				return true;
		}
		return false;
	}



	//- URLs

	static b8 UrlUnreserved(c8 c)
	{
		return C8IsAlphaNumeric(c) || c == '-' || c == '.' || c == '_' || c == '~';
	}

	b8 UrlParse(s8 text, Url* outUrl)
	{
		*outUrl = {};

		u64 schemeEnd = S8Find(text, S8("://"));
		if (schemeEnd == S8_NONE || schemeEnd == 0)
			return false;
		for (u64 i = 0; i < schemeEnd; i++)
		{
			c8 c = text.str[i];
			if (!C8IsAlpha(c) && (i == 0 || (!C8IsDigit(c) && c != '+' && c != '-' && c != '.')))
				return false;
		}
		s8 scheme = S8Slice(text, 0, schemeEnd);

		u64 authorityStart = schemeEnd + 3;
		u64 authorityEnd = authorityStart;
		while (authorityEnd < text.size && text.str[authorityEnd] != '/' && text.str[authorityEnd] != '?' && text.str[authorityEnd] != '#')
			authorityEnd++;
		s8 authority = S8Slice(text, authorityStart, authorityEnd);
		if (S8Find(authority, S8("@")) != S8_NONE)
			return false;

		s8 host;
		s8 portText = {};
		b8 hasPort = false;
		if (authority.size > 0 && authority.str[0] == '[')
		{
			u64 close = S8Find(authority, S8("]"));
			if (close == S8_NONE)
				return false;
			host = S8Slice(authority, 1, close);
			s8 rest = S8Slice(authority, close + 1, authority.size);
			if (rest.size > 0)
			{
				if (rest.str[0] != ':')
					return false;
				portText = S8Slice(rest, 1, rest.size);
				hasPort = true;
			}
		}
		else
		{
			hasPort = S8Cut(authority, S8(":"), &host, &portText);
		}
		if (host.size == 0)
			return false;

		u64 port = 0;
		if (hasPort)
		{
			if (!ParseDecimal(portText, 65535, &port))
				return false;
		}
		else if (S8EqualIgnoreCase(scheme, S8("http")) || S8EqualIgnoreCase(scheme, S8("ws")))
		{
			port = 80;
		}
		else if (S8EqualIgnoreCase(scheme, S8("https")) || S8EqualIgnoreCase(scheme, S8("wss")))
		{
			port = 443;
		}

		s8 rest = S8Slice(text, authorityEnd, text.size);
		s8 fragment = {};
		u64 hash = S8Find(rest, S8("#"));
		if (hash != S8_NONE)
		{
			fragment = S8Slice(rest, hash + 1, rest.size);
			rest = S8Slice(rest, 0, hash);
		}
		s8 path = rest;
		s8 query = {};
		u64 question = S8Find(rest, S8("?"));
		if (question != S8_NONE)
		{
			query = S8Slice(rest, question + 1, rest.size);
			path = S8Slice(rest, 0, question);
		}

		outUrl->scheme = scheme;
		outUrl->host = host;
		outUrl->port = (u16)port;
		outUrl->path = path;
		outUrl->query = query;
		outUrl->fragment = fragment;
		return true;
	}

	s8 UrlEncode(s8 text, MemoryAllocator allocator)
	{
		static constexpr c8 digits[] = "0123456789ABCDEF";

		u64 size = 0;
		for (u64 i = 0; i < text.size; i++)
			size += UrlUnreserved(text.str[i]) ? 1 : 3;

		c8* out = (c8*)AllocatorAlloc(allocator, size + 1);
		if (out == nullptr)
			return {};

		u64 at = 0;
		for (u64 i = 0; i < text.size; i++)
		{
			u8 c = (u8)text.str[i];
			if (UrlUnreserved((c8)c))
			{
				out[at++] = (c8)c;
				continue;
			}
			out[at++] = '%';
			out[at++] = digits[c >> 4];
			out[at++] = digits[c & 15];
		}
		out[size] = 0;
		return { out, size };
	}

	s8 UrlDecode(s8 text, MemoryAllocator allocator)
	{
		c8* out = (c8*)AllocatorAlloc(allocator, text.size + 1);
		if (out == nullptr)
			return {};

		u64 size = 0;
		for (u64 i = 0; i < text.size; i++)
		{
			if (text.str[i] == '%' && i + 2 < text.size)
			{
				i32 high = HexValue(text.str[i + 1]);
				i32 low = HexValue(text.str[i + 2]);
				if (high >= 0 && low >= 0)
				{
					out[size++] = (c8)(high * 16 + low);
					i += 2;
					continue;
				}
			}
			out[size++] = text.str[i];
		}
		out[size] = 0;
		return { out, size };
	}



	//- Messages
	// Shared by the server and the client. A line ends with LF, its CR optional (RFC 9112 2.2).

	enum HttpFraming
	{
		HttpFramingNone,    // No body
		HttpFramingLength,  // Content-Length bytes
		HttpFramingChunked, // Transfer-Encoding: chunked
		HttpFramingClose,   // Until the connection closes; responses only
	};

	static const s8 HttpMethodNames[] = {
		{},
		S8("GET"),
		S8("HEAD"),
		S8("POST"),
		S8("PUT"),
		S8("DELETE"),
		S8("PATCH"),
		S8("OPTIONS"),
	};

	static s8 HttpStatusText(u32 status)
	{
		switch (status)
		{
		case 100: return S8("Continue");
		case 101: return S8("Switching Protocols");
		case 200: return S8("OK");
		case 201: return S8("Created");
		case 202: return S8("Accepted");
		case 204: return S8("No Content");
		case 206: return S8("Partial Content");
		case 301: return S8("Moved Permanently");
		case 302: return S8("Found");
		case 303: return S8("See Other");
		case 304: return S8("Not Modified");
		case 307: return S8("Temporary Redirect");
		case 308: return S8("Permanent Redirect");
		case 400: return S8("Bad Request");
		case 401: return S8("Unauthorized");
		case 403: return S8("Forbidden");
		case 404: return S8("Not Found");
		case 405: return S8("Method Not Allowed");
		case 408: return S8("Request Timeout");
		case 409: return S8("Conflict");
		case 410: return S8("Gone");
		case 411: return S8("Length Required");
		case 413: return S8("Content Too Large");
		case 414: return S8("URI Too Long");
		case 415: return S8("Unsupported Media Type");
		case 422: return S8("Unprocessable Content");
		case 429: return S8("Too Many Requests");
		case 431: return S8("Request Header Fields Too Large");
		case 500: return S8("Internal Server Error");
		case 501: return S8("Not Implemented");
		case 502: return S8("Bad Gateway");
		case 503: return S8("Service Unavailable");
		case 504: return S8("Gateway Timeout");
		case 505: return S8("HTTP Version Not Supported");
		}
		return {};
	}

	// The head's size through its empty line, 0 while incomplete
	static u64 HeadSize(const u8* data, u64 size)
	{
		for (u64 i = 1; i < size; i++)
		{
			if (data[i] != '\n')
				continue;
			if (data[i - 1] == '\n' || (i >= 2 && data[i - 1] == '\r' && data[i - 2] == '\n'))
				return i + 1;
		}
		return 0;
	}

	// Takes the next line off 'text', without its line end
	static b8 NextLine(s8* text, s8* outLine)
	{
		if (text->size == 0)
			return false;

		u64 end = 0;
		while (end < text->size && text->str[end] != '\n')
			end++;

		s8 line = { text->str, end };
		if (line.size > 0 && line.str[line.size - 1] == '\r')
			line.size--;

		u64 next = end < text->size ? end + 1 : end;
		text->str += next;
		text->size -= next;
		*outLine = line;
		return true;
	}

	// Header lines through the empty one. 0, or the status a server answers with: 400 malformed, 431 too many.
	static u32 ParseHeaders(s8* text, HttpHeader* headers, u32* outCount)
	{
		u32 count = 0;
		s8 line;
		while (NextLine(text, &line))
		{
			if (line.size == 0)
			{
				*outCount = count;
				return 0;
			}

			// Obsolete line folding is refused (RFC 9112 5.2)
			if (line.str[0] == ' ' || line.str[0] == '\t')
				return 400;

			u64 colon = S8Find(line, S8(":"));
			if (colon == S8_NONE || colon == 0)
				return 400;
			s8 name = S8Slice(line, 0, colon);
			for (u64 i = 0; i < name.size; i++)
				if (!IsTokenChar(name.str[i]))
					return 400;

			if (count == TOOL_HTTP_HEADER_MAX)
				return 431;
			headers[count++] = { name, S8Trim(S8Slice(line, colon + 1, line.size)) };
		}
		return 400;
	}

	// How a message's body is framed (RFC 9112 6.3). 0, or 400 for a bad length, 501 for a coding other than chunked.
	static u32 ParseFraming(const HttpHeader* headers, u32 count, HttpFraming* outFraming, u64* outLength)
	{
		*outFraming = HttpFramingNone;
		*outLength = 0;

		s8 encoding = HttpHeaderFind(headers, count, S8("Transfer-Encoding"));
		if (encoding.str != nullptr)
		{
			if (!S8EqualIgnoreCase(S8Trim(encoding), S8("chunked")))
				return 501;
			*outFraming = HttpFramingChunked;
			return 0;
		}

		// Repeats must agree
		b8 found = false;
		for (u32 i = 0; i < count; i++)
		{
			if (!S8EqualIgnoreCase(headers[i].name, S8("Content-Length")))
				continue;
			u64 length;
			if (!ParseDecimal(headers[i].value, U64_MAX >> 4, &length) || (found && length != *outLength))
				return 400;
			*outLength = length;
			found = true;
		}
		if (found)
			*outFraming = HttpFramingLength;
		return 0;
	}

	// A chunked body (RFC 9112 7.1): 1 complete, 0 incomplete, -1 malformed. With 'join', moves the chunks' data
	// together at 'data'. Sets 'outSize' to the data's size and 'outUsed' to the bytes taken, trailers included.
	static i32 Dechunk(u8* data, u64 size, b8 join, u64* outSize, u64* outUsed)
	{
		u64 read = 0;
		u64 write = 0;
		for (;;)
		{
			u64 lineEnd = read;
			while (lineEnd < size && data[lineEnd] != '\n')
				lineEnd++;
			if (lineEnd == size)
				return 0;

			u64 chunk = 0;
			u64 at = read;
			while (at < lineEnd && HexValue((c8)data[at]) >= 0)
			{
				if (at - read == 15)
					return -1;
				chunk = chunk * 16 + (u64)HexValue((c8)data[at]);
				at++;
			}
			if (at == read)
				return -1;
			// Extensions after ';' are ignored
			if (at < lineEnd && data[at] != ';' && data[at] != ' ' && data[at] != '\t' && data[at] != '\r')
				return -1;
			read = lineEnd + 1;

			if (chunk == 0)
				break;

			// The data and at least its LF
			if (size - read < chunk + 1)
				return 0;
			if (join)
				memmove(data + write, data + read, chunk);
			write += chunk;
			read += chunk;

			if (data[read] == '\r')
			{
				if (read + 1 == size)
					return 0;
				read++;
			}
			if (data[read] != '\n')
				return -1;
			read++;
		}

		// Trailers, through an empty line
		for (;;)
		{
			u64 lineEnd = read;
			while (lineEnd < size && data[lineEnd] != '\n')
				lineEnd++;
			if (lineEnd == size)
				return 0;
			b8 empty = lineEnd == read || (lineEnd == read + 1 && data[read] == '\r');
			read = lineEnd + 1;
			if (empty)
				break;
		}

		*outSize = write;
		*outUsed = read;
		return 1;
	}

	s8 HttpHeaderFind(const HttpHeader* headers, u32 count, s8 name)
	{
		for (u32 i = 0; i < count; i++)
			if (S8EqualIgnoreCase(headers[i].name, name))
				return headers[i].value;
		return {};
	}

	//~ Buffers

	struct HttpBuffer
	{
		u8* data;
		u64 size;
		u64 capacity;
	};

	static b8 BufferReserve(HttpBuffer* buffer, u64 capacity, MemoryAllocator allocator)
	{
		if (buffer->capacity >= capacity)
			return true;

		u64 grown = buffer->capacity < 4096 ? 4096 : buffer->capacity;
		while (grown < capacity)
			grown *= 2;

		u8* data = (u8*)AllocatorAlloc(allocator, grown);
		if (data == nullptr)
			return TOOL_FAIL("Out of memory for an HTTP buffer");
		if (buffer->size > 0)
			memcpy(data, buffer->data, buffer->size);
		if (buffer->data != nullptr)
			AllocatorDealloc(allocator, buffer->data);
		buffer->data = data;
		buffer->capacity = grown;
		return true;
	}

	static void BufferFree(HttpBuffer* buffer, MemoryAllocator allocator)
	{
		if (buffer->data != nullptr)
			AllocatorDealloc(allocator, buffer->data);
		*buffer = {};
	}

	// Removes 'size' bytes from the front
	static void BufferDrop(HttpBuffer* buffer, u64 size)
	{
		memmove(buffer->data, buffer->data + size, buffer->size - size);
		buffer->size -= size;
	}

	static b8 BufferAppend(HttpBuffer* buffer, const void* data, u64 size, MemoryAllocator allocator)
	{
		if (!BufferReserve(buffer, buffer->size + size, allocator))
			return false;
		if (size > 0)
			memcpy(buffer->data + buffer->size, data, size);
		buffer->size += size;
		return true;
	}

	static b8 BufferAppend(HttpBuffer* buffer, s8 text, MemoryAllocator allocator)
	{
		return BufferAppend(buffer, text.str, text.size, allocator);
	}

	static b8 BufferAppendNumber(HttpBuffer* buffer, u64 value, MemoryAllocator allocator)
	{
		c8 digits[20];
		u32 at = 20;
		do
		{
			digits[--at] = (c8)('0' + value % 10);
			value /= 10;
		} while (value != 0);
		return BufferAppend(buffer, digits + at, 20 - at, allocator);
	}

	static b8 BufferAppendHeaders(HttpBuffer* buffer, const HttpHeader* headers, u32 count, MemoryAllocator allocator)
	{
		b8 ok = true;
		for (u32 i = 0; i < count; i++)
		{
			ok = ok && BufferAppend(buffer, headers[i].name, allocator) && BufferAppend(buffer, S8(": "), allocator);
			ok = ok && BufferAppend(buffer, headers[i].value, allocator) && BufferAppend(buffer, S8("\r\n"), allocator);
		}
		return ok;
	}



	//- Server

	enum HttpConnectionState
	{
		HttpConnectionStateFree,    // No connection
		HttpConnectionStateReading, // Reading a request
		HttpConnectionStateReady,   // Request complete, not yet handed out
		HttpConnectionStateHeld,    // Request handed out, waiting for HttpRespond
		HttpConnectionStateWriting, // Sending a response
		HttpConnectionStateClosing, // Sent the last response; reading what's left until the other end closes
	};

	// ms a closing connection waits for the other end
	static constexpr u32 HttpClosingTimeout = 2000;

	// Kept for the next connection once closed, buffers included
	struct HttpConnection
	{
		Socket socket;
		Address from;
		HttpConnectionState state;
		u32 generation; // Counts closes, so a stale request's handle no longer matches
		Timepoint lastProgress;

		HttpBuffer input;
		HttpBuffer output;
		u64 outputSent;
		b8 closeAfterWrite;

		// The request being read or held
		b8 continueSent;
		b8 keepAlive;
		u64 requestSize; // Input bytes it takes, once complete
		HttpMethod method;
		s8 path;
		s8 query;
		s8 body;
		HttpHeader headers[TOOL_HTTP_HEADER_MAX];
		u32 headerCount;
	};

	struct HttpServerData
	{
		MemoryAllocator allocator;
		Socket listener;
		u64 maxRequestSize;
		u32 nextHandOut; // Round robin, so one busy connection can't starve the rest
		HttpConnection* connections[TOOL_HTTP_CONNECTION_MAX];
	};

	static void ServerCloseConnection(HttpConnection* connection)
	{
		SocketClose(connection->socket);
		connection->socket = 0;
		connection->state = HttpConnectionStateFree;
		connection->generation++;
		connection->input.size = 0;
		connection->output.size = 0;
	}

	// Queues a response. False if out of memory, the connection then closed.
	static b8 ServerWrite(HttpServerData* server, HttpConnection* connection, u32 status, s8 contentType, s8 body, const HttpHeader* headers, u32 headerCount, b8 close)
	{
		MemoryAllocator allocator = server->allocator;
		HttpBuffer* out = &connection->output;
		out->size = 0;
		connection->outputSent = 0;

		// RFC 9110 6.4.1: no body, nor a length, for these
		b8 bodyless = status < 200 || status == 204 || status == 304;
		b8 sendBody = !bodyless && connection->method != HttpMethodHead;

		b8 ok = BufferAppend(out, S8("HTTP/1.1 "), allocator) && BufferAppendNumber(out, status, allocator);
		ok = ok && BufferAppend(out, S8(" "), allocator) && BufferAppend(out, HttpStatusText(status), allocator) && BufferAppend(out, S8("\r\n"), allocator);
		if (contentType.size > 0)
			ok = ok && BufferAppend(out, S8("Content-Type: "), allocator) && BufferAppend(out, contentType, allocator) && BufferAppend(out, S8("\r\n"), allocator);
		if (!bodyless)
			ok = ok && BufferAppend(out, S8("Content-Length: "), allocator) && BufferAppendNumber(out, body.size, allocator) && BufferAppend(out, S8("\r\n"), allocator);
		if (close)
			ok = ok && BufferAppend(out, S8("Connection: close\r\n"), allocator);
		ok = ok && BufferAppendHeaders(out, headers, headerCount, allocator) && BufferAppend(out, S8("\r\n"), allocator);
		if (sendBody)
			ok = ok && BufferAppend(out, body, allocator);

		if (!ok)
		{
			ServerCloseConnection(connection);
			return false;
		}

		connection->closeAfterWrite = close;
		connection->state = HttpConnectionStateWriting;
		connection->lastProgress = TimepointNow();
		return true;
	}

	// Answers a request the server refuses, and closes once it's sent
	static void ServerRefuse(HttpServerData* server, HttpConnection* connection, u32 status)
	{
		connection->method = HttpMethodNone;
		ServerWrite(server, connection, status, S8("text/plain"), HttpStatusText(status), nullptr, 0, true);
	}

	static void ServerParse(HttpServerData* server, HttpConnection* connection);

	// Sends what it can. Once the response is out, closes or moves on to the next request.
	static void ServerSend(HttpServerData* server, HttpConnection* connection)
	{
		HttpBuffer* out = &connection->output;
		while (connection->outputSent < out->size)
		{
			u64 sent;
			if (!TcpSend(connection->socket, out->data + connection->outputSent, out->size - connection->outputSent, &sent))
			{
				ServerCloseConnection(connection);
				return;
			}
			if (sent == 0)
				return;
			connection->outputSent += sent;
			connection->lastProgress = TimepointNow();
		}

		if (connection->closeAfterWrite)
		{
			// Reading on until the other end closes, so unread data doesn't reset the connection before the
			// response arrives
			TcpShutdown(connection->socket);
			connection->input.size = 0;
			connection->state = HttpConnectionStateClosing;
			return;
		}

		BufferDrop(&connection->input, connection->requestSize);
		connection->state = HttpConnectionStateReading;
		ServerParse(server, connection);
	}

	// Moves a Reading connection to Ready once its request is in, or refuses it
	static void ServerParse(HttpServerData* server, HttpConnection* connection)
	{
		HttpBuffer* in = &connection->input;

		// Empty lines before a request are ignored (RFC 9112 2.2)
		u64 skip = 0;
		while (skip < in->size && (in->data[skip] == '\r' || in->data[skip] == '\n'))
			skip++;
		if (skip > 0)
			BufferDrop(in, skip);

		u64 headSize = HeadSize(in->data, in->size);
		if (headSize == 0)
		{
			if (in->size >= server->maxRequestSize)
				ServerRefuse(server, connection, 431);
			else if (in->size == 0)
				connection->continueSent = false;
			return;
		}
		if (headSize > server->maxRequestSize)
			return ServerRefuse(server, connection, 431);

		// The request line: method, target, version
		s8 head = { (c8*)in->data, headSize };
		s8 line;
		s8 methodText;
		s8 rest;
		s8 target;
		s8 version;
		NextLine(&head, &line);
		if (!S8Cut(line, S8(" "), &methodText, &rest) || !S8Cut(rest, S8(" "), &target, &version) || target.size == 0)
			return ServerRefuse(server, connection, 400);

		if (S8Equal(version, S8("HTTP/1.1")))
			connection->keepAlive = true;
		else if (S8Equal(version, S8("HTTP/1.0")))
			connection->keepAlive = false;
		else
			return ServerRefuse(server, connection, S8StartsWith(version, S8("HTTP/")) ? 505 : 400);

		HttpMethod method = HttpMethodNone;
		for (u32 i = 1; i < sizeof(HttpMethodNames) / sizeof(HttpMethodNames[0]); i++)
			if (S8Equal(methodText, HttpMethodNames[i]))
				method = (HttpMethod)i;
		if (method == HttpMethodNone)
			return ServerRefuse(server, connection, 501);

		// Origin form "/path?query", "*", or absolute form "http://host/path?query"
		s8 path = target;
		s8 query = {};
		if (target.str[0] == '/')
		{
			S8Cut(target, S8("?"), &path, &query);
		}
		else if (!S8Equal(target, S8("*")))
		{
			Url url;
			if (!UrlParse(target, &url))
				return ServerRefuse(server, connection, 400);
			path = url.path.size > 0 ? url.path : S8("/");
			query = url.query;
		}

		u32 headerCount = 0;
		u32 refusal = ParseHeaders(&head, connection->headers, &headerCount);
		if (refusal != 0)
			return ServerRefuse(server, connection, refusal);

		s8 connectionHeader = HttpHeaderFind(connection->headers, headerCount, S8("Connection"));
		if (ListHasToken(connectionHeader, S8("close")))
			connection->keepAlive = false;
		else if (ListHasToken(connectionHeader, S8("keep-alive")))
			connection->keepAlive = true;

		// Requests without a length have no body
		HttpFraming framing;
		u64 length;
		refusal = ParseFraming(connection->headers, headerCount, &framing, &length);
		if (refusal != 0)
			return ServerRefuse(server, connection, refusal);
		if (framing == HttpFramingLength && length > server->maxRequestSize - headSize)
			return ServerRefuse(server, connection, 413);

		u8* bodyStart = in->data + headSize;
		u64 available = in->size - headSize;
		s8 body = { (c8*)bodyStart, 0 };
		u64 requestSize = headSize;
		b8 complete = true;
		if (framing == HttpFramingLength)
		{
			complete = available >= length;
			body.size = length;
			requestSize += length;
		}
		else if (framing == HttpFramingChunked)
		{
			u64 size;
			u64 used;
			i32 result = Dechunk(bodyStart, available, false, &size, &used);
			if (result < 0)
				return ServerRefuse(server, connection, 400);
			complete = result > 0;
			if (complete)
			{
				Dechunk(bodyStart, available, true, &size, &used);
				body.size = size;
				requestSize += used;
			}
			else if (in->size >= server->maxRequestSize)
			{
				return ServerRefuse(server, connection, 413);
			}
		}

		if (!complete)
		{
			// The client may wait for this before sending the body (RFC 9110 10.1.1)
			s8 expect = HttpHeaderFind(connection->headers, headerCount, S8("Expect"));
			if (!connection->continueSent && S8EqualIgnoreCase(expect, S8("100-continue")))
			{
				static constexpr c8 interim[] = "HTTP/1.1 100 Continue\r\n\r\n";
				u64 sent;
				if (!TcpSend(connection->socket, interim, sizeof(interim) - 1, &sent) || sent != sizeof(interim) - 1)
					return ServerCloseConnection(connection);
				connection->continueSent = true;
			}
			return;
		}

		connection->method = method;
		connection->path = path;
		connection->query = query;
		connection->body = body;
		connection->headerCount = headerCount;
		connection->requestSize = requestSize;
		connection->continueSent = false;
		connection->state = HttpConnectionStateReady;
	}

	// Reads what's there, then parses
	static void ServerReceive(HttpServerData* server, HttpConnection* connection)
	{
		HttpBuffer* in = &connection->input;
		for (;;)
		{
			if (in->size >= server->maxRequestSize)
				break;
			if (in->size == in->capacity && !BufferReserve(in, in->size + 1, server->allocator))
				return ServerCloseConnection(connection);

			u64 limit = in->capacity < server->maxRequestSize ? in->capacity : server->maxRequestSize;
			u64 received;
			if (!TcpReceive(connection->socket, in->data + in->size, limit - in->size, &received))
			{
				// The other end may stop sending once its request is out
				ServerParse(server, connection);
				if (connection->state == HttpConnectionStateReady)
					connection->keepAlive = false;
				else if (connection->state == HttpConnectionStateReading)
					ServerCloseConnection(connection);
				return;
			}
			if (received == 0)
				break;
			in->size += received;
			connection->lastProgress = TimepointNow();
		}
		ServerParse(server, connection);
	}

	// Discards what a closing connection sends, closing it at its end
	static void ServerDrain(HttpConnection* connection)
	{
		u8 discard[4096];
		for (;;)
		{
			u64 received;
			if (!TcpReceive(connection->socket, discard, sizeof(discard), &received))
				return ServerCloseConnection(connection);
			if (received == 0)
				return;
			connection->lastProgress = TimepointNow();
		}
	}

	static void ServerAccept(HttpServerData* server)
	{
		for (u32 i = 0; i < TOOL_HTTP_CONNECTION_MAX; i++)
		{
			HttpConnection* connection = server->connections[i];
			if (connection != nullptr && connection->state != HttpConnectionStateFree)
				continue;

			Socket socket;
			Address from;
			if (!TcpAccept(server->listener, &socket, &from))
				return;

			if (connection == nullptr)
			{
				connection = (HttpConnection*)AllocatorAlloc(server->allocator, sizeof(HttpConnection));
				if (connection == nullptr)
				{
					SocketClose(socket);
					TOOL_FAIL("Out of memory for an HTTP connection");
					return;
				}
				memset(connection, 0, sizeof(HttpConnection));
				server->connections[i] = connection;
			}

			connection->socket = socket;
			connection->from = from;
			connection->state = HttpConnectionStateReading;
			connection->lastProgress = TimepointNow();
			connection->input.size = 0;
			connection->output.size = 0;
			connection->continueSent = false;
		}
	}

	// Closes connections that stopped making progress
	static void ServerExpire(HttpServerData* server)
	{
		Timepoint now = TimepointNow();
		for (u32 i = 0; i < TOOL_HTTP_CONNECTION_MAX; i++)
		{
			HttpConnection* connection = server->connections[i];
			if (connection == nullptr)
				continue;

			u32 timeout;
			switch (connection->state)
			{
			case HttpConnectionStateReading:
			case HttpConnectionStateWriting:
				timeout = TOOL_HTTP_IDLE_TIMEOUT;
				break;
			case HttpConnectionStateClosing:
				timeout = HttpClosingTimeout;
				break;
			default:
				continue;
			}

			if (NanosecondsFromTo(connection->lastProgress, now) > (i64)timeout * 1000000)
				ServerCloseConnection(connection);
		}
	}

	static b8 ServerHandOut(HttpServerData* server, HttpRequest* outRequest)
	{
		for (u32 n = 0; n < TOOL_HTTP_CONNECTION_MAX; n++)
		{
			u32 i = (server->nextHandOut + n) % TOOL_HTTP_CONNECTION_MAX;
			HttpConnection* connection = server->connections[i];
			if (connection == nullptr || connection->state != HttpConnectionStateReady)
				continue;

			connection->state = HttpConnectionStateHeld;
			server->nextHandOut = i + 1;

			outRequest->method = connection->method;
			outRequest->path = connection->path;
			outRequest->query = connection->query;
			outRequest->body = connection->body;
			outRequest->headers = connection->headers;
			outRequest->headerCount = connection->headerCount;
			outRequest->from = connection->from;
			outRequest->handle = ((u64)connection->generation << 32) | (u64)(i + 1);
			return true;
		}
		return false;
	}

	b8 HttpServerOpen(HttpServer* server, const Address* address, MemoryAllocator allocator, u64 maxRequestSize)
	{
		*server = {};

		HttpServerData* data = (HttpServerData*)AllocatorAlloc(allocator, sizeof(HttpServerData));
		if (data == nullptr)
			return TOOL_FAIL("Out of memory for an HTTP server");
		memset(data, 0, sizeof(HttpServerData));
		data->allocator = allocator;
		data->maxRequestSize = maxRequestSize < 1024 ? 1024 : maxRequestSize;

		if (!TcpListen(&data->listener, address))
		{
			AllocatorDealloc(allocator, data);
			return false;
		}

		server->handle = (u64)data;
		return true;
	}

	b8 HttpServerNext(HttpServer* server, HttpRequest* outRequest, u32 timeout)
	{
		*outRequest = {};
		HttpServerData* data = (HttpServerData*)server->handle;
		if (data == nullptr)
			return false;

		Timepoint start = TimepointNow();
		for (;;)
		{
			if (ServerHandOut(data, outRequest))
				return true;

			SocketPoll polls[TOOL_SOCKET_POLL_MAX];
			HttpConnection* owners[TOOL_SOCKET_POLL_MAX];
			u32 count = 0;
			b8 room = false;
			for (u32 i = 0; i < TOOL_HTTP_CONNECTION_MAX; i++)
			{
				HttpConnection* connection = data->connections[i];
				if (connection == nullptr || connection->state == HttpConnectionStateFree)
				{
					room = true;
					continue;
				}

				u32 wait;
				if (connection->state == HttpConnectionStateReading || connection->state == HttpConnectionStateClosing)
					wait = SocketPollFlagsRead;
				else if (connection->state == HttpConnectionStateWriting)
					wait = SocketPollFlagsWrite;
				else
					continue;

				polls[count] = { connection->socket, wait, 0 };
				owners[count++] = connection;
			}
			if (room)
			{
				polls[count] = { data->listener, SocketPollFlagsRead, 0 };
				owners[count++] = nullptr;
			}

			// Only HttpRespond can change anything
			if (count == 0)
				return false;

			i64 elapsed = NanosecondsSince(start) / 1000000;
			u32 wait = elapsed >= (i64)timeout ? 0 : (u32)((i64)timeout - elapsed);
			SocketsPoll(polls, count, wait);

			for (u32 i = 0; i < count; i++)
			{
				if (polls[i].ready == 0)
					continue;

				HttpConnection* connection = owners[i];
				if (connection == nullptr)
					ServerAccept(data);
				else if (connection->state == HttpConnectionStateReading)
					ServerReceive(data, connection);
				else if (connection->state == HttpConnectionStateWriting)
					ServerSend(data, connection);
				else if (connection->state == HttpConnectionStateClosing)
					ServerDrain(connection);
			}
			ServerExpire(data);

			if (ServerHandOut(data, outRequest))
				return true;
			if (wait == 0)
				return false;
		}
	}

	// The connection holding 'request', null if it's stale
	static HttpConnection* ServerHeld(HttpServerData* server, const HttpRequest* request)
	{
		u32 index = (u32)(request->handle & 0xFFFFFFFF);
		if (server == nullptr || index == 0 || index > TOOL_HTTP_CONNECTION_MAX)
			return nullptr;

		HttpConnection* connection = server->connections[index - 1];
		if (connection == nullptr || connection->generation != (u32)(request->handle >> 32) || connection->state != HttpConnectionStateHeld)
			return nullptr;
		return connection;
	}

	void HttpRespond(HttpServer* server, const HttpRequest* request, u32 status, s8 contentType, s8 body, const HttpHeader* headers, u32 headerCount)
	{
		HttpServerData* data = (HttpServerData*)server->handle;
		HttpConnection* connection = ServerHeld(data, request);
		if (connection == nullptr)
			return;

		if (ServerWrite(data, connection, status, contentType, body, headers, headerCount, !connection->keepAlive))
			ServerSend(data, connection);
	}

	b8 HttpServerLocalAddress(HttpServer* server, Address* outAddress)
	{
		HttpServerData* data = (HttpServerData*)server->handle;
		if (data == nullptr)
		{
			*outAddress = {};
			return TOOL_FAIL("The HTTP server isn't open");
		}
		return SocketLocalAddress(data->listener, outAddress);
	}

	void HttpServerClose(HttpServer* server)
	{
		HttpServerData* data = (HttpServerData*)server->handle;
		if (data == nullptr)
			return;

		for (u32 i = 0; i < TOOL_HTTP_CONNECTION_MAX; i++)
		{
			HttpConnection* connection = data->connections[i];
			if (connection == nullptr)
				continue;
			SocketClose(connection->socket);
			BufferFree(&connection->input, data->allocator);
			BufferFree(&connection->output, data->allocator);
			AllocatorDealloc(data->allocator, connection);
		}
		SocketClose(data->listener);
		AllocatorDealloc(data->allocator, data);
		*server = {};
	}



	//- Client

	enum HttpClientPhase
	{
		HttpClientPhaseSending,   // Connecting and sending the request
		HttpClientPhaseReceiving, // Reading the response
		HttpClientPhaseDone,      // Response in
		HttpClientPhaseFailed,    // Gave up; recorded once
	};

	struct HttpClientData
	{
		MemoryAllocator allocator;
		HttpClientPhase phase;
		b8 head; // HEAD request: the response has no body
		HttpBuffer output;
		HttpBuffer input;
		HttpHeader headers[TOOL_HTTP_HEADER_MAX];
#if TOOL_WEB
		i32 fetch;
#else
		Socket socket;
		u64 outputSent;
#endif
	};

	static void ClientFree(HttpClientData* data)
	{
		MemoryAllocator allocator = data->allocator;
		BufferFree(&data->output, allocator);
		BufferFree(&data->input, allocator);
		AllocatorDealloc(allocator, data);
	}

	static HttpClientState ClientState(HttpClientData* data)
	{
		if (data->phase == HttpClientPhaseDone)
			return HttpClientStateDone;
		if (data->phase == HttpClientPhaseFailed)
			return HttpClientStateFailed;
		return HttpClientStatePending;
	}



	//- WebSocket

	enum WebSocketPhase
	{
		WebSocketPhaseHandshake, // Client: waiting for the server's 101
		WebSocketPhaseOpen,      // Exchanging messages
		WebSocketPhaseClosed,    // Ended, Closed not yet given
		WebSocketPhaseEnded,     // Closed given
	};

	struct WebSocketData
	{
		MemoryAllocator allocator;
		WebSocketPhase phase;
		u64 maxMessageSize;
		HttpBuffer message; // Joined fragments, or Closed's reason
#if TOOL_WEB
		i32 id;
#else
		Socket socket;
		b8 client;           // Masks what it sends
		RandomStream random; // Client: masking keys
		c8 accept[28];       // Client: the Sec-WebSocket-Accept expected
		u64 handshakeSize;   // Client: output bytes sendable before the handshake completes
		HttpBuffer input;
		u64 consumed; // Input bytes given out as a message, dropped at the next call
		HttpBuffer output;
		u64 outputSent;
		b8 closeSent;
		b8 fragmenting;
		WebSocketMessageType fragmentType;
#endif
	};

	static WebSocketData* WebSocketCreate(MemoryAllocator allocator, u64 maxMessageSize)
	{
		WebSocketData* data = (WebSocketData*)AllocatorAlloc(allocator, sizeof(WebSocketData));
		if (data == nullptr)
		{
			TOOL_FAIL("Out of memory for a WebSocket");
			return nullptr;
		}
		memset(data, 0, sizeof(WebSocketData));
		data->allocator = allocator;
		data->maxMessageSize = maxMessageSize;
		return data;
	}
} //namespace Tool



#if TOOL_WEB

//- Client on the web, through fetch()

EM_JS_DEPS(ToolHttp, "$UTF8ToString,$stringToUTF8,$lengthBytesUTF8");

// 'headers' as "Name: value\r\n" lines. Returns the fetch's id.
EM_JS(Tool::i32, HttpFetchStart, (const Tool::c8* method, const Tool::c8* url, const Tool::c8* headers, const void* body, Tool::u32 bodySize), {
	var http = Module.toolHttp || (Module.toolHttp = { next: 1, fetches: {} });
	var id = http.next++;
	var entry = { state: 0, status: 0, headers: "", body: null, error: "", abort: new AbortController() };
	http.fetches[id] = entry;

	var init = { method: UTF8ToString(method), headers: [], signal: entry.abort.signal };
	UTF8ToString(headers).split("\r\n").forEach(function(line) {
		var colon = line.indexOf(":");
		if (colon > 0)
			init.headers.push([line.substring(0, colon), line.substring(colon + 1).trim()]);
	});
	if (bodySize > 0)
		init.body = HEAPU8.slice(body, body + bodySize);

	fetch(UTF8ToString(url), init).then(function(response) {
		entry.status = response.status;
		response.headers.forEach(function(value, name) { entry.headers += name + ": " + value + "\r\n"; });
		return response.arrayBuffer();
	}).then(function(buffer) {
		entry.body = new Uint8Array(buffer);
		entry.state = 1;
	}).catch(function(error) {
		entry.error = String(error);
		entry.state = 2;
	});
	return id;
});

// 0 under way, 1 done, 2 failed
EM_JS(Tool::i32, HttpFetchState, (Tool::i32 id), {
	return Module.toolHttp.fetches[id].state;
});

EM_JS(Tool::u32, HttpFetchStatus, (Tool::i32 id), {
	return Module.toolHttp.fetches[id].status;
});

// Bytes the headers take, terminator included
EM_JS(Tool::u32, HttpFetchHeadersSize, (Tool::i32 id), {
	return lengthBytesUTF8(Module.toolHttp.fetches[id].headers) + 1;
});

EM_JS(Tool::u32, HttpFetchBodySize, (Tool::i32 id), {
	return Module.toolHttp.fetches[id].body.length;
});

EM_JS(void, HttpFetchCopy, (Tool::i32 id, Tool::c8* headers, Tool::u32 headersSize, Tool::u8* body), {
	var entry = Module.toolHttp.fetches[id];
	stringToUTF8(entry.headers, headers, headersSize);
	HEAPU8.set(entry.body, body);
});

EM_JS(void, HttpFetchError, (Tool::i32 id, Tool::c8* text, Tool::u32 capacity), {
	stringToUTF8(Module.toolHttp.fetches[id].error, text, capacity);
});

// Aborts one under way
EM_JS(void, HttpFetchEnd, (Tool::i32 id), {
	var entry = Module.toolHttp.fetches[id];
	if (entry.state == 0)
		entry.abort.abort();
	delete Module.toolHttp.fetches[id];
});

namespace Tool
{
	b8 HttpClientSend(HttpClient* client, HttpMethod method, s8 url, s8 body, MemoryAllocator allocator, const HttpHeader* headers, u32 headerCount)
	{
		*client = {};
		if (method <= HttpMethodNone || method > HttpMethodOptions)
			return TOOL_FAIL("Not an HTTP method");

		HttpClientData* data = (HttpClientData*)AllocatorAlloc(allocator, sizeof(HttpClientData));
		if (data == nullptr)
			return TOOL_FAIL("Out of memory for an HTTP request");
		memset(data, 0, sizeof(HttpClientData));
		data->allocator = allocator;
		data->head = method == HttpMethodHead;

		// One buffer: the method, the URL and the header lines, each terminated
		HttpBuffer* out = &data->output;
		b8 ok = BufferAppend(out, HttpMethodNames[method], allocator) && BufferAppend(out, "", 1, allocator);
		ok = ok && BufferAppend(out, url, allocator) && BufferAppend(out, "", 1, allocator);
		ok = ok && BufferAppendHeaders(out, headers, headerCount, allocator) && BufferAppend(out, "", 1, allocator);
		if (!ok)
		{
			ClientFree(data);
			return false;
		}

		const c8* methodText = (const c8*)out->data;
		const c8* urlText = methodText + HttpMethodNames[method].size + 1;
		const c8* headerText = urlText + url.size + 1;
		data->fetch = HttpFetchStart(methodText, urlText, headerText, body.str, (u32)body.size);
		client->handle = (u64)data;
		return true;
	}

	HttpClientState HttpClientPoll(HttpClient* client, u32)
	{
		HttpClientData* data = (HttpClientData*)client->handle;
		if (data == nullptr)
			return HttpClientStateFailed;
		if (data->phase != HttpClientPhaseSending)
			return ClientState(data);

		i32 state = HttpFetchState(data->fetch);
		if (state == 0)
			return HttpClientStatePending;

		if (state == 2)
		{
			c8 reason[256];
			HttpFetchError(data->fetch, reason, sizeof(reason));
			TOOL_FAIL("fetch() failed: %s", reason);
			data->phase = HttpClientPhaseFailed;
			return HttpClientStateFailed;
		}

		// The headers as a head, its empty line added, then the body
		u32 headersSize = HttpFetchHeadersSize(data->fetch);
		u32 bodySize = HttpFetchBodySize(data->fetch);
		HttpBuffer* in = &data->input;
		if (!BufferReserve(in, (u64)headersSize + 2 + bodySize, data->allocator))
		{
			data->phase = HttpClientPhaseFailed;
			return HttpClientStateFailed;
		}
		u8* body = in->data + headersSize + 1;
		HttpFetchCopy(data->fetch, (c8*)in->data, headersSize, body);
		in->data[headersSize - 1] = '\r';
		in->data[headersSize] = '\n';

		s8 head = { (c8*)in->data, (u64)headersSize + 1 };
		u32 headerCount = 0;
		if (ParseHeaders(&head, data->headers, &headerCount) != 0)
		{
			TOOL_FAIL("fetch() gave unreadable headers");
			data->phase = HttpClientPhaseFailed;
			return HttpClientStateFailed;
		}

		client->status = HttpFetchStatus(data->fetch);
		client->body = { (c8*)body, bodySize };
		client->headers = data->headers;
		client->headerCount = headerCount;
		data->phase = HttpClientPhaseDone;
		return HttpClientStateDone;
	}

	void HttpClientClose(HttpClient* client)
	{
		HttpClientData* data = (HttpClientData*)client->handle;
		if (data == nullptr)
			return;
		HttpFetchEnd(data->fetch);
		ClientFree(data);
		*client = {};
	}
} //namespace Tool

//- WebSocket on the web, through the browser's

// Returns the socket's id. Messages wait in its inbox: type 0 text, 1 binary, 2 closed, 3 closed abnormally.
EM_JS(Tool::i32, WebSocketJsOpen, (const Tool::c8* url), {
	var sockets = Module.toolWebSockets || (Module.toolWebSockets = { next: 1, open: {} });
	var id = sockets.next++;
	var entry = { socket: null, inbox: [], outbox: [] };
	sockets.open[id] = entry;
	try {
		entry.socket = new WebSocket(UTF8ToString(url));
	} catch (error) {
		entry.inbox.push({ type: 3, data: new TextEncoder().encode(String(error)) });
		return id;
	}
	entry.socket.binaryType = "arraybuffer";
	entry.socket.onopen = function() {
		entry.outbox.forEach(function(message) { entry.socket.send(message); });
		entry.outbox = [];
	};
	entry.socket.onmessage = function(event) {
		if (typeof event.data === "string")
			entry.inbox.push({ type: 0, data: new TextEncoder().encode(event.data) });
		else
			entry.inbox.push({ type: 1, data: new Uint8Array(event.data) });
	};
	entry.socket.onclose = function(event) {
		entry.inbox.push({ type: event.wasClean ? 2 : 3, data: new TextEncoder().encode(event.reason) });
	};
	return id;
});

// 0 if closed
EM_JS(Tool::i32, WebSocketJsSend, (Tool::i32 id, Tool::i32 binary, const void* data, Tool::u32 size), {
	var socket = Module.toolWebSockets.open[id].socket;
	if (!socket || socket.readyState > 1)
		return 0;
	var bytes = HEAPU8.slice(data, data + size);
	var message = binary ? bytes : new TextDecoder().decode(bytes);
	if (socket.readyState == 0)
		Module.toolWebSockets.open[id].outbox.push(message);
	else
		socket.send(message);
	return 1;
});

// The next message's size, -1 if none
EM_JS(Tool::i32, WebSocketJsPeek, (Tool::i32 id), {
	var inbox = Module.toolWebSockets.open[id].inbox;
	return inbox.length > 0 ? inbox[0].data.length : -1;
});

// Copies out the next message, returning its type
EM_JS(Tool::i32, WebSocketJsTake, (Tool::i32 id, Tool::u8* destination), {
	var message = Module.toolWebSockets.open[id].inbox.shift();
	HEAPU8.set(message.data, destination);
	return message.type;
});

EM_JS(void, WebSocketJsClose, (Tool::i32 id), {
	var socket = Module.toolWebSockets.open[id].socket;
	if (socket && socket.readyState < 2)
		socket.close(1000);
	delete Module.toolWebSockets.open[id];
});

namespace Tool
{
	b8 WebSocketConnect(WebSocket* socket, s8 url, MemoryAllocator allocator, u64 maxMessageSize)
	{
		*socket = {};
		Url parsed;
		if (!UrlParse(url, &parsed) || (!S8EqualIgnoreCase(parsed.scheme, S8("ws")) && !S8EqualIgnoreCase(parsed.scheme, S8("wss"))))
			return TOOL_FAIL("Not a ws:// or wss:// URL: %.*s", (i32)url.size, url.str);

		WebSocketData* data = WebSocketCreate(allocator, maxMessageSize);
		if (data == nullptr)
			return false;

		// The URL, terminated
		if (!BufferAppend(&data->message, url, allocator) || !BufferAppend(&data->message, "", 1, allocator))
		{
			BufferFree(&data->message, allocator);
			AllocatorDealloc(allocator, data);
			return false;
		}
		data->id = WebSocketJsOpen((const c8*)data->message.data);
		data->message.size = 0;
		data->phase = WebSocketPhaseOpen;
		socket->handle = (u64)data;
		return true;
	}

	b8 WebSocketAccept(WebSocket* socket, HttpServer*, const HttpRequest*, MemoryAllocator, u64)
	{
		*socket = {};
		return TOOL_FAIL("No WebSocket server on the web");
	}

	b8 WebSocketSend(WebSocket* socket, WebSocketMessageType type, s8 data)
	{
		WebSocketData* socketData = (WebSocketData*)socket->handle;
		if (socketData == nullptr || socketData->phase != WebSocketPhaseOpen || type == WebSocketMessageTypeClosed)
			return TOOL_FAIL("The WebSocket is closed");
		if (!WebSocketJsSend(socketData->id, type == WebSocketMessageTypeBinary, data.str, (u32)data.size))
			return TOOL_FAIL("The WebSocket is closed");
		return true;
	}

	b8 WebSocketNext(WebSocket* socket, WebSocketMessage* outMessage, u32)
	{
		*outMessage = {};
		WebSocketData* data = (WebSocketData*)socket->handle;
		if (data == nullptr || data->phase == WebSocketPhaseEnded)
			return false;

		i32 size = WebSocketJsPeek(data->id);
		if (size < 0)
			return false;

		HttpBuffer* message = &data->message;
		message->size = 0;
		if (!BufferReserve(message, (u64)size + 1, data->allocator))
			return false;
		i32 type = WebSocketJsTake(data->id, message->data);
		message->size = (u64)size;

		if (type >= 2 || (u64)size > data->maxMessageSize)
		{
			if (type == 3)
				TOOL_FAIL("The WebSocket closed abnormally");
			else if (type < 2)
				TOOL_FAIL("WebSocket message too large");
			if (type < 2)
				message->size = 0;
			data->phase = WebSocketPhaseEnded;
			outMessage->type = WebSocketMessageTypeClosed;
		}
		else
		{
			outMessage->type = type == 0 ? WebSocketMessageTypeText : WebSocketMessageTypeBinary;
		}
		outMessage->data = { (c8*)message->data, message->size };
		return true;
	}

	void WebSocketClose(WebSocket* socket)
	{
		WebSocketData* data = (WebSocketData*)socket->handle;
		if (data == nullptr)
			return;
		WebSocketJsClose(data->id);
		BufferFree(&data->message, data->allocator);
		AllocatorDealloc(data->allocator, data);
		*socket = {};
	}
} //namespace Tool

#else

//- Client over sockets

namespace Tool
{
	static HttpClientState ClientFail(HttpClientData* data)
	{
		SocketClose(data->socket);
		data->socket = 0;
		data->phase = HttpClientPhaseFailed;
		return HttpClientStateFailed;
	}

	// Parses what's in; at the end of the stream with 'ended'
	static void ClientParse(HttpClient* client, HttpClientData* data, b8 ended)
	{
		HttpBuffer* in = &data->input;
		for (;;)
		{
			u64 headSize = HeadSize(in->data, in->size);
			if (headSize == 0)
			{
				if (ended)
				{
					TOOL_FAIL("The HTTP connection closed before a response");
					ClientFail(data);
				}
				return;
			}

			// "HTTP/1.x 200 OK"
			s8 head = { (c8*)in->data, headSize };
			s8 line;
			s8 version;
			s8 rest;
			s8 code;
			u64 status;
			NextLine(&head, &line);
			S8Cut(line, S8(" "), &version, &rest);
			S8Cut(rest, S8(" "), &code, nullptr);
			if (!S8StartsWith(version, S8("HTTP/1.")) || code.size != 3 || !ParseDecimal(code, 999, &status) || status < 100)
			{
				TOOL_FAIL("Not an HTTP response");
				ClientFail(data);
				return;
			}

			// Interim responses come before the real one
			if (status < 200 && status != 101)
			{
				BufferDrop(in, headSize);
				continue;
			}

			u32 headerCount = 0;
			HttpFraming framing;
			u64 length;
			u32 problem = ParseHeaders(&head, data->headers, &headerCount);
			if (problem == 0)
				problem = ParseFraming(data->headers, headerCount, &framing, &length);
			if (problem != 0)
			{
				TOOL_FAIL(problem == 501 ? "Unsupported HTTP transfer coding" : "Malformed HTTP response headers");
				ClientFail(data);
				return;
			}

			// RFC 9112 6.3: no body for these, whatever the headers say; until close without a length
			if (data->head || status < 200 || status == 204 || status == 304)
				framing = HttpFramingNone;
			else if (framing == HttpFramingNone)
				framing = HttpFramingClose;

			u8* bodyStart = in->data + headSize;
			u64 available = in->size - headSize;
			u64 bodySize = 0;
			if (framing == HttpFramingLength)
			{
				if (available < length)
				{
					if (ended)
					{
						TOOL_FAIL("The HTTP connection closed during the response");
						ClientFail(data);
					}
					return;
				}
				bodySize = length;
			}
			else if (framing == HttpFramingChunked)
			{
				u64 used;
				i32 result = Dechunk(bodyStart, available, false, &bodySize, &used);
				if (result < 0 || (result == 0 && ended))
				{
					TOOL_FAIL(result < 0 ? "Malformed chunked HTTP response" : "The HTTP connection closed during the response");
					ClientFail(data);
					return;
				}
				if (result == 0)
					return;
				Dechunk(bodyStart, available, true, &bodySize, &used);
			}
			else if (framing == HttpFramingClose)
			{
				if (!ended)
					return;
				bodySize = available;
			}

			client->status = (u32)status;
			client->body = { (c8*)bodyStart, bodySize };
			client->headers = data->headers;
			client->headerCount = headerCount;
			SocketClose(data->socket);
			data->socket = 0;
			data->phase = HttpClientPhaseDone;
			return;
		}
	}

	// Does all it can without waiting
	static void ClientStep(HttpClient* client, HttpClientData* data)
	{
		if (data->phase == HttpClientPhaseSending)
		{
			HttpBuffer* out = &data->output;
			while (data->outputSent < out->size)
			{
				u64 sent;
				if (!TcpSend(data->socket, out->data + data->outputSent, out->size - data->outputSent, &sent))
				{
					ClientFail(data);
					return;
				}
				if (sent == 0)
					return;
				data->outputSent += sent;
			}
			data->phase = HttpClientPhaseReceiving;
		}

		HttpBuffer* in = &data->input;
		while (data->phase == HttpClientPhaseReceiving)
		{
			if (in->capacity - in->size < 4096 && !BufferReserve(in, in->size + 4096, data->allocator))
			{
				ClientFail(data);
				return;
			}

			u64 received;
			if (!TcpReceive(data->socket, in->data + in->size, in->capacity - in->size, &received))
			{
				ClientParse(client, data, true);
				return;
			}
			if (received == 0)
				return;
			in->size += received;
			ClientParse(client, data, false);
		}
	}

	// The URL parsed, its scheme checked, and its host made an address: literal, or looked up
	static b8 ClientResolve(s8 url, s8 scheme, Url* outUrl, Address* outAddress)
	{
		if (!UrlParse(url, outUrl))
			return TOOL_FAIL("Not a URL: %.*s", (i32)url.size, url.str);
		if (!S8EqualIgnoreCase(outUrl->scheme, scheme))
			return TOOL_FAIL("Only %s:// URLs are supported: %.*s", scheme.str, (i32)url.size, url.str);

		if (AddressParse(outUrl->host, outAddress))
		{
			outAddress->port = outUrl->port;
			return true;
		}

		c8 name[256];
		if (outUrl->host.size >= sizeof(name))
			return TOOL_FAIL("Host name too long: %.*s", (i32)url.size, url.str);
		memcpy(name, outUrl->host.str, outUrl->host.size);
		name[outUrl->host.size] = 0;
		return AddressResolve(name, outUrl->port, outAddress, 1) != 0;
	}

	// "GET /path?query HTTP/1.1" and Host, with brackets around IPv6 and the port unless 80
	static b8 BufferAppendRequestHead(HttpBuffer* out, HttpMethod method, const Url* url, MemoryAllocator allocator)
	{
		b8 bracket = S8Find(url->host, S8(":")) != S8_NONE;
		b8 ok = BufferAppend(out, HttpMethodNames[method], allocator) && BufferAppend(out, S8(" "), allocator);
		ok = ok && BufferAppend(out, url->path.size > 0 ? url->path : S8("/"), allocator);
		if (url->query.size > 0)
			ok = ok && BufferAppend(out, S8("?"), allocator) && BufferAppend(out, url->query, allocator);
		ok = ok && BufferAppend(out, S8(" HTTP/1.1\r\nHost: "), allocator);
		ok = ok && BufferAppend(out, bracket ? S8("[") : S8(""), allocator) && BufferAppend(out, url->host, allocator) && BufferAppend(out, bracket ? S8("]") : S8(""), allocator);
		if (url->port != 80)
			ok = ok && BufferAppend(out, S8(":"), allocator) && BufferAppendNumber(out, url->port, allocator);
		return ok && BufferAppend(out, S8("\r\n"), allocator);
	}

	b8 HttpClientSend(HttpClient* client, HttpMethod method, s8 url, s8 body, MemoryAllocator allocator, const HttpHeader* headers, u32 headerCount)
	{
		*client = {};
		if (method <= HttpMethodNone || method > HttpMethodOptions)
			return TOOL_FAIL("Not an HTTP method");

		Url parsed;
		Address address;
		if (!ClientResolve(url, S8("http"), &parsed, &address))
			return false;

		HttpClientData* data = (HttpClientData*)AllocatorAlloc(allocator, sizeof(HttpClientData));
		if (data == nullptr)
			return TOOL_FAIL("Out of memory for an HTTP request");
		memset(data, 0, sizeof(HttpClientData));
		data->allocator = allocator;
		data->head = method == HttpMethodHead;

		HttpBuffer* out = &data->output;
		b8 ok = BufferAppendRequestHead(out, method, &parsed, allocator) && BufferAppend(out, S8("Connection: close\r\n"), allocator);
		if (body.size > 0 || method == HttpMethodPost || method == HttpMethodPut || method == HttpMethodPatch)
			ok = ok && BufferAppend(out, S8("Content-Length: "), allocator) && BufferAppendNumber(out, body.size, allocator) && BufferAppend(out, S8("\r\n"), allocator);
		ok = ok && BufferAppendHeaders(out, headers, headerCount, allocator) && BufferAppend(out, S8("\r\n"), allocator);
		ok = ok && BufferAppend(out, body, allocator);
		if (!ok)
		{
			ClientFree(data);
			return false;
		}

		if (!TcpConnect(&data->socket, &address, 0))
		{
			ClientFree(data);
			return false;
		}

		client->handle = (u64)data;
		ClientStep(client, data);
		return true;
	}

	HttpClientState HttpClientPoll(HttpClient* client, u32 timeout)
	{
		HttpClientData* data = (HttpClientData*)client->handle;
		if (data == nullptr)
			return HttpClientStateFailed;

		Timepoint start = TimepointNow();
		for (;;)
		{
			ClientStep(client, data);
			if (data->phase == HttpClientPhaseDone || data->phase == HttpClientPhaseFailed)
				return ClientState(data);

			i64 elapsed = NanosecondsSince(start) / 1000000;
			if (elapsed >= (i64)timeout)
				return HttpClientStatePending;

			SocketPoll poll = { data->socket, data->phase == HttpClientPhaseSending ? (u32)SocketPollFlagsWrite : (u32)SocketPollFlagsRead, 0 };
			SocketsPoll(&poll, 1, (u32)((i64)timeout - elapsed));
		}
	}

	void HttpClientClose(HttpClient* client)
	{
		HttpClientData* data = (HttpClientData*)client->handle;
		if (data == nullptr)
			return;
		SocketClose(data->socket);
		ClientFree(data);
		*client = {};
	}



	//- WebSocket over sockets

	// Bytes allowed for the server's handshake response
	static constexpr u64 WebSocketHeadMax = 16384;

	static u32 Rotate(u32 value, u32 bits)
	{
		return (value << bits) | (value >> (32 - bits));
	}

	// Base64 (RFC 4648), padded. Writes 4 * ceil(size / 3) characters, no terminator.
	static void Base64(const u8* data, u32 size, c8* out)
	{
		static constexpr c8 alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		for (u32 i = 0; i < size; i += 3)
		{
			u32 left = size - i;
			u32 bits = (u32)data[i] << 16 | (left > 1 ? (u32)data[i + 1] << 8 : 0) | (left > 2 ? (u32)data[i + 2] : 0);
			*out++ = alphabet[(bits >> 18) & 63];
			*out++ = alphabet[(bits >> 12) & 63];
			*out++ = left > 1 ? alphabet[(bits >> 6) & 63] : '=';
			*out++ = left > 2 ? alphabet[bits & 63] : '=';
		}
	}

	// Sec-WebSocket-Accept for a 24-character key (RFC 6455 4.2.2): base64 of the SHA-1 of the key and a GUID
	static void WebSocketAcceptFor(s8 key, c8* out)
	{
		static constexpr c8 guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

		// 60 bytes of message, padded to two blocks with its length in bits
		u8 message[128] = {};
		memcpy(message, key.str, 24);
		memcpy(message + 24, guid, 36);
		message[60] = 0x80;
		message[126] = (u8)(480 >> 8);
		message[127] = (u8)(480 & 0xFF);

		u32 hash[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
		for (u32 block = 0; block < 2; block++)
		{
			u32 w[80];
			for (u32 i = 0; i < 16; i++)
			{
				const u8* p = message + block * 64 + i * 4;
				w[i] = (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | (u32)p[3];
			}
			for (u32 i = 16; i < 80; i++)
				w[i] = Rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

			u32 a = hash[0];
			u32 b = hash[1];
			u32 c = hash[2];
			u32 d = hash[3];
			u32 e = hash[4];
			for (u32 i = 0; i < 80; i++)
			{
				u32 f;
				u32 k;
				if (i < 20)
				{
					f = (b & c) | (~b & d);
					k = 0x5A827999;
				}
				else if (i < 40)
				{
					f = b ^ c ^ d;
					k = 0x6ED9EBA1;
				}
				else if (i < 60)
				{
					f = (b & c) | (b & d) | (c & d);
					k = 0x8F1BBCDC;
				}
				else
				{
					f = b ^ c ^ d;
					k = 0xCA62C1D6;
				}
				u32 next = Rotate(a, 5) + f + e + k + w[i];
				e = d;
				d = c;
				c = Rotate(b, 30);
				b = a;
				a = next;
			}
			hash[0] += a;
			hash[1] += b;
			hash[2] += c;
			hash[3] += d;
			hash[4] += e;
		}

		u8 digest[20];
		for (u32 i = 0; i < 20; i++)
			digest[i] = (u8)(hash[i / 4] >> (24 - 8 * (i % 4)));
		Base64(digest, 20, out);
	}

	struct WebSocketFrame
	{
		b8 fin;
		u8 opcode;
		u8* payload;
		u64 length;
		u64 size; // Header and payload
	};

	// One frame (RFC 6455 5.2): 1 parsed, its payload unmasked in place; 0 incomplete; -1 malformed; -2 beyond 'max'
	static i32 FrameParse(u8* data, u64 size, b8 masked, u64 max, WebSocketFrame* outFrame)
	{
		if (size < 2)
			return 0;

		u8 first = data[0];
		u8 second = data[1];
		u8 opcode = first & 0x0F;
		if ((first & 0x70) != 0 || ((second & 0x80) != 0) != masked)
			return -1;

		u64 length = second & 0x7F;
		u64 at = 2;
		if (length == 126)
		{
			if (size < 4)
				return 0;
			length = (u64)data[2] << 8 | data[3];
			at = 4;
		}
		else if (length == 127)
		{
			if (size < 10)
				return 0;
			length = 0;
			for (u32 i = 0; i < 8; i++)
				length = length << 8 | data[2 + i];
			if (length >> 63)
				return -1;
			at = 10;
		}

		// Control frames are whole and short
		if (opcode >= 8 && ((first & 0x80) == 0 || length > 125))
			return -1;
		if (length > max)
			return -2;

		u8 mask[4] = {};
		if (masked)
		{
			if (size < at + 4)
				return 0;
			memcpy(mask, data + at, 4);
			at += 4;
		}
		if (size - at < length)
			return 0;

		u8* payload = data + at;
		if (masked)
			for (u64 i = 0; i < length; i++)
				payload[i] ^= mask[i & 3];

		*outFrame = { (first & 0x80) != 0, opcode, payload, length, at + length };
		return 1;
	}

	static b8 WebSocketQueue(WebSocketData* data, u8 opcode, const void* payload, u64 size)
	{
		u8 header[14];
		u32 count = 0;
		u8 maskBit = data->client ? 0x80 : 0;
		header[count++] = (u8)(0x80 | opcode);
		if (size < 126)
		{
			header[count++] = (u8)(maskBit | size);
		}
		else if (size <= 0xFFFF)
		{
			header[count++] = (u8)(maskBit | 126);
			header[count++] = (u8)(size >> 8);
			header[count++] = (u8)size;
		}
		else
		{
			header[count++] = (u8)(maskBit | 127);
			for (i32 i = 7; i >= 0; i--)
				header[count++] = (u8)(size >> (8 * i));
		}

		u8 mask[4];
		if (data->client)
		{
			u32 key = RandomU32Next(&data->random);
			memcpy(mask, &key, 4);
			memcpy(header + count, mask, 4);
			count += 4;
		}

		HttpBuffer* out = &data->output;
		if (!BufferAppend(out, header, count, data->allocator) || !BufferAppend(out, payload, size, data->allocator))
			return false;
		if (data->client)
			for (u64 i = 0; i < size; i++)
				out->data[out->size - size + i] ^= mask[i & 3];
		return true;
	}

	static void WebSocketQueueClose(WebSocketData* data, u16 code)
	{
		if (data->closeSent)
			return;
		u8 payload[2] = { (u8)(code >> 8), (u8)code };
		WebSocketQueue(data, 8, payload, 2);
		data->closeSent = true;
	}

	// Sends what it can, during the client's handshake only the request. False if the connection failed.
	static b8 WebSocketSendOut(WebSocketData* data)
	{
		HttpBuffer* out = &data->output;
		u64 limit = data->phase == WebSocketPhaseHandshake ? data->handshakeSize : out->size;
		while (data->outputSent < limit)
		{
			u64 sent;
			if (!TcpSend(data->socket, out->data + data->outputSent, limit - data->outputSent, &sent))
				return false;
			if (sent == 0)
				return true;
			data->outputSent += sent;
		}
		if (data->outputSent == out->size)
		{
			out->size = 0;
			data->outputSent = 0;
			data->handshakeSize = 0;
		}
		return true;
	}

	// Sends what it can of what's queued, closes, and has Closed come next with 'reason'
	static void WebSocketEnd(WebSocketData* data, s8 reason)
	{
		data->message.size = 0;
		BufferAppend(&data->message, reason, data->allocator);
		if (data->phase != WebSocketPhaseHandshake)
			WebSocketSendOut(data);
		SocketClose(data->socket);
		data->socket = 0;
		data->phase = WebSocketPhaseClosed;
	}

	static void WebSocketFail(WebSocketData* data, u16 code, const c8* reason)
	{
		TOOL_FAIL("%s", reason);
		WebSocketQueueClose(data, code);
		WebSocketEnd(data, {});
	}

	// Reads what's there. False at the end of the stream.
	static b8 WebSocketReceive(WebSocketData* data)
	{
		HttpBuffer* in = &data->input;
		u64 max = data->maxMessageSize + WebSocketHeadMax;
		for (;;)
		{
			if (in->size >= max)
				return true;
			if (in->size == in->capacity && !BufferReserve(in, in->size + 1, data->allocator))
				return false;

			u64 limit = in->capacity < max ? in->capacity : max;
			u64 received;
			if (!TcpReceive(data->socket, in->data + in->size, limit - in->size, &received))
				return false;
			if (received == 0)
				return true;
			in->size += received;
		}
	}

	// The client's handshake: the server's 101 checked (RFC 6455 4.1)
	static void WebSocketHandshake(WebSocketData* data)
	{
		HttpBuffer* in = &data->input;
		u64 headSize = HeadSize(in->data, in->size);
		if (headSize == 0)
		{
			if (in->size >= WebSocketHeadMax)
				WebSocketFail(data, 1002, "WebSocket handshake response too large");
			return;
		}

		s8 head = { (c8*)in->data, headSize };
		s8 line;
		s8 version;
		s8 rest;
		s8 code;
		NextLine(&head, &line);
		S8Cut(line, S8(" "), &version, &rest);
		S8Cut(rest, S8(" "), &code, nullptr);

		HttpHeader headers[TOOL_HTTP_HEADER_MAX];
		u32 headerCount = 0;
		b8 ok = S8StartsWith(version, S8("HTTP/1.")) && S8Equal(code, S8("101")) && ParseHeaders(&head, headers, &headerCount) == 0;
		ok = ok && ListHasToken(HttpHeaderFind(headers, headerCount, S8("Upgrade")), S8("websocket"));
		ok = ok && ListHasToken(HttpHeaderFind(headers, headerCount, S8("Connection")), S8("upgrade"));
		ok = ok && S8Equal(HttpHeaderFind(headers, headerCount, S8("Sec-WebSocket-Accept")), { data->accept, 28 });
		if (!ok)
		{
			TOOL_FAIL("WebSocket handshake refused: %.*s", (i32)line.size, line.str);
			return WebSocketEnd(data, {});
		}

		BufferDrop(in, headSize);
		data->phase = WebSocketPhaseOpen;
	}

	// Handles what's in. True with a message to give.
	static b8 WebSocketProcess(WebSocketData* data, WebSocketMessage* outMessage)
	{
		if (data->phase == WebSocketPhaseHandshake)
			WebSocketHandshake(data);

		HttpBuffer* in = &data->input;
		HttpBuffer* message = &data->message;
		while (data->phase == WebSocketPhaseOpen)
		{
			WebSocketFrame frame;
			i32 result = FrameParse(in->data, in->size, !data->client, data->maxMessageSize, &frame);
			if (result == 0)
				return false;
			if (result < 0)
			{
				WebSocketFail(data, result == -2 ? 1009 : 1002, result == -2 ? "WebSocket message too large" : "Malformed WebSocket frame");
				return false;
			}

			s8 payload = { (c8*)frame.payload, frame.length };
			switch (frame.opcode)
			{
			case 1: // Text
			case 2: // Binary
			{
				WebSocketMessageType type = frame.opcode == 1 ? WebSocketMessageTypeText : WebSocketMessageTypeBinary;
				if (data->fragmenting)
				{
					WebSocketFail(data, 1002, "WebSocket message within another's fragments");
					return false;
				}
				if (!frame.fin)
				{
					data->fragmenting = true;
					data->fragmentType = type;
					message->size = 0;
					if (!BufferAppend(message, payload, data->allocator))
						return false;
					BufferDrop(in, frame.size);
					continue;
				}
				if (type == WebSocketMessageTypeText && !S8IsValidUTF8(payload))
				{
					WebSocketFail(data, 1007, "WebSocket text isn't UTF-8");
					return false;
				}
				outMessage->type = type;
				outMessage->data = payload;
				data->consumed = frame.size;
				return true;
			}

			case 0: // Continuation
			{
				if (!data->fragmenting)
				{
					WebSocketFail(data, 1002, "WebSocket continuation without a message");
					return false;
				}
				if (message->size + frame.length > data->maxMessageSize)
				{
					WebSocketFail(data, 1009, "WebSocket message too large");
					return false;
				}
				if (!BufferAppend(message, payload, data->allocator))
					return false;
				BufferDrop(in, frame.size);
				if (!frame.fin)
					continue;

				data->fragmenting = false;
				s8 joined = { (c8*)message->data, message->size };
				if (data->fragmentType == WebSocketMessageTypeText && !S8IsValidUTF8(joined))
				{
					WebSocketFail(data, 1007, "WebSocket text isn't UTF-8");
					return false;
				}
				outMessage->type = data->fragmentType;
				outMessage->data = joined;
				return true;
			}

			case 8: // Close: a code, then a reason (RFC 6455 5.5.1). Echoes the code
			{
				if (frame.length == 1)
				{
					WebSocketFail(data, 1002, "Malformed WebSocket close frame");
					return false;
				}
				u16 code = frame.length >= 2 ? (u16)(frame.payload[0] << 8 | frame.payload[1]) : 1000;
				WebSocketQueueClose(data, code);
				WebSocketEnd(data, frame.length > 2 ? S8Slice(payload, 2, payload.size) : s8{});
				return false;
			}

			case 9: // Ping, answered with its payload
				WebSocketQueue(data, 10, payload.str, payload.size);
				BufferDrop(in, frame.size);
				continue;

			case 10: // Pong
				BufferDrop(in, frame.size);
				continue;

			default:
				WebSocketFail(data, 1002, "Unknown WebSocket opcode");
				return false;
			}
		}
		return false;
	}

	b8 WebSocketConnect(WebSocket* socket, s8 url, MemoryAllocator allocator, u64 maxMessageSize)
	{
		*socket = {};
		Url parsed;
		Address address;
		if (!ClientResolve(url, S8("ws"), &parsed, &address))
			return false;

		WebSocketData* data = WebSocketCreate(allocator, maxMessageSize);
		if (data == nullptr)
			return false;
		data->client = true;
		data->phase = WebSocketPhaseHandshake;
		RandomStreamInit(&data->random, (u64)TimepointNow() ^ SystemTimepointNow(false) ^ (u64)data);

		// A random key, and the answer it should get
		u32 nonce[4];
		for (u32 i = 0; i < 4; i++)
			nonce[i] = RandomU32Next(&data->random);
		c8 key[24];
		Base64((const u8*)nonce, 16, key);
		WebSocketAcceptFor({ key, 24 }, data->accept);

		HttpBuffer* out = &data->output;
		b8 ok = BufferAppendRequestHead(out, HttpMethodGet, &parsed, allocator);
		ok = ok && BufferAppend(out, S8("Upgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: "), allocator);
		ok = ok && BufferAppend(out, key, 24, allocator) && BufferAppend(out, S8("\r\n\r\n"), allocator);
		data->handshakeSize = out->size;

		if (!ok || !TcpConnect(&data->socket, &address, 0))
		{
			BufferFree(out, allocator);
			AllocatorDealloc(allocator, data);
			return false;
		}

		socket->handle = (u64)data;
		WebSocketSendOut(data);
		return true;
	}

	b8 WebSocketAccept(WebSocket* socket, HttpServer* server, const HttpRequest* request, MemoryAllocator allocator, u64 maxMessageSize)
	{
		*socket = {};
		HttpServerData* serverData = (HttpServerData*)server->handle;
		HttpConnection* connection = ServerHeld(serverData, request);
		if (connection == nullptr)
			return TOOL_FAIL("Not a request waiting for an answer");

		// RFC 6455 4.2.1
		s8 key = HttpHeaderFind(request->headers, request->headerCount, S8("Sec-WebSocket-Key"));
		b8 upgrade = request->method == HttpMethodGet && key.size == 24;
		upgrade = upgrade && ListHasToken(HttpHeaderFind(request->headers, request->headerCount, S8("Upgrade")), S8("websocket"));
		upgrade = upgrade && ListHasToken(HttpHeaderFind(request->headers, request->headerCount, S8("Connection")), S8("upgrade"));
		upgrade = upgrade && S8Equal(HttpHeaderFind(request->headers, request->headerCount, S8("Sec-WebSocket-Version")), S8("13"));
		if (!upgrade)
			return TOOL_FAIL("Not a WebSocket upgrade request");

		WebSocketData* data = WebSocketCreate(allocator, maxMessageSize);
		if (data == nullptr)
			return false;
		data->phase = WebSocketPhaseOpen;

		// The 101, then whatever the client sent after its request
		c8 accept[28];
		WebSocketAcceptFor(key, accept);
		HttpBuffer* in = &connection->input;
		b8 ok = BufferAppend(&data->output, S8("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "), allocator);
		ok = ok && BufferAppend(&data->output, accept, 28, allocator) && BufferAppend(&data->output, S8("\r\n\r\n"), allocator);
		ok = ok && BufferAppend(&data->input, in->data + connection->requestSize, in->size - connection->requestSize, allocator);
		if (!ok)
		{
			BufferFree(&data->output, allocator);
			BufferFree(&data->input, allocator);
			AllocatorDealloc(allocator, data);
			return false;
		}

		// The connection leaves the server, its socket left open
		data->socket = connection->socket;
		connection->socket = 0;
		ServerCloseConnection(connection);

		socket->handle = (u64)data;
		WebSocketSendOut(data);
		return true;
	}

	b8 WebSocketSend(WebSocket* socket, WebSocketMessageType type, s8 data)
	{
		WebSocketData* socketData = (WebSocketData*)socket->handle;
		if (socketData == nullptr || socketData->phase >= WebSocketPhaseClosed || socketData->closeSent || type == WebSocketMessageTypeClosed)
			return TOOL_FAIL("The WebSocket is closed");
		if (!WebSocketQueue(socketData, type == WebSocketMessageTypeText ? 1 : 2, data.str, data.size))
			return false;
		if (!WebSocketSendOut(socketData))
			WebSocketEnd(socketData, {});
		return true;
	}

	b8 WebSocketNext(WebSocket* socket, WebSocketMessage* outMessage, u32 timeout)
	{
		*outMessage = {};
		WebSocketData* data = (WebSocketData*)socket->handle;
		if (data == nullptr || data->phase == WebSocketPhaseEnded)
			return false;

		if (data->consumed > 0)
		{
			BufferDrop(&data->input, data->consumed);
			data->consumed = 0;
		}

		Timepoint start = TimepointNow();
		for (;;)
		{
			if (data->phase == WebSocketPhaseClosed)
			{
				outMessage->type = WebSocketMessageTypeClosed;
				outMessage->data = { (c8*)data->message.data, data->message.size };
				data->phase = WebSocketPhaseEnded;
				return true;
			}

			if (!WebSocketSendOut(data))
			{
				WebSocketEnd(data, {});
				continue;
			}

			b8 open = WebSocketReceive(data);
			if (WebSocketProcess(data, outMessage))
				return true;
			if (data->phase == WebSocketPhaseClosed)
				continue;
			if (!open)
			{
				TOOL_FAIL("The WebSocket connection ended without a close frame");
				WebSocketEnd(data, {});
				continue;
			}

			i64 elapsed = NanosecondsSince(start) / 1000000;
			if (elapsed >= (i64)timeout)
				return false;

			u64 sendable = data->phase == WebSocketPhaseHandshake ? data->handshakeSize : data->output.size;
			u32 wait = SocketPollFlagsRead | (data->outputSent < sendable ? (u32)SocketPollFlagsWrite : 0);
			SocketPoll poll = { data->socket, wait, 0 };
			SocketsPoll(&poll, 1, (u32)((i64)timeout - elapsed));
		}
	}

	void WebSocketClose(WebSocket* socket)
	{
		WebSocketData* data = (WebSocketData*)socket->handle;
		if (data == nullptr)
			return;

		if (data->phase == WebSocketPhaseOpen)
		{
			WebSocketQueueClose(data, 1000);
			WebSocketSendOut(data);
		}
		SocketClose(data->socket);
		BufferFree(&data->input, data->allocator);
		BufferFree(&data->output, data->allocator);
		BufferFree(&data->message, data->allocator);
		AllocatorDealloc(data->allocator, data);
		*socket = {};
	}
} //namespace Tool

#endif
