#include "tinyxml2.h"

#include <lua.hpp>

#ifdef __cplusplus
extern "C" {
#endif


static int parseDoc(lua_State *L) {
    const char* fname = luaL_checkstring(L, 1);
    const char* tag = luaL_checkstring(L, 2);

    XMLDocument doc;
    if (doc.LoadFile(fname) != XML_SUCCESS(0)) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error trying to parse document: %s\n", fname);
	return 2;
    }

    lua_newtable(L);
    int i = 1;
    const int K = doc.LastChildElement(tag).GetLineNum();
    XMLElement* ele = doc.FirstChildElement(tag);

    do {
	lua_pushstring(L, ele.Value());
	lua_rawseti(L, i++);
	ele = doc.NextSiblingElement(tag);
    } while (ele.GetLineNum() != K);

    return 1;
}



static const struct luaL_Reg xml_funcs[] = {
  {"fromTable", fromTable},
  {NULL, NULL}
};


int luaopen_locv (lua_State *L) {
  // create the library
  luaL_newlib(L, cv_funcs);
  return 1;
}



#ifdef __cplusplus
}
#endif


