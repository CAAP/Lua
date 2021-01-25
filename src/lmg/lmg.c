#include <lua.h>
#include <lauxlib.h>

#include "mongoose.h"

static struct mg_mgr *MGR, MMGR;

#define checktimer(L) (struct mg_timer *)luaL_checkudata(L, 1, "caap.mg.timer")

#define newtimer(L) (struct mg_timer *)lua_newuserdata(L, sizeof(struct mg_timer));\
    luaL_getmetatable(L, "caap.mg.timer");\
    lua_setmetatable(L, -2)

#define checkconn(L) *(struct mg_connection **)luaL_checkudata(L, 1, "caap.mg.connection")

#define newconn(L) (struct mg_connection **)lua_newuserdata(L, sizeof(struct mg_connection *));\
    luaL_getmetatable(L, "caap.mg.connection");\
    lua_setmetatable(L, -2)

typedef struct lmg_udata {
    lua_State *L;
    uint8_t flags;
} lmg_udata;

#define WEBSOCKET 2
#define SSL 4

// EVENTS
//
//  MG_EV_ERROR,     // Error                        char *error_message
//  MG_EV_POLL,      // mg_mgr_poll iteration        unsigned long *millis
//  MG_EV_RESOLVE,   // Host name is resolved        NULL
//  MG_EV_CONNECT,   // Connection established       NULL
//  MG_EV_ACCEPT,    // Connection accepted          NULL
//  MG_EV_READ,      // Data received from socket    struct mg_str *
//  MG_EV_WRITE,     // Data written to socket       int *num_bytes_written
//  MG_EV_CLOSE,     // Connection closed            NULL
//  MG_EV_HTTP_MSG,  // HTTP request/response        struct mg_http_message *
//  MG_EV_MQTT_CMD,   // MQTT command                struct mg_mqtt_message *
//  MG_EV_MQTT_MSG,   // MQTT message                struct mg_mqtt_message *
//  MG_EV_MQTT_OPEN,  // MQTT CONNACK received       int *connack_status_code
//  MG_EV_WS_OPEN,    // Websocket handshake done     NULL
//  MG_EV_WS_MSG,     // Websocket message received   struct mg_ws_message *
//  MG_EV_WS_CTL,     // Websocket control message    struct mg_ws_message *
//  MG_EV_USER,       // Starting ID for user events
//
//
//struct mg_connection {
//  struct mg_connection *next;  // Linkage in struct mg_mgr :: connections
//  struct mg_mgr *mgr;          // Our container
//  struct mg_addr peer;         // Remote peer address
//  void *fd;                    // Connected socket, or LWIP data
//  struct mg_iobuf recv;        // Incoming data
//  struct mg_iobuf send;        // Outgoing data
//  mg_event_handler_t fn;       // User-specified event handler function
//  void *fn_data;               // User-speficied function parameter
//  mg_event_handler_t pfn;      // Protocol-specific handler function
//  void *pfn_data;              // Protocol-specific function parameter
//  char label[32];              // Arbitrary label
//  void *tls;                   // TLS specific data
//  unsigned is_listening : 1;   // Listening connection
//  unsigned is_client : 1;      // Outbound (client) connection
//  unsigned is_accepted : 1;    // Accepted (server) connection
//  unsigned is_resolving : 1;   // Non-blocking DNS resolv is in progress
//  unsigned is_connecting : 1;  // Non-blocking connect is in progress
//  unsigned is_tls : 1;         // TLS-enabled connection
//  unsigned is_tls_hs : 1;      // TLS handshake is in progress
//  unsigned is_udp : 1;         // UDP connection
//  unsigned is_websocket : 1;   // WebSocket connection
//  unsigned is_hexdumping : 1;  // Hexdump in/out traffic
//  unsigned is_draining : 1;    // Send remaining data, then close and free
//  unsigned is_closing : 1;     // Close and free the connection immediately
//  unsigned is_readable : 1;    // Connection is ready to read
//  unsigned is_writable : 1;    // Connection is ready to write
//};
//

//struct mg_http_message {
//  //        GET /foo/bar/baz?aa=b&cc=ddd HTTP/1.1
//  // method |-| |----uri---| |--query--| |proto-|
//
//  struct mg_str method, uri, query, proto;  // Request/response line
//  struct mg_http_header headers[MG_MAX_HTTP_HEADERS];  // Headers
//  struct mg_str body;                       // Body
//  struct mg_str message;                    // Request line + headers + body
//};
//

static void http_msg(lua_State *L, struct mg_http_message *m) {
    struct mg_str *ss = &m->method;
    lua_pushlstring(L, ss->ptr, ss->len);
    ss = &m->uri;
    lua_pushlstring(L, ss->ptr, ss->len);
    ss = &m->query;
    lua_pushlstring(L, ss->ptr, ss->len);
    ss = &m->body;
    if (ss->len > 0)
	lua_pushlstring(L, ss->ptr, ss->len);
    else
	lua_pushliteral(L, "");
}

//

static void mqtt_cmd(lua_State *L, struct mg_mqtt_message *m) {
    lua_pushinteger(L, m->cmd);
    lua_replace(L, -2); // replace MG's for MQTT's event
    struct mg_str *ss;
    switch(m->cmd) {
	case MQTT_CMD_SUBSCRIBE:
	    ss = &m->topic;
	    lua_pushlstring(L, ss->ptr, ss->len);
	    break;
	case MQTT_CMD_PUBLISH:
	    ss = &m->topic;
	    lua_pushlstring(L, ss->ptr, ss->len);
	    ss = &m->data;
	    lua_pushlstring(L, ss->ptr, ss->len);
	    break;
    }
}

static void mqtt_msg(lua_State *L, struct mg_mqtt_message *m) {
    struct mg_str *ss = &m->topic;
    lua_pushlstring(L, ss->ptr, ss->len);
    ss = &m->data;
    lua_pushlstring(L, ss->ptr, ss->len);
}

static void ws_msg(lua_State *L, struct mg_ws_message *m) {
    struct mg_str *ss = &m->data;
    int f = m->flags & WEBSOCKET_OP_TEXT;
    lua_pushlstring(L, ss->ptr, ss->len);
    lua_pushinteger(L, f ? WEBSOCKET_OP_TEXT : WEBSOCKET_OP_BINARY);
}

static void wrapper(lua_State *L, struct mg_connection *c, void *p) {
    luaL_getmetatable(L, "caap.mg.connection");
    lua_rawgetp(L, -1, p); // userdatum
    lua_replace(L, -2); // replace metatable w userdatum
    lua_getuservalue(L, -1); // handler +1
    luaL_checktype(L, -1, LUA_TFUNCTION);
    lua_replace(L, -2); // replace userdatum w handler
    struct mg_connection **pc = newconn(L); // connection +1
    *pc = c;
}

static void ev_handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    lmg_udata *pu = (lmg_udata *)fn_data;
    lua_State *L = pu->L;
    int N = lua_gettop(L);
    struct mg_str *ss;

    wrapper(L, c, fn_data); // +2   -> hanlder + connection
    lua_pushinteger(L, ev); // +1  -> event

    switch(ev) {
	case MG_EV_ACCEPT:
	    if (pu->flags & SSL) {
		struct mg_tls_opts opts = {
//		    .ca = "/etc/ssl/ca.pem",
		    .cert = "/etc/ssl/server.pem",
		    .certkey = "/etc/ssl/server.pem"
		};
		mg_tls_init(c, &opts);
	    }
	    break;
	case MG_EV_HTTP_MSG:
	    if (pu->flags & WEBSOCKET)
		mg_ws_upgrade(c, (struct mg_http_message *)ev_data);
	    else
		http_msg(L, (struct mg_http_message *)ev_data); // +4
	    break;
	case MG_EV_MQTT_CMD: // MQTT low-level command
	    mqtt_msg(L, (struct mg_mqtt_message *)ev_data); // +1,2
	    break;
	case MG_EV_MQTT_MSG: // MQTT PUBLISH received
	    mqtt_msg(L, (struct mg_mqtt_message *)ev_data); // +2
	    break;
	case MG_EV_WS_MSG:
	case MG_EV_WS_CTL:
	    ws_msg(L, (struct mg_ws_message *)ev_data); // +2
	    break;
	case MG_EV_READ:
	    ss = (struct mg_str *)ev_data;
	    lua_pushlstring(L, ss->ptr, ss->len); // +1
	    break;
	case MG_EV_WRITE:
	case MG_EV_MQTT_OPEN: // MQTT CONNACK received
	    lua_pushinteger(L, *(int *)ev_data); // +1
	    break;
	case MG_EV_ERROR:
	    lua_pushstring(L, (char *)ev_data); // +1
	    break;
    }
    lua_pcall(L, (lua_gettop(L)-N-1), 0, 0); // in case of ERROR XXX
    lua_settop(L, N);
}

/*   ******************************   */

static int mgr_connect(lua_State *L) {
    const char *uri = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int http = lua_tointeger(L, 3);

    // keep Lua state and function handler on metatable
    luaL_getmetatable(L, "caap.mg.connection");
    lmg_udata *pu = (lmg_udata *)lua_newuserdata(L, sizeof(lmg_udata)); // Lua state
    pu->L = L;
    pu->flags = 0;
    lua_pushvalue(L, 2); // ev_function 4 handler
    lua_setuservalue(L, -2); // set as uservalue for userdatum
    lua_rawsetp(L, -2, (void *)pu); // set data into metatable, key is the pointer
    lua_pop(L, 1); // pop metatable

    struct mg_connection *c;
    if (http) {
	switch(http) {
	    case MG_EV_HTTP_MSG: c = mg_http_connect(MGR, uri, ev_handler, (void *)pu); break;
	    case MG_EV_WS_MSG: c = mg_ws_connect(MGR, uri, ev_handler, (void *)pu, NULL); break;
	}

	if (mg_url_is_ssl(uri)) {
	    struct mg_tls_opts opts = {.ca = "/etc/ssl/ca.pem"};
	    mg_tls_init(c, &opts);
	}

	if (http == MG_EV_HTTP_MSG)
	    mg_printf(c, "GET %s HTTP/1.0\r\n\r\n", mg_url_uri(uri));

    } else
	c = mg_connect(MGR, uri, ev_handler, (void *)pu);

    if (c == NULL) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error: failed connecting to %s\n ", uri);
	return 2;
    }

    struct mg_connection **nc = newconn(L);
    *nc = c;
    return 1;
}

/*   ******************************   */

static int mgr_bind(lua_State *L) {
    const char *uri = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int http = lua_tointeger(L, 3);

    uint8_t flags = mg_url_is_ssl(uri) ? SSL : 0;
    flags = (MG_EV_WS_MSG == http) ? flags|WEBSOCKET : flags;

    // keep Lua state and function handler on metatable
    luaL_getmetatable(L, "caap.mg.connection");
    lmg_udata *pu = (lmg_udata *)lua_newuserdata(L, sizeof(lmg_udata)); // Lua state
    pu->L = L;
    pu->flags = flags;
    lua_pushvalue(L, 2); // ev_function 4 handler
    lua_setuservalue(L, -2); // set as uservalue for userdatum
    lua_rawsetp(L, -2, (void *)pu); // set data into metatable, key is the pointer
    lua_pop(L, 1); // pop metatable

    struct mg_connection *c;
    if (http)
	c = mg_http_listen(MGR, uri, ev_handler, (void *)pu);
    else
	c = mg_listen(MGR, uri, ev_handler, (void *)pu);

    if (c == NULL) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error: failed to create listener on %s\n ", uri);
	return 2;
    }

    struct mg_connection **nc = newconn(L);
    *nc = c;
    return 1;
}

/*   ******************************   */

static void timer_handler(void *pu) {
    lua_State *L = ((lmg_udata *)pu)->L;
    int N = lua_gettop(L);

    luaL_getmetatable(L, "caap.mg.timer");
    lua_rawgetp(L, -1, pu); // userdatum
    lua_replace(L, -2); // replace metatable w userdatum
    lua_getuservalue(L, -1); // handler +1
    luaL_checktype(L, -1, LUA_TFUNCTION);
    lua_replace(L, -2); // replace userdatum w handler

    lua_pcall(L, (lua_gettop(L)-N-1), 0, 0); // in case of ERROR XXX
    lua_settop(L, N);
}

static int mgr_timer(lua_State *L) {
    int mills = luaL_checkinteger(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int flags = lua_toboolean(L, 3);

    struct mg_timer *t = newtimer(L);
    luaL_getmetatable(L, "caap.mg.timer");
    lmg_udata *pu = (lmg_udata *)lua_newuserdata(L, sizeof(lmg_udata)); // Lua state
    pu->L = L;
    lua_pushvalue(L, 2); // timer_function 4 handler
    lua_setuservalue(L, -2); // set as uservalue for userdatum
    lua_rawsetp(L, -2, (void *)pu); // set data into metatable, key is the pointer
    lua_pop(L, 1); // pop metatable

    mg_timer_init(t, mills, flags, timer_handler, (void *)pu);

    return 1;
}

/*   ******************************   */

static int mgr_poll(lua_State *L) {
    int mills = luaL_checkinteger(L, 1);
    mg_mgr_poll(MGR, mills);
    lua_pushboolean(L, 1);
    return 1;
}

/*   ******************************   */

static int next_connection(lua_State *L) {
    struct mg_connection *c = NULL;
    if (lua_isuserdata(L, 2))
	c = (*(struct mg_connection **)lua_touserdata(L, 2))->next;
    else
	c = MGR->conns;
	
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

/*   ******************************   */

static int mgr_asstr(lua_State *L) {
    lua_pushliteral(L, "Mongoose Library, event manager (active)");
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

static int timer_asstr(lua_State *L) {
    lua_pushliteral(L, "Mongoose Library Timer (active)");
    return 1;
}

static int timer_gc(lua_State *L) {
    struct mg_timer *t = checktimer(L);
    if (t != NULL) {
	luaL_getmetatable(L, "caap.mg.timer");
	lua_pushnil(L);
	lua_rawsetp(L, -2, (void *)t);
	lua_pop(L, 1);
	mg_timer_free(t);
	t = NULL;
    }
    return 0;
}

/*   ******************************   */

static int conn_option(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    const char *opt = luaL_checkstring(L, 2);
    int isnum;
    uint8_t v = lua_tointegerx(L, 3, &isnum);
    size_t len;
    const char *lbl = lua_tolstring(L, 3, &len);
    int k = lua_getfield(L, lua_upvalueindex(1), opt);
    switch(k) {
	case 1: lua_pushboolean(L, c->is_listening); break;
	case 2: lua_pushboolean(L, c->is_client); break;
	case 3: lua_pushboolean(L, c->is_tls); break;
	case 4: lua_pushboolean(L, c->is_udp); break;
	case 5: lua_pushboolean(L, c->is_websocket); break;
	case 6:
	    if (isnum) {
		c->is_draining = v;
		lua_pushboolean(L, 1);
	    } else
		lua_pushboolean(L, c->is_draining);
	    break;
	case 7:
	    if (isnum) {
		c->is_closing = v;
		lua_pushboolean(L, 1);
	    } else
		lua_pushboolean(L, c->is_closing);
	    break;
	case 8:
	    if (lbl == NULL || len == 0) {
		if ((c->label) == NULL)
		    lua_pushnil(L);
		else
		    lua_pushstring(L, c->label);
	    } else {
		strncpy(c->label, lbl, 32);
		lua_pushboolean(L, 1);
	    }
	    break;
    }
    return 1;
}

static void set_conn_opts(lua_State *L) {
    luaL_getmetatable(L, "caap.mg.connection");
    lua_newtable(L);
    lua_pushinteger(L, 1); lua_setfield(L, -2, "listening");
    lua_pushinteger(L, 2); lua_setfield(L, -2, "client");
    lua_pushinteger(L, 3); lua_setfield(L, -2, "tls");
    lua_pushinteger(L, 4); lua_setfield(L, -2, "udp");
    lua_pushinteger(L, 5); lua_setfield(L, -2, "websocket");
    lua_pushinteger(L, 6); lua_setfield(L, -2, "draining");
    lua_pushinteger(L, 7); lua_setfield(L, -2, "closing");
    lua_pushcclosure(L, &conn_option, 1);
    lua_setfield(L, -2, "opt");
    lua_pop(L, 1);
}

static int conn_http_reply(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    const int code = luaL_checkinteger(L, 2);
    size_t len;
    const char *msg = luaL_checklstring(L, 3, &len);
    if (len > 0)  {
	mg_http_reply(c, code, "%s", msg);
    } else

	mg_http_reply(c, code, NULL, NULL);

    lua_pushboolean(L, 1);
    return 1;
}

static int conn_send(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    size_t len;
    const char *msg = luaL_checklstring(L, 2, &len);
    int op = lua_tointeger(L, 3);
    if (c->is_websocket)
	mg_ws_send(c, msg, len, op);
    else
	mg_send(c, msg, len);
    lua_pushboolean(L, 1);
    return 1;
}

static int conn_ip_address(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    char ip[100];
    lua_pushstring(L, mg_straddr(c, ip, sizeof(ip)));
    return 1;
}

static int conn_asstr(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    lua_pushfstring(L, "Mongoose Connection (%p)", c->pfn_data);
    return 1;
}

static int conn_gc(lua_State *L) {
    struct mg_connection *c = checkconn(L);
    if (c != NULL)
	c = NULL;
    return 0;
}

/*   ******************************   */

static void set_events(lua_State *L) {
    lua_newtable(L); // upvalue
    lua_pushinteger(L, MG_EV_POLL); lua_setfield(L, -2, "POLL");
    lua_pushinteger(L, MG_EV_ACCEPT); lua_setfield(L, -2, "ACCEPT");
    lua_pushinteger(L, MG_EV_CONNECT); lua_setfield(L, -2, "CONNECT");
    lua_pushinteger(L, MG_EV_READ); lua_setfield(L, -2, "READ");
    lua_pushinteger(L, MG_EV_WRITE); lua_setfield(L, -2, "WRITE");
    lua_pushinteger(L, MG_EV_CLOSE); lua_setfield(L, -2, "CLOSE");
    lua_pushinteger(L, MG_EV_ERROR); lua_setfield(L, -2, "ERROR");
    lua_pushinteger(L, MG_EV_HTTP_MSG); lua_setfield(L, -2, "HTTP");
    lua_pushinteger(L, MG_EV_WS_MSG); lua_setfield(L, -2, "WS");
    lua_pushinteger(L, MG_EV_WS_OPEN); lua_setfield(L, -2, "OPEN");
    lua_pushinteger(L, MG_EV_USER); lua_setfield(L, -2, "USER");
    lua_setfield(L, -2, "events");
}

static void set_mqtt_commands(lua_State *L) {
    lua_newtable(L); // upvalue
    lua_pushinteger(L, MQTT_CMD_CONNECT); lua_setfield(L, -2, "CONNECT");
    lua_pushinteger(L, MQTT_CMD_CONNACK); lua_setfield(L, -2, "CONNACK");
    lua_pushinteger(L, MQTT_CMD_PUBLISH); lua_setfield(L, -2, "PUBLISH");
    lua_pushinteger(L, MQTT_CMD_PUBACK); lua_setfield(L, -2, "PUBACK");
    lua_pushinteger(L, MQTT_CMD_PUBREC); lua_setfield(L, -2, "PUBREC");
    lua_pushinteger(L, MQTT_CMD_PUBREL); lua_setfield(L, -2, "PUBREL");
    lua_pushinteger(L, MQTT_CMD_PUBCOMP); lua_setfield(L, -2, "PUBCOMP");
    lua_pushinteger(L, MQTT_CMD_SUBSCRIBE); lua_setfield(L, -2, "SUBSCRIBE");
    lua_pushinteger(L, MQTT_CMD_SUBACK); lua_setfield(L, -2, "SUBACK");
    lua_pushinteger(L, MQTT_CMD_UNSUBSCRIBE); lua_setfield(L, -2, "UNSUBSCRIBE");
    lua_pushinteger(L, MQTT_CMD_UNSUBACK); lua_setfield(L, -2, "UNSUBACK");
    lua_pushinteger(L, MQTT_CMD_PINGREQ); lua_setfield(L, -2, "PINGREQ");
    lua_pushinteger(L, MQTT_CMD_PINGRESP); lua_setfield(L, -2, "PINGRESP");
    lua_pushinteger(L, MQTT_CMD_DISCONNECT); lua_setfield(L, -2, "DISCONNECT");
    lua_setfield(L, -2, "commands");
}

static void set_ws_ops(lua_State *L) {
    lua_newtable(L); // upvalue
    lua_pushinteger(L, WEBSOCKET_OP_TEXT); lua_setfield(L, -2, "TEXT");
    lua_pushinteger(L, WEBSOCKET_OP_BINARY); lua_setfield(L, -2, "BINARY");
    lua_setfield(L, -2, "ops");
}

/*   ******************************   */

static void mg_init(lua_State *L) {
    set_events(L);
    set_mqtt_commands(L);
    set_ws_ops(L);
    // initialize the Event Manager
    MGR = &MMGR;
    mg_mgr_init(MGR);
}

/*   ******************************   */

static const struct luaL_Reg mg_funcs[] = {
    {"poll",	   mgr_poll},
    {"bind", 	   mgr_bind},
    {"connect",	   mgr_connect},
    {"peers", 	   mgr_iterator},
    {"timer", 	   mgr_timer},
    {"__tostring", mgr_asstr},
    {"__gc",	   mgr_gc},
    {NULL,	   NULL}
};

/*   ******************************   */

static const struct luaL_Reg timer_meths[] = {
    {"__tostring",  timer_asstr},
    {"__gc",	    timer_gc},
    {"remove",      timer_gc},
    {NULL,	    NULL}
};

/*   ******************************   */

static const struct luaL_Reg conn_meths[] = {
    {"reply",	    conn_http_reply},
    {"send",	    conn_send},
    {"ip", 	    conn_ip_address},
    {"__tostring",  conn_asstr},
    {"__gc",	    conn_gc},
    {NULL,	    NULL}
};

/*   ******************************   */

int luaopen_lmg (lua_State *L) {

    luaL_newmetatable(L, "caap.mg.connection");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, conn_meths, 0);
    set_conn_opts(L);

    luaL_newmetatable(L, "caap.mg.timer");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, timer_meths, 0);

    // create library
    luaL_newlib(L, mg_funcs);

    // initialize the Mongoose library
    mg_init(L);

    // metatable for library is itself
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    return 1;
}
