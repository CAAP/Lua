#include <lua.h>
#include <lauxlib.h>

#include <apcluster.h>

#include <stdio.h>

/* {=====================================================================
 *    Auxiliary
 * ======================================================================} */
static double geti (lua_State *L, int i) {
  double ret;

  lua_rawgeti(L, 2, i);
  ret = lua_tointeger(L, 3); // in case of NULL returns 0
  lua_pop(L, 1);

  return ret;
}

static int seti (lua_State *L, int i, double x) {
  lua_pushnumber(L, x);
  lua_rawseti(L, -2, i);
  return 0;
}

static double getfield(lua_State *L, int index, const char *key) {
  double ret = -1.0;
  lua_getfield(L, index, key);
  if ( lua_isnumber(L, -1) )
    ret = lua_tonumber(L, -1);
  lua_pop(L, 1);
  return ret;
}
/* ================================================== */

static int cluster (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE); // table containing data

  unsigned int *j, k, m, N = luaL_len(L, 1); // number of similarities & preferences
  unsigned int *ij = (unsigned int *)lua_newuserdata(L, 2*N * sizeof(unsigned int));
  double *sij = (double *)lua_newuserdata(L, N  * sizeof(double));

  j = ij+N;

  for (k=1; k<=N; k++) {
    lua_rawgeti(L, 1, k);

    luaL_checktype(L, 2, LUA_TTABLE);
    *ij++ = (unsigned int)geti(L, 1);
    *j++ = (unsigned int)geti(L, 2);
    *sij++ = geti(L, 3);

    lua_pop(L, 1);
  }

printf("Passed second flag.\n");

  // find number of points
  m = ij[0];
  for (k=1; k<N; k++) {
    if (ij[k] > m) break;
    else m = ij[k];
  }
  m++; //ij is 0-based

printf("Passed third flag.\n");

  APOPTIONS apoptions={0};
  int *idx = (int *)lua_newuserdata(L, m * sizeof(int)); // memory for returning clusters of the data points
  double netsim = 0.0; // variable for returning the final net similarity

  apoptions.cbSize = sizeof(APOPTIONS);
  apoptions.lambda = 0.9;
  apoptions.minimum_iterations = 1;
  apoptions.converge_iterations = 200;
  apoptions.maximum_iterations = 2000;
  apoptions.nonoise = 0;
  apoptions.progress=NULL;
  apoptions.progressf=NULL;

  // check whether there are extra args
  if (lua_gettop(L) > 1 ) {
    // is table with apOptions?
    luaL_checktype(L, 2, LUA_TTABLE);
    
    double ret;
    if ( (ret = getfield(L, 2, "lambda")) > 0.0 )
	    apoptions.lambda = ret;
    if ( (ret = getfield(L, 2, "convits")) > 0.0 )
	    apoptions.converge_iterations = (int)ret;
    if ( (ret = getfield(L, 2, "maxits")) > 0.0 )
	    apoptions.maximum_iterations = (int)ret;
  }

printf("Passed fourth flag.\n");

  printf("Number of similarities & preferences: %d\n", N);

  printf("Number of data points: %d\n", m);

  printf("\tReady to call affinity propagation.\n");

  int iter = apcluster32(&sij[0], &ij[0], &ij[N], N, idx, &netsim, &apoptions); /* actual function call */

  printf("\tAPCluster has finished.\n");

  lua_newtable(L);

  if( iter>0 )
    for(k=0; k<m; k++)
      seti(L, k+1, idx[k]);

  lua_pushnumber(L, netsim);
  lua_pushinteger(L, iter); // error code: 0, else: success

  return 3;
/*
  lua_pushinteger(L, 0);
  lua_pushinteger(L, 0);
  lua_pushinteger(L, 0);
  return 3;
*/
}

/* {=====================================================================
 *    Interface
 * ======================================================================} */

static const struct luaL_Reg apc_funcs[] = {
    /* probability dists */
    {"cluster", cluster},
     {NULL, NULL}
};

int luaopen_lapc (lua_State *L) {
    // create library
    luaL_newlib(L, apc_funcs);
    return 1;
}

