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


static int mgr_gc(lua_State *L) {
    struct mg_mgr *mgr = checkmgr(L);
    if (mgr != NULL) {
	mg_mgr_free(mgr);
	mgr = NULL;
    }
    return 0;
}

static mgr_bind_http(lua_State *L) {
    struct mg_mgr *mgr = checkmgr(L);
    const char *port = luaL_checkstring(L, 2);
    ;
    struct mg_connection *c = mg_bind(mgr, port, handler);
    mg_set_protocol_http_websocket(c);
}

static int mgr_poll(lua_State *L) {
    struct mg_mgr *mgr = checkmgr(L);
    int milis = lua_checkinteger(L, 2);
    return mg_mgr_poll(mgr, milis);
}




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


static void lmg_mgr(lua_State *L) {
    // create a manager
    struct mg_mgr *mgr = (struct mg_mgr *)lua_newuserdata(L, sizeof(struct mg_mgr));
    //
    MRG = mgr;
    // set its metatable
    luaL_getmetatable(L, "caap.mg.mgr");
    lua_setmetatable(L, -2);
    // create the mongoose mgr
    mg_mgr_init(mgr, L);
    // add user value to mg_mgr userdata
    lua_newtable(L);
    lua_setuservalue(L, -2);
    // ;
    lua_pushlightuserdata(L, &MRG);
    lua_pushvalue(L, -2);
    lua_rawset(L, LUA_REGISTRYINDEX);
}


/*   ******************************   */


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
