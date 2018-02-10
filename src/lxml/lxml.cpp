#include "tinyxml2.h"

#include <lua.hpp>

#ifdef __cplusplus
extern "C" {
#endif


static int parseDoc(lua_State *L) {
    const char* fname = luaL_checkstring(L, 1);
    const char* tag = luaL_checkstring(L, 2);

    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(fname) != tinyxml2::XMLError::XML_SUCCESS) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error trying to parse document: %s\n", fname);
	return 2;
    }

    lua_newtable(L);
    int i = 1;
    const int K = doc.LastChildElement(tag)->GetLineNum();
    tinyxml2::XMLElement* ele = doc.FirstChildElement(tag);

    do {
	lua_pushstring(L, ele->Value());
	lua_rawseti(L, -1, i++);
	ele = doc.NextSiblingElement(tag);
    } while (ele->GetLineNum() != K);

    return 1;
}


static const struct luaL_Reg xml_funcs[] = {
  {"parse", parseDoc},
  {NULL, NULL}
};


int luaopen_lxml (lua_State *L) {
  // create the library
  luaL_newlib(L, xml_funcs);
  return 1;
}



#ifdef __cplusplus
}
#endif


