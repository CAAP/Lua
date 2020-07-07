#include <lua.h>
#include <lauxlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


// -1 means error, 0, 1, ... means success of some kind
static int does_file_exists(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, access(path, F_OK) != -1);
    return 1;
}

/* ***************** */

static const struct luaL_Reg bsd_funcs[] = {
    {"file_exists", does_file_exists},
    {NULL, NULL}
};

int luaopen_lbsd (lua_State *L) {
    // create library
    luaL_newlib(L, bsd_funcs);

    return 1;
}
