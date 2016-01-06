#include <lua.h>
#include <lauxlib.h>
#include <sqlite3.h>
#include <string.h>
#include <ctype.h>

#define checkconn(L) *(sqlite3 **)luaL_checkudata(L, 1, "carlos.sqlite3")

#define checkstmt(L) *(sqlite3_stmt **)luaL_checkudata(L, 1, "carlos.sqlite3.statement")

static int connect (lua_State *L) {
    const char* dbname = luaL_checkstring(L, 1);

    /* create userdatum to store a sqlite3 connection object. */
    sqlite3 **ppDB = (sqlite3 **)lua_newuserdata(L, sizeof(sqlite3 *));

    /* set its metatable */
    luaL_getmetatable(L, "carlos.sqlite3");
    lua_setmetatable(L, -2);

    /* try to open the given database */
    int error = sqlite3_open_v2(dbname, ppDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (*ppDB == NULL || error != SQLITE_OK) {
	    lua_pushnil(L);
	    lua_pushfstring(L, "Error opening database \"%s\": %s\n", dbname, sqlite3_errmsg(*ppDB) );
	    return 2;
    }

    return 1;
}

static int prepareStmt (lua_State *L) {
    sqlite3 *conn = checkconn(L);
    const char *zSql = luaL_checkstring(L, 2);

    if (conn == NULL) {
	    lua_pushnil(L);
	    lua_pushstring(L, "Sqlite3 driver misuse: Prepare statement after DB closed.");
	    return 2;
    }

    /* create userdatum to store a sqlite3_stmt */
    sqlite3_stmt **ppStmt = (sqlite3_stmt **)lua_newuserdata(L, sizeof(sqlite3_stmt *));

    /* set its metatable */
    luaL_getmetatable(L, "carlos.sqlite3.statement");
    lua_setmetatable(L, -2);

    /* try to create a statement */
    int error = sqlite3_prepare_v2(conn, zSql, -1, ppStmt, NULL);
    if (error != SQLITE_OK) {
	    lua_pop(L, 1); // pop userdatum
	    lua_pushnil(L);
	    lua_pushfstring(L, "Error compiling prepared statement: %s\n", sqlite3_errmsg(conn));
	    return 2;
    }

    return 1;
}

// It returns the number of columns in the result set;
// but it returns 0 if zSql is an SQL stmt that does not return data.
static int hasTable (lua_State *L) {
    sqlite3 *conn = checkconn(L);
    const char *zSql = luaL_checkstring(L, 2);
    sqlite3_stmt *pStmt = NULL;

    int error = sqlite3_prepare_v2(conn, zSql, -1, &pStmt, NULL);
    if( error != SQLITE_OK ) {
	    lua_pushnil(L);
	    lua_pushstring(L, sqlite3_errmsg( conn ));
	    return 2;
    }

    lua_pushinteger(L, sqlite3_column_count( pStmt ) );
    sqlite3_finalize( pStmt ); pStmt = NULL;

    return 1;
}

static int exec (lua_State *L) {
    sqlite3 *conn = checkconn(L);
    const char *zSql = luaL_checkstring(L, 2);

    int error = sqlite3_exec(conn, zSql, 0, 0, 0);
    if( error ) {
	    lua_pushnil(L);
	    lua_pushstring(L, sqlite3_errmsg( conn ));
	    return 2;
    }

    lua_pushboolean(L, 1);

    return 1;
}

static void stmt_bind (lua_State *L, sqlite3_stmt *pStmt, int N);

static int import (lua_State *L) {
    sqlite3 *conn = checkconn(L);
    const char *zSql = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    int needCommit = (lua_gettop(L) > 3) && lua_toboolean(L, 4);
    sqlite3_stmt *pStmt = NULL;

    int error = sqlite3_prepare_v2(conn, zSql, -1, &pStmt, NULL);
    if( error != SQLITE_OK ) {
	    sqlite3_finalize( pStmt ); pStmt = NULL;
	    lua_pushnil(L);
	    lua_pushstring(L, sqlite3_errmsg( conn ));
	    return 2;
    }

    int nCol = sqlite3_bind_parameter_count( pStmt );
    int errors=0, k = 1, rows = luaL_len(L, 2);
    lua_newtable(L); /* error messages */

    if (needCommit) sqlite3_exec(conn, "BEGIN TRANSACTION", 0, 0, 0);
    while( k <= rows ) {
	    lua_rawgeti(L, 3, k++);
	    luaL_checktype(L, -1, LUA_TTABLE);
	    int cols = luaL_len(L, -1);
	    stmt_bind(L, pStmt, (cols<nCol) ? cols : nCol );
	    lua_pop(L, 1);
	    if (cols < nCol) { lua_pushfstring(L, "Warning: row %d: expected %d columns but found %d; filling rest with NULL.", k, nCol, cols); lua_rawseti(L, -2, ++errors); }
	    if (cols > nCol) { lua_pushfstring(L, "Warning: row %d: expected %d columns but found %d; extras ignored.", k, nCol, cols); lua_rawseti(L, -2, ++errors); }
	    while (cols < nCol) { sqlite3_bind_null( pStmt, cols++); }
	    if( sqlite3_step( pStmt ) != SQLITE_DONE ) { lua_pushfstring(L, "Error: row %d: INSERT failed: %s.", k, sqlite3_errmsg(conn)); lua_rawseti(L, -2, ++errors); }
	    // if version > 3.6.23.1 then sqlite3_reset( pStmt );
    }
    if (needCommit) sqlite3_exec(conn, "COMMIT TRANSACTION", 0, 0, 0);
    sqlite3_finalize( pStmt ); pStmt = NULL;

    return 1;
}

static void stmt_bind (lua_State *L, sqlite3_stmt *pStmt, int N) {
    int k;
    for( k = 1; k <= N; k++ ) {
	lua_rawgeti(L, -1, k);
	const char *pArg = lua_tostring(L, -1);
	while( isspace( (unsigned char)pArg[0]) ) pArg++;
	if( strlen(pArg) ) sqlite3_bind_text(pStmt, k, pArg, -1, SQLITE_TRANSIENT);
	else sqlite3_bind_null(pStmt, k);
	pArg = NULL;
	lua_pop(L, 1);
    }
}

static int stmt_insert (lua_State *L, sqlite3_stmt *pStmt) {
    int error, k = 1, N = luaL_len(L, 2);
    int nCol = sqlite3_bind_parameter_count( pStmt );
    while( k <= N ) {
	    lua_rawgeti(L, 2, k++);
	    luaL_checktype(L, -1, LUA_TTABLE);
	    int cols = luaL_len(L, -1);
	    stmt_bind(L, pStmt, (cols<nCol) ? cols : nCol );
	    lua_pop(L, 1);
	    while (cols < nCol) { sqlite3_bind_null( pStmt, cols++); }
	    error = sqlite3_step( pStmt );
	    // if version > 3.6.23.1 then sqlite3_reset( pStmt );
    	    if (error != SQLITE_DONE) {
		    return error; // ends abruptly due to error
	    }
    }
    return error;
}

static int stmt_exec (lua_State *L) {
    sqlite3_stmt *pStmt = checkstmt(L);
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_getuservalue(L, 1);
    sqlite3 *conn = *(sqlite3 **)luaL_checkudata(L, -1, "carlos.sqlite3");
    if( conn == NULL) {
	    lua_pushnil(L);
	    lua_pushstring(L, "Sqlite3 driver misuse: Attempt to call statement after DB closed.");
	    return 2;
    }

    lua_pushboolean(L, 1);

    return 1;
}

static int sqlite2string (lua_State *L){
    lua_pushstring(L, "Sqlite3{active=true}");
    return 1;
}

static int sqlite_gc (lua_State *L) {
    sqlite3 *conn = checkconn(L);
    if (conn && (sqlite3_close_v2(conn) == SQLITE_OK))
	    conn = NULL;
    return 0;
}

static int stmt_gc (lua_State *L) {
    sqlite3_stmt *pStmt = *(sqlite3_stmt **)lua_touserdata(L, 1);
    if (pStmt && (sqlite3_finalize(pStmt) == SQLITE_OK))
	    pStmt = NULL;
    return 0;
}

static int version (lua_State *L) {
    lua_pushstring(L, sqlite3_libversion());
    return 1;
}

static const struct luaL_Reg sql_funcs[] = {
    {"connect", connect},
    {"version", version},
    {NULL, NULL}
};

static const struct luaL_Reg sqlite_meths[] = {
    {"__tostring", sqlite2string},
    {"__gc", sqlite_gc},
    {"prepare", prepareStmt},
    {"hasTable", hasTable},
    {"exec", exec},
    {"import", import},
    {NULL, NULL}
};

static const struct luaL_Reg stmt_meths[] = {
    {"__gc", stmt_gc},
    {"exec", stmt_exec},
    {NULL, NULL}
};

int luaopen_lsql (lua_State *L) {
    luaL_newmetatable(L, "carlos.sqlite3");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, sqlite_meths, 0);

    luaL_newmetatable(L, "carlos.sqlite3.statement");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, stmt_meths, 0);

    // create library
    luaL_newlib(L, sql_funcs);
    return 1;
}

