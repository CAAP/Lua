#include <lua.h>
#include <lauxlib.h>

#include "mongoose.h"

static struct mg_mgr *MGR;

#define checkmgr(L) (struct mg_mgr *)luaL_checkudata(L, 1, "caap.mg.mgr")

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
int mg_parse_uri(const struct mg_str uri, struct mg_str *scheme, struct mg_str *user_info, struct mg_str *host, unsigned int *port, struct mg_str *path, struct mg_str *query, struct mg_str *fragment);


struct websocket_message {
    unsigned char *data;
    size_t size;
    unsigned char flags;
};





static int bind(lua_State *L) {
    const char* port = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    struct mg_connection *nc = 
    nc = mg_bind( &MGR, port,  );
}




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

static void save_handler(lua_State *L, struct mg_connection *c, lua_CFunction f) {
    // get table of handlers
    lua_pushlightuservalue(L, &MGR);
    lua_rawget(L, LUA_REGISTRYINDEX);
    // store handler function
    lua_pushlightuservalue(L, &c);
    lua_pushcfunction(L, f);
    lua_rawset(L, -3);
    lua_pop(L, 1); // pop table of handlers
}


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

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    lua_CFunction f = (lua_CFunction)c->user_data;
    if (f) {
	lua_State *L = (lua_State *)MGR->user_data;
	// get handler from table XXX
	lua_pushlightuserdata(L, ev_data);
	f(L);
    }
}

static int http_handler(lua_State *L) {
    void *ev_data = lua_touserdata(L, -1);
    lua_pop(l, 1); // pop lightuserdata
    struct http_message *m = (struct http_message *)ev_data;
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



    return 0;
}

static mgr_bind_http(lua_State *L) {
    const char *host = luaL_checkstring(L, 1);

    struct mg_bind_opts bind_opts;
    const char *err;
    memset(&bind_opts, 0, sizeof(bind_opts));
    bind_opts.error_string = &err;
    // create a closure to save handler function as upvalue XXX
    bind_opts.user_data = (void *)&http_handler;

    struct mg_connection *c = mg_bind_opt(MGR, host, ev_handler, bind_opts);
    if (c == NULL) {
	lua_pushnil(L);
	lua_pushfstring("Error: failed to create listener on host %s\n ", host);
	return 2;
    }
    mg_set_protocol_http_websocket(c);
    lua_pushboolean(L, 1);
    return 1;
}

static int mgr_poll(lua_State *L) {
    int mills = lua_checkinteger(L, 1);
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
    // table to store handlers
    lua_newtable(L);
    lua_setservalue(L, -1);
}

/*   ******************************   */

static const struct luaL_Reg mgr_meths[] = {
    {"__tostring", mgr_asstr},
    {"__gc",	   mgr_gc},
    {NULL,	   NULL}
};

/*   ******************************   */

int luaopen_lmg (lua_State *L) {
    luaL_newmetatable(L, "caap.mg.mgr");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, mgr_meths, 0);
//    socket_fn(L);

    lmg_mgr(L);

    return 1;
}
