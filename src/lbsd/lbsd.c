#include <lua.h>
#include <lauxlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/*
    int unveil(const char *path, const char *flags);
*/
static int unveil_raw(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const char *flags = luaL_checkstring(L, 2);

    lua_pushboolean(L, 1);
    if(0 == unveil(path, flags))
	return 1;
    else {
	lua_pushnil(L);
	lua_pushstring(L, strerror(errno));
	return 2;
    }
}

/*
    unsigned int sleep(unsigned int);
*/
static int sleep_raw(lua_State *L) {
    const unsigned int secs = luaL_checkinteger(L, 1);

    lua_pushboolean(L, 1);
    if(0 == sleep(secs))
	return 1;
    else {
	lua_pushnil(L);
	lua_pushstring(L, strerror(errno));
	return 2;
    }
}

/*
    int pledge(const char *promises, const char *execpromises);
*/


static const struct luaL_Reg bsd_funcs[] = {
    {"unveil", unveil_raw},
    {"sleep", sleep_raw},
    {NULL, NULL}
};

int luaopen_lbsd (lua_State *L) {
    // create library
    luaL_newlib(L, bsd_funcs);
    return 1;
}
