#include <lua.h>
#include <lauxlib.h>

#include <stdint.h>
#include <stdio.h>


///////////////////////////////////
static int get_uint16(lua_State *L) {
    const uint8_t *buff = (const uint8_t*)luaL_checkstring(L, 1);
    lua_pushinteger(L, (((uint16_t)buff[0]) << 8) | ((uint16_t)buff[1]));
    return 1;
}

static int put_uint16(lua_State *L) {
    uint16_t val = (uint16_t)luaL_checkinteger(L, 1);
    uint8_t buff[3];
    buff[0] = (uint8_t)((val >> 8) & 0xff);
    buff[1] = (uint8_t)(val & 0xff);
    buff[2] = '\0';
    lua_pushlstring(L, (const char*)buff, 3);
    return 1;
}

static int cast_uint16(lua_State *L) {
    const uint16_t *buff = (const uint16_t *)luaL_checkstring(L, 1);
    lua_pushinteger(L, *buff);
    return 1;
}

//////
static int get_uint32(lua_State *L) {
    const uint8_t *buff = (const uint8_t *)luaL_checkstring(L, 1);
    lua_pushinteger(L, (((uint32_t)buff[0]) << 24) | (((uint32_t)buff[1]) << 16) | (((uint32_t)buff[2]) << 8) | ((uint16_t)buff[3]));
    return 1;
}

static int put_uint32(lua_State *L) {
    uint32_t val = (uint32_t)luaL_checknumber(L, 1);
    uint8_t buff[5];
    buff[0] = (uint8_t)((val >> 24) & 0xff);
    buff[1] = (uint8_t)((val >> 16) & 0xff);
    buff[2] = (uint8_t)((val >> 8) & 0xff);
    buff[3] = (uint8_t)(val & 0xff);
    buff[4] = '\0';
    lua_pushlstring(L, (const char*)buff, 5);
    return 1;
}

static int cast_uint32(lua_State *L) {
    const uint32_t *buff = (const uint32_t *)luaL_checkstring(L, 1);
    lua_pushinteger(L, *buff);
    return 1;
}

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

int luaopen_lints (lua_State *L) {
    // create library
    luaL_newlib(L, dgst_funcs);
    return 1;
}

