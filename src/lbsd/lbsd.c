#include <lua.h>
#include <lauxlib.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <netinet/in.h>
#include <resolv.h>

#include <uuid.h>

// // // // // // // // //

static uuid_t *UUID, UID;

#define checkuuid(L, k) (uint8_t *)luaL_checkudata(L, k, "caap.bsd.uuid")

// // // // // // // // //


//		  UUID	  	      //
/*   ******************************   */

static void init_uuid(lua_State *L) {
    uint32_t rc;
    uuid_create_nil(UUID, &rc);
    if (rc != uuid_s_ok)
	luaL_error(L, "error while computing UUID");
}

static int uuid_as_octet(uint8_t *octet) {
    uint32_t rc;
    uuid_create(UUID, &rc);
    if (rc != uuid_s_ok)
	return -1;

    uuid_enc_be((void *)octet, UUID);
    return 0;
}

static int32_t uuid_compare_octet(uint8_t *x, uint8_t *y) {
    uint32_t rc;
    int32_t p;
    uuid_t u1, u2;
    uuid_dec_be((void *)x, &u1);
    uuid_dec_be((void *)y, &u2);
    p = uuid_compare(&u1, &u2, &rc);
    if (rc != uuid_s_ok)
	return 7;

    return p;
}

static int octet_as_b64(uint8_t *octet, char *uuid) {
    size_t N = 16, M = 25;

    if (-1 == b64_ntop(octet, N, uuid, M))
	return -1;

    return 0;
}

static int new_uuid(lua_State *L) {
    uint8_t *octet = (uint8_t *)lua_newuserdata(L, 16*sizeof(uint8_t));
    if (octet == NULL) {
	lua_pushnil(L);
	lua_pushliteral(L, "error while creating userdata for uuid");
	return 2;
    }
    luaL_setmetatable(L, "caap.bsd.uuid");
    if (-1 == uuid_as_octet(octet)) {
	lua_pushnil(L);
	lua_pushliteral(L, "error while computing UUID");
	return 2;
    }
    return 1;
}

static int uuid_comparison(lua_State *L) {
    uint8_t *x = checkuuid(L, 1);
    uint8_t *y = checkuuid(L, 2);
    int32_t rc = uuid_compare_octet(x, y);
    if (rc == 7) {
	lua_pushnil(L);
	lua_pushliteral(L, "error comparing uuid's");
	return 2;
    }
    lua_pushinteger(L, rc);
    return 1;
}

static int uuid_lt(lua_State *L) {
    uint8_t *x = checkuuid(L, 1);
    uint8_t *y = checkuuid(L, 2);
    int32_t rc = uuid_compare_octet(x, y);
    if (rc == 7) {
	lua_pushnil(L);
	lua_pushliteral(L, "error comparing uuid's");
	return 2;
    }
    lua_pushboolean(L, rc == -1);
    return 1;
}

static int uuid_le(lua_State *L) {
    uint8_t *x = checkuuid(L, 1);
    uint8_t *y = checkuuid(L, 2);
    int32_t rc = uuid_compare_octet(x, y);
    if (rc == 7) {
	lua_pushnil(L);
	lua_pushliteral(L, "error comparing uuid's");
	return 2;
    }
    lua_pushboolean(L, rc != 1);
    return 1;
}

static int uuid_eq(lua_State *L) {
    uint8_t *x = checkuuid(L, 1);
    uint8_t *y = checkuuid(L, 2);

    uint32_t rc;
    int p;
    uuid_t u1, u2;
    uuid_dec_be((void *)x, &u1);
    uuid_dec_be((void *)y, &u2);
    p = uuid_equal(&u1, &u2, &rc);
    if (rc != uuid_s_ok) {
	lua_pushnil(L);
	lua_pushliteral(L, "error comparing uuid's");
	return 2;
    }

    lua_pushboolean(L, rc);
    return 1;
}

static int uuid_asb64(lua_State *L) {
    uint8_t *octet = checkuuid(L, 1);
    char uuid[25];
    if (-1 == octet_as_b64(octet, uuid)) {
	lua_pushnil(L);
	lua_pushliteral(L, "error while converting octet to base64");
	return 2;
    }
    lua_pushstring(L, uuid);
    return 1;
}

static int uuid_gc(lua_State *L) {
    uint8_t *octet = checkuuid(L, 1);
    if (octet != NULL)
	octet = NULL;
    return 0;
}

static int uuid_asstr(lua_State *L) {
    uint8_t *octet = checkuuid(L, 1);
    lua_pushstring(L, (char *)octet);
    return 1;
}

/*   ******************************   */

// -1 means error, 0, 1, ... means success of some kind
static int does_file_exists(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, access(path, F_OK) != -1);
    return 1;
}

/*   ******************************   */

static int sleep_msec(lua_State *L) {
    long msec = luaL_checkinteger(L, 1); // milliseconds
    struct timespec ts;
    ts.tv_sec = (time_t)msec/1000;
    ts.tv_nsec = (msec%1000) * 1000000L;

    if (-1 == nanosleep( &ts, NULL )) {
	lua_pushnil(L);
	lua_pushstring(L, "ERROR in function nanosleep");
	return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

//		 BASE64		      //
/*   ******************************   */

static int str2b64(lua_State *L) {
    size_t N;
    const unsigned char *y = (unsigned char *)luaL_checklstring(L, 1, &N);
    size_t M = 4*(N/3+1)+1;

    char *z  = (char *)lua_newuserdata(L, M);
    if (-1 == b64_ntop(y, N, z, M)) {
	lua_pushnil(L);
	lua_pushliteral(L, "ERROR: b64_ntop encoding base64");
	return 2;
    } else
	lua_pushstring(L, z);

    return 1;
}

static int b642str(lua_State *L) {
    size_t N;
    const char *y = luaL_checklstring(L, 1, &N);
    size_t M = 3*N/4 + 1;

    unsigned char *z  = (unsigned char *)lua_newuserdata(L, M);
    int len = b64_pton(y, z, M);
    if (-1 == len) {
	lua_pushnil(L);
	lua_pushliteral(L, "ERROR: b64_pton encoding base64");
	return 2;
    } else
	lua_pushlstring(L, (char *)z, len);

    return 1;
}

/*   ******************************   */

static const struct luaL_Reg bsd_funcs[] = {
    {"file_exists", does_file_exists},
    {"sleep", 	    sleep_msec},
    {"uuid",	    new_uuid},
    {"asB64", 	    str2b64},
    {"fromB64",	    b642str},
    {NULL, NULL}
};

/*   ******************************   */

static const struct luaL_Reg uuid_meths[] = {
    {"__tostring",  uuid_asstr},
    {"__gc",	    uuid_gc},
    {"__eq", 	    uuid_eq},
    {"__lt",	    uuid_lt},
    {"__le",	    uuid_le},
    {"b64",         uuid_asb64},
    {"compare",     uuid_comparison},
    {NULL,	    NULL}
};

/*   ******************************   */

int luaopen_lbsd (lua_State *L) {
    // uuid metatable
    luaL_newmetatable(L, "caap.bsd.uuid");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, uuid_meths, 0);

    // check uuid library is up and running
    UUID = &UID;
    init_uuid(L);

    // create library
    luaL_newlib(L, bsd_funcs);

    return 1;
}
