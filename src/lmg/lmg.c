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

static void http_request(lua_State *L, struct http_message *m) {
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

    wrapper(L, c); // +2   -> hanlder + connection
    lua_pushinteger(L, ev); // +1  -> event

    switch(ev) {
	case MG_EV_HTTP_REQUEST:
		http_request(L, (struct http_message *)ev_data); // +4
	    break;
//	case MG_EV_HTTP_REPLY:
//	case MG_EV_RECV:
/* error in case conection was unexpectedly dropped by server */
//	case MG_EV_CLOSE:
// return address | error in case of faulty connection to server XXX
	case MG_EV_CONNECT:
	    lua_pushboolean(L, ~*(int *)ev_data); // +1
	    break;
    }
//    lua_rotate(L, N+1, 2);
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
    mg_send(c, msg, len);
    if (sendclose)
	c->flags |= MG_F_SEND_AND_CLOSE;
    lua_pushboolean(L, 1);
    return 1;
}

static int conn_recv_buffer(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    const int closep = lua_toboolean(L, 2);
    struct mbuf *data = &c->recv_mbuf;
    lua_pushlstring(L, data->buf, data->len);
    mbuf_remove(data, data->len);
    if (closep)
	c->flags |= MG_F_CLOSE_IMMEDIATELY;
    return 1;
}

/*
static int conn_get_flag(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    const int flag = luaL_checkint(L, 2);

}
*/

/*
static int conn_get_sock_address(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    lua_pushlightuserdata(L, (void *)&c->sock);
    return 1;
}
*/

static int conn_get_sock_address(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    lua_pushfstring(L, "%p", (void *)&c->sock);
    return 1;
}

static int conn_asstr(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    lua_pushfstring(L, "Mongoose Connection (%p) (%p)", (void *)c->user_data,(void *)&c->sock);
    return 1;
}

static int conn_gc(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    if (c != NULL) {
	c = NULL;
    }
    return 0;
}

/*   ******************************   */

static int mgr_connect(lua_State *L) {
    const char *host = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
//    int http = lua_toboolean(L, 3);

    struct mg_connection **nc = newconn(L);

    struct mg_connect_opts connect_opts;
    const char *err;
    memset(&connect_opts, 0, sizeof(connect_opts));
    connect_opts.error_string = &err;
    connect_opts.user_data = (void *)nc;

//    struct mg_connection *c = mg_connect_opt(MGR, &ev_handler, connect_opts, host, NULL, NULL);
    struct mg_connection *c = mg_connect_opt(MGR, host, &ev_handler, connect_opts);

    if (c == NULL) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error: failed to create listener on host %s\n ", host);
	return 2;
    }
    *nc = c;

    luaL_getmetatable(L, "caap.mg.connection");
    lua_pushlightuserdata(L, (void *)nc);
    lua_pushvalue(L, 2);
    lua_rawset(L, -3);
    lua_pop(L, 1);

    return 1;
}

static int mgr_bind(lua_State *L) {
    const char *host = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int http = lua_toboolean(L, 3);

    struct mg_connection **nc = newconn(L);

    struct mg_bind_opts bind_opts;
    const char *err;
    memset(&bind_opts, 0, sizeof(bind_opts));
    bind_opts.error_string = &err;
    bind_opts.user_data = (void *)nc;

    struct mg_connection *c = mg_bind_opt(MGR, host, &ev_handler, bind_opts);
    if (c == NULL) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error: failed to create listener on host %s\n ", host);
	return 2;
    }
    if (http)
	mg_set_protocol_http_websocket(c);
    *nc = c;

    luaL_getmetatable(L, "caap.mg.connection");
    lua_pushlightuserdata(L, (void *)nc);
    lua_pushvalue(L, 2);
    lua_rawset(L, -3);
    lua_pop(L, 1);

    return 1;
}

static int mgr_poll(lua_State *L) {
    int mills = luaL_checkinteger(L, 1);
    return mg_mgr_poll(MGR, mills);
}

static int next_connection(lua_State *L) {
    struct mg_connection *c = NULL;
    if (lua_isuserdata(L, 2))
	c = *(struct mg_connection **)lua_touserdata(L, 2);
	
    c = mg_next(MGR, c);

    if (c == NULL)
	return 0;
    struct mg_connection **pc = newconn(L);
    *pc = c;
    return 1;
}

static int mgr_iterator(lua_State *L) {
    lua_pushcclosure(L, next_connection, 0); // iter
    lua_pushboolean(L, 1);
    lua_pushnil(L);
    return 3; // iter + state + connection
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

static void set_events(lua_State *L) {
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

/*
static void set_flags(lua_State *L) {
    lua_newtable(L); // upvalue
    lua_pushinteger(L, MG_F_SEND_AND_CLOSE); lua_setfield(L, -2, "SENDC");
    lua_pushinteger(L, MG_F_BUFFER_BUT_DONT_SEND); lua_setfield(L, -2, "BUFFER");
    lua_pushinteger(L, MG_F_CLOSE_IMMEDIATELY); lua_setfield(L, -2, "CLOSE");
    lua_pushinteger(L, MG_F_IS_WEBSOCKET); lua_setfield(L, -2, "ISWS");
    lua_setfield(L, -2, "");
}
*/

/*   ******************************   */

static const struct luaL_Reg mgr_meths[] = {
    {"poll",	   mgr_poll},
    {"bind", 	   mgr_bind},
    {"connect",	   mgr_connect},
    {"iter", 	   mgr_iterator},
    {"__tostring", mgr_asstr},
    {"__gc",	   mgr_gc},
    {NULL,	   NULL}
};

/*   ******************************   */

static const struct luaL_Reg conn_meths[] = {
    {"reply",	    conn_send_reply},
    {"send",	    conn_send},
    {"recv", 	    conn_recv_buffer},
    {"sock", 	    conn_get_sock_address},
    {"__tostring",  conn_asstr},
    {"__gc",	    conn_gc},
    {NULL,	    NULL}
};

/*   ******************************   */

int luaopen_lmg (lua_State *L) {
    luaL_newmetatable(L, "caap.mg.mgr");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, mgr_meths, 0);
    set_events(L);

    luaL_newmetatable(L, "caap.mg.connection");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, conn_meths, 0);
//    set_flags(L);

    lmg_mgr(L);

    return 1;
}
