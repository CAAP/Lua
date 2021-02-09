#include <lua.h>
#include <lauxlib.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <zmq.h>
#include <errno.h>

#define randof(num) (int)((float)(num)*rand()/(RAND_MAX+1.0))

static void *CTX;

typedef struct {
    uint8_t public_key [32];
    uint8_t secret_key [32];
    char public_txt [41];
    char secret_txt [41];
} cert_t;

#define checkctx(L) *(void **)luaL_checkudata(L, 1, "caap.zmq.context")
#define checkskt(L,k) *(void **)luaL_checkudata(L, k, "caap.zmq.socket")
#define checkkey(L) (cert_t *)luaL_checkudata(L, 1, "caap.zmq.keypair")

extern int errno;

#define zmqError(L, rc, emsg)\
    if ((rc)) {\
	lua_pushnil(L);\
	lua_pushfstring(L, "%s: %s\n", (emsg), zmq_strerror( errno ));\
	return 2;\
    }


// Key pair
//
static int new_keypair(lua_State *L) {
    cert_t *cert = (cert_t *)lua_newuserdata(L, sizeof(cert_t));
    luaL_setmetatable(L, "caap.zmq.keypair");

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
    int K = -1;

    if (lua_type(L, 2) == LUA_TNUMBER)
	K = luaL_checkinteger(L, 2);

    zmq_pollitem_t *pit, *it = (zmq_pollitem_t *)lua_newuserdata(L, N*sizeof(zmq_pollitem_t));

    for (i=0; i<N;) {
	pit = it+i;
	pit->fd = 0;
	pit->revents = 0;
	pit->events = ZMQ_POLLIN;
	    lua_rawgeti(L, 1, ++i); // +1
	pit->socket = checkskt(L, -1);;
	    lua_pop(L, 1); // -1
    }

    int rc = zmq_poll(it, N, K);
    zmqError(L, rc == -1, "ERROR: Unable to poll event");

    lua_pushinteger(L, rc);
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
    void *frontend = checkskt(L, 1);
    void *backend = checkskt(L, 2); // luaL_checkudata(L, 2, "caap.zmq.socket")
    void *capture = NULL;
    if (lua_isuserdata(L, 3))
	capture = checkskt(L, 3); // luaL_checkudata(L, 3, "caap.zmq.socket")
    int rc = zmq_proxy(frontend, backend, capture);
    zmqError(L, rc == -1, "ERROR: Unable to create proxy");

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
    checkskt(L, 1);
    lua_getuservalue(L, 1);
    lua_pushfstring(L, "zmq{Socket: %s}", lua_tostring(L, 2));
    return 1;
}

static int skt_gc(lua_State *L) {
    void *skt = checkskt(L, 1);
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
    void *skt = checkskt(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    int rc = zmq_bind(skt, addr);
    zmqError(L, rc == -1, "ERROR: Unable to bind socket to endpoint");

    lua_pushboolean(L, 1);
    return 1;
}

// Unbind a socket specified by the socket
// argument from the endpoint specified by
// the endpoint argument
static int skt_unbind(lua_State *L) {
    void *skt = checkskt(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    int rc = zmq_unbind(skt, addr);
    zmqError(L, rc == -1, "ERROR: Unable to unbind socket to endpoint"); // addr

    lua_pushboolean(L, 1);
    return 1;
}

// Create outgoing connection from socket
// connects the socket to an endpoint (port)
// and then accepts incoming connections
static int skt_connect (lua_State *L) {
    void *skt = checkskt(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    int rc = zmq_connect(skt, addr);
    zmqError(L, rc == -1, "ERROR: Unable to connect socket to endpoint"); // addr

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
    void *skt = checkskt(L, 1);
    const char *addr = luaL_checkstring(L, 2);
    int rc = zmq_disconnect(skt, addr);
    zmqError(L, rc == -1, "ERROR: Unable to disconnect socket to endpoint");

    lua_pushboolean(L, 1);
    return 1;
}

// Send a message part on a socket
// queue a message created from the buffer
// referenced by zmq_msg_t
// ZMQ_SNDMORE the message being sent
// is a multi-part message, further
// message parts are to follow
// An application that sends multi-part
// messages must use the ZMQ_SNDMORE flag
// when sending each message except the final
// The zmq_msg_t structure passed to
// zmq_msg_send is nullified during the call
// Upon success, message is queued on the socket
// and do not need to call zmq_msg_close
// There is no way to cancel a partially sent
// message, except by closing the socket

static int send_msg(lua_State *L, void *skt, int idx, int flags) {
    size_t len = 0;
    zmq_msg_t msg;

    const char *data = luaL_checklstring(L, idx, &len);

    int rc = zmq_msg_init_data( &msg, (void *)data, len, NULL, NULL );
    if (rc == -1)
	return rc;
    return zmq_msg_send( &msg, skt, flags ); // either error or length of message sent
}

// sends ALL or NONE
static int skt_send_mult_msg(lua_State *L) {
    void *skt = checkskt(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    int nowait = lua_toboolean(L, 3);

    int flags = ZMQ_SNDMORE | nowait;

    int i, N = luaL_len(L, 2);
    for (i=1; i<N; i++) {
	lua_rawgeti(L, 2, i);
	if (-1 == send_msg(L, skt, -1, flags)) {
	    lua_pushnil(L);
	    lua_pushfstring(L, "ERROR: message could not be sent, %s!", zmq_strerror( errno ));
	    return 2;
	}
	lua_pop(L, 1);
    }
    lua_rawgeti(L, 2, N);
    if (-1 == send_msg(L, skt, -1, nowait)) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: message could not be sent, %s!", zmq_strerror( errno ));
	return 2;
    }
    lua_pop(L, 1);
    lua_pushinteger(L, N); // number of messages sent
    return 1;
}

static int skt_send_msg(lua_State *L) {
    void *skt = checkskt(L, 1);
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

/*
const char* skt_transport_event(uint16_t e) {
    switch(e) {
	case ZMQ_EVENT_CONNECTED:  return "CONNECTED"; break;
	case ZMQ_EVENT_CONNECT_DELAYED: return "CONNECT_DELAYED"; break;
	case ZMQ_EVENT_CONNECT_RETRIED: return "CONNECT_RETRIED"; break;
	case ZMQ_EVENT_LISTENING: return "LISTENING"; break;
	case ZMQ_EVENT_BIND_FAILED: return "BIND_FAILED"; break;
	case ZMQ_EVENT_ACCEPTED: return "ACCEPTED"; break;
	case ZMQ_EVENT_ACCEPT_FAILED: return "ACCEPT_FAILED"; break;
	case ZMQ_EVENT_CLOSED: return "CLOSED"; break;
	case ZMQ_EVENT_CLOSE_FAILED: return "CLOSE_FAILED"; break;
	case ZMQ_EVENT_DISCONNECTED: return "DISCONNECTED"; break;
	case ZMQ_EVENT_MONITOR_STOPPED: return "STOPPED"; break;
	case ZMQ_EVENT_HANDSHAKE_FAILED_NO_DETAIL: return zmq_strerror( errno ); break;
	default: return "unknown";
    }
}

static int skt_monitor_event(lua_State *L) {
    checkskt(L, 1);
    uint16_t e = (uint16_t)luaL_checkinteger(L, 2);
    lua_pushstring(L, skt_transport_event(e));
    return 1;
}
*/

// Receive a message part from a socket.
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

static int recv_msg(lua_State *L, void *skt, int nowait) {
    int rc;
    zmq_msg_t msg;
    rc = zmq_msg_init( &msg ); // always returns ZERO, no errors are defined
    rc = zmq_msg_recv(&msg, skt, nowait ? ZMQ_DONTWAIT : 0);
    if (rc == -1) {
	lua_pushfstring(L, "ERROR: receiving message from a socket failed, %s!", zmq_strerror( errno ));
	return rc;
    }
    if (rc > 0) { // *IO* - number of bytes in the message
	size_t len = zmq_msg_size( &msg );
	uint8_t *data = (uint8_t *)zmq_msg_data( &msg );
	rc = strlen( lua_pushlstring(L, (const char *)data, len) );
    } else // empty message ;(
	lua_pushstring(L,"");
    rc = zmq_msg_close( &msg );
    if (rc == -1) {
	lua_pushfstring(L, "ERROR: message could not be closed properly, %s!", zmq_strerror( errno ));
	return rc;
    }
    return zmq_msg_more( &msg );
}

static int mult_part_msg(lua_State *L) {
    int nowait = lua_toboolean(L, lua_upvalueindex(1));
    void *skt = checkskt(L, 1);	   // state
    int flag = lua_toboolean(L, 2); // MORE_FLAG

    if (!flag)
	return 0;

    lua_pushboolean(L, recv_msg(L, skt, nowait) == 1); // msg & 'more'
    lua_rotate(L, 3, 1); // 'more' & msg
    return 2;
}

static int skt_iter_msg(lua_State *L) {
    checkskt(L,1);
    int nowait = lua_toboolean(L, 2); // NOWAIT flag
    lua_pushboolean(L, nowait);
    lua_pushcclosure(L, &mult_part_msg, 1);	// iter function + upvalue(NOWAIT flag)
    lua_pushvalue(L, 1);			// state := socket
    lua_pushboolean(L, 1);			// initialize MORE_FLAG to TRUE
    return 3;
}


static int skt_recv_mult_msg(lua_State *L) {
    void *skt = checkskt(L, 1);
    int nowait = lua_toboolean(L, 2); // NOWAIT flag
    lua_newtable(L); int k = 1;
    while( recv_msg(L, skt, nowait) == 1)
	lua_rawseti(L, -2, k++);
    lua_rawseti(L, -2, k++); // last message received
    return 1;
}

static int skt_recv_msg(lua_State *L) {
    void *skt = checkskt(L, 1);
    int nowait = lua_toboolean(L, 2); // NOWAIT flag
    lua_pushboolean(L, recv_msg(L, skt, nowait) == 1); // msg & 'more'
    return 2;
}
/*
static int skt_monitor(lua_State *L) {
    void *skt = checkskt(L, 1); // cliente a monitorear
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
*/
// To become a CURVE client, the application sets
// the ZMQ_CURVE_SERVERKEY option with the public key
// of the server it intends to connect to
// and then sets the ZMQ_CURVE_ PUBLICKEY & SECRETKEY
static int skt_curve_client(lua_State *L) {
    void *skt = checkskt(L, 1);
    size_t len;
    const char *server_public_key = luaL_checklstring(L, 2, &len);

    if (len != 40) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR: key-pair out of size, string greater than 40 chars.\n");
	return 2;
    }

    int rc = zmq_setsockopt(skt, ZMQ_CURVE_SERVERKEY, server_public_key, len);
    if (rc != 0)
	goto ZERROR;

    char public_key [41];
    char secret_key [41];
    rc = zmq_curve_keypair(public_key, secret_key);
    if (rc != 0)
	goto ZERROR;
    len = sizeof(public_key);
    rc = zmq_setsockopt(skt, ZMQ_CURVE_PUBLICKEY, public_key, len);
    if (rc != 0)
	goto ZERROR;

    len = sizeof(secret_key);
    rc = zmq_setsockopt(skt, ZMQ_CURVE_SECRETKEY, secret_key, len);

    ZERROR:
    zmqError(L, rc != 0, "ERROR: unable to configure CURVE client");

    lua_pushboolean(L, 1);
    return 1;
}


// To become a CURVE server, the application sets
// the ZMQ_CURVE_SERVER option on the socket
// and then sets the ZMQ_CURVE_SECRETKEY option
// with its secret key
static int skt_curve_server(lua_State *L) {
    void *skt = checkskt(L, 1);
    int rc;
    size_t len;
    const char* secret_key = luaL_checklstring(L, 2, &len);

    if (len > 40) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR: key-pair out of size, string greater than 40 chars.\n");
	return 2;
    }

    rc = zmq_setsockopt(skt, ZMQ_CURVE_SECRETKEY, secret_key, len);
    if (rc != 0)
	goto ZERRORS;

    int issrv = 1;
    len = sizeof(issrv);
    rc = zmq_setsockopt(skt, ZMQ_CURVE_SERVER, &issrv, len);

    ZERRORS:
    zmqError(L, rc != 0, "ERROR: unable to configure CURVE server");

    lua_pushboolean(L, 1);
    return 1;
}

/* *********************************************** */
/*		OPTIONS				   */
/* *********************************************** */

static int boolean_opt(lua_State *L, const int N, const uint8_t opt) {
    void *skt = checkskt(L, 1);
    int rc, v;
    if (N == 2) {
	size_t len = sizeof(v);
	rc = zmq_getsockopt(skt, opt, &v, &len);
	zmqError(L, rc == -1, "error while getting value for socket option\n");
	lua_pushboolean(L, v); // +1
    } else {
	v = lua_toboolean(L, 3);
	rc = zmq_setsockopt(skt, opt, &v, sizeof(v));
	zmqError(L, rc == -1, "error while setting value for socket option\n");
	lua_pushboolean(L, 1); // +1
    }
    return 1;
}

static int integer_opt(lua_State *L, const int N, const uint8_t opt) {
    void *skt = checkskt(L, 1);
    int rc, v;
    if (N == 2) {
	size_t len = sizeof(v);
	rc = zmq_getsockopt(skt, opt, &v, &len);
	zmqError(L, rc == -1, "error while getting value for socket option\n");
	lua_pushinteger(L, v); // +1
    } else {
	v = luaL_optinteger(L, 3, 0);
	rc = zmq_setsockopt(skt, opt, &v, sizeof(v));
	zmqError(L, rc == -1, "error while setting value for socket option\n");
	lua_pushboolean(L, 1); // +1
    }
    return 1;
}

static int string_opt(lua_State *L, const int N, const uint8_t opt) {
    void *skt = checkskt(L, 1);
    int rc;
    if (N == 2) {
	char v[255];
	size_t len = sizeof(v);
	rc = zmq_getsockopt(skt, opt, v, &len);
	zmqError(L, rc == -1, "error while getting value for socket option\n");
	lua_pushlstring(L, v, len); // +1
    } else {
	size_t len;
	const char *v = luaL_optlstring(L, 3, "", &len);
	rc = zmq_setsockopt(skt, opt, v, len);
	zmqError(L, rc == -1, "error while setting value for socket option\n");
	lua_pushboolean(L, 1); // +1
    }
    return 1;
}

static int skt_option(lua_State *L) {
    void *skt = checkskt(L, 1);
    const int N = lua_gettop(L);
    int rc = lua_getfield(L, lua_upvalueindex(1), luaL_checkstring(L, 2)); // +1
    zmqError(L, rc != LUA_TNUMBER, "error, unknown socket option\n");
    const uint8_t opt = lua_tointeger(L, -1);
    lua_pop(L, 1); // -1

    int skt_type, v;
    size_t len = sizeof(skt_type);
    rc = zmq_getsockopt(skt, ZMQ_TYPE, &skt_type, &len);
    zmqError(L, rc == -1, "error while getting socket's type option\n");

    switch(opt) {
	case ZMQ_IMMEDIATE:
		return boolean_opt(L, N, opt);

	case ZMQ_THREAD_SAFE:
		return boolean_opt(L, 2, opt);

	case ZMQ_AFFINITY:
	case ZMQ_BACKLOG:
	case ZMQ_LINGER:
	case ZMQ_RCVHWM:
	case ZMQ_RCVTIMEO:
	case ZMQ_SNDHWM:
	case ZMQ_SNDTIMEO:
		return integer_opt(L, N, opt);

	case ZMQ_LAST_ENDPOINT:
		return string_opt(L, 2, opt);


	case ZMQ_PROBE_ROUTER: // only connected to ROUTER socket
	    if (skt_type == ZMQ_ROUTER || skt_type == ZMQ_DEALER || skt_type == ZMQ_REQ)
		return boolean_opt(L, 3, opt);

	case ZMQ_CONFLATE:
	    if (skt_type == ZMQ_PULL || skt_type == ZMQ_PUSH || skt_type == ZMQ_SUB || skt_type == ZMQ_PUB || skt_type == ZMQ_DEALER)
		return boolean_opt(L, 3, opt);

	case ZMQ_ROUTING_ID:
	    if (skt_type == ZMQ_ROUTER || skt_type == ZMQ_DEALER || skt_type == ZMQ_REQ || skt_type == ZMQ_REP)
		return string_opt(L, N, opt);

	case ZMQ_CONNECT_ROUTING_ID:
	    if (skt_type == ZMQ_ROUTER || skt_type == ZMQ_STREAM)
		return string_opt(L, 3, opt);

	case ZMQ_STREAM_NOTIFY:
	    if (skt_type == ZMQ_STREAM)
		return boolean_opt(L, 3, opt);

	case ZMQ_SUBSCRIBE:
	case ZMQ_UNSUBSCRIBE:
	    if (skt_type == ZMQ_SUB) // ZMQ_XSUB
		return string_opt(L, 3, opt);

	case ZMQ_ROUTER_MANDATORY:
	    if (skt_type == ZMQ_ROUTER)
		return boolean_opt(L, 3, opt);

	case ZMQ_EVENTS:
	    len = sizeof(v);
	    rc = zmq_getsockopt(skt, opt, &v, &len);
	    zmqError(L, rc == -1, "error while getting value for socket option\n");
	    lua_newtable(L);
	    if (v & ZMQ_POLLIN) {
		lua_pushboolean(L, 1);
		lua_setfield(L, -2, "pollin");
	    }
	    if (v & ZMQ_POLLOUT) {
		lua_pushboolean(L, 1);
		lua_setfield(L, -2, "pollout");
	    }
	    return 1;
    }

    lua_pushnil(L);
    return 1;
}

//
// CONTEXT
//

static int ctx_gc (lua_State *L) {
    if (CTX && (zmq_ctx_term(CTX) == 0))
	CTX = NULL;
    return 0;
}

static int ctx_asstr (lua_State *L) {
    lua_pushliteral(L, "ZeroMQ library, context (active)");
    return 1;
}


static int new_socket(lua_State *L) {
    const char* ttype = luaL_checkstring(L, 1);

    void **pskt = (void **)lua_newuserdata(L, sizeof(void *));
    luaL_setmetatable(L, "caap.zmq.socket");
    //
    lua_pushvalue(L, 1);
    lua_setuservalue(L, -2); // required by asstr
    //
    lua_getfield(L, lua_upvalueindex(1), ttype); // +1
    int type = lua_tointeger(L, -1);
    lua_pop(L, 1); // -1

    *pskt = zmq_socket(CTX, type);
    zmqError(L, *pskt == NULL, "Error creating ZMQ socket");

    return 1;
}

static int ctx_int_opt(lua_State *L, const int N, const uint8_t opt) {
    int rc, v;
    if (N == 1) {
	rc = zmq_ctx_get(CTX, opt);
	zmqError(L, rc == -1, "error while getting context option\n");
	lua_pushinteger(L, rc); // +1
    } else {
	v = luaL_optinteger(L, 2, 0);
	rc = zmq_ctx_set(CTX, opt, v);
	zmqError(L, rc == -1, "error while setting value for context option\n");
	lua_pushboolean(L, 1); // +1
    }
    return 1;
}

static int ctx_option(lua_State *L) {
    const int N = lua_gettop(L);
    int rc = lua_getfield(L, lua_upvalueindex(1), luaL_checkstring(L, 1)); // +1
    zmqError(L, rc != LUA_TNUMBER, "error while computing value of context option\n");
    const uint8_t opt = lua_tointeger(L, -1);
    lua_pop(L, 1); // -1

    switch(opt) {
	case ZMQ_BLOCKY:
	case ZMQ_IO_THREADS:
	case ZMQ_THREAD_NAME_PREFIX:
	case ZMQ_MAX_SOCKETS:
	    return ctx_int_opt(L, N, opt);
    }

    lua_pushnil(L);
    return 1;
}

static void context_opt_func(lua_State *L) {
    lua_newtable(L); // upvalue
    lua_pushinteger(L, ZMQ_BLOCKY); lua_setfield(L, -2, "blocky");
    lua_pushinteger(L, ZMQ_IO_THREADS);  lua_setfield(L, -2, "threads");
    lua_pushinteger(L, ZMQ_THREAD_NAME_PREFIX);  lua_setfield(L, -2, "prefix");
    lua_pushinteger(L, ZMQ_MAX_SOCKETS);  lua_setfield(L, -2, "sockets");
    lua_pushcclosure(L, &ctx_option, 1);
    lua_setfield(L, -2, "opt");
}

/*   ******************************   */

static void init_context(lua_State *L) {
    if (NULL == (CTX = zmq_ctx_new()))
	luaL_error(L, "fatal error creating ZMQ contexa, should abort\n");
}

/*   ******************************   */

// // // // // // // // // // // // //
//
//  add "socket types" as upvalue to method "socket"
//
static void socket_new_func(lua_State *L) {
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

static void socket_opt_func(lua_State *L) {
    lua_newtable(L); // upvalue
    lua_pushinteger(L, ZMQ_SUBSCRIBE); lua_setfield(L, -2, "subscribe");
    lua_pushinteger(L, ZMQ_UNSUBSCRIBE);  lua_setfield(L, -2, "unsubscribe");
    lua_pushinteger(L, ZMQ_CONNECT_ROUTING_ID);  lua_setfield(L, -2, "routing-id");
    lua_pushinteger(L, ZMQ_ROUTING_ID);  lua_setfield(L, -2, "id");
    lua_pushinteger(L, ZMQ_STREAM_NOTIFY);  lua_setfield(L, -2, "notify");
    lua_pushinteger(L, ZMQ_IMMEDIATE); lua_setfield(L, -2, "immediate");
    lua_pushinteger(L, ZMQ_CONNECT_TIMEOUT); lua_setfield(L, -2, "timeout");
    lua_pushinteger(L, ZMQ_FD); lua_setfield(L, -2, "fd");
    lua_pushinteger(L, ZMQ_EVENTS); lua_setfield(L, -2, "events");
    lua_pushinteger(L, ZMQ_POLLIN); lua_setfield(L, -2, "pollin");
    lua_pushinteger(L, ZMQ_EVENT_ALL); lua_setfield(L, -2, "pollout");
    lua_pushinteger(L, ZMQ_LINGER); lua_setfield(L, -2, "linger");
    lua_pushinteger(L, ZMQ_THREAD_SAFE); lua_setfield(L, -2, "safe");
    lua_pushinteger(L, ZMQ_AFFINITY); lua_setfield(L, -2, "affinity");
    lua_pushinteger(L, ZMQ_BACKLOG); lua_setfield(L, -2, "backlog");
    lua_pushinteger(L, ZMQ_RCVHWM); lua_setfield(L, -2, "recv-hwm");
    lua_pushinteger(L, ZMQ_RCVTIMEO); lua_setfield(L, -2, "recv-timeo");
    lua_pushinteger(L, ZMQ_SNDHWM); lua_setfield(L, -2, "send-hwm");
    lua_pushinteger(L, ZMQ_SNDTIMEO); lua_setfield(L, -2, "send-timeo");
    lua_pushinteger(L, ZMQ_LAST_ENDPOINT); lua_setfield(L, -2, "endpoint");
    lua_pushinteger(L, ZMQ_PROBE_ROUTER); lua_setfield(L, -2, "probe"); // only connected to ROUTER socket
    lua_pushinteger(L, ZMQ_CONFLATE); lua_setfield(L, -2, "conflate");
    lua_pushinteger(L, ZMQ_ROUTER_MANDATORY); lua_setfield(L, -2, "mandatory");
    lua_pushcclosure(L, &skt_option, 1);
    lua_setfield(L, -2, "opt");
}

// // // // // // // // // // // // //

static const struct luaL_Reg zmq_meths[] = {
    {"__tostring", ctx_asstr},
    {"__gc", 	   ctx_gc},
    {NULL, 	   NULL}
};

static const struct luaL_Reg zmq_funcs[] = {
    {"proxy",	   new_proxy},
    {"pollin", 	   new_poll_in},
    {"keypair",    new_keypair},
    {NULL,	   NULL}
};

static const struct luaL_Reg key_meths[] = {
    {"public",     key_public},
    {"secret",     key_secret},
    {"__tostring", key_asstr},
    {"__gc",	   key_gc},
    {NULL,	   NULL}
};

static const struct luaL_Reg skt_meths[] = {
    {"__gc", 	   skt_gc},
    {"__tostring", skt_asstr},
    {NULL,	   NULL}
};

static const struct luaL_Reg skt_funcs[] = {
    {"bind",	   skt_bind},
    {"unbind",	   skt_unbind},
    {"connect",	   skt_connect},
    {"disconnect", skt_disconnect},
    {"send_msg",   skt_send_msg},
    {"send_msgs",  skt_send_mult_msg},
    {"recv_msg",   skt_recv_msg},
    {"recv_msgs",  skt_recv_mult_msg},
    {"msgs",	   skt_iter_msg},
    {"server", 	   skt_curve_server},
    {"client", 	   skt_curve_client},
    {NULL,	   NULL}
};

/*   ******************************   */

int luaopen_lzmq (lua_State *L) {
    luaL_newmetatable(L, "caap.zmq.socket");
    luaL_newlib(L, skt_funcs);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, skt_meths, 0);
    socket_opt_func(L);

    luaL_newmetatable(L, "caap.zmq.keypair");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, key_meths, 0);

    // initialize ZeroMQ Context and store reference
    init_context(L);

    // create library
    luaL_newlib(L, zmq_funcs);

    // metatable for library: close & release resources
    luaL_newmetatable(L, "caap.zmq.library");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, zmq_meths, 0);
    socket_new_func(L);
    context_opt_func(L);

    lua_setmetatable(L, -2);

    return 1;
}

