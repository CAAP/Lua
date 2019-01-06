#include <lua.h>
#include <lauxlib.h>

#include <string.h>
#include <errno.h>

#include <zmq.h>

#define checkctx(L) *(void **)luaL_checkudata(L, 1, "caap.zmq.context")
#define checkskt(L) *(void **)luaL_checkudata(L, 1, "caap.zmq.socket")

extern int errno;

static char *err2str() {
    switch(errno) {
	case EAGAIN: return "non-blocking mode was requested and the message cannot be sent";
	case ENOTSUP: return "operation not supperted by this socket type";
	case EINVAL: return "trying to send multipart data, which this socket type does not allow";
	case ETERM: return "the context associated with this socket was terminated";
	case EFSM: return "the context associated with this socket was terminated";
	case ENOTSOCK: return "this socket is invalid";
	case EINTR: return "the operation was interrupted by a signal before it was done";
	case EHOSTUNREACH: return "the message cannot be routed";
	default: return "undefined error ocurred";
    }
}

// Thread-safe SOCKETS:
// ZMQ_CLIENT, ZMQ_SERVER, ZMQ_DISH, ZMQ_RADIO, ZMQ_SCATTER, ZMQ_GATHER

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

static int skt_asstr(lua_State *L) {
    lua_pushstring(L, "zmq{Active Socket}");
    return 1;
}

static int skt_gc (lua_State *L) {
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

// Receive a message part from a socket
// receive a message from the socket referenced
// by the socket argument and store it in the
// buffer buf. Any bytes exceeding the length
// shall be truncated.
// An application that process multi-part messages
// must use the ZMQ_RCVMORE option to determine if
// there are futher parts to receive.
// Shall return number of bytes in the message.
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

static int streaming(lua_State *L) {
    void *skt = checkskt(L);

    char id [256];
    size_t id_size = zmq_recv(skt, id, 256, 0);
    if (id_size < 1) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR: socket received an invalid ID!");
	return 2;
    }

    lua_pushlstring(L, id, id_size);
    size_t len;
    const char *myid = lua_tolstring(L, -1, &len);

    if(len != id_size || strncmp(myid, id, len)) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR: ID becomes invalid after assignment to pointer.");
	return 2;
    }

    int rc = zmq_send(skt, myid, len, ZMQ_SNDMORE);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: socket ID could not be sent due to %s!", err2str());
	return 2;
    }

//    size_t len = 0;
    const char *msg = luaL_checklstring(L, 2, &len);

    if (0 == len)
	rc = zmq_send(skt, 0, 0, 0);
    else
	rc = zmq_send(skt, msg, len, ZMQ_SNDMORE);

    if (rc == -1) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR: socket could not send message!");
	return 2;
    }

    rc = zmq_send(skt, myid, len, ZMQ_SNDMORE);
    rc = zmq_send(skt, 0, 0, ZMQ_SNDMORE);

    lua_pushboolean(L, 1);
    return 1;
}

////////   ID   /////////

static int id_asstr(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_pushfstring(L, "zmq{Active ID, size=%d}", lua_tointeger(L, -1));
    lua_pop(L, 1);
    return 1;
}

static int id_gc (lua_State *L) {
    uint8_t *myid = (uint8_t *)luaL_checkudata(L, 2, "caap.zmq.id");
    if (myid)
	myid = NULL;
    return 0;
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
    {"send",	   skt_send},
    {"recv",	   skt_recv},
    {"send_id",	   skt_send_id},
    {"recv_id",	   skt_recv_id},
    {"stream",	   streaming},
    {"__tostring", skt_asstr},
    {"__gc",	   skt_gc},
    {NULL,	   NULL}
};

static const struct luaL_Reg id_meths[] = {
    {"__tostring", id_asstr},
    {"__gc",	   id_gc},
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

    luaL_newmetatable(L, "caap.zmq.id");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, id_meths, 0);

    // create library
    luaL_newlib(L, zmq_funcs);
    return 1;
}
