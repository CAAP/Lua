#include <lua.h>
#include <lauxlib.h>

#include "svm.h"

#include <stdio.h>

#define checknodes(L) (struct svm_node *)luaL_checkudata(L, 1, "caap.svm.nodes")

typedef struct svm_problem   svm_prob;
typedef struct svm_node      svm_node;
typedef struct svm_parameter svm_param;
typedef struct svm_model     svm_model;

/*
 *  struct svm_parameter
 *
*/

void getParams(lua_State *L, int j, svm_param *param) {
    luaL_checktype(L, j, LUA_TTABLE);

    int stype, ktype;

    param->coef0 = 0.0; param->cache_size = 100.0; param->eps = 1e-3; param->p = 0.1;
    param->nr_weight = 0; param->weight_label = NULL; param->weight = NULL;
    param->probability = 0; param->shrinking = 1;

    lua_getfield(L, j, "svmtype");
    lua_getfield(L, lua_upvalueindex(1), lua_tostring(L, -1)); // string -> enum
    stype = lua_tointeger(L, -1);
    param->svm_type = stype;
    lua_pop(L, 2);

    lua_getfield(L, j, "ktype");
    lua_getfield(L, lua_upvalueindex(1), lua_tostring(L, -1)); // string -> enum
    ktype = lua_tointeger(L, -1);
    param->kernel_type = ktype;
    lua_pop(L, 2);

    param->degree = 3;
    if (ktype == POLY) {
	lua_getfield(L, j, "degree");
	param->degree = lua_tointeger(L, -1);
	lua_pop(L, 2);
    }

    param->gamma = 0.0;
    if (ktype == RBF || ktype == SIGMOID) {
	lua_getfield(L, j, "gamma");
	param->gamma = lua_tonumber(L, -1);
	lua_pop(L, 2);
    }

    param->C = 1.0;
    if (stype == C_SVC || stype == EPSILON_SVR || stype == NU_SVR) {
	lua_getfield(L, j, "c");
	param->C = lua_tonumber(L, -1);
	lua_pop(L, 2);
    }

    param->nu = 0.5;
    if (stype == NU_SVC || stype == ONE_CLASS || stype == NU_SVR) {
	lua_getfield(L, j, "nu");
	param->nu = lua_tonumber(L, -1);
	lua_pop(L, 2);
    }
}

/************************/

int pushSV(lua_State *L, svm_node **SV, int nr_sv) {
    int i, j, k = nr_sv;
    svm_node *nodes, node;

    for (i=0; i<nr_sv; i++) {
	nodes = SV[i];
	j = 0;
	while (nodes[j++].index != -1)
	    k++;
    }

    svm_node *svs = (svm_node *)lua_newuserdata(L, k*sizeof(svm_node));
    luaL_getmetatable(L, "caap.svm.nodes");
    lua_setmetatable(L, -2);
    lua_pushnumber(L, nr_sv); lua_pushinteger(L, k);

    k = 0;
    for (i=0; i<nr_sv; i++) {
	nodes = SV[i];
	j = 0;
	while (nodes[j].index != -1) {
	    node = svs[k++];
	    node.index = nodes[j].index;
	    node.value = nodes[j++].value;
	}
	node = svs[k++]; node.index = -1; node.value = 0.0;
    }

    return 3;
}

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
 *  struct svm_problem { int l; double *y; struct svm_node **x; }
 *
 *  l is the number of training data,
 *  y is an array containing their target values (ints in classification),
 *  x is an array pointers, pointing to sparse representation
*/

static int nodes_train(lua_State *L) {
    svm_node *nodes = checknodes(L);
    luaL_checktype(L, 2, LUA_TTABLE); // y's
    luaL_checktype(L, 3, LUA_TTABLE); // params

    int i, j, M = luaL_len(L, 2);
    double *y = (double *)lua_newuserdata(L, M*sizeof(double));
    for(i=0; i<M; i++) {
	lua_rawgeti(L, 2, i+1);
	y[i] = lua_tonumber(L, -1);
	lua_pop(L, 1);
    }

    svm_node **x = (svm_node **)lua_newuserdata(L, M*sizeof(svm_node *));
    x[0] = &nodes[0]; // first reference done!
    j = 0;
    for (i=1; i<M; i++) {
	while (nodes[j++].index != -1)
	    ;
	x[i] = &nodes[j++];
    };

    svm_prob prob;
    prob.l = M;
    prob.y = y;
    prob.x = x;

    svm_param params;
    getParams(L, 3, &params);
    lua_pop(L, 1);

    svm_model *model = svm_train(&prob, &params);
    svm_node **SV = model->SV;

    // SVs
    int nr_sv = model->l; // total #SVs
    pushSV(L, SV, nr_sv);

/*
    int stype = params.svm_type;
    if (stype == EPSILON_SVR || stype == NU_SVR) {
	d
    } else { // classification
	lua_newtable(L); // SVs
	
	int i,j;
	for (i=0; i<nr_sv; i++) {
	    f
	}
    }
*/

    svm_free_and_destroy_model( &model );
    svm_destroy_param( &params );

    return 3;
}

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
