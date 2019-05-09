#include <lua.h>
#include <lauxlib.h>

#include "svm.h"

#include <stdio.h>

#define checknodes(L) (struct svm_node *)luaL_checkudata(L, 1, "caap.svm.nodes")

typedef struct svm_problem svm_prob;
typedef struct svm_node svm_node;

/*
 *  struct svm_node { int index; double value; }
 *
 *  index -1 indicates the end of the vector,
 *  indices must be in ascending(ASC) order
 *
*/

static int nodes(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    int N = luaL_checkinteger(L, 2);

    svm_node *x = (svm_node *)lua_newuserdata(L, N*sizeof(svm_node));
    luaL_getmetatable(L, "caap.svm.nodes");
    lua_setmetatable(L, -2);

    N = luaL_len(L, 1) + 1;
    int M, i, j, k = 0;
    for (i=1; i<N;) {
	lua_rawgeti(L, 1, i++);
// sparse values + endofvector (-1)
	M = luaL_len(L, -1) + 1;
	for (j=2; j<M; k++) { // first element is the label; skip it
	    lua_rawgeti(L, -1, j++);
	    x[k].index = lua_tointeger(L, -1);
	    lua_pop(L, 1);
	    lua_rawgeti(L, -1, j++);
	    x[k].value = lua_tonumber(L, -1);
	    lua_pop(L, 1);
	}
	x[k].index = -1; x[k].value = 0.0; k++;
// done
	lua_pop(L, 1);
    }

    return 1;
}

/*
static int get_node(lua_State *L) {
    svm_node *nodes = checknodes(L);
    int M = luaL_checkinteger(L, 2);
    int N = luaL_checkinteger(L, 3);

    lua_newtable(L);
    int k, m = 1;
    for (k=0; k<N; k++) {
	if (m == M) {
	    lua_
	}
	if ((nodes[k].index == -1) && (++m > M))
	    break;
    }

    lua_pushnumber(L, 1);

    return 1;
}
*/

/*
 *  struct svm_problem { int l; double *y; struct svm_node **x; }
 *
 *  l is the number of training data,
 *  y is an array containing their target values (ints in classification),
 *  x is an array pointers, pointing to sparse representation
*/

static int nodes_gc(lua_State *L) {
    svm_node *nodes = checknodes(L);
    if (nodes != NULL)
	nodes = NULL;
    return 0;
}

static int nodes2string(lua_State *L) {
    lua_pushstring(L, "svmNODES");
    return 1;
}

// // // // // // // // // // // // //
//
//  add "svm & kernel types" as upvalue to method "train"
//
static void svm_fn(lua_State *L) {
    lua_newtable(L); // upvalue
    lua_pushinteger(L, C_SVC); lua_setfield(L, -2, "C");
    lua_pushinteger(L, NU_SVC);  lua_setfield(L, -2, "NU");
    lua_pushinteger(L, ONE_CLASS);  lua_setfield(L, -2, "ONE");
    lua_pushinteger(L, EPSILON_SVR);  lua_setfield(L, -2, "ESVR");
    lua_pushinteger(L, NU_SVR);  lua_setfield(L, -2, "NSVR");
    lua_pushinteger(L, LINEAR); lua_setfield(L, -2, "LINEAR");
    lua_pushinteger(L, POLY); lua_setfield(L, -2, "POLY");
    lua_pushinteger(L, RBF); lua_setfield(L, -2, "RBF");
    lua_pushinteger(L, SIGMOID); lua_setfield(L, -2, "SIGMOID");
    lua_pushinteger(L, PRECOMPUTED); lua_setfield(L, -2, "PREC");
    lua_pushcclosure(L, &nodes_train, 1);
    lua_setfield(L, -2, "train");
}

static const struct luaL_Reg svm_funcs[] = {
    {"nodes", nodes},
    {NULL, NULL}
};

static const struct luaL_Reg nodes_meths[] = {
    {"__tostring", nodes2string},
    {"__gc", nodes_gc},
    {NULL, NULL}
};

int luaopen_lsvm (lua_State *L) {
    luaL_newmetatable(L, "caap.svm.nodes");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, nodes_meths, 0);
    svm_fn(L);

    // create library
    luaL_newlib(L, svm_funcs);
    return 1;
}
