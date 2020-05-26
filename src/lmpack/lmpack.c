#include <lua.h>
#include <lauxlib.h>

#include <msgpack.h>
#include <stdio.h>

///////////////////////////////////

int addlstring(void *data, const char* buf, size_t len) {
    luaL_addlstring((luaL_Buffer *)data, buf, len);
    return 0;
}

void addSimple(lua_State *L, int k, int tt, msgpack_packer *pk) {

    if (tt == LUA_TNIL)
	msgpack_pack_nil(pk);

    else if (tt == LUA_TBOOLEAN) {
	if (lua_toboolean(L, k))
	    msgpack_pack_true(pk);
	else
	    msgpack_pack_false(pk);

    } else if (tt == LUA_TNUMBER) {
	if (lua_isinteger(L, k)) {
	    lua_Integer x = lua_tointeger(L, k);
	    if (x < 0) {
		if (x >= SCHAR_MIN) { msgpack_pack_int8(pk, x); }
		else if (x >= SHRT_MIN) { msgpack_pack_int16(pk, x); }
		else if (x >= INT_MIN) { msgpack_pack_int32(pk, x); }
		else { msgpack_pack_int64(pk, x); }
	    } else {
		if (x <= UCHAR_MAX) { msgpack_pack_uint8(pk, x); }
		else if (x <= USHRT_MAX) { msgpack_pack_uint16(pk, x); }
		else if (x <= UINT_MAX) { msgpack_pack_uint32(pk, x); }
		else { msgpack_pack_uint64(pk, x); }
	    }

	} else
	    msgpack_pack_double(pk, lua_tonumber(L, k));

    } else if (tt == LUA_TSTRING) {
	size_t len;
	const char *ss = lua_tolstring(L, k, &len);
	msgpack_pack_str(pk, len);
	msgpack_pack_str_body(pk, (const void *)ss, len);
    }

}

///////////////////////////////////

static int pack(lua_State *L) {
    const int tt = lua_type(L, 1);

    luaL_Buffer b;
    luaL_buffinit(L, &b);

    msgpack_packer pk;
    msgpack_packer_init(&pk, (void *)&b, &addlstring);

    addSimple(L, 1, tt, &pk);

    luaL_pushresult(&b);
    return 1;
}

static int pack_array(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_Integer N = luaL_len(L, 1);

    luaL_Buffer b;
    luaL_buffinit(L, &b);

    msgpack_packer pk;
    msgpack_packer_init(&pk, (void *)&b, &addlstring);

    msgpack_pack_array(&pk, N);
    int i;
    for (i=1; i<N+1; i++) {
	lua_rawgeti(L, 1, i);
	addSimple(L, -1, lua_type(L, -1), &pk);
	lua_pop(L, 1);
    }

    luaL_pushresult(&b);
    return 1;
}

static int pack_table(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    luaL_Buffer b;
    luaL_buffinit(L, &b);

    msgpack_packer pk;
    msgpack_packer_init(&pk, (void *)&b, &addlstring);

    int N = 0;
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
	if (lua_type(L, -2) == LUA_TSTRING)
	    N++;
	lua_pop(L, 1);
    }

    msgpack_pack_map(&pk, N);
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
	if (lua_type(L, -2) == LUA_TSTRING) {
	    addSimple(L, -2, LUA_TSTRING, &pk);
	    addSimple(L, -1, lua_type(L, -1), &pk);
	}
	lua_pop(L, 1);
    }

    luaL_pushresult(&b);
    return 1;
}

//////

void getSimple(lua_State *L, msgpack_object o) {
    switch(o.type) {
	case MSGPACK_OBJECT_NIL: lua_pushnil(L); break;
	case MSGPACK_OBJECT_BOOLEAN: lua_pushboolean(L, o.via.boolean); break;
	case MSGPACK_OBJECT_POSITIVE_INTEGER: lua_pushinteger(L, o.via.u64); break;
	case MSGPACK_OBJECT_NEGATIVE_INTEGER: lua_pushinteger(L, o.via.i64); break;
	case MSGPACK_OBJECT_FLOAT32:
	case MSGPACK_OBJECT_FLOAT64: lua_pushnumber(L, o.via.f64); break;
	case MSGPACK_OBJECT_STR: lua_pushlstring(L, o.via.str.ptr, o.via.str.size); break;
	default: break;
    }
}

static int unpack(lua_State *L) {
    size_t N;
    const char *msg = luaL_checklstring(L, 1, &N);

    msgpack_unpacked ans;
    msgpack_unpack_return q;
    msgpack_unpacked_init( &ans );

    size_t M = 0;
    q = msgpack_unpack_next(&ans, msg, N, &M);

    if (q == MSGPACK_UNPACK_PARSE_ERROR) {
	lua_pushnil(L);
	lua_pushliteral(L, "ERROR: msgpack unpack parse error.\n");
	return 2;
    }

    msgpack_object o = ans.data;
    if (o.type ==  MSGPACK_OBJECT_ARRAY) {
	lua_newtable(L);
	int k = 1;
	if (o.via.array.size != 0) {
	    msgpack_object *po = o.via.array.ptr;
	    msgpack_object const *pend = po + o.via.array.size;
	    for (; po < pend; ++po) {
		getSimple(L, *po);
		lua_rawseti(L, -2, k++);
	    }
	}
    } else if (o.type ==  MSGPACK_OBJECT_MAP) {
	lua_newtable(L);
	if (o.via.array.size != 0) {
	    msgpack_object_kv *po = o.via.map.ptr;
	    msgpack_object_kv const *pend = po + o.via.map.size;
	    for (; po < pend; ++po) {
		getSimple(L, po->key);
		getSimple(L, po->val);
		lua_setfield(L, -3, lua_tostring(L, -2));
		lua_pop(L, 1); // key
	    }
	}
    } else
	getSimple(L, o);


    msgpack_unpacked_destroy( &ans );
    return 1;
}

////////////////////////////////////////////

static const struct luaL_Reg dgst_funcs[] = {
    {"pack",	pack},
    {"array", 	pack_array},
    {"table",	pack_table},
    {"unpack",	unpack},
    {NULL, 	NULL}
};

int luaopen_lmpack (lua_State *L) {
    // create library
    luaL_newlib(L, dgst_funcs);
    return 1;
}

