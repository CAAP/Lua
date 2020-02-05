#include <lua.h>
#include <lauxlib.h>

#include "svm.h"

#include <stdio.h>
#include <math.h>

#define checknodes(L) (struct svm_node *)luaL_checkudata(L, 1, "caap.svm.nodes")
#define checksvecs(L) (struct svm_node **)luaL_checkudata(L, 1, "caap.svm.svecs")

typedef struct svm_problem   svm_prob;
typedef struct svm_node      svm_node;
typedef struct svm_parameter svm_param;
typedef struct svm_model     svm_model;

/*
 *  struct svm_parameter
 *
*/

double round(double x) { return floor(x*100)/100.0; }

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
	lua_pop(L, 1);
    }

    param->gamma = 0.0;
    if (ktype == RBF || ktype == SIGMOID || ktype == POLY) {
	lua_getfield(L, j, "gamma");
	param->gamma = lua_tonumber(L, -1);
	lua_pop(L, 1);
    }

    param->C = 1.0;
    if (stype == C_SVC || stype == EPSILON_SVR || stype == NU_SVR) {
	lua_getfield(L, j, "c");
	param->C = lua_tonumber(L, -1);
	lua_pop(L, 1);
    }

    param->nu = 0.5;
    if (stype == NU_SVC || stype == ONE_CLASS || stype == NU_SVR) {
	lua_getfield(L, j, "nu");
	param->nu = lua_tonumber(L, -1);
	lua_pop(L, 1);
    }
}

/************************/

int indices(lua_State *L, svm_model *model) {
    lua_newtable(L);

    if (model->sv_indices != NULL)
	for(int i=0;i<model->l;) {
	    lua_pushinteger( L, model->sv_indices[i] );
    	    lua_rawseti(L, -2, ++i);
	}

    return 1;
}

int predict_node(lua_State *L, svm_model *model, svm_node *x) {
    int i,nr_class = model->nr_class;
    int k = 1;
    if(!(model->param.svm_type == ONE_CLASS ||
	model->param.svm_type == EPSILON_SVR ||
	model->param.svm_type == NU_SVR))
	    k = nr_class*(nr_class-1)/2;

    if (k > 1) {
	double *dec_values = (double *)lua_newuserdata(L, k*sizeof(double));
	svm_predict_values(model, x, dec_values);
	lua_newtable(L);
	for (i=0; i<k;) {
	    lua_pushnumber(L, round(dec_values[i++]));
	    lua_rawseti(L, -2, i);
	}
    } else {
	double dec = 0.0;
	svm_predict_values(model, x, &dec);
	lua_pushnumber(L, round(dec));
    }

    return 1;
}

int predict_nodes(lua_State *L, svm_model *model, svm_prob *prob) {
    int i,N = prob->l;
    svm_node **xs = prob->x;

    lua_newtable(L);
    for (i=0; i<N;) {
	predict_node(L, model, xs[i++]); // push preds on stack
	lua_rawseti(L, -2, i);
    }

    return 1;
}

//    svm_get_sv_indices(const struct svm_model *model, int *sv_indices);

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

void target(lua_State *L, const int M, svm_prob *prob) {
    int i;
    for(i=0; i<M; i++) {
	lua_rawgeti(L, 2, i+1);
	prob->y[i] = lua_tonumber(L, -1);
	lua_pop(L, 1);
    }
}

void coords(lua_State *L, int M, svm_node *nodes, svm_prob *prob) {
    prob->x[0] = &nodes[0]; // first reference done!
    int i, j = 0;
    for (i=1; i<M; i++) {
	while (nodes[j++].index != -1)
	    ;
	prob->x[i] = &nodes[j++];
    };
}


/*
 *  data: { <label>, <idx1>,<val1>, <idx2>,<val2>, <idx3>,<val3>, ... }
 *
 *  struct svm_node { int index; double value; }
 *
 *  index -1 indicates the end of the vector,
 *  indices must be in ascending(ASC) order
 *
*/

static int nodes(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    int M = 0, N = luaL_len(L, 1) + 1;
    int i;
    for (i=1; i<N;) {
	lua_rawgeti(L, 1, i++);
	M += (luaL_len(L, -1) + 1)/2; // total # of pairs (idx,val) + one more pair
	lua_pop(L, 1);
    }

    svm_node *x = (svm_node *)lua_newuserdata(L, M*sizeof(svm_node));
    luaL_getmetatable(L, "caap.svm.nodes");
    lua_setmetatable(L, -2);

    int j, k = 0;
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

    int M = luaL_len(L, 2);

    svm_prob prob;
    prob.l = M;
    prob.y = (double *)lua_newuserdata(L, M*sizeof(double));
    prob.x = (svm_node **)lua_newuserdata(L, M*sizeof(svm_node *));

    target(L, M, &prob);
    coords(L, M, nodes, &prob);

    svm_param params;
    getParams(L, 3, &params);

    if ( !(params.gamma > 0.0) )
	params.gamma = 1.0/M;

    int i;
    if ( params.kernel_type == PRECOMPUTED )
	for (i=0; i<M; i++) {
	    if (prob.x[i][0].index != 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Error: first column must be 0:serial_number\n");
		return 2;
	    }
	}

    const char *err = svm_check_parameter(&prob, &params);
    if (err) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error: %s\n", err);
	return 2;
    }

    svm_model *model = svm_train(&prob, &params);
//    svm_node **SV = model->SV;

    // SVs
//    int nr_sv = model->l; // total #SVs
//    pushSV(L, SV, nr_sv); // SVs, #SVs, #nodes
    indices( L, model ); // push table of indices
    predict_nodes(L, model, &prob);

    svm_free_and_destroy_model( &model );
    svm_destroy_param( &params );

//    return 3;
    return 2;
}

static int nodes_crossv(lua_State *L) {
    svm_node *nodes = checknodes(L);
    luaL_checktype(L, 2, LUA_TTABLE); // y's
    luaL_checktype(L, 3, LUA_TTABLE); // params

    int M = luaL_len(L, 2);

    svm_prob prob;
    prob.l = M;
    prob.y = (double *)lua_newuserdata(L, M*sizeof(double));
    prob.x = (svm_node **)lua_newuserdata(L, M*sizeof(svm_node *));

    target(L, M, &prob);
    coords(L, M, nodes, &prob);

    svm_param params;
    getParams(L, 3, &params);

    const char *err = svm_check_parameter(&prob, &params);
    if (err) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error: %s\n", err);
	return 2;
    }

    lua_getfield(L, 3, "folds");
    int nr_fold = lua_tointeger(L, -1); // XXX assert > 2 & < M
    lua_pop(L, 1);

    double *gold = (double *)lua_newuserdata(L, M*sizeof(double));

    svm_cross_validation(&prob, &params, nr_fold, gold);

    lua_newtable(L);
    int i, correct = 0;
    for (i=0; i<M; i++)
	if (gold[i] == prob.y[i])
	    ++correct;

    lua_pushnumber(L, 100.0*correct/M);
    return 1;
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

static int nodes_len(lua_State *L) {
    svm_node *nodes = checknodes(L);
    int i, j = 0, N = luaL_checkinteger(L, 2);

    for (i=0; i<N; i++) {
	while (nodes[j++].index != -1)
		;
    }

    lua_pushinteger(L, j-N);
    return 1;
}

// // // // // // // // // // // // //
//
//  add "svm & kernel types" as upvalue to method "train"
//
static void train_upvalue(lua_State *L) {
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
}

static const struct luaL_Reg nodes_meths[] = {
    {"len", nodes_len},
    {"__tostring", nodes2string},
    {"__gc", nodes_gc},
    {NULL, NULL}
};

static const struct luaL_Reg train_meths[] = {
    {"train", nodes_train},
    {"crossv", nodes_crossv},
    {NULL, NULL}
};

static const struct luaL_Reg svm_funcs[] = {
    {"nodes", nodes},
    {NULL, NULL}
};

int luaopen_lsvm (lua_State *L) {
    luaL_newmetatable(L, "caap.svm.nodes");
    lua_pushvalue(L, -1); // copy of metatable
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, nodes_meths, 0);
    train_upvalue(L); // create upvalue for train_meths
    luaL_setfuncs(L, train_meths, 1);


    // create library
    luaL_newlib(L, svm_funcs);
    return 1;
}
