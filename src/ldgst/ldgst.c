#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdio.h>
#include <openssl/evp.h>

static int getDigest(lua_State *L) {
    const char *dgst = lua_tostring(L, 1);
    const int type = lua_type(L, 2);
    char *mssg;
    int N;
    if (type == LUA_TSTRING)
	*mssg = lua_tostring(L, 2);
    else
	lua_
    if (dgst == NULL || mssg == NULL) { return 0; }

    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    OpenSSL_add_all_digests();

    md = EVP_get_digestbyname( dgst );
    if (md == NULL) {
	EVP_cleanup();
	lua_pushnil(L);
	lua_pushfstring(L, "Unknown message digest %s\n", dgst);
	return 2;
    }

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
    EVP_cleanup();
    return 1;
}

/*
static int md5(lua_State *L) {
    return digest(L, "md5");
}
*/

static const struct luaL_Reg dgst_funcs[] = {
    {"digest", getDigest},
    {NULL, NULL}
};

int luaopen_ldgst (lua_State *L) {
    // create library
    luaL_newlib(L, dgst_funcs);
    return 1;
}

