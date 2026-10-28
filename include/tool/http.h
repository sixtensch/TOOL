#ifndef _TOOL_HTTP_H
#define _TOOL_HTTP_H

#include "basics.h"
#include "memory.h"
#include "network.h"
#include "text.h"



//~ HTTP
//
// URLs, an HTTP/1.1 server and client, and WebSocket at both ends, over network.h. All polled from the caller's loop,
// with no threads of their own. Serving needs TOOL_SOCKETS; on the web, clients go through the browser's fetch() and
// WebSocket. No TLS yet: http:// and ws:// only, but for the browser's.
//
// HttpServer server = {};
// Address address = AddressLoopback(AddressFamilyIPv4, 8080);
// if (HttpServerOpen(&server, &address, Allocator()))
// {
//     HttpRequest request;
//     while (HttpServerNext(&server, &request, 1000)) // Or 0 each frame
//         HttpRespond(&server, &request, 200, S8("text/plain"), S8("Hello"));
//     HttpServerClose(&server);
// }
//
// HttpClient client = {};
// if (HttpClientSend(&client, HttpMethodGet, S8("http://127.0.0.1:8080/"), {}, Allocator()))
// {
//     while (HttpClientPoll(&client, 100) == HttpClientStatePending)
//         ;
//     ...client.status, client.body...
//     HttpClientClose(&client);
// }
//
// WebSocket socket = {};
// if (WebSocketAccept(&socket, &server, &request, Allocator())) // Or WebSocketConnect(&socket, url, Allocator())
// {
//     WebSocketMessage message;
//     while (WebSocketNext(&socket, &message)) // Each frame
//         if (message.type != WebSocketMessageTypeClosed)
//             WebSocketSend(&socket, message.type, message.data);
//     ...
//     WebSocketClose(&socket);
// }



//~ Definitions

// Most header lines in a request or response
#define TOOL_HTTP_HEADER_MAX 64

// Most connections a server holds at once; the rest wait in the listener's backlog
#define TOOL_HTTP_CONNECTION_MAX (TOOL_SOCKET_POLL_MAX - 1)

// ms a server connection may sit without progress, request held by the caller excepted, before it's closed
#define TOOL_HTTP_IDLE_TIMEOUT 60000



namespace Tool
{
	//- Types

	// Views into the parsed text
	struct Url
	{
		s8 scheme;   // "http", as written
		s8 host;     // Name or address; IPv6 without brackets
		u16 port;    // As written, else the scheme's (80 for http and ws, 443 for https and wss), else 0
		s8 path;     // From '/', empty if absent
		s8 query;    // After '?', without it
		s8 fragment; // After '#', without it
	};

	enum HttpMethod
	{
		HttpMethodNone,    // Invalid
		HttpMethodGet,     // GET
		HttpMethodHead,    // HEAD: answered as GET, the body left out
		HttpMethodPost,    // POST
		HttpMethodPut,     // PUT
		HttpMethodDelete,  // DELETE
		HttpMethodPatch,   // PATCH
		HttpMethodOptions, // OPTIONS
	};

	struct HttpHeader
	{
		s8 name;
		s8 value;
	};

	// Zeroed is closed
	struct HttpServer
	{
		u64 handle; // Internal
	};

	// Views into the server's buffers, valid until HttpRespond
	struct HttpRequest
	{
		HttpMethod method;
		s8 path;  // Percent-encoded, as sent. "*" for OPTIONS of the whole server
		s8 query; // After '?', without it
		s8 body;  // Chunked bodies come joined
		const HttpHeader* headers;
		u32 headerCount;
		Address from;
		u64 handle; // Internal
	};

	enum HttpClientState
	{
		HttpClientStatePending, // Under way
		HttpClientStateDone,    // Response in; any status
		HttpClientStateFailed,  // No response; the reason is recorded
	};

	// Zeroed is closed. The response fields are set once Done, views valid until HttpClientClose.
	struct HttpClient
	{
		u32 status;
		s8 body;
		const HttpHeader* headers;
		u32 headerCount;
		u64 handle; // Internal
	};

	enum WebSocketMessageType
	{
		WebSocketMessageTypeText,   // UTF-8, validated
		WebSocketMessageTypeBinary, // Bytes
		WebSocketMessageTypeClosed, // The connection ended; the last message. Data: the other end's reason, if any
	};

	// 'data' valid until the next WebSocketNext or WebSocketClose
	struct WebSocketMessage
	{
		WebSocketMessageType type;
		s8 data;
	};

	// Zeroed is closed
	struct WebSocket
	{
		u64 handle; // Internal
	};



	//- Functions

	//~ URLs

	// "scheme://host[:port][/path][?query][#fragment]". IPv6 hosts in brackets; no user info.
	b8 UrlParse(s8 text, Url* outUrl);

	// Percent-encoding (RFC 3986): all but A-Z a-z 0-9 - . _ ~ become %XX
	s8 UrlEncode(s8 text, MemoryAllocator allocator);

	// %XX to bytes; malformed escapes and '+' stay as they are
	s8 UrlDecode(s8 text, MemoryAllocator allocator);

	//~ Headers

	// Case-insensitive. The first match's value; str null if absent.
	s8 HttpHeaderFind(const HttpHeader* headers, u32 count, s8 name);

	//~ Server
	// Malformed and oversized requests are answered and closed by the server. Keep-alive and pipelining follow the
	// request; one request per connection is out at a time. Memory: buffers per connection, grown as needed and kept
	// for the next connection, so an arena works but the heap returns what's outgrown.

	// 'maxRequestSize' bounds head and body together; larger requests get 413 or 431.
	b8 HttpServerOpen(HttpServer* server, const Address* address, MemoryAllocator allocator, u64 maxRequestSize = 1 << 20);

	// Accepts, reads and sends, waiting up to 'timeout' ms for a complete request. False when none came.
	b8 HttpServerNext(HttpServer* server, HttpRequest* outRequest, u32 timeout = 0);

	// Answers a request from HttpServerNext. Copies everything; sending continues in HttpServerNext. 'contentType'
	// empty for none. Requests whose connection is gone are dropped silently.
	void HttpRespond(HttpServer* server, const HttpRequest* request, u32 status, s8 contentType, s8 body, const HttpHeader* headers = nullptr, u32 headerCount = 0);

	b8 HttpServerLocalAddress(HttpServer* server, Address* outAddress); // Tells the port picked for port 0
	void HttpServerClose(HttpServer* server);

	//~ Client
	// One connection per request ("Connection: close"); no redirects followed. Name lookup blocks (network.h).

	// Starts a request. 'body' empty for none; Content-Length and Host are added. Copies everything.
	b8 HttpClientSend(HttpClient* client, HttpMethod method, s8 url, s8 body, MemoryAllocator allocator, const HttpHeader* headers = nullptr, u32 headerCount = 0);

	// Waits up to 'timeout' ms for the response; on the web, never waits.
	HttpClientState HttpClientPoll(HttpClient* client, u32 timeout = 0);

	void HttpClientClose(HttpClient* client); // Cancels one under way

	//~ WebSocket
	// RFC 6455. Pings are answered and fragments joined inside. Abnormal ends are recorded and come as Closed.
	// Masking keys aren't cryptographically random.

	// ws:// (and wss:// on the web). Sends before it opens are queued. Name lookup blocks (network.h).
	b8 WebSocketConnect(WebSocket* socket, s8 url, MemoryAllocator allocator, u64 maxMessageSize = 1 << 20);

	// Takes over an upgrade request's connection; the request needs no HttpRespond. False if it isn't one, the
	// request then still to be answered (426, say).
	b8 WebSocketAccept(WebSocket* socket, HttpServer* server, const HttpRequest* request, MemoryAllocator allocator, u64 maxMessageSize = 1 << 20);

	b8 WebSocketSend(WebSocket* socket, WebSocketMessageType type, s8 data); // Copies. False once closed

	// Sends, reads and answers pings, waiting up to 'timeout' ms for a message; on the web, never waits. False when
	// none came, and always after Closed.
	b8 WebSocketNext(WebSocket* socket, WebSocketMessage* outMessage, u32 timeout = 0);

	void WebSocketClose(WebSocket* socket); // Sends a close frame if it can
} //namespace Tool

#endif //_TOOL_HTTP_H
