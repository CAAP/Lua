#include <lua.h>
#include <lauxlib.h>

#include "mongoose.h"

static struct mg_mgr *MGR;

#define checkmgr(L) (struct mg_mgr *)luaL_checkudata(L, 1, "caap.mg.mgr")

#define checkconn(L) *(struct mg_connection **)luaL_checkudata(L, 1, "caap.mg.connection")

#define newconn(L) (struct mg_connection **)lua_newuserdata(L, sizeof(struct mg_connection *));\
    luaL_getmetatable(L, "caap.mg.connection");\
    lua_setmetatable(L, -2)

/*
MG_F_LISTENING // THIS CONNECTION IS LISTENING
MG_F_CONNECTING // connect() CALL IN PROGRESS
MG_F_IS_WEBSOCKET // WEBSOCKET SPECIFIC
MG_F_RECV_AND_CLOSE // DRAIN RX AND CLOSE CONNECTION

/-------------------------------------/

MG_F_SEND_AND_CLOSE // PUSH REMAINING DATA AND CLOSE
MG_F_CLOSE_IMMMEDIATELY // DISCONNECT

MG_F_PROTO_1
MG_F_PROTO_2
MG_F_ENABLE_BROADCAST

MG_F_USER_1
MG_F_USER_2
MG_F_USER_3
MG_F_USER_4
MG_F_USER_5
MG_F_USER_6
*/

// HELPERS
// 
// struct mg_str {
// 	const char *p;
// 	size_t len;
// }
//
// struct mg_str mg_mk_str(const char *s);
// struct mg_str mg_mk_str_n(const char *s, size_t len);
//
//


/*
 * Parses an URI, general syntax is:
 * 	[scheme://[user_info@]]host[:port][/patch][?query][#fragment]
 * NULL pointers will be ignored.
 * returns 0 on success, -1 on error.
*/
//int mg_parse_uri(const struct mg_str uri, struct mg_str *scheme, struct mg_str *user_info, struct mg_str *host, unsigned int *port, struct mg_str *path, struct mg_str *query, struct mg_str *fragment);

/*
struct websocket_message {
    unsigned char *data;
    size_t size;
    unsigned char flags;
};
*/

/*
static send_response(lua_State *L) {
    struct mg_connection *nc = ;
    int code = luaL_checkinteger(L, 2);
    const char *headers = luaL_checkstring(L, 3);


    mg_send_response_line(nc, code, headers);
    lua_pushboolean(L, 1);
    return 1;
}

static send_error(lua_State *L) {
    struct mg_connection *nc = ;
    int code = luaL_checkinteger(L, 2);
    const char *errorstr = luaL_checkstring(L, 3);


    mg_http_send_error(nc, code, errorstr);
    lua_pushboolean(L, 1);
    return 1;
}
*/

// EVENTS
//
// MG_EV_POLL
// MG_EV_ACCEPT		socket_address
// MG_EV_CONNECT	int
// MG_EV_RECV		int *num_bytes
// MG_EV_SEND		int *num_bytes
// MG_EV_CLOSE
// MG_EV_TIMER		double
//
// MG_EV_HTTP_REQUEST	struct http_message
// MG_EV_HTTP_REPLY			"
// MG_EV_HTTP_CHUNK			"
// MG_EV_WEBSOCKET_HANDSHAKE_REQUEST	"
// MG_EV_WEBSOCKET_HANDSHAKE_DONE	"
// 			
// MG_EV_WEBSOCKET_FRAME	struct websocket_message

// cast (void *)ev_data -> (struct http_message *)
// returns
// 	http method, http uri, http query string, http body
static void http_message(lua_State *L, struct http_message *m) {
    struct mg_str *ss = &m->method;
    lua_pushlstring(L, ss->p, ss->len);
    ss = &m->uri;
    lua_pushlstring(L, ss->p, ss->len);
    ss = &m->query_string;
    lua_pushlstring(L, ss->p, ss->len);
    ss = &m->body;
    if (ss->len > 0)
	lua_pushlstring(L, ss->p, ss->len);
    else
	lua_pushliteral(L, "");
}

static void recv_buffer(lua_State *L, struct mg_connection *c) {
    struct mbuf *data = &c->recv_mbuf;
    lua_pushlstring(L, data->buf, data->len);
    mbuf_remove(data, data->len);
}

static void wrapper(lua_State *L, struct mg_connection *c) {
    void *p = c->user_data;
    luaL_getmetatable(L, "caap.mg.connection");
    lua_pushlightuserdata(L, p);
    lua_rawget(L, -2); // handler +1
    luaL_checktype(L, -1, LUA_TFUNCTION);
    lua_replace(L, -2);
    struct mg_connection **pc = newconn(L); // connection +1
    *pc = c;
}

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    lua_State *L = MGR->user_data;
    int N = lua_gettop(L);
    lua_pushinteger(L, ev); // +1
    switch(ev) {
	case MG_EV_HTTP_REQUEST:
	    http_message(L, (struct http_message *)ev_data); // +4
	    break;
	case MG_EV_ACCEPT:
	    break;
	case MG_EV_RECV:
	    recv_buffer(L, c); // +1
	    break;
    }
    wrapper(L, c); // +2
    lua_rotate(L, N+1, 2);
    lua_pcall(L, (lua_gettop(L)-N-1), 0, 0); // in case of ERROR XXX
    lua_settop(L, N);
}

/*   ******************************   */

static int conn_send_reply(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    const int code = luaL_checkinteger(L, 2);
    size_t len;
    const char *msg = luaL_checklstring(L, 3, &len);
    const char *headers = luaL_checkstring(L, 4);
    int sendclose = lua_toboolean(L, 5);
    if (len > 0)  {
	mg_send_head(c, code, len, headers);
	mg_printf(c, "%s", msg);
    } else
	mg_send_response_line(c, code, headers);
    if (sendclose)
	c->flags |= MG_F_SEND_AND_CLOSE;
    lua_pushboolean(L, 1);
    return 1;
}

static int conn_send(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    size_t len;
    const char *msg = luaL_checklstring(L, 2, &len);
    int sendclose = lua_toboolean(L, 3);
    mg_send(c, msg, (int)len);
    if (sendclose)
	c->flags |= MG_F_SEND_AND_CLOSE;
    lua_pushboolean(L, 1);
    return 1;
}

static int conn_http_websocket(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    mg_set_protocol_http_websocket(c);
    lua_pushboolean(L, 1);
    return 1;
}
static int conn_asstr(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    lua_pushfstring(L, "Mongoose Connection (%p)", (void *)c->user_data);
    return 1;
}

static int conn_gc(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    if (c != NULL) {
	luaL_getmetatable(L, "caap.mg.connection");
	lua_pushlightuserdata(L, c);
	lua_pushnil(L);
	lua_rawset(L, -3);
	lua_pop(L, 1);
	c = NULL;
    }
    return 0;
}

/*   ******************************   */

static int mgr_bind(lua_State *L) {
    const char *host = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_CFunction handler = lua_tocfunction(L, 2);

    struct mg_bind_opts bind_opts;
    const char *err;
    memset(&bind_opts, 0, sizeof(bind_opts));
    bind_opts.error_string = &err;
    bind_opts.user_data = (void *)&handler;

    struct mg_connection *c = mg_bind_opt(MGR, host, &ev_handler, bind_opts);
    if (c == NULL) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error: failed to create listener on host %s\n ", host);
	return 2;
    }

    struct mg_connection **nc = (struct mg_connection **)lua_newuserdata(L, sizeof(struct mg_connection *));
    luaL_getmetatable(L, "caap.mg.connection");
    lua_setmetatable(L, -2);
    *nc = c;

    luaL_getmetatable(L, "caap.mg.connection");
    lua_pushlightuserdata(L, &handler);
    lua_pushvalue(L, 2);
    lua_rawset(L, -3);
    lua_pop(L, 1);

    return 1;
}

static int mgr_poll(lua_State *L) {
    int mills = luaL_checkinteger(L, 1);
    return mg_mgr_poll(MGR, mills);
}

static int mgr_asstr(lua_State *L) {
    lua_pushliteral(L, "Mongoose Event Manager (active)");
    return 1;
}

static int mgr_gc(lua_State *L) {
    if (MGR != NULL) {
	mg_mgr_free(MGR);
	MGR = NULL;
    }
    return 0;
}

/*   ******************************   */

static void lmg_mgr(lua_State *L) {
    // create a manager
    struct mg_mgr *mgr = (struct mg_mgr *)lua_newuserdata(L, sizeof(struct mg_mgr));
    //
    MGR = mgr;
    // set its metatable
    luaL_getmetatable(L, "caap.mg.mgr");
    lua_setmetatable(L, -2);
    // create the mongoose mgr, keeping lua_State as userdata
    mg_mgr_init(mgr, L);
}

static void handler_fn(lua_State *L) {
    lua_newtable(L); // upvalue
    lua_pushinteger(L, MG_EV_POLL); lua_setfield(L, -2, "POLL");
    lua_pushinteger(L, MG_EV_ACCEPT); lua_setfield(L, -2, "ACCEPT");
    lua_pushinteger(L, MG_EV_CONNECT); lua_setfield(L, -2, "CONNECT");
    lua_pushinteger(L, MG_EV_RECV); lua_setfield(L, -2, "RECV");
    lua_pushinteger(L, MG_EV_SEND); lua_setfield(L, -2, "SEND");
    lua_pushinteger(L, MG_EV_CLOSE); lua_setfield(L, -2, "CLOSE");
    lua_pushinteger(L, MG_EV_TIMER); lua_setfield(L, -2, "TIMER");
    lua_pushinteger(L, MG_EV_HTTP_REQUEST); lua_setfield(L, -2, "REQUEST");
    lua_pushinteger(L, MG_EV_HTTP_REPLY); lua_setfield(L, -2, "REPLY");
    lua_pushinteger(L, MG_EV_HTTP_CHUNK); lua_setfield(L, -2, "CHUNK");
    lua_pushinteger(L, MG_EV_WEBSOCKET_HANDSHAKE_REQUEST); lua_setfield(L, -2, "HANDSHAKE_REQUEST");
    lua_pushinteger(L, MG_EV_WEBSOCKET_HANDSHAKE_DONE); lua_setfield(L, -2, "HANDSHAKE_DONE");
    lua_pushinteger(L, MG_EV_WEBSOCKET_FRAME); lua_setfield(L, -2, "FRAME");
    lua_setfield(L, -2, "events");
}

/*   ******************************   */

static const struct luaL_Reg mgr_meths[] = {
    {"poll",	   mgr_poll},
    {"bind",	   mgr_bind},
    {"__tostring", mgr_asstr},
    {"__gc",	   mgr_gc},
    {NULL,	   NULL}
};

/*   ******************************   */

static const struct luaL_Reg conn_meths[] = {
    {"reply",	   conn_send_reply},
    {"send",	   conn_send},
    {"http",	   conn_http_websocket},
    {"__tostring", conn_asstr},
    {"__gc",	   conn_gc},
    {NULL,	   NULL}
};

/*   ******************************   */

int luaopen_lmg (lua_State *L) {
    luaL_newmetatable(L, "caap.mg.mgr");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, mgr_meths, 0);
    handler_fn(L);

    luaL_newmetatable(L, "caap.mg.connection");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, conn_meths, 0);

    lmg_mgr(L);

    return 1;
}
