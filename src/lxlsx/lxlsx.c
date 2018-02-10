#include <xlsxio_read.h>

#include <lua.h>
#include <>

static int openFile(lua_State *L) {
    const char* fname = luaL_checkstring(L, 1);

    xlsxioreader reader;
    if ((reader = xlsxioread_open(fname)) == NULL) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error opening xlsx file: %s\n", fname);
	return 2;
    }

    p

    xlsxioread_close( reader );

    return 1;
}




