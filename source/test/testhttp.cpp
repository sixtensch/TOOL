#include "tool.h"
#include "test.h"
#include <stdio.h>
#include <string.h>

using namespace Tool;



//~ URLs

static void CheckUrl(const c8* text, const c8* scheme, const c8* host, u16 port, const c8* path, const c8* query, const c8* fragment)
{
	Url url;
	TOOL_ASSERT(UrlParse(S8(text), &url), "%s", text);
	TOOL_ASSERT(S8Equal(url.scheme, S8(scheme)) && S8Equal(url.host, S8(host)) && url.port == port, "%s", text);
	TOOL_ASSERT(S8Equal(url.path, S8(path)) && S8Equal(url.query, S8(query)) && S8Equal(url.fragment, S8(fragment)), "%s", text);
}

static void TestUrls()
{
	CheckUrl("http://example.com", "http", "example.com", 80, "", "", "");
	CheckUrl("https://example.com/", "https", "example.com", 443, "/", "", "");
	CheckUrl("ws://a:8080/x/y?b=1&c=2#top", "ws", "a", 8080, "/x/y", "b=1&c=2", "top");
	CheckUrl("wss://a?q", "wss", "a", 443, "", "q", "");
	CheckUrl("http://127.0.0.1:1/#f", "http", "127.0.0.1", 1, "/", "", "f");
	CheckUrl("http://[::1]:8080/p", "http", "::1", 8080, "/p", "", "");
	CheckUrl("http://[fe80::1%3]/", "http", "fe80::1%3", 80, "/", "", "");
	CheckUrl("custom+x.y-z://h/p", "custom+x.y-z", "h", 0, "/p", "", "");

	const c8* bad[] = { "", "example.com", "://a", "1http://a", "http://", "http://:80", "http://a:", "http://a:65536",
		"http://a:8x", "http://user@a", "http://[::1", "http://[::1]x", "ht tp://a" };
	for (const c8* text : bad)
	{
		Url url;
		TOOL_ASSERT(!UrlParse(S8(text), &url), "%s", text);
	}

	Arena arena = {};
	ArenaInit(&arena, Allocator());
	MemoryAllocator allocator = Allocator(&arena);

	s8 encoded = UrlEncode(S8("a b/c?d=e&f~g.h_i-j\xC3\xA5"), allocator);
	TOOL_ASSERT(S8Equal(encoded, S8("a%20b%2Fc%3Fd%3De%26f~g.h_i-j%C3%A5")), "%s", encoded.str);
	TOOL_ASSERT(encoded.str[encoded.size] == 0);
	s8 decoded = UrlDecode(encoded, allocator);
	TOOL_ASSERT(S8Equal(decoded, S8("a b/c?d=e&f~g.h_i-j\xC3\xA5")));
	TOOL_ASSERT(S8Equal(UrlDecode(S8("%41%4a%zz%4+%"), allocator), S8("AJ%zz%4+%")));
	TOOL_ASSERT(UrlEncode({}, allocator).size == 0 && UrlDecode({}, allocator).size == 0);

	ArenaDeInit(&arena);
}

#if TOOL_SOCKETS

//~ Loopback

// Echoes the body, with the path and query as headers. "/missing" gets 404, "/empty" 204.
static void Answer(HttpServer* server, const HttpRequest* request)
{
	if (S8Equal(request->path, S8("/missing")))
		return HttpRespond(server, request, 404, S8("text/plain"), S8("Nope"));
	if (S8Equal(request->path, S8("/empty")))
		return HttpRespond(server, request, 204, {}, {});

	HttpHeader headers[] = {
		{ S8("X-Path"), request->path },
		{ S8("X-Query"), request->query },
		{ S8("X-Agent"), HttpHeaderFind(request->headers, request->headerCount, S8("user-agent")) },
	};
	HttpRespond(server, request, 200, S8("application/octet-stream"), request->body, headers, 3);
}

// Runs the server until the client is done
static HttpClientState Pump(HttpServer* server, HttpClient* client)
{
	Timepoint start = TimepointNow();
	for (;;)
	{
		HttpRequest request;
		while (HttpServerNext(server, &request, 1))
			Answer(server, &request);

		HttpClientState state = HttpClientPoll(client, 1);
		if (state != HttpClientStatePending)
			return state;
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0, "HTTP exchange timed out");
	}
}

static void TestExchange(AddressFamily family)
{
	HttpServer server = {};
	Address address = AddressLoopback(family, 0);
	TOOL_ASSERT(HttpServerOpen(&server, &address, Allocator(), 4 << 20), "%s", ErrorMessage());
	Address bound;
	TOOL_ASSERT(HttpServerLocalAddress(&server, &bound));

	c8 base[128];
	snprintf(base, sizeof(base), family == AddressFamilyIPv4 ? "http://127.0.0.1:%u" : "http://[::1]:%u", (u32)bound.port);
	c8 url[256];

	// A GET with a query and a header
	snprintf(url, sizeof(url), "%s/some/path?a=1&b=2#ignored", base);
	HttpHeader agent = { S8("User-Agent"), S8("TOOLTest") };
	HttpClient client = {};
	TOOL_ASSERT(HttpClientSend(&client, HttpMethodGet, S8(url), {}, Allocator(), &agent, 1), "%s", ErrorMessage());
	TOOL_ASSERT(Pump(&server, &client) == HttpClientStateDone, "%s", ErrorMessage());
	TOOL_ASSERT(client.status == 200 && client.body.size == 0);
	TOOL_ASSERT(S8Equal(HttpHeaderFind(client.headers, client.headerCount, S8("x-path")), S8("/some/path")));
	TOOL_ASSERT(S8Equal(HttpHeaderFind(client.headers, client.headerCount, S8("X-QUERY")), S8("a=1&b=2")));
	TOOL_ASSERT(S8Equal(HttpHeaderFind(client.headers, client.headerCount, S8("X-Agent")), S8("TOOLTest")));
	TOOL_ASSERT(HttpHeaderFind(client.headers, client.headerCount, S8("X-None")).str == nullptr);
	HttpClientClose(&client);

	// A large POST, echoed
	u64 size = 3 << 20;
	u8* data = (u8*)ClassicAlloc(4 << 20);
	for (u64 i = 0; i < (4 << 20); i++)
		data[i] = (u8)(i * 7 + (i >> 11));
	snprintf(url, sizeof(url), "%s/echo", base);
	TOOL_ASSERT(HttpClientSend(&client, HttpMethodPost, S8(url), { (c8*)data, size }, Allocator()));
	TOOL_ASSERT(Pump(&server, &client) == HttpClientStateDone, "%s", ErrorMessage());
	TOOL_ASSERT(client.status == 200 && client.body.size == size && memcmp(client.body.str, data, size) == 0);
	HttpClientClose(&client);

	// Too large for the server: refused from the head, the body drained
	TOOL_ASSERT(HttpClientSend(&client, HttpMethodPut, S8(url), { (c8*)data, 4 << 20 }, Allocator()));
	TOOL_ASSERT(Pump(&server, &client) == HttpClientStateDone && client.status == 413, "%s", ErrorMessage());
	ClassicDealloc(data);
	HttpClientClose(&client);

	// HEAD has the length but no body; 404 and 204
	TOOL_ASSERT(HttpClientSend(&client, HttpMethodHead, S8(url), {}, Allocator()));
	TOOL_ASSERT(Pump(&server, &client) == HttpClientStateDone && client.status == 200 && client.body.size == 0);
	TOOL_ASSERT(S8Equal(HttpHeaderFind(client.headers, client.headerCount, S8("Content-Length")), S8("0")));
	HttpClientClose(&client);

	snprintf(url, sizeof(url), "%s/missing", base);
	TOOL_ASSERT(HttpClientSend(&client, HttpMethodDelete, S8(url), {}, Allocator()));
	TOOL_ASSERT(Pump(&server, &client) == HttpClientStateDone && client.status == 404 && S8Equal(client.body, S8("Nope")));
	HttpClientClose(&client);

	snprintf(url, sizeof(url), "%s/empty", base);
	TOOL_ASSERT(HttpClientSend(&client, HttpMethodOptions, S8(url), {}, Allocator()));
	TOOL_ASSERT(Pump(&server, &client) == HttpClientStateDone && client.status == 204 && client.body.size == 0);
	HttpClientClose(&client);

	// Several at once, answered out of order: the server holds each request until answered
	const u32 count = 8;
	HttpClient clients[count] = {};
	for (u32 i = 0; i < count; i++)
	{
		snprintf(url, sizeof(url), "%s/many/%u", base, i);
		TOOL_ASSERT(HttpClientSend(&clients[i], HttpMethodPost, S8(url), S8(url), Allocator()));
	}
	HttpRequest held[count];
	u32 heldCount = 0;
	Timepoint start = TimepointNow();
	while (heldCount < count)
	{
		if (HttpServerNext(&server, &held[heldCount], 1))
			heldCount++;
		for (u32 i = 0; i < count; i++)
			HttpClientPoll(&clients[i], 0);
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0, "Requests didn't arrive");
	}
	for (u32 i = count; i > 0; i--)
		Answer(&server, &held[i - 1]);
	for (u32 i = 0; i < count; i++)
	{
		TOOL_ASSERT(Pump(&server, &clients[i]) == HttpClientStateDone && clients[i].status == 200);
		snprintf(url, sizeof(url), "%s/many/%u", base, i);
		TOOL_ASSERT(S8Equal(clients[i].body, S8(url)));
		HttpClientClose(&clients[i]);
	}

	// A stale request is ignored
	HttpRespond(&server, &held[0], 200, {}, {});

	HttpServerClose(&server);
	TOOL_ASSERT(server.handle == 0);
}

//~ Raw connections
// What the client doesn't send: pipelining, chunked bodies, malformed requests

// Sends 'request' to the server, answering with Answer, and reads until the server closes. 'later' is sent once a
// reply starts.
static s8 Raw(HttpServer* server, const c8* request, c8* response, u64 capacity, const c8* later = nullptr)
{
	Address address;
	TOOL_ASSERT(HttpServerLocalAddress(server, &address));
	address = AddressLoopback(address.family, address.port);

	Socket socket;
	TOOL_ASSERT(TcpConnect(&socket, &address, 5000), "%s", ErrorMessage());

	u64 length = strlen(request);
	u64 sent = 0;
	u64 size = 0;
	Timepoint start = TimepointNow();
	for (;;)
	{
		if (sent < length)
		{
			u64 now;
			TOOL_ASSERT(TcpSend(socket, request + sent, length - sent, &now));
			sent += now;
		}

		HttpRequest incoming;
		while (HttpServerNext(server, &incoming, 1))
			Answer(server, &incoming);

		u64 received;
		if (!TcpReceive(socket, response + size, capacity - 1 - size, &received))
			break;
		size += received;
		TOOL_ASSERT(size < capacity - 1, "Response too long");

		if (later != nullptr && size > 0 && sent == length)
		{
			request = later;
			length = strlen(later);
			sent = 0;
			later = nullptr;
		}
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0, "Raw exchange timed out");
	}

	SocketClose(socket);
	response[size] = 0;
	return { response, size };
}

static u32 Occurrences(s8 text, s8 needle)
{
	u32 count = 0;
	for (u64 at = S8Find(text, needle); at != S8_NONE; at = S8Find(text, needle, at + 1))
		count++;
	return count;
}

static void TestRaw()
{
	HttpServer server = {};
	Address address = AddressLoopback(AddressFamilyIPv4, 0);
	TOOL_ASSERT(HttpServerOpen(&server, &address, Allocator(), 4096));
	static c8 response[1 << 16];

	// Pipelined on one connection, the last asking to close; a bare LF line end and a leading empty line too
	s8 text = Raw(&server,
		"GET /one HTTP/1.1\r\nHost: x\r\n\r\n"
		"\r\nPOST /two?x HTTP/1.1\nHost: x\nContent-Length: 3\n\nabc"
		"GET /three HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n",
		response, sizeof(response));
	TOOL_ASSERT(Occurrences(text, S8("HTTP/1.1 200 OK\r\n")) == 3, "%s", text.str);
	TOOL_ASSERT(S8Find(text, S8("X-Path: /one")) < S8Find(text, S8("X-Path: /two")), "%s", text.str);
	TOOL_ASSERT(S8Find(text, S8("X-Query: x\r\n")) != S8_NONE && S8Find(text, S8("\r\n\r\nabc")) != S8_NONE, "%s", text.str);
	TOOL_ASSERT(S8EndsWith(text, S8("X-Path: /three\r\nX-Query: \r\nX-Agent: \r\n\r\n")), "%s", text.str);
	TOOL_ASSERT(Occurrences(text, S8("Connection: close")) == 1, "%s", text.str);

	// A chunked body with an extension and a trailer, after 100 Continue
	text = Raw(&server,
		"POST /chunks HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\nExpect: 100-continue\r\nConnection: close\r\n\r\n",
		response, sizeof(response),
		"5;ext=1\r\nhello\r\n1\r\n \r\nA\r\n0123456789\r\n0\r\nTrailer: t\r\n\r\n");
	TOOL_ASSERT(S8StartsWith(text, S8("HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\n")), "%s", text.str);
	TOOL_ASSERT(S8EndsWith(text, S8("\r\n\r\nhello 0123456789")), "%s", text.str);

	// HTTP/1.0 closes after its response
	text = Raw(&server, "GET /old HTTP/1.0\r\n\r\n", response, sizeof(response));
	TOOL_ASSERT(S8StartsWith(text, S8("HTTP/1.1 200 OK\r\n")) && S8Find(text, S8("Connection: close")) != S8_NONE, "%s", text.str);

	// Refusals
	struct Refusal
	{
		const c8* request;
		const c8* status;
	};
	Refusal refusals[] = {
		{ "GARBAGE\r\n\r\n", "400" },
		{ "GET /\r\n\r\n", "400" },
		{ "GET / HTTP/2.0\r\n\r\n", "505" },
		{ "GET / SPDY\r\n\r\n", "400" },
		{ "BREW /pot HTTP/1.1\r\n\r\n", "501" },
		{ "GET / HTTP/1.1\r\nBad Header: x\r\n\r\n", "400" },
		{ "GET / HTTP/1.1\r\nNoColon\r\n\r\n", "400" },
		{ "GET / HTTP/1.1\r\nA: b\r\n folded\r\n\r\n", "400" },
		{ "POST / HTTP/1.1\r\nContent-Length: x\r\n\r\n", "400" },
		{ "POST / HTTP/1.1\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\n", "400" },
		{ "POST / HTTP/1.1\r\nTransfer-Encoding: gzip\r\n\r\n", "501" },
		{ "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\nzz\r\n", "400" },
		{ "POST / HTTP/1.1\r\nContent-Length: 5000\r\n\r\n", "413" },
	};
	for (Refusal refusal : refusals)
	{
		text = Raw(&server, refusal.request, response, sizeof(response));
		TOOL_ASSERT(S8StartsWith(text, S8("HTTP/1.1 ")) && S8StartsWith(S8Slice(text, 9, text.size), S8(refusal.status)), "%s: %s", refusal.request, text.str);
		TOOL_ASSERT(S8Find(text, S8("Connection: close")) != S8_NONE, "%s", refusal.request);
	}

	// Too many headers, and a head beyond the limit
	c8 request[8192];
	u64 at = (u64)snprintf(request, sizeof(request), "GET / HTTP/1.1\r\n");
	for (u32 i = 0; i <= TOOL_HTTP_HEADER_MAX; i++)
		at += (u64)snprintf(request + at, sizeof(request) - at, "H%u: v\r\n", i);
	snprintf(request + at, sizeof(request) - at, "\r\n");
	text = Raw(&server, request, response, sizeof(response));
	TOOL_ASSERT(S8StartsWith(text, S8("HTTP/1.1 431")), "%s", text.str);

	memset(request, 'a', sizeof(request) - 1);
	memcpy(request, "GET /", 5);
	request[sizeof(request) - 1] = 0;
	text = Raw(&server, request, response, sizeof(response));
	TOOL_ASSERT(S8StartsWith(text, S8("HTTP/1.1 431")), "%s", text.str);

	HttpServerClose(&server);
}

//~ Client against raw responses
// Framings the server doesn't send

// Serves one canned response to the client's request
static HttpClientState Canned(const c8* canned, HttpClient* client, HttpMethod method = HttpMethodGet)
{
	Socket listener;
	Address address = AddressLoopback(AddressFamilyIPv4, 0);
	TOOL_ASSERT(TcpListen(&listener, &address));
	TOOL_ASSERT(SocketLocalAddress(listener, &address));

	c8 url[64];
	snprintf(url, sizeof(url), "http://127.0.0.1:%u/", (u32)address.port);
	TOOL_ASSERT(HttpClientSend(client, method, S8(url), {}, Allocator()));

	SocketPoll poll = { listener, SocketPollFlagsRead };
	TOOL_ASSERT(SocketsPoll(&poll, 1, 5000) == 1);
	Socket connection;
	TOOL_ASSERT(TcpAccept(listener, &connection));

	// Waits for the whole request before answering
	c8 request[4096];
	u64 size = 0;
	Timepoint start = TimepointNow();
	while (S8Find({ request, size }, S8("\r\n\r\n")) == S8_NONE)
	{
		HttpClientPoll(client, 1);
		u64 received;
		TOOL_ASSERT(TcpReceive(connection, request + size, sizeof(request) - size, &received));
		size += received;
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0);
	}
	TOOL_ASSERT(S8StartsWith({ request, size }, method == HttpMethodHead ? S8("HEAD / HTTP/1.1\r\nHost: 127.0.0.1:") : S8("GET / HTTP/1.1\r\nHost: 127.0.0.1:")));

	u64 length = strlen(canned);
	u64 sent = 0;
	while (sent < length)
	{
		u64 now;
		TOOL_ASSERT(TcpSend(connection, canned + sent, length - sent, &now));
		sent += now;
	}
	SocketClose(connection);
	SocketClose(listener);

	HttpClientState state;
	while ((state = HttpClientPoll(client, 100)) == HttpClientStatePending)
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0);
	return state;
}

static void TestClientFramings()
{
	HttpClient client = {};

	TOOL_ASSERT(Canned("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n2;x\r\nde\r\n0\r\nT: 1\r\n\r\n", &client) == HttpClientStateDone);
	TOOL_ASSERT(client.status == 200 && S8Equal(client.body, S8("abcde")));
	HttpClientClose(&client);

	TOOL_ASSERT(Canned("HTTP/1.0 201 Created\r\nX: y\r\n\r\nuntil the end", &client) == HttpClientStateDone);
	TOOL_ASSERT(client.status == 201 && S8Equal(client.body, S8("until the end")));
	TOOL_ASSERT(S8Equal(HttpHeaderFind(client.headers, client.headerCount, S8("x")), S8("y")));
	HttpClientClose(&client);

	TOOL_ASSERT(Canned("HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\nContent-Length: 2\n\nokEXTRA", &client) == HttpClientStateDone);
	TOOL_ASSERT(client.status == 200 && S8Equal(client.body, S8("ok")));
	HttpClientClose(&client);

	TOOL_ASSERT(Canned("HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n", &client, HttpMethodHead) == HttpClientStateDone);
	TOOL_ASSERT(client.status == 200 && client.body.size == 0);
	HttpClientClose(&client);

	const c8* broken[] = {
		"HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nshort",
		"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nab",
		"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nq\r\n",
		"HTTP/1.1 200 OK\r\nContent-Le",
		"SMTP 220 hello\r\n\r\n",
		"",
	};
	for (const c8* canned : broken)
	{
		TOOL_ASSERT(Canned(canned, &client) == HttpClientStateFailed, "%s", canned);
		HttpClientClose(&client);
	}

	// Nothing listening
	Socket listener;
	Address address = AddressLoopback(AddressFamilyIPv4, 0);
	TOOL_ASSERT(TcpListen(&listener, &address) && SocketLocalAddress(listener, &address));
	SocketClose(listener);
	c8 url[64];
	snprintf(url, sizeof(url), "http://127.0.0.1:%u/", (u32)address.port);
	TOOL_ASSERT(HttpClientSend(&client, HttpMethodGet, S8(url), {}, Allocator()));
	HttpClientState state;
	Timepoint start = TimepointNow();
	while ((state = HttpClientPoll(&client, 100)) == HttpClientStatePending)
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0);
	TOOL_ASSERT(state == HttpClientStateFailed);
	HttpClientClose(&client);

	TOOL_ASSERT(!HttpClientSend(&client, HttpMethodGet, S8("https://127.0.0.1/"), {}, Allocator()));
	TOOL_ASSERT(!HttpClientSend(&client, HttpMethodGet, S8("not a url"), {}, Allocator()));
	TOOL_ASSERT(client.handle == 0 && HttpClientPoll(&client) == HttpClientStateFailed);
}

//~ WebSocket

// Pumps the server until an upgrade comes, and accepts it
static void AcceptOne(HttpServer* server, WebSocket* socket, u64 maxMessageSize, WebSocket* client = nullptr)
{
	Timepoint start = TimepointNow();
	for (;;)
	{
		HttpRequest request;
		if (HttpServerNext(server, &request, 1))
		{
			TOOL_ASSERT(S8Equal(request.path, S8("/socket")));
			TOOL_ASSERT(WebSocketAccept(socket, server, &request, Allocator(), maxMessageSize), "%s", ErrorMessage());
			return;
		}
		WebSocketMessage message;
		if (client != nullptr)
			TOOL_ASSERT(!WebSocketNext(client, &message));
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0, "No upgrade came");
	}
}

// Pumps both until 'to' has a message
static WebSocketMessage Receive(WebSocket* from, WebSocket* to)
{
	Timepoint start = TimepointNow();
	WebSocketMessage message;
	for (;;)
	{
		if (WebSocketNext(to, &message, 1))
			return message;
		WebSocketMessage other;
		TOOL_ASSERT(!WebSocketNext(from, &other), "Unexpected message of type %u", (u32)other.type);
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0, "No message came");
	}
}

static void TestWebSocket()
{
	HttpServer server = {};
	Address address = AddressLoopback(AddressFamilyIPv4, 0);
	TOOL_ASSERT(HttpServerOpen(&server, &address, Allocator()));
	TOOL_ASSERT(HttpServerLocalAddress(&server, &address));
	c8 url[64];
	snprintf(url, sizeof(url), "ws://127.0.0.1:%u/socket", (u32)address.port);

	// Sent before the handshake completes, then echoed both ways
	WebSocket client = {};
	WebSocket accepted = {};
	TOOL_ASSERT(WebSocketConnect(&client, S8(url), Allocator()), "%s", ErrorMessage());
	TOOL_ASSERT(WebSocketSend(&client, WebSocketMessageTypeText, S8("early")));
	AcceptOne(&server, &accepted, 1 << 20, &client);

	WebSocketMessage message = Receive(&client, &accepted);
	TOOL_ASSERT(message.type == WebSocketMessageTypeText && S8Equal(message.data, S8("early")));

	u64 sizes[] = { 0, 125, 126, 65535, 65536, 300000 };
	static u8 bytes[300000];
	for (u64 i = 0; i < sizeof(bytes); i++)
		bytes[i] = (u8)(i * 31 + (i >> 9));
	for (u64 size : sizes)
	{
		TOOL_ASSERT(WebSocketSend(&client, WebSocketMessageTypeBinary, { (c8*)bytes, size }));
		message = Receive(&client, &accepted);
		TOOL_ASSERT(message.type == WebSocketMessageTypeBinary && message.data.size == size && memcmp(message.data.str, bytes, size) == 0);

		TOOL_ASSERT(WebSocketSend(&accepted, WebSocketMessageTypeBinary, message.data));
		message = Receive(&accepted, &client);
		TOOL_ASSERT(message.type == WebSocketMessageTypeBinary && message.data.size == size && memcmp(message.data.str, bytes, size) == 0);
	}

	// Closing ends the other side with a clean Closed
	WebSocketClose(&client);
	TOOL_ASSERT(client.handle == 0);
	Timepoint start = TimepointNow();
	while (!WebSocketNext(&accepted, &message, 1))
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0);
	TOOL_ASSERT(message.type == WebSocketMessageTypeClosed && message.data.size == 0);
	TOOL_ASSERT(!WebSocketNext(&accepted, &message) && !WebSocketSend(&accepted, WebSocketMessageTypeText, S8("late")));
	WebSocketClose(&accepted);

	// Too large for the server: both get Closed
	TOOL_ASSERT(WebSocketConnect(&client, S8(url), Allocator()));
	AcceptOne(&server, &accepted, 1000, &client);
	TOOL_ASSERT(WebSocketSend(&client, WebSocketMessageTypeBinary, { (c8*)bytes, 1001 }));
	message = Receive(&client, &accepted);
	TOOL_ASSERT(message.type == WebSocketMessageTypeClosed);
	start = TimepointNow();
	while (!WebSocketNext(&client, &message, 1))
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0);
	TOOL_ASSERT(message.type == WebSocketMessageTypeClosed);
	WebSocketClose(&client);
	WebSocketClose(&accepted);

	// Not an upgrade: refused, and the request still answered
	HttpClient http = {};
	snprintf(url, sizeof(url), "http://127.0.0.1:%u/socket", (u32)address.port);
	TOOL_ASSERT(HttpClientSend(&http, HttpMethodGet, S8(url), {}, Allocator()));
	start = TimepointNow();
	for (;;)
	{
		HttpRequest request;
		if (HttpServerNext(&server, &request, 1))
		{
			TOOL_ASSERT(!WebSocketAccept(&accepted, &server, &request, Allocator()) && accepted.handle == 0);
			HttpRespond(&server, &request, 426, {}, {});
		}
		if (HttpClientPoll(&http, 1) != HttpClientStatePending)
			break;
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0);
	}
	TOOL_ASSERT(http.status == 426);
	HttpClientClose(&http);

	// Nothing listening
	HttpServerClose(&server);
	TOOL_ASSERT(WebSocketConnect(&client, S8("ws://127.0.0.1:1/"), Allocator()));
	start = TimepointNow();
	while (!WebSocketNext(&client, &message, 100))
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0);
	TOOL_ASSERT(message.type == WebSocketMessageTypeClosed);
	WebSocketClose(&client);
	TOOL_ASSERT(!WebSocketConnect(&client, S8("http://127.0.0.1/"), Allocator()));
}

// A masked client frame
static u64 Frame(u8* out, u8 first, const c8* payload, u64 size, b8 masked = true)
{
	static constexpr u8 mask[4] = { 0x12, 0x34, 0x56, 0x78 };
	u64 at = 0;
	out[at++] = first;
	out[at++] = (u8)((masked ? 0x80 : 0) | size);
	if (masked)
		for (u32 i = 0; i < 4; i++)
			out[at++] = mask[i];
	for (u64 i = 0; i < size; i++)
		out[at++] = (u8)payload[i] ^ (masked ? mask[i & 3] : 0);
	return at;
}

static void SendAll(Socket socket, const void* data, u64 size)
{
	u64 sent = 0;
	while (sent < size)
	{
		u64 now;
		TOOL_ASSERT(TcpSend(socket, (const u8*)data + sent, size - sent, &now));
		sent += now;
	}
}

// Reads from 'socket' while pumping 'accepted' (echoing), until 'size' bytes are in or the socket closes
static u64 ReadRaw(Socket socket, WebSocket* accepted, u8* out, u64 size, WebSocketMessage* outClosed)
{
	u64 at = 0;
	Timepoint start = TimepointNow();
	while (at < size)
	{
		WebSocketMessage message;
		if (WebSocketNext(accepted, &message, 1))
		{
			if (message.type == WebSocketMessageTypeClosed)
				*outClosed = message;
			else
				WebSocketSend(accepted, message.type, message.data);
		}

		u64 received;
		if (!TcpReceive(socket, out + at, size - at, &received))
			break;
		at += received;
		TOOL_ASSERT(SecondsSince<f64>(start) < 10.0, "Raw WebSocket read timed out");
	}
	return at;
}

static void TestWebSocketRaw()
{
	HttpServer server = {};
	Address address = AddressLoopback(AddressFamilyIPv4, 0);
	TOOL_ASSERT(HttpServerOpen(&server, &address, Allocator()));
	TOOL_ASSERT(HttpServerLocalAddress(&server, &address));

	// RFC 6455 1.3's example key and answer
	Socket socket;
	TOOL_ASSERT(TcpConnect(&socket, &address, 5000));
	const c8* upgrade = "GET /socket HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: keep-alive, Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
	SendAll(socket, upgrade, strlen(upgrade));

	// A fragmented text with a ping between, sent with the upgrade's leftovers
	u8 frames[256];
	u64 size = Frame(frames, 0x01, "Hel", 3);
	size += Frame(frames + size, 0x89, "pi", 2);
	size += Frame(frames + size, 0x80, "lo", 2);
	SendAll(socket, frames, size);

	WebSocket accepted = {};
	AcceptOne(&server, &accepted, 1 << 20);

	const c8* expected = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n\r\n";
	u64 expectedSize = strlen(expected);
	u8 in[512];
	WebSocketMessage closed = {};
	u64 got = ReadRaw(socket, &accepted, in, expectedSize, &closed);
	TOOL_ASSERT(got == expectedSize && memcmp(in, expected, expectedSize) == 0, "%.*s", (i32)got, (const c8*)in);

	// The pong, then the echo, unmasked
	static constexpr u8 pongAndEcho[] = { 0x8A, 2, 'p', 'i', 0x81, 5, 'H', 'e', 'l', 'l', 'o' };
	TOOL_ASSERT(ReadRaw(socket, &accepted, in, sizeof(pongAndEcho), &closed) == sizeof(pongAndEcho));
	TOOL_ASSERT(memcmp(in, pongAndEcho, sizeof(pongAndEcho)) == 0);

	// Closing with a reason: echoed, and Closed carries it
	size = Frame(frames, 0x88, "\x03\xE8" "bye", 5);
	SendAll(socket, frames, size);
	static constexpr u8 closeEcho[] = { 0x88, 2, 0x03, 0xE8 };
	TOOL_ASSERT(ReadRaw(socket, &accepted, in, sizeof(in), &closed) == sizeof(closeEcho) && memcmp(in, closeEcho, sizeof(closeEcho)) == 0);
	TOOL_ASSERT(closed.type == WebSocketMessageTypeClosed && S8Equal(closed.data, S8("bye")));
	SocketClose(socket);
	WebSocketClose(&accepted);

	// Protocol errors close with 1002 or 1007
	struct Violation
	{
		u8 first;
		const c8* payload;
		u64 size;
		b8 masked;
		u16 code;
	};
	Violation violations[] = {
		{ 0x81, "x", 1, false, 1002 },    // Unmasked from a client
		{ 0x80, "x", 1, true, 1002 },     // Continuation of nothing
		{ 0xC1, "x", 1, true, 1002 },     // Reserved bit
		{ 0x83, "x", 1, true, 1002 },     // Unknown opcode
		{ 0x09, "x", 1, true, 1002 },     // Fragmented ping
		{ 0x81, "\xC3", 1, true, 1007 },  // Broken UTF-8
	};
	for (Violation violation : violations)
	{
		TOOL_ASSERT(TcpConnect(&socket, &address, 5000));
		SendAll(socket, upgrade, strlen(upgrade));
		AcceptOne(&server, &accepted, 1 << 20);
		TOOL_ASSERT(ReadRaw(socket, &accepted, in, expectedSize, &closed) == expectedSize);

		size = Frame(frames, violation.first, violation.payload, violation.size, violation.masked);
		SendAll(socket, frames, size);
		closed = {};
		u64 read = ReadRaw(socket, &accepted, in, sizeof(in), &closed);
		TOOL_ASSERT(read == 4 && in[0] == 0x88 && in[1] == 2 && (u16)(in[2] << 8 | in[3]) == violation.code, "%02x", violation.first);
		TOOL_ASSERT(closed.type == WebSocketMessageTypeClosed);
		SocketClose(socket);
		WebSocketClose(&accepted);
	}

	HttpServerClose(&server);
}

#endif // TOOL_SOCKETS

void TestHttp()
{
	TestUrls();

#if TOOL_SOCKETS
	TestExchange(AddressFamilyIPv4);
	TestExchange(AddressFamilyIPv6);
	TestRaw();
	TestClientFramings();
	TestWebSocket();
	TestWebSocketRaw();
#endif

	printf("Http: passed\n");
}
