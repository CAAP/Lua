#include <lua.h>
#include <lauxlib.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

// -1 means error, 0, 1, ... means success of some kind
static int does_file_exists(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, access(path, F_OK) != -1);
    return 1;
}

static int sleep_msec(lua_State *L) {
    long msec = luaL_checkinteger(L, 1); // milliseconds
    struct timespec ts;
    ts.tv_sec = (time_t)msec/1000;
    ts.tv_nsec = (msec%1000) * 1000000L;

    if (-1 == nanosleep( &ts, NULL )) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR in function nanosleep");
	return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/* ***************** */

static const struct luaL_Reg bsd_funcs[] = {
    {"file_exists", does_file_exists},
    {"sleep", sleep_msec},
    {NULL, NULL}
};

int luaopen_lbsd (lua_State *L) {
    // create library
    luaL_newlib(L, bsd_funcs);

    return 1;
}
