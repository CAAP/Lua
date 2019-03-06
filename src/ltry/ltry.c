#include <lua.h>
#include <lauxlib.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <zmq.h>

extern int errno;

char *err2str() {
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

// POLL
//
// Built-in ZMO poll
//
// Input/Output multiplexing
//
// A mechanism for applications to multiplex input/output events
// in a level-triggered fashion over a set of sockets. Each member
// of the array pointed to by the items argument is a zmq_pollitem_t
// structure.
// zmq_poll shall examine either the ZMQ socket/standard socket
// specified, for the event(s) specified in events.
// If none of the requested events have occurred, zmq_poll shall
// wait timeout milliseconds for an event to occur on any of the
// requested items. If the value of timeout is 0, it shall return
// immediately. If the value of timeout is -1, it shall block
// indefinitely until a requested event has occurred.
// The "events and revents" are bit masks constructed by OR'ing a
// combination of the following event flags:
// 	ZMQ_POLLIN	at least one message may be received wo blocking
// 	ZMQ_POLLOUT	at least one message may be sent wo blocking
// 	ZMQ_POLLERR	some sort of error condition is present
// 	ZMQ_POLLPRI	of NO use
//
// function that in case of success, before timeout, identifies
// the item which successfully received the event, and returns
// its index, or nil if timeout.
static int poll_now(lua_State *L) {
    const long timeout = luaL_checkinteger(L, 1); // milliseconds
    zmq_pollitem_t *items = (zmq_pollitem_t *)lua_touserdata(L, lua_upvalueindex(1));
    int i, N = lua_tointeger(L, lua_upvalueindex(2));
//    int M = lua_tointeger(L, lua_upvalueindex(3));
    int rc = zmq_poll(items, N, timeout);
    if (rc == -1) {
	lua_pushnil(L);
	lua_pushfstring(L, "ERROR: Unable to poll event: %s\n", err2str());
	return 2;
    }
/*    if (rc == 0) {
	lua_pushinteger(L, 0); //in case of timeout
	return 1;
    }*/
    for (i=0; i<N;) {
	if (items[i].revents & ZMQ_POLLIN) { // (i<M ? ZMQ_POLLIN : ZMQ_POLLOUT)
		printf("Event received: %d\n", i);
	    lua_pushinteger(L, ++i);
	    return 1;
	}
    }
    return 0;
}

static int new_poll_in(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
//    int M = luaL_checkinteger(L, 2); // # of socket events of type POLLIN

    int i, N = luaL_len(L, 1);
    zmq_pollitem_t *it = (zmq_pollitem_t *)lua_newuserdata(L, N*sizeof(zmq_pollitem_t));
    luaL_getmetatable(L, "caap.zmq.pollitem");
    lua_setmetatable(L, -2);

    for (i=0; i<N; it++) {
	it->fd = 0;
	it->revents = 0;
	it->events = ZMQ_POLLIN; // i<M ? ZMQ_POLLIN : ZMQ_POLLOUT;
	    lua_rawgeti(L, 1, ++i);
	    void *skt = *(void **)luaL_checkudata(L, -1, "caap.zmq.socket");
	it->socket = skt;
	    lua_pop(L, 1);
    }

    lua_pushinteger(L, N);
//    lua_pushinteger(L, M);
    lua_pushcclosure(L, &poll_now, 2); // upvalue: pollitem, N
    return 1;
}

static int poll_asstr(lua_State *L) {
    lua_pushstring(L, "zmq{Poll Item}");
    return 1;
}

static int poll_gc (lua_State *L) {
    zmq_pollitem_t *items = (zmq_pollitem_t *)luaL_checkudata(L, 1, "caap.zmq.pollitem");
    if (items)
	items = NULL;
    return 0;
}

// // // // // // // // // // // // //

static const struct luaL_Reg zmq_funcs[] = {
    {"pollin",  new_poll_in},
    {NULL, 	NULL}
};

static const struct luaL_Reg poll_meths[] = {
    {"__tostring", poll_asstr},
    {"__gc",	   poll_gc},
    {NULL,	   NULL}
};

/*   ******************************   */
int luaopen_ltry (lua_State *L) {
    luaL_newmetatable(L, "caap.zmq.pollitem");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, poll_meths, 0);

    // create library
    luaL_newlib(L, zmq_funcs);
    return 1;
}

