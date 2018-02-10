#include <lua.hpp>
#include <snappy.h>

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

//////////////////////////////

static int compress(lua_State *L) {
    const char* input = luaL_checkstring(L, 1);
    std::string output;

    snappy::Compress(input, strlen(input), &output);

    lua_pushstring(L, output.c_str());
    return 1;
}

/////////////////////////////

static const struct luaL_Reg lsanp_funcs[] = {
  {"compress", compress},
  {NULL, NULL}
};

int luaopen_lsnap (lua_State *L) {
  // create the library
  luaL_newlib(L, lsanp_funcs);
  return 1;
}

#ifdef __cplusplus
} // extern C
#endif

