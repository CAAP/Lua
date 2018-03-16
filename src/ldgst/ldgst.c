#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdio.h>
#include <openssl/evp.h>


static int digestData(lua_State *L, const EVP_MD *md, const char *mssg) {
    EVP_MD_CTX *mdctx;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    mdctx = EVP_MD_CTX_create();
    EVP_DigestInit_ex(mdctx, md, NULL);
    // digests the data in mssg string(s)
    EVP_DigestUpdate(mdctx, mssg, strlen(mssg));
    // execute hashing function
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_destroy(mdctx);

    char c, str[md_len*2], *h = str, lookup[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    int i;
    for( i=0; i<md_len; i++ ) {
	c = md_value[i];
	*h++ = lookup[ (c >> 4) & 0x0F ];
	*h++ = lookup[ c & 0x0F ];
    }

    lua_pushlstring(L, str, md_len*2);
    return 1;
}

///////////////////////////////////

static int digestString(lua_State *L) {
    const char *message = luaL_checkstring(L, 1);
    EVP_MD *md = *(EVP_MD **)lua_touserdata(L, lua_upvalueindex(1));
    return digestData(L, md, message);
}

static int getDigest(lua_State *L) {
    const char *dgst = lua_tostring(L, 1);

    const EVP_MD *md;
    EVP_MD **pmd = (EVP_MD **)lua_newuserdata(L, sizeof(EVP_MD *));
    luaL_getmetatable(L, "caap.openssl.digest");
    lua_setmetatable(L, -2);

    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname( dgst );
    *pmd = md;
    if (md == NULL) {
	EVP_cleanup();
	lua_pushnil(L);
	lua_pushfstring(L, "Unknown message digest %s\n", dgst);
	return 2;
    }

    lua_pushcclosure(L, &digestString, 1); // pmd
    return 1;
}

//////

static int cleanUp(lua_State *L) {
    EVP_MD **pmd = luaL_checkudata(L, 1, "caap.openssl.digest");
    if (*pmd != NULL) {
	EVP_cleanup();
	*pmd = NULL;
    }
    return 0;
}

////////////////////////////////////////////

static const struct luaL_Reg dgst_funcs[] = {
    {"digest", getDigest},
    {NULL, NULL}
};

static const struct luaL_Reg dgst_meths[] = {
    {"__gc", cleanUp},
    {NULL, NULL}
};

int luaopen_ldgst (lua_State *L) {
    // DIGEST
    luaL_newmetatable(L, "caap.openssl.digest");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, dgst_meths, 0);

    // create library
    luaL_newlib(L, dgst_funcs);
    return 1;
}

