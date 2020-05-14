#include <lua.h>
#include <lauxlib.h>

#include <stdint.h>
#include <stdio.h>


///////////////////////////////////
static int get_uint16(lua_State *L) {
    const unsigned char *buff = (const unsigned char*)luaL_checkstring(L, 1);
    lua_pushinteger(L, (((uint16_t)buff[0]) << 8) | ((uint16_t)buff[1]));
    return 1;
}

static int put_uint16(lua_State *L) {
    uint16_t val = (uint16_t)luaL_checkinteger(L, 1);
    unsigned char buff[3];
    buff[0] = (unsigned char)((val >> 8) & 0xff);
    buff[1] = (unsigned char)(val & 0xff);
    buff[2] = '\0';
    lua_pushlstring(L, (const char*)buff, 3);
    return 1;
}

//////
static int get_uint32(lua_State *L) {
    const unsigned char *buff = (const unsigned char*)luaL_checkstring(L, 1);
    lua_pushinteger(L, (((uint32_t)buff[0]) << 24) | (((uint32_t)buff[1]) << 16) | (((uint32_t)buff[2]) << 8) | ((uint16_t)buff[3]));
    return 1;
}

static int put_uint32(lua_State *L) {
    uint32_t val = (uint32_t)luaL_checknumber(L, 1);
    unsigned char buff[5];
    buff[0] = (unsigned char)((val >> 24) & 0xff);
    buff[1] = (unsigned char)((val >> 16) & 0xff);
    buff[2] = (unsigned char)((val >> 8) & 0xff);
    buff[3] = (unsigned char)(val & 0xff);
    buff[4] = '\0';
    lua_pushlstring(L, (const char*)buff, 5);
    return 1;
}


////////////////////////////////////////////

static const struct luaL_Reg dgst_funcs[] = {
    {"asU16", get_uint16},
    {"asU32", get_uint32},
    {"u16", put_uint16},
    {"u32", put_uint32},
    {NULL, NULL}
};

int luaopen_lints (lua_State *L) {
    // create library
    luaL_newlib(L, dgst_funcs);
    return 1;
}

