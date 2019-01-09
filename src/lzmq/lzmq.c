#include <lua.h>
#include <lauxlib.h>

#include <string.h>
#include <errno.h>

#include <zmq.h>

#define randof(num) (int)((float)(num)*random()/(RAND_MAX+1.0))

#define checkctx(L) *(void **)luaL_checkudata(L, 1, "caap.zmq.context")
#define checkskt(L) *(void **)luaL_checkudata(L, 1, "caap.zmq.socket")

extern int errno;

static char *err2str() {
    switch(errno) {
	case EAGAIN: return "non-blocking mode was requested and the message cannot be sent";
	case ENOTSUP: return "operation not supperted by this socket type";
	case EINVAL: return "trying to send multipart data, which this socket type does not allow";
	case ETERM: return "the context associated with this socket was terminated";
	case EFSM: return "socket not being in an appropriate state";
	case EFAULT: return "message passed was invalid";
	case ENOTSOCK: return "this socket is invalid";
	case EINTR: return "the operation was interrupted by a signal before it was done";
	case EHOSTUNREACH: return "the message cannot be routed";
	default: return "undefined error ocurred";
    }
}

// Thread-safe SOCKETS:
// ZMQ_CLIENT, ZMQ_SERVER, ZMQ_DISH, ZMQ_RADIO, ZMQ_SCATTER, ZMQ_GATHER
//
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

static int new_socket(lua_State *L) {
    void *ctx = checkctx(L);
    int type = ZMQ_STREAM;

    void **pskt = (void **)lua_newuserdata(L, sizeof(void *));
    luaL_getmetatable(L, "caap.zmq.socket");
    lua_setmetatable(L, -2);

    *pskt = zmq_socket(ctx, type);
    if (*pskt == NULL) {
	lua_pushnil(L);
	lua_pushstring(L, "Error creating ZMQ socket\n");
	return 2;
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

// Accept incoming connections on a socket
// binds the socket to a local endpoint (port)
// and then accepts incoming connections
static int skt_bind (lua_State *L) {
    void *skt = checkskt(L);
    const char *addr = luaL_checkstring(L, 2);
    if (zmq_bind(skt, addr) == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: Unable to bind socket to endpoint: %s\n", addr);
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
	lua_pushfstring(L, "ERROR: Unable to connect socket to endpoint: %s\n", addr);
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

int send_msg(lua_State *L, void *skt, int idx, int multip) {
    size_t len = 0;
    const char *data = luaL_checklstring(L, idx, &len);
    zmq_msg_t msg;
    int rc = zmq_msg_init_data( &msg, len==0 ? 0 :(void *)data, len, NULL, NULL ); // XXX send 0-length data
    if (rc == -1)
	return rc;
    rc = zmq_msg_send( &msg, skt, multip );
    return rc; // either error or success
}

// XXX return value is number or errors, should add table w errno's & msg's
static int skt_send_mult_msg(lua_State *L) {
    void *skt = checkskt(L);
    luaL_checktype(L, 2, LUA_TTABLE);
    int i, rc = 0, N = luaL_len(L, 2);
    for (i=1; i<N; i++) {
	lua_rawgeti(L, 2, i);
	if (-1 == send_msg(L, skt, 3, ZMQ_SNDMORE))
	    rc++;
	lua_pop(L, 1);
    }
    lua_rawgeti(L, 2, N);
    if (-1 == send_msg(L, skt, 3, 0))
	rc++;
    lua_pop(L, 1);
    lua_pushinteger(L, rc);
    return 1;
}

static int skt_send_msg(lua_State *L) {
    void *skt = checkskt(L);
    int mult = luaL_checkinteger(L, 2);
    int rc = send_msg(L, skt, 2, mult);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: message could not be sent, %s!", err2str());
	return 2;
    }
    lua_pushinteger(L, rc);
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

int recv_msg(lua_State *L, void *skt) {
    int rc;
    zmq_msg_t msg;
    rc = zmq_msg_init( &msg ); // always returns ZERO, no errors are defined
    rc = zmq_msg_recv(&msg, skt, 0); // BLOCK until receive a message from a socket
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: receiving message from a socket failed, %s!", err2str());
	return 2;
    }
    if (rc > 0)
	lua_pushlstring(L, zmq_msg_data( &msg ), zmq_msg_size( &msg ));
    else
	lua_pushnil(L);
    rc = zmq_msg_close( &msg );
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: message could not be closed properly, %s!", err2str());
	return 2;
    }
    return zmq_msg_more( &msg );
}

static int mult_part_msg(lua_State *L) {
    void *skt = checkskt(L);	   // state
    int cnt = lua_tointeger(L, 2); // counter
    if (cnt > 0 && lua_toboolean(L, 3) == 0) // NO more msg's in queue
	return 0;
    lua_pushinteger(L, ++cnt);	   // increment counter
    lua_pushboolean(L, recv_msg(L, skt)); // msg & 'more'
    return 3;
}

static int skt_recv_mult_msg(lua_State *L) {
    lua_pushboolean(L, 1);			// upvalue 'more'
    lua_pushcclosure(L, &mult_part_msg, 1);	// iter function
    lua_pushvalue(L, 1);			// state := socket
    lua_pushinteger(L, 0);			// initialize counter to zero
    return 3;
}

static int skt_recv_msg(lua_State *L) {
    void *skt = checkskt(L);
    lua_pushboolean(L, recv_msg(L, skt)); // could 'pushboolean' to know if a multi-part msg
    return 2;
}

// Set simple random printable identity on socket
static int skt_set_id(lua_State *L) {
    void *skt = checkskt(L);
    char identity[10];
    sprintf(identity, "%04X-%04X", randof(0x10000), randof(0x10000));
    size_t len = strlen(identity);
    int rc = zmq_setsockopt(skt, ZMQ_IDENTITY, identity, len);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: random identity could not be set on socket, %s!", err2str());
	return 2;
    }
    lua_pushlstring(L, identity, len);
    return 1;
}

// // // // // // // // // // // // //

static const struct luaL_Reg zmq_funcs[] = {
    {"context", new_context},
    {NULL, NULL}
};

static const struct luaL_Reg ctx_meths[] = {
    {"socket",	   new_socket},
    {"__tostring", ctx_asstr},
    {"__gc",	   ctx_gc},
    {NULL,	   NULL}
};

static const struct luaL_Reg skt_meths[] = {
    {"bind",	   skt_bind},
    {"connect",	   skt_connect},
    {"send_msg",   skt_send_msg},
    {"send_msgs",  skt_send_mult_msg},
    {"recv_msg",   skt_recv_msg},
    {"recv_msgs",  skt_recv_mult_msg},
    {"set_id",	   skt_set_id},
    {"__tostring", skt_asstr},
    {"__gc",	   skt_gc},
    {NULL,	   NULL}
};

int luaopen_lzmq (lua_State *L) {
    luaL_newmetatable(L, "caap.zmq.context");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, ctx_meths, 0);

    luaL_newmetatable(L, "caap.zmq.socket");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, skt_meths, 0);

    // create library
    luaL_newlib(L, zmq_funcs);
    return 1;
}

/*

static int skt_send (lua_State *L) {
    void *skt = checkskt(L);
    size_t len = 0;
    const char *msg = luaL_checklstring(L, 2, &len);
    int rc, multip;

    multip = lua_toboolean(L, 3) ? ZMQ_SNDMORE : 0;

    if (0 == len)
	rc = zmq_send(skt, 0, 0, 0);
    else
	rc = zmq_send(skt, msg, len, multip);

    if (rc == -1) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR: socket could not send message!");
	return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

static int skt_recv (lua_State *L) {
    void *skt = checkskt(L);
    int allp = 0;

    char raw [256];
    size_t raw_size = 256;

   if (lua_toboolean(L, 2)) {
	allp = 1;
	lua_newtable(L);
   }

    do {
	raw_size = zmq_recv(skt, raw, 256, 0);
	if (raw_size > 0)
	    lua_pushlstring(L, raw, raw_size);
	else
	    lua_pushnil(L);
	if (allp)
	    lua_rawseti(L, -2, allp++);
    } while (allp && (raw_size == 256));

    return 1;
}

static int skt_recv_id (lua_State *L) {
    void *skt = checkskt(L);

    char id [256];
    size_t id_size = zmq_recv(skt, id, 256, 0);
    if (id_size < 1) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR: socket received an invalid ID!");
	return 2;
    }

    lua_pushlstring(L, id, id_size);
    return 1;
}

static int skt_send_id (lua_State *L) {
    void *skt = checkskt(L);
    size_t id_size;
    const char *id = luaL_checklstring(L, 2, &id_size);

    int rc = zmq_send(skt, id, id_size, ZMQ_SNDMORE);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: socket ID could not be sent due to %s!", err2str());
	return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

*/
