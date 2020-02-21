#include <lua.h>
#include <lauxlib.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <zmq.h>

#define randof(num) (int)((float)(num)*rand()/(RAND_MAX+1.0))

typedef struct {
    uint8_t public_key [32];
    uint8_t secret_key [32];
    char public_txt [41];
    char secret_txt [41];
} cert_t;

#define checkctx(L) *(void **)luaL_checkudata(L, 1, "caap.zmq.context")
#define checkskt(L) *(void **)luaL_checkudata(L, 1, "caap.zmq.socket")
#define checkkey(L) (cert_t *)luaL_checkudata(L, 1, "caap.zmq.keypair")

extern int errno;

// ZeroMQ patterns encapsulate hard-earned experience of the best ways to
// distribute data and work. ZeroMQ patterns are implemented by pairs of
// sockets with matching types. The built-in core ZeroMQ patterns are:
// Request-reply: connects a set of clients to a set of services, this is
// a remote procedure call and task distribution pattern.
// Pub-sub: connects a set of publishers to a set of subscribers, this is
// a data distribution pattern.
// Pipeline: connects nodes in a fan-out/fan-in pattern that can have
// multiple steps and loops, this is a parallel task distribution pattern.
//
// PATTERNS:
// client-server
// A single ZMQ_SERVER server talks to one or more ZMQ_CLIENT clients.
// The usual and recommended model is to bind the ZMQ_SERVER and connect
// the ZMQ_CLIENT.
// ZMQ_CLIENT & ZMQ_SERVER sockets do not accept ZMQ_SNDMORE | ZMQ_RCVMORE
// this limits them to single part data.
//
// Native
// a ZMQ_STREAM is used to send and receive TCP data from a non-ZMQ peer
// when using the tcp::// transport.
//
// Reply Message Envelopes
// An envelope is a way of safely packaging up data with an address,
// without touching the data itself. By separating reply addresses
// into an envelope we make it possible to write general purpose
// intermediaries such as APIs and proxies.
// When you use REQ & REP sockets you don't even see envelopes.
//
// Simple Reply Envelope
// A request-reply exchange consists of a request message, and an eventual
// reply message. There's only one reply for each request.
// The ZeroMQ reply envelope formally consists of zero or more reply
// addresses, followed by an empty frame, followed by the message body.
// The REQ socket creates the simplest possible reply envelope, which
// has no address, just an empty delimiter frame and the message
// frame; this is a two-frame message.
//
// REQ - REQ
// The REQ socket sends, to the network, an empty delimiter frame in
// front of the message data. REQ sockets are synchronous, they send
// one request and then wait for one reply.
// The REP socket reads and saves all identity frames up to and incluiding
// the empty delimiter, then passes the following frame or frames to the
// caller. REP sockets are synchronous and talk to one peer at a time.
//
// DEALER
// The DEALER socket is oblivious to the reply envelope. Sockets are
// asynchronous and like PUSH and PULL combined. They distribute sent
// messages among all connections, and fair-queue received messages.
// A DEALER gives us an asynchronous client that can talk to multiple
// REP servers. We would be able to send any number of requests without
// waiting for replies.
//
// ROUTER
// The router socket, unlike other sockets, tracks every connection it has,
// and tells the caller about these, by sticking the connection identity
// in front of each message received. An identity is just a binary string
// with no meaning. When you send a message via a ROUTER socket, you first
// send an identity frame.
// When "receiving" messages a ZMQ_ROUTER socket shall prepend a message
// part containing the identity of the originating peer to the message
// before passing it to the application.
// When "sending" messages a ZMQ_ROUTER socket shall remove the first part
// of the message and use it to determine the identity of the peer the
// message shall be routed to.
// The ROUTER socket invents a random identity for each connection with
// which it works.
// Each time ROUTER gives you a message, it tells you what peer that came
// from, as an identity. ROUTER will route messages asynchronously to any
// peer connected to it, if you prefix the identity as the first frame.
// A ROUTER gives us an asynchronous server that can talk to multiple
// REQ clients at the same time. We would be able to process any number
// of request in parallel.
// We can use ROUTER in two distinct ways:
// As a "proxy" that switches messages between front- & backend sockets.
// An an "application" that reads the message and acts on it. The
// ROUTER must know the format of the 'reply envelope' it's being sent.


//
// CONTEXT
//
static int new_context(lua_State *L) {
    void **pctx = (void **)lua_newuserdata(L, sizeof(void *));
    luaL_getmetatable(L, "caap.zmq.context");
    lua_setmetatable(L, -2);

    *pctx = zmq_ctx_new();
    if (*pctx == NULL) {
	lua_pushnil(L);
	lua_pushstring(L, "Error creating ZMQ context\n");
	return 2;
    }

    return 1;
}

static int skt_subscribe(lua_State *L);
static int skt_unsubscribe(lua_State *L);
static int skt_set_rid(lua_State *L);
static int skt_stream_notify(lua_State *L);
static int skt_immediate(lua_State *L);
static int skt_set_id(lua_State *L);

static int new_socket(lua_State *L) {
    void *ctx = checkctx(L);
    const char* ttype = luaL_checkstring(L, 2);

    lua_getfield(L, lua_upvalueindex(1), ttype);
    int type = lua_tointeger(L, -1);
    lua_pop(L, 1);

    void **pskt = (void **)lua_newuserdata(L, sizeof(void *));
    luaL_getmetatable(L, "caap.zmq.socket");
    lua_setmetatable(L, -2);

    *pskt = zmq_socket(ctx, type);
    if (*pskt == NULL) {
	lua_pushnil(L);
	lua_pushstring(L, "Error creating ZMQ socket\n");
	return 2;
    }

    if(type == ZMQ_SUB) { //   || type == ZMQ_XSUB
	luaL_getmetatable(L, "caap.zmq.socket");
	lua_pushcclosure(L, &skt_subscribe, 0);
	lua_setfield(L, -2, "subscribe");
	lua_pushcclosure(L, &skt_unsubscribe, 0);
	lua_setfield(L, -2, "unsubscribe");
	lua_pop(L, 1); // pop metatable
    }

    if(type == ZMQ_STREAM) {
	luaL_getmetatable(L, "caap.zmq.socket");
	lua_pushcclosure(L, &skt_stream_notify, 0);
	lua_setfield(L, -2, "notify");
	lua_pop(L, 1); // pop metatable
    }

    if(type == ZMQ_STREAM || type == ZMQ_ROUTER) {
	luaL_getmetatable(L, "caap.zmq.socket");
	lua_pushcclosure(L, &skt_set_rid, 0);
	lua_setfield(L, -2, "set_rid");
	lua_pop(L, 1); // pop metatable
    }

    if(type == ZMQ_REQ || type == ZMQ_PUSH || type == ZMQ_DEALER) {
	luaL_getmetatable(L, "caap.zmq.socket");
	lua_pushcclosure(L, &skt_immediate, 0);
	lua_setfield(L, -2, "immediate");
	lua_pop(L, 1); // pop metatable
    }

    if(type == ZMQ_REQ || type == ZMQ_REP || type == ZMQ_DEALER || type == ZMQ_ROUTER ) {
	luaL_getmetatable(L, "caap.zmq.socket");
	lua_pushcclosure(L, &skt_set_id, 0);
	lua_setfield(L, -2, "set_id");
	lua_pop(L, 1); // pop metatable
    }

    return 1;
}

static int ctx_asstr(lua_State *L) {
    lua_pushstring(L, "zmq{Active Context}");
    return 1;
}

static int ctx_gc (lua_State *L) {
    void *ctx = checkctx(L);
    if (ctx && (zmq_ctx_term(ctx) == 0))
	ctx = NULL;
    return 0;
}

// Key pair
//
static int new_keypair(lua_State *L) {
    cert_t *cert = (cert_t *)lua_newuserdata(L, sizeof(cert_t));
    luaL_getmetatable(L, "caap.zmq.keypair");
    lua_setmetatable(L, -2);

    if (lua_gettop(L) > 1) {
	const char *public = luaL_checkstring(L, 1);
	const char *secret = luaL_checkstring(L, 2);
	if ((strlen(public) > 40) || (strlen(secret) > 40)) {
	    lua_pushnil(L);
	    lua_pushstring(L, "ERROR: key-pair out of size, string greater than 41 chars.\n");
	    return 2;
	} else {
	    strcpy(cert->public_txt, public);
	    strcpy(cert->secret_txt, secret);
	}
    } else
	if (zmq_curve_keypair(cert->public_txt, cert->secret_txt) != 0) {
	    lua_pushnil(L);
	    lua_pushfstring(L, "ERROR: unable to create curve keypair: %s\n", zmq_strerror( errno ));
	    return 2;
	}

    zmq_z85_decode(cert->public_key, cert->public_txt);
    zmq_z85_decode(cert->secret_key, cert->secret_txt);

    return 1;
}

static int key_public(lua_State *L) {
    cert_t *cert = checkkey(L);
    lua_pushstring(L, cert->public_txt);
    return 1;
}

static int key_secret(lua_State *L) {
    cert_t *cert = checkkey(L);
    lua_pushstring(L, cert->secret_txt);
    return 1;
}

/*
// To become a CURVE server, the application sets
// the ZMQ_CURVE_SERVER option on the socket
// and then sets the ZMQ_CURVE_SECRETKEY option
// with its secret key
static int key_server(lua_State *L) {
    cert_t *cert = checkkey(L);
    void *skt = *(void **)luaL_checkudata(L, 2, "caap.zmq.socket");
    int issrv = 1;
    size_t len = sizeof(cert->secret_key);

    int rc = zmq_setsockopt(skt, ZMQ_CURVE_SERVER, &issrv, sizeof(int));
    zmq_setsockopt(skt, ZMQ_CURVE_SECRETKEY, cert->secret_key, len);
    if (rc != 0) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: unable to configure CURVE server: %s\n", zmq_strerror( errno ));
	return 2;
    } else {
	lua_pushboolean(L, 1);
	return 1;
    }
}
*/

// To become a CURVE client, the application sets
// the ZMQ_CURVE_SERVERKEY option with the public key
// of the server it intends to connect to
// and then sets the ZMQ_CURVE_ PUBLICKEY & SECRETKEY
static int key_client(lua_State *L) {
    cert_t *cert = checkkey(L);
    void *skt = *(void **)luaL_checkudata(L, 2, "caap.zmq.socket");
    const char *public_txt = luaL_checkstring(L, 3);

    if (strlen(public_txt) > 40) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR: key-pair out of size, string greater than 40 chars.\n");
	return 2;
    }

    uint8_t public_key [32];
    zmq_z85_decode(public_key, public_txt);
    int rc = zmq_setsockopt(skt, ZMQ_CURVE_SERVERKEY, public_key, sizeof(public_key));
    if (rc != 0)
	goto ZERROR;

    int len = sizeof(cert->public_key);
    rc = zmq_setsockopt(skt, ZMQ_CURVE_PUBLICKEY, cert->public_key, len);
    if (rc != 0)
	goto ZERROR;

    len = sizeof(cert->secret_key);
    rc = zmq_setsockopt(skt, ZMQ_CURVE_SECRETKEY, cert->secret_key, len);

    ZERROR:
    if (rc != 0) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: unable to configure CURVE client: %s\n", zmq_strerror( errno ));
	return 2;
    } else {
	lua_pushboolean(L, 1);
	return 1;
    }
}

static int key_asstr(lua_State *L) {
    lua_pushstring(L, "zmq{Active Key-Pair}");
    return 1;
}

static int key_gc (lua_State *L) {
    cert_t *cert = checkkey(L);
    if (cert)
	cert = NULL;
    return 0;
}


// POLLER
//
static int new_poll_in(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    int i, N = luaL_len(L, 1);
    zmq_pollitem_t *pit, *it = (zmq_pollitem_t *)lua_newuserdata(L, N*sizeof(zmq_pollitem_t));

    for (i=0; i<N;) {
	pit = it+i;
	pit->fd = 0;
	pit->revents = 0;
	pit->events = ZMQ_POLLIN;
	    lua_rawgeti(L, 1, ++i);
	    void *skt = *(void **)luaL_checkudata(L, -1, "caap.zmq.socket");
	pit->socket = skt;
	    lua_pop(L, 1);
    }

    int rc = zmq_poll(it, N, -1);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: Unable to poll event: %s\n", zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);

    return 1;
}


// PROXY
//
// Built-in ZMQ proxy
// The proxy connects a frontend socket to a backend socket.
// Conceptually, data flows from frontend to backend. The proxy
// is fully symmetric and there is no technical difference
// between frontend and backend.
// Before calling zmq_proxy you must set any socket options, and
// connect or bind both frontend and backend sockets.
// zmq_proxy runs in the current thread and returns only if/when
// the current context is closed.
// If the capture socket is not NULL, the proxy shall send all
// messages, received on both frontend and backend, to the capture
// socket.

static int new_proxy(lua_State *L) {
    void *frontend = checkskt(L);
    void *backend = *(void **)luaL_checkudata(L, 2, "caap.zmq.socket");
    void *capture = NULL;
    if (lua_isuserdata(L, 3))
	capture = *(void **)luaL_checkudata(L, 3, "caap.zmq.socket");
    int rc = zmq_proxy(frontend, backend, capture);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: Unable to create proxy: %s\n", zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;

}

//
// SOCKET
//
// To create a connection between two nodes, you use zmq_bind and zmq_connect in the other.
// As a rule of thumb, the node that does zmq_bind is a "SERVER", and the node which does
// zmq_connect is a "CLIENT", with arbitrary network address. We bind a socket to an
// endpoint and connect a socket to an endpoint, the endpoint being a well-known address.
// When a socket is bound to an endpoint it automatically starts accepting connections. ZMQ
// will automatically reconnect if the network connection is broken.
// A server node can bind to many endpoints (a combination of protocol and address) and it
// can do this using a single socket.
// The "ipc" transport does allow a process to bind to an endpoint already used by a first
// process. It's meant to allow a process to recover after a crash.
// For most cases, use "tcp". It is elastic, portable and fast. It is a disconnected
// transport because ZeroMQ tcp transport doesn't require that the endpoint exists before
// you connect to it. Clients and servers can connect and bind at any time, can go and come
// back, and it remains transparent to applications.
// The inter-process "ipc" transport is disconnected, like tcp.
// The inter-thread transport, "inproc", is a connected signaling transport. It is much
// faster than tcp or ipc. However, the server MUST issue a BIND before any client issues
// a CONNECT. We create and bind one socket and start the child threads, which create and
// connect the other sockets.


static int skt_asstr(lua_State *L) {
    lua_pushstring(L, "zmq{Active Socket}");
    return 1;
}

static int skt_gc(lua_State *L) {
    void *skt = checkskt(L);
    if (skt && (zmq_close(skt) == 0))
	skt = NULL;
    return 0;
}

// Following a "zmq_bind", the socket enters a "mute" state unless or until at least one
// incoming or outgoing connection is made, at which point the socket enters a ready state.
// In the mute state, the socket blocks or drops messages according to the socket type. By
// contrast, following a "zmq_connect", the socket enters the "ready" state.

// Accept incoming connections on a socket
// binds the socket to a local endpoint (port)
// and then accepts incoming connections
static int skt_bind (lua_State *L) {
    void *skt = checkskt(L);
    const char *addr = luaL_checkstring(L, 2);
    if (zmq_bind(skt, addr) == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: Unable to bind socket to endpoint: %s: %s\n", addr, zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Unbind a socket specified by the socket
// argument from the endpoint specified by
// the endpoint argument
static int skt_unbind(lua_State *L) {
    void *skt = checkskt(L);
    const char *addr = luaL_checkstring(L, 2);
    if (-1 == zmq_unbind(skt, addr)) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: Unable to unbind socket to endpoint: %s: %s\n", addr, zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Create outgoing connection from socket
// connects the socket to an endpoint (port)
// and then accepts incoming connections
static int skt_connect (lua_State *L) {
    void *skt = checkskt(L);
    const char *addr = luaL_checkstring(L, 2);
    if (zmq_connect(skt, addr) == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: Unable to connect socket to endpoint: %s: %s\n", addr, zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Disconnect a socket from the endpoint.
// Any outstanding messages physically
// received from the network but not yet
// received by the application shall be
// discarded. The behaviour for discarding
// messages sent but not yet typically
// transferred to the network depends on
// the value of the ZMQ_LINGER option.
static int skt_disconnect (lua_State *L) {
    void *skt = checkskt(L);
    const char *addr = luaL_checkstring(L, 2);
    if (zmq_disconnect(skt, addr) == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: Unable to connect socket to endpoint: %s: %s\n", addr, zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Send a message part on a socket
// queue a message created from the buffer
// referenced by
// ZMQ_SNDMORE the message being sent
// is a multi-part message, further
// message parts are to follow
// a successful invocation indicates that
// the message has been queued on the socket
// An application that sends multi-part
// messages must use the ZMQ_SNDMORE flag
// when sending each message except the final
// The zmq_msg_t structure passed to
// zmq_msg_send is nullified during the call
// There is no way to cancel a partially sent
// message, except by closing the socket

int send_msg(lua_State *L, void *skt, int idx, int flags) {
    size_t len = 0;
    const char *data = luaL_checklstring(L, idx, &len);
    zmq_msg_t msg;
    int rc = zmq_msg_init_data( &msg, len==0 ? 0 :(void *)data, len, NULL, NULL ); // XXX send 0-length data
    if (rc == -1)
	return rc;
    return zmq_msg_send( &msg, skt, flags ); // either error or success
}

// sends ALL or NONE
static int skt_send_mult_msg(lua_State *L) {
    void *skt = checkskt(L);
    luaL_checktype(L, 2, LUA_TTABLE);
    int nowait = lua_toboolean(L, 3);

    int flags = ZMQ_SNDMORE | nowait;

    int i, N = luaL_len(L, 2);
    for (i=1; i<N; i++) {
	lua_rawgeti(L, 2, i);
	if (-1 == send_msg(L, skt, 3, flags)) {
	    lua_pushnil(L);
	    lua_pushfstring(L, "ERROR: message could not be sent, %s!", zmq_strerror( errno ));
	    return 2;
	}
	lua_pop(L, 1);
    }
    lua_rawgeti(L, 2, N);
    if (-1 == send_msg(L, skt, 3, nowait)) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: message could not be sent, %s!", zmq_strerror( errno ));
	return 2;
    }
    lua_pop(L, 1);
    lua_pushinteger(L, N); // number of messages sent
    return 1;
}

static int skt_send_msg(lua_State *L) {
    void *skt = checkskt(L);
    int nowait = lua_toboolean(L, 3);

    int rc = send_msg(L, skt, 2, nowait);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: message could not be sent, %s!", zmq_strerror( errno ));
	return 2;
    }
    lua_pushinteger(L, rc); // length of message sent
    return 1;
}

// Receive a message part from a socket
// receive a message from the socket referenced
// by the socket argument and store it in the
// buffer buf. Any bytes exceeding the length
// shall be truncated.
// An application that process multi-part messages
// must use the ZMQ_RCVMORE option to determine if
// there are futher parts to receive.
// Shall return number of bytes in the message.
//
// MESSAGES
//
// A routing ID is set on all messages received by a ZMQ_SERVER socket.
// To send a message, your must set the routing ID of a connected peer.
//
// *IO*
// data from zmq_msg_data should be read as
// byte sequence (cast: void * -> uint8_t *)
// special case for monitoring events

const char* skt_transport_events(int e) {
    switch(e) {
	case ZMQ_EVENT_CONNECTED: return "CONNECTED"; break;
	case ZMQ_EVENT_CONNECT_DELAYED: return "CONNECT_DELAYED"; break;
	case ZMQ_EVENT_CONNECT_RETRIED: return "CONNECT_RETRIED"; break;
	case ZMQ_EVENT_LISTENING: return "LISTENING"; break;
	case ZMQ_EVENT_BIND_FAILED: return "BIND_FAILED"; break;
	case ZMQ_EVENT_ACCEPTED: return "ACCEPTED"; break;
	case ZMQ_EVENT_ACCEPT_FAILED: return "ACCEPT_FAILED"; break;
	case ZMQ_EVENT_CLOSED: return "CLOSED"; break;
	case ZMQ_EVENT_CLOSE_FAILED: return "CLOSE_FAILED"; break;
	case ZMQ_EVENT_DISCONNECTED: return "DISCONNECTED"; break;
	default: return "unknown";
    }
}

int recv_msg(lua_State *L, void *skt, int nowait) {
    int rc;
    zmq_msg_t msg;
    rc = zmq_msg_init( &msg ); // always returns ZERO, no errors are defined
    rc = zmq_msg_recv(&msg, skt, nowait ? ZMQ_DONTWAIT : 0);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: receiving message from a socket failed, %s!", zmq_strerror( errno ));
	return 2;
    }
    if (rc > 0) { // *IO*
	size_t len = zmq_msg_size( &msg );
	rc = strlen( lua_pushlstring(L, zmq_msg_data( &msg ), len) );
	if ((rc != len) && (len > 2)) {
	    uint8_t *data = (uint8_t *)zmq_msg_data( &msg );
	    const char* mev = skt_transport_events(*(uint16_t *)data);
	    if (strcmp(mev, "unknown") == 0)
		lua_pushlstring(L, (const char *)data, len);
	    else
		lua_pushfstring(L, "%s %d", mev, *(uint32_t *)(data + 2));
	}
    }
    else
	lua_pushnil(L);
    rc = zmq_msg_close( &msg );
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: message could not be closed properly, %s!", zmq_strerror( errno ));
	return 2;
    }
    return zmq_msg_more( &msg );
}

static int mult_part_msg(lua_State *L) {
    int nowait = lua_toboolean(L, lua_upvalueindex(1));
    void *skt = checkskt(L);	   // state
    int cnt = lua_tointeger(L, 2); // counter
    if (cnt > 0 && lua_toboolean(L, 3) == 0) // NO more msg's in queue
	return 0;
    lua_pushinteger(L, ++cnt);	   // increment counter
    lua_pushboolean(L, recv_msg(L, skt, nowait)); // msg & 'more'
    return 3;
}

static int skt_recv_mult_msg(lua_State *L) {
    int nowait = lua_toboolean(L, 2); // NOWAIT flag
    lua_pushboolean(L, nowait);
    lua_pushcclosure(L, &mult_part_msg, 1);	// iter function + upvalue(NOWAIT flag)
    lua_pushvalue(L, 1);			// state := socket
    lua_pushinteger(L, 0);			// initialize counter to zero
    return 3;
}

static int skt_recv_msg(lua_State *L) {
    void *skt = checkskt(L);
    int nowait = lua_toboolean(L, 2); // NOWAIT flag
    lua_pushboolean(L, recv_msg(L, skt, nowait)); // msg & 'more'
    return 2;
}

// Set the ROUTING ID when connecting to a ROUTER
// if empty, sets a simple random printable identity
// An applicaction that uses a ROUTER to talk to specific
// peers can convert a logical address to an identity.
// Because the ROUTER only announces the identity when
// that peer sends a message, you can only really reply.
// However you can force the ROUTER to use a logical
// address in place of its identity, setting the socket
// identiy, using zmq_setsockopt. At connection time the
// peer socket tells the router socket, use this identity
// for this connection. Otherwise, the ROUTER generates
// its usual arbitrary random identity.
static int skt_set_id(lua_State *L) {
    void *skt = checkskt(L);
    char identity[10];
    size_t len;
    int rc;

    if(lua_gettop(L) == 2) {
	const char* idd = luaL_checklstring(L, 2, &len);
	rc = zmq_setsockopt(skt, ZMQ_ROUTING_ID, idd, len);
    } else {
	sprintf(identity, "%04X-%04X", randof(0x10000), randof(0x10000));
	len = strlen(identity);
	rc = zmq_setsockopt(skt, ZMQ_ROUTING_ID, identity, len);
    }
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: random identity could not be set on socket, %s!", zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Set the PEER ID of the peer connected via zmq_connect
// Only applies to the first subsequent call to zmq_connect
// if empty, sets a simple random printable identity
static int skt_set_rid(lua_State *L) {
    void *skt = checkskt(L);

    char identity[10];
    size_t len;
    int rc;

    if(lua_gettop(L) == 2) {
	const char* idd = luaL_checklstring(L, 2, &len);
	rc = zmq_setsockopt(skt, ZMQ_CONNECT_ROUTING_ID, idd, len);
    } else {
	sprintf(identity, "%04X-%04X", randof(0x10000), randof(0x10000));
	len = strlen(identity);
	rc = zmq_setsockopt(skt, ZMQ_CONNECT_ROUTING_ID, identity, len);
    }
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: random identity could not be set on socket, %s!", zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int skt_subscribe(lua_State *L) {
    void *skt = checkskt(L);
    const char* filter = luaL_checkstring(L, 2);
    int rc = zmq_setsockopt(skt, ZMQ_SUBSCRIBE, filter, strlen(filter));
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: setting subscribe filter for socket, %s!", zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int skt_unsubscribe(lua_State *L) {
    void *skt = checkskt(L);
    const char* filter = luaL_checkstring(L, 2);
    int rc = zmq_setsockopt(skt, ZMQ_UNSUBSCRIBE, filter, strlen(filter));
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: unsetting subscribe filter for socket, %s!", zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int skt_stream_notify(lua_State *L) {
    void *skt = checkskt(L);
    int notify = lua_toboolean(L, 2);
    size_t len = sizeof(notify);

    int rc = zmq_setsockopt(skt, ZMQ_STREAM_NOTIFY, &notify, len);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: setting stream notifications for socket, %s!", zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

// by default queues will fill on outgoing connections
// enve if the connection has not completed. This can
// lead to lost messages. If this option is set to 1
// messages shall be queued only to completed connections.
static int skt_immediate(lua_State *L) {
    void *skt = checkskt(L);
    int swift = lua_toboolean(L, 2);
    size_t len = sizeof(swift);

    int rc = zmq_setsockopt(skt, ZMQ_IMMEDIATE, &swift, len);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: setting immediate flag for socket, %s!", zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int skt_events(lua_State *L) {
    void *skt = checkskt(L);
    int flag = ZMQ_POLLIN;
    size_t len = sizeof(flag);
    if (zmq_getsockopt(skt, ZMQ_EVENTS, &flag, &len) == 0) { // success!!!
	if (flag & ZMQ_POLLIN)
	    lua_pushstring(L, "POLLIN");
	else if (flag & ZMQ_POLLOUT)
	    lua_pushstring(L, "POLLOUT");
	else
	    lua_pushboolean(L, 0);
	return 1;
    } else {
	lua_pushnil(L);
    	lua_pushfstring(L, "ERROR: getting sockopt EVENTS for socket, %s!", zmq_strerror( errno ));
	return 2;
    }
}

static int skt_monitor(lua_State *L) {
    void *skt = checkskt(L); // cliente a monitorear
    const char* endpoint = luaL_checkstring(L, 2);

    int rc = zmq_socket_monitor(skt, endpoint, ZMQ_EVENT_ALL);
    if (rc != 0) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: starting monitor on %s, %s!", endpoint, zmq_strerror( errno ));
	return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

// set the linger period which determines how long pending
// messages which have yet to be sent to a peer shall linger
// in memory after a socket is disconnected or closed.
// a default value of -1 specifies an infinite linger period.
// a value of 0 specifies no linger period.
// positive values specify an upper bound for the linger
// period in milliseconds.
static int skt_linger(lua_State *L) {
    void *skt = checkskt(L);
    int msecs = luaL_checkinteger(L, 2);
    size_t len = sizeof(msecs);

    int rc = zmq_setsockopt(skt, ZMQ_IMMEDIATE, &msecs, len);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: setting linger period for socket, %s!", zmq_strerror( errno ));
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

// To become a CURVE server, the application sets
// the ZMQ_CURVE_SERVER option on the socket
// and then sets the ZMQ_CURVE_SECRETKEY option
// with its secret key
static int skt_curve_server(lua_State *L) {
    void *skt = checkskt(L);
    const char* secret_txt = luaL_checkstring(L, 2);

    if (strlen(secret_txt) > 40) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR: key-pair out of size, string greater than 40 chars.\n");
	return 2;
    }

    uint8_t secret_key [32];
    zmq_z85_decode(secret_key, secret_txt);
    int issrv = 1;
    int rc = zmq_setsockopt(skt, ZMQ_CURVE_SERVER, &issrv, sizeof(int));
    if (rc != 0)
	goto ZERRORS;

    rc = zmq_setsockopt(skt, ZMQ_CURVE_SECRETKEY, secret_key, sizeof(secret_key));

    ZERRORS:
    if (rc != 0) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: unable to configure CURVE server: %s\n", zmq_strerror( errno ));
	return 2;
    } else {
	lua_pushboolean(L, 1);
	return 1;
    }
}


// // // // // // // // // // // // //
//
//  add "socket types" as upvalue to method "socket"
//
static void socket_fn(lua_State *L) {
    lua_newtable(L); // upvalue
    lua_pushinteger(L, ZMQ_PAIR); lua_setfield(L, -2, "PAIR");
    lua_pushinteger(L, ZMQ_PUB);  lua_setfield(L, -2, "PUB");
    lua_pushinteger(L, ZMQ_SUB);  lua_setfield(L, -2, "SUB");
    lua_pushinteger(L, ZMQ_REQ);  lua_setfield(L, -2, "REQ");
    lua_pushinteger(L, ZMQ_REP);  lua_setfield(L, -2, "REP");
    lua_pushinteger(L, ZMQ_DEALER); lua_setfield(L, -2, "DEALER");
    lua_pushinteger(L, ZMQ_ROUTER); lua_setfield(L, -2, "ROUTER");
    lua_pushinteger(L, ZMQ_PULL); lua_setfield(L, -2, "PULL");
    lua_pushinteger(L, ZMQ_PUSH); lua_setfield(L, -2, "PUSH");
    lua_pushinteger(L, ZMQ_XPUB); lua_setfield(L, -2, "XPUB");
    lua_pushinteger(L, ZMQ_XSUB); lua_setfield(L, -2, "XSUB");
    lua_pushinteger(L, ZMQ_STREAM); lua_setfield(L, -2, "STREAM");
    lua_pushcclosure(L, &new_socket, 1);
    lua_setfield(L, -2, "socket");
}

// // // // // // // // // // // // //

static const struct luaL_Reg zmq_funcs[] = {
    {"context", new_context},
    {"proxy",   new_proxy},
    {"pollin",  new_poll_in},
    {"keypair", new_keypair},
    {NULL, 	NULL}
};

static const struct luaL_Reg ctx_meths[] = {
    {"__tostring", ctx_asstr},
    {"__gc",	   ctx_gc},
    {NULL,	   NULL}
};

static const struct luaL_Reg key_meths[] = {
    {"public", key_public},
    {"secret", key_secret},
    {"client", key_client},
    {"__tostring", key_asstr},
    {"__gc",	   key_gc},
    {NULL,	   NULL}
};

static const struct luaL_Reg skt_meths[] = {
    {"bind",	   skt_bind},
    {"unbind",	   skt_unbind},
    {"connect",	   skt_connect},
    {"disconnect", skt_disconnect},
    {"linger",     skt_linger},
    {"send_msg",   skt_send_msg},
    {"send_msgs",  skt_send_mult_msg},
    {"recv_msg",   skt_recv_msg},
    {"recv_msgs",  skt_recv_mult_msg},
    {"events",	   skt_events},
    {"monitor",    skt_monitor},
    {"curve", 	   skt_curve_server},
    {"__tostring", skt_asstr},
    {"__gc",	   skt_gc},
    {NULL,	   NULL}
};

/*   ******************************   */
int luaopen_lzmq (lua_State *L) {
    luaL_newmetatable(L, "caap.zmq.context");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, ctx_meths, 0);
    socket_fn(L);

    luaL_newmetatable(L, "caap.zmq.socket");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, skt_meths, 0);

    luaL_newmetatable(L, "caap.zmq.keypair");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, key_meths, 0);

    // create library
    luaL_newlib(L, zmq_funcs);
    return 1;
}

