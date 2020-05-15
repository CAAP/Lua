#include <lua.h>
#include <lauxlib.h>

#include <msgpack.h>
#include <stdio.h>

///////////////////////////////////
/* Simple buffer */
typedef struct my_sbuffer {
    size_t size;
    char data[1];
    size_t alloc;
} my_sbuffer;

//////


static int

////////////////////////////////////////////

static const struct luaL_Reg dgst_funcs[] = {
    {"getU16", get_uint16},
    {"getU32", get_uint32},
    {"putU16", put_uint16},
    {"putU32", put_uint32},
    {"castU16", cast_uint16},
    {"castU32", cast_uint32},
    {NULL, NULL}
};

int luaopen_lmsgpack (lua_State *L) {
    // create library
    luaL_newlib(L, dgst_funcs);
    return 1;
}

