#include <lua.h>
#include <lauxlib.h>

#include <string.h>

#include "include/libstemmer.h"

/* {=====================================================================
 *    Auxiliary
 * ======================================================================} */

#define checkstem(L,i) *(struct sb_stemmer **)luaL_checkudata(L, i, "caap.stemmer.stem")
#define newstem(L) (struct sb_stemmer **)lua_newuserdata(L, sizeof(struct sb_stemmer *)); luaL_getmetatable(L, "caap.stemmer.stem"); lua_setmetatable(L, -2)

/* ================================================== */

static int algos(lua_State *L) {
    lua_newtable(L);

    const char ** list = sb_stemmer_list();
    int i = 0;
    while (list[i] != NULL) {
	lua_pushstring(L, list[i++]);
	lua_rawseti(L, -2, i);
    }

    return 1;
}

static int newStemmer(lua_State *L) {
    const char *algo = lua_tostring(L, 1);
    const char *enc = lua_tostring(L, 2);

    struct sb_stemmer **stemmer = newstem(L);
    *stemmer = sb_stemmer_new(algo, enc);
    if (*stemmer == NULL)
	luaL_error(L, "Error creating stemmer for %s with %s encoding.\n", algo, enc);

    return 1;
}

// XXX

static int stemming(lua_State *L) {
    struct sb_stemmer *stemmer = checkstem(L, 1);
    const char *wrd = lua_tostring(L, 2);

    int size = (int)strlen(wrd);

    const sb_symbol *ans;
    ans = sb_stemmer_stem(stemmer, (const sb_symbol *)wrd, size);
    if (ans == NULL) {
	lua_pushnil(L);
	lua_pushfstring(L, "Out-of-memory error stemming word: %s\n", wrd);
    }
    lua_pushlstring(L, (char *)ans, (size_t)sb_stemmer_length(stemmer));

    return 1;
}

static int deleteStem(lua_State *L) {
    struct sb_stemmer *stemmer = checkstem(L, 1);
    if (stemmer != NULL) {
	sb_stemmer_delete(stemmer);
	stemmer = NULL;
    }
    return 0;
}

static int printStem(lua_State *L) {
    lua_pushstring(L, "libStemmer");
    return 1;
}

/* {=====================================================================
 *    Interface
 * ======================================================================} */

static const struct luaL_Reg stem_funs[] = {
    /* probability dists */
    {"valid", algos},
    {"stemmer", newStemmer},
    {NULL, NULL}
};

static const struct luaL_Reg stem_meths[] = {
    /* probability dists */
    {"__gc", deleteStem},
    {"__tostring", printStem},
    {"stem", stemming},
    {NULL, NULL}
};

int luaopen_lstem (lua_State *L) {
    // metatable
    luaL_newmetatable(L, "caap.stemmer.stem");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, stem_meths, 0);

    // create library
    luaL_newlib(L, stem_funs);
    return 1;
}

