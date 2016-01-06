#include <lua.h>
#include <lauxlib.h>

#include <spssdio.h>

/* ********* CONNECTION ********* */

static int count_vars( lua_State *L ) {
    const char* filename = luaL_checkstring(L, 1);

    /* try to open the spss file */
    int fH, error, count;
    error = spssOpenRead( filename, &fH );
    if (error != SPSS_OK) { lua_pushnil(L); return 1; }

    /* get the number of variables in the file */
    error = spssGetNumberofVariables(fH, &count);
    if ( error == SPSS_OK )
	lua_pushinteger(L, count);
    else
	lua_pushnil(L);

    spssCloseRead(fH);

    return 1;
}

static int get_vars( lua_State *L ) {
    const char* filename = luaL_checkstring(L, 1);

    /* try to open the spss file */
    int fH, error;
    error = spssOpenRead( filename, &fH );
    if (error != SPSS_OK) { lua_pushnil(L); return 1; }

    /* get variable names and types */
    int i, numV, *typesV;
    char **namesV;
    error = spssGetVarNames(fH, &numV, &namesV, &typesV);
    if (error == SPSS_OK) {
	lua_newtable(L);
        for (i = 0; i < numV;) {
	    lua_pushstring(L, namesV[i] );
	    lua_rawseti(L, -2, ++i);
     	}
	spssFreeVarNames(namesV, typesV, numV);
	spssCloseRead(fH);
    } else { lua_pushnil(L); }

    spssCloseRead(fH);

    return 1;
}

static int get_labels( lua_State *L ) {
    const char* filename = luaL_checkstring(L, 1);

    /* try to open the spss file */
    int fH, error;
    error = spssOpenRead(filename, &fH);
    if (error != SPSS_OK) { lua_pushnil(L); return 1; }

    /* get variable names and types */
    int i, numV, *typesV;
    char **namesV, vLabel[121];
    error = spssGetVarNames(fH, &numV, &namesV, &typesV);
    if (error == SPSS_OK) {
	lua_newtable(L);
        for (i = 0; i < numV; i++) {
	    error = spssGetVarLabel(fH, namesV[i], vLabel);
	    if (error == SPSS_OK) { lua_pushstring(L, vLabel); lua_rawseti(L, -2, ++i); }
     	}
    } else { lua_pushnil(L); }

    spssFreeVarNames(namesV, typesV, numV);
    spssCloseRead(fH);

    return 1;
}

static int get_cases( lua_State *L ) {
    const char* filename = luaL_checkstring(L, 1);

    /* try to open the spss file */
    int fH, error;
    error = spssOpenRead(filename, &fH);
    if (error != SPSS_OK) { lua_pushnil(L); return 1; }

    /* get variable names and types */
    int i, m, numV, *typesV;
    char **namesV;
    error = spssGetVarNames(fH, &numV, &namesV, &typesV);
    if (error == SPSS_OK) {
	/* get handles */
	double *handlesV = (double *)lua_newuserdata(L, numV*sizeof(double));
	for (i=0; i < numV; i++)
	    error = spssGetVarHandle(fH, namesV[i], handlesV+i);
        /* get cases */
        long nCases; double nValue; char cValue[256];
        error = spssGetNumberofCases(fH, &nCases);
        if (error == SPSS_OK) {
	    lua_newtable(L);
            for(m = 0; m < nCases;) {
	        error = spssReadCaseRecord(fH);
	        if (error == SPSS_OK) {
		    lua_newtable(L); // individual case
		    for(i=0; i < numV; i++) {
		        if (typesV[i] == 0) {
			    error = spssGetValueNumeric(fH, handlesV[i], &nValue);
			    if (error == SPSS_OK) { lua_pushnumber(L, nValue); lua_setfield(L, -2, namesV[i]); }
   		        } else {
			    error = spssGetValueChar(fH, handlesV[i], cValue, 256);
			    if (error == SPSS_OK) { lua_pushstring(L, cValue); lua_setfield(L, -2, namesV[i]); }
		        }
		    }
		    lua_rawseti(L, -2, ++m);
	        }
            }
        } else { lua_pushnil(L); }
    } else { lua_pushnil(L); }

    spssFreeVarNames(namesV, typesV, numV);
    spssCloseRead(fH);

    return 1;
}

static int get_value( lua_State *L ) {
    const char* filename = luaL_checkstring(L, 1);
    const char* varName = luaL_checkstring(L, 2);
    double value = lua_tonumber(L, 3);

    /* try to open the spss file */
    int fH, error;
    error = spssOpenRead(filename, &fH);
    if (error != SPSS_OK) { lua_pushnil(L); return 1; }

    /* get value for label */
    char vLab[61];
    error = spssGetVarNValueLabel(fH, varName, value, vLab);
    if (error == SPSS_OK)
	lua_pushstring(L, vLab);
    else
	lua_pushnil(L);

    spssCloseRead(fH);

    return 1;
}

/* *********************************** */

static const struct luaL_Reg spss_funcs[] = {
    {"nvars", count_vars},
    {"vars", get_vars},
    {"labels", get_labels},
    {"cases", get_cases},
    {"value", get_value},
    {NULL, NULL}
};

int luaopen_lspss (lua_State *L) {
    // create library
    luaL_newlib(L, spss_funcs);
    return 1;
}
