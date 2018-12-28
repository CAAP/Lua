#include <lua.h>
#include <lauxlib.h>

#include <string.h>

#include <zmq.h>

#define checkctx(L) *(void **)luaL_checkudata(L, 1, "caap.zmq.context")
#define checkskt(L) *(void **)luaL_checkudata(L, 1, "caap.zmq.socket")

typedef struct Map {
    const char *name;
    const int value;
} Map;

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
    }

    return 1;
}

static int new_socket(lua_State *L) {
    void *ctx = checkctx(L);
    int type;

    void **pskt = (void **)lua_newuserdata(L, sizeof(void *));
    luaL_getmetatable(L, "caap.zmq.socket");
    lua_setmetatable(L, -2);

    *pskt = zmq_socket(ctx, type);
    if (*pskt == NULL) {
	lua_pushnil(L);
	lua_pushstring(L, "Error creating ZMQ socket\n");
    }

    return 1;
}

static int asstr(lua_State *L) {
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
    int multip = 0;

    if (lua_gettop(L) > 2)
	multip = luaL_checkinteger(L, 3);

    zmq_send(skt, msg, len, multip)
}

static int skt_recv (lua_State *L) {
    void *skt = checkskt(L);

}

// // // // // // // // // // // // //

static const struct luaL_Reg zmq_funcs[] = {
    {"context", new_context},
    {NULL, NULL}
};

static const struct luaL_Reg ctx_meths[] = {
    {"socket", new_socket},
    {"__gc", ctx_gc},
    {NULL, NULL}
};

static const struct luaL_Reg skt_meths[] = {
    {"bind",    skt_bind},
    {"connect", skt_connect},
    {"send",    skt_send},
    {"recv",    skt_recv},
    {"__gc", skt_gc},
    {NULL, NULL}
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
