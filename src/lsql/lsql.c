#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sqlite3.h>

#define checkconn(L) *(sqlite3 **)luaL_checkudata(L, 1, "caap.sqlite3.connection")
#define checkstmt(L, i) *(sqlite3_stmt **)luaL_checkudata(L, i, "caap.sqlite3.statement")
#define newStmt(L) (sqlite3_stmt **)lua_newuserdata(L, sizeof(sqlite3_stmt *));luaL_getmetatable(L, "caap.sqlite3.statement");lua_setmetatable(L, -2)

/*
static void output_html_string(const char *z) {
    int i;
    if (z==0) z = "";
    while( *Z ) {
	for(i=0; z[i]
		&& z[i] != '<'
		&& z[i] != '&'
		&& z[i] != '>'
		&& z[i] != '\"'
		&& z[i] != '\'';
	i++){}
    }
}

// Return true if string z[] has nothing but whitespace and comments
static int wstoEol(const char *z) {
    int i;
    for(i=0; z[i]; i++) {
	if( z[i]=='\n' ) return 1;
	if( IsSpace(z[i]) ) continue;
	if( z[i]=='-' && z[i+1]=='-' ) return 1;
	return 0;
    }
    return 1;
}
*/


static int statement (lua_State *L, sqlite3 *conn) {
    const char *zSql = luaL_checkstring(L, 2);

    /* create userdatum to store a sqlite3_stmt, and set its metatable */
    sqlite3_stmt **ppStmt = newStmt(L); 

    /* try to create a statement */
    int error = sqlite3_prepare_v2(conn, zSql, -1, ppStmt, 0); // pointer, thus, NULL := 0
    if (error ) { // error != SQLITE_OK := 0
	    lua_pop(L, 1); // pop userdatum
	    lua_pushnil(L);
	    lua_pushfstring(L, "Error compiling prepared statement: %s\n", sqlite3_errmsg(conn));
	    return 2;
    }

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

static int next (lua_State *L, sqlite3_stmt *pStmt) {
    if ( sqlite3_step(pStmt) == SQLITE_ROW ) {
	int nCol = sqlite3_column_count( pStmt );
	if (nCol > 0) {
	    lua_newtable(L);
	    int k;
	    for (k=0; k<nCol; k++) {
		const char *tmps = (char *)sqlite3_column_text(pStmt, k);
		if (tmps==0) tmps = "";
		if (tmps && strlen( tmps ) > 0) {
		    if (!lua_stringtonumber(L, tmps)) // if conversion succeds then push Num/Int and returns non-zero value
			lua_pushstring(L, tmps); // else push string
//		    lua_pushstring(L, tmps);
//		    if ( lua_isnumber(L, -1) ) { double x = lua_tonumber(L, -1); lua_pop(L, 1); lua_pushnumber(L, x); }
		    lua_setfield(L, -2, sqlite3_column_name(pStmt, k));
		}
	    }
	    return 1;
	}
    }
    return 0;
}

static int buildMessage (lua_State *L) {
    luaL_Buffer b;
    lua_Integer i=1, last = luaL_len(L, -1);
    luaL_buffinit(L, &b);
    for (; i <= last; i++) {
	lua_rawgeti(L, -1, i);
	luaL_addvalue( &b );
    }
    luaL_pushresult(&b);
    return 1;
}

static int insert (lua_State *L, sqlite3 *conn, sqlite3_stmt *pStmt) {
    int nCol = sqlite3_bind_parameter_count( pStmt );

    // data values is a table given as last argument
    lua_rawgeti(L, -1, 1);
    int many = lua_istable(L, -1);
    lua_pop(L, 1);

    if (many) {
	int k, errors=0, rows = luaL_len(L, -1), needCommit = sqlite3_get_autocommit( conn );
	lua_newtable(L); /* error messages TABLE */
	if (needCommit) sqlite3_exec(conn, "BEGIN", 0, 0, 0);
	for( k = 1; k <= rows; k++ ) {
	    lua_rawgeti(L, -2, k);
	    luaL_checktype(L, -1, LUA_TTABLE);
	    int cols = luaL_len(L, -1);
	    if (cols != nCol) { lua_pop(L, 1); lua_pushfstring(L, "\tWarning: row %d: expected %d columns but found %d.\n", k, nCol, cols); lua_rawseti(L, -2, ++errors); continue; }
	    stmt_bind( L, pStmt, cols ); sqlite3_step( pStmt ); lua_pop(L, 1);
	    if( sqlite3_reset( pStmt) ) { lua_pushfstring(L, "\nError: row %d: INSERT failed: %s.", k, sqlite3_errmsg(conn)); lua_rawseti(L, -2, ++errors); }
	    // if version > 3.6.23.1 then sqlite3_reset( pStmt );
	}
    	if (needCommit) sqlite3_exec(conn, "COMMIT", 0, 0, 0);
	buildMessage(L);
    } else {
	int cols = luaL_len(L, -1);
    	if (cols != nCol) { lua_pushnil(L); lua_pushfstring(L, "Warning: expected %d columns but found %d.", nCol, cols); return 2; }
    	stmt_bind( L, pStmt, cols ); sqlite3_step( pStmt );
    	if( sqlite3_reset( pStmt) ) { lua_pushnil(L); lua_pushfstring(L, "Error: INSERT failed: %s.", sqlite3_errmsg(conn)); return 2; }
    	lua_pushboolean(L, 1);
    }

    return 1;
}

static int version (lua_State *L) {
    lua_pushstring(L, sqlite3_libversion());
    return 1;
}

/* ********* CONNECTION ********* */

static int connect (lua_State *L) {
    const char* dbname = luaL_checkstring(L, 1);

    /* create userdatum to store a sqlite3 connection object. */
    sqlite3 **ppDB = (sqlite3 **)lua_newuserdata(L, sizeof(sqlite3 *));

    /* set its metatable */
    luaL_getmetatable(L, "caap.sqlite3.connection");
    lua_setmetatable(L, -2);

    /* try to open the given database */
    // sqlite3_open_v2(dbname, ppDB, SQLITE_OPEN_READONLY, 0);
    // sqlite3_open(dbname, ppDB);
    int error = sqlite3_open_v2(dbname, ppDB, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, 0);
    if (*ppDB == NULL || error ) {
	    lua_pushnil(L);
	    lua_pushfstring(L, "Error opening database \"%s\": %s\n", dbname, sqlite3_errmsg(*ppDB) );
	    return 2;
    }

    return 1;
}

static int inmemory (lua_State *L) {
    /* create userdatum to store a sqlite3 connection object. */
    sqlite3 **ppDB = (sqlite3 **)lua_newuserdata(L, sizeof(sqlite3 *));

    /* set its metatable */
    luaL_getmetatable(L, "caap.sqlite3.connection");
    lua_setmetatable(L, -2);

    /* try to open the given database */
    int error = sqlite3_open(":memory:", ppDB);
    if (*ppDB == NULL || error ) {
	    lua_pushnil(L);
	    lua_pushfstring(L, "Error opening in-memory database: %s\n", sqlite3_errmsg(*ppDB) );
	    return 2;
    }

    return 1;
}

static int temporary (lua_State *L) {
    /* create userdatum to store a sqlite3 connection object. */
    sqlite3 **ppDB = (sqlite3 **)lua_newuserdata(L, sizeof(sqlite3 *));

    /* set its metatable */
    luaL_getmetatable(L, "caap.sqlite3.connection");
    lua_setmetatable(L, -2);

    /* try to open the given database */
    int error = sqlite3_open("", ppDB);
    if (*ppDB == NULL || error ) {
	    lua_pushnil(L);
	    lua_pushfstring(L, "Error opening a temporary database: %s\n", sqlite3_errmsg(*ppDB) );
	    return 2;
    }

    return 1;
}

/******************************************/

static int prepareStmt (lua_State *L) {
    sqlite3 *conn = checkconn(L);
    return statement(L, conn);
}

static int exec (lua_State *L) {
    sqlite3 *conn = checkconn(L);
    sqlite3_stmt *pStmt = checkstmt(L, 2);

    sqlite3_step( pStmt );
    int error = sqlite3_reset( pStmt );
    if( error ) {
	    lua_pushnil(L);
	    lua_pushstring(L, sqlite3_errmsg( conn ));
	    return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int import (lua_State *L) {
    sqlite3 *conn = checkconn(L);
    sqlite3_stmt *pStmt = checkstmt(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    return insert(L, conn, pStmt);
}

/*
    int needCommit = sqlite3_get_autocommit( conn );
    int nCol = sqlite3_bind_parameter_count( pStmt );
    int errors=0, rows = luaL_len(L, 3);
    lua_newtable(L);

    if (needCommit) sqlite3_exec(conn, "BEGIN", 0, 0, 0);
    int k;
    for( k = 1; k <= rows; k++ ) {
	    lua_rawgeti(L, 3, k);
	    luaL_checktype(L, -1, LUA_TTABLE);
	    int cols = luaL_len(L, -1);
printf("Number of cols: %d\n", cols);
return 0;
	    stmt_bind(L, pStmt, (cols<nCol) ? cols : nCol );
	    lua_pop(L, 1);
	    if (cols < nCol) { lua_pushfstring(L, "Warning: row %d: expected %d columns but found %d; filling rest with NULL.", k, nCol, cols); lua_rawseti(L, -2, ++errors); }
	    if (cols > nCol) { lua_pushfstring(L, "Warning: row %d: expected %d columns but found %d; extras ignored.", k, nCol, cols); lua_rawseti(L, -2, ++errors); }
	    while (cols < nCol) { sqlite3_bind_null(pStmt, cols++); }
	    sqlite3_step( pStmt );
	    if( sqlite3_reset( pStmt) ) { lua_pushfstring(L, "Error: row %d: INSERT failed: %s.", k, sqlite3_errmsg(conn)); lua_rawseti(L, -2, ++errors); }
	    // if version > 3.6.23.1 then sqlite3_reset( pStmt );
    }
    if (needCommit) sqlite3_exec(conn, "COMMIT", 0, 0, 0);

    return 1;
}
*/

/* ********** BACKUP *********** */

static int stepforth(lua_State *L) {
    sqlite3_backup *pBackup = *(sqlite3_backup **)lua_touserdata(L, lua_upvalueindex(2));
    const int steps = lua_tointeger(L, lua_upvalueindex(3));

    int rc;
    do {
	rc = sqlite3_backup_step(pBackup, steps);
	if (SQLITE_OK==rc) return sqlite3_backup_remaining(pBackup);
	if (SQLITE_DONE==rc) return 0;
	sqlite3_sleep(250);
    } while( 1 );
}

static int newBackup(lua_State *L) {
    sqlite3 *connInMem = checkconn(L);
    const char *dbname = luaL_checkstring(L, 2);
    luaL_checkinteger(L, 3);

    /*************** CONNECTION ********************/
    /* create userdatum to store a sqlite3 connection object. */
    sqlite3 **ppDB = (sqlite3 **)lua_newuserdata(L, sizeof(sqlite3 *));
    /* set its metatable */
    luaL_getmetatable(L, "caap.sqlite3.connection");
    lua_setmetatable(L, -2);
    /* try to open the given database */
    int error = sqlite3_open(dbname, ppDB);
    if (*ppDB == NULL || error ) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error opening database \"%s\": %s\n", dbname, sqlite3_errmsg(*ppDB) );
	return 2;
    }

    /*************** BACKUP *******************/
    /* create userdatum to store a sqlite3_backup object. */
    sqlite3_backup **ppBackup = (sqlite3_backup **)lua_newuserdata(L, sizeof(sqlite3_backup *));
    /* set its metatable */
    luaL_getmetatable(L, "caap.sqlite3.backup"); // ADD SUPPORT for this USERDATA XXX
    lua_setmetatable(L, -2);
    /* try to initialize the backup process */
    *ppBackup = sqlite3_backup_init(*ppDB, "main", connInMem, "main");
    if (*ppBackup == NULL) {
	lua_pushnil(L);
	lua_pushfstring(L, "Error initializing backup process \"%s\"\n", dbname);
	return 2;
    }

    lua_pushcclosure(L, &stepforth, 3); // steps, conn & backup
    return 1;
}

/* ******* SINK ******** */

static int onestep(lua_State *L) {
    sqlite3 *conn = checkconn(L);
    sqlite3_stmt *pStmt = *(sqlite3_stmt **)lua_touserdata(L, lua_upvalueindex(1));
    luaL_checktype(L, 2, LUA_TTABLE);
    return insert(L, conn, pStmt);
}

static int newSink(lua_State *L) {
    sqlite3 *conn = checkconn(L);
    luaL_checkstring(L, 2);
    statement(L, conn); // stmt
    lua_pushcclosure(L, &onestep, 1); // stmt
    return 1;
}

/* *********************************** */

/* ******* RESULT-SET ********** */

static int iter (lua_State *L) {
    sqlite3_stmt *pStmt = *(sqlite3_stmt **)lua_touserdata(L, lua_upvalueindex(1));
    int cnt = lua_tointeger(L, 2);
    lua_pushinteger(L, ++cnt); // increment counter
    if (next(L, pStmt)) { return 2; } // push result table
    lua_pop(L, 1); // pop counter
    return 0;
}

static int newIter (lua_State *L) {
    sqlite3 *conn = checkconn(L);
    luaL_checkstring(L, 2);
    statement(L, conn); // stmt
    lua_pushcclosure(L, &iter, 1); // stmt
    return 1;
}

/* ***************************** */

static int conn2string (lua_State *L){
    const int mutex = sqlite3_threadsafe();
    lua_pushfstring(L, "Sqlite3{active=true, mutex=%d}", mutex);
    return 1;
}

static int conn_gc (lua_State *L) {
    sqlite3 *conn = checkconn(L);
    if (conn && (sqlite3_close_v2(conn) == SQLITE_OK))
	    conn = NULL;
    return 0;
}

/* ***************************** */

/* ********* STATEMENT *******/

static int stmt2string (lua_State *L) {
    sqlite3_stmt *pStmt = checkstmt(L, 1);
    lua_pushstring(L, sqlite3_sql( pStmt ));
    return 1;
}

// It returns the number of columns in the result set;
// but it returns 0 if zSql is an SQL stmt that does not return data.
static int stmt_count (lua_State *L) {
    sqlite3_stmt *pStmt = checkstmt(L, 1);
    lua_pushinteger(L, sqlite3_column_count( pStmt ) );
    return 1;
}

static int stmt_gc (lua_State *L) {
    sqlite3_stmt *pStmt = checkstmt(L, 1);
    if (pStmt && (sqlite3_finalize(pStmt) == SQLITE_OK))
	    pStmt = NULL;
    return 0;
}

/* ***************************** */

/***********  BACKUP  ***********/

static int backup_gc (lua_State *L) {
    sqlite3_backup *pBackup = *(sqlite3_backup **)luaL_checkudata(L, 1, "caap.sqlite3.backup");
    if (pBackup && (sqlite3_backup_finish(pBackup) == SQLITE_OK))
	    pBackup = NULL;
    return 0;
}

/* **************************** */


/*
static int bind (lua_State *L) {
    sqlite3_stmt *pStmt = checkstmt(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_pushvalue(L, 2);
    int nCol = sqlite3_bind_parameter_count( pStmt );
    int cols = luaL_len(L, 2);
    stmt_bind(L, pStmt, (cols<nCol) ? cols : nCol );
    lua_pop(L, 1);

    while (cols < nCol) { sqlite3_bind_null(pStmt, cols++); }
    if (cols < nCol) { lua_pushfstring(L, "Warning: expected %d columns but found %d; filling rest with NULL.", nCol, cols); }
    else if (cols > nCol) { lua_pushfstring(L, "Warning: expected %d columns but found %d; extras ignored.", nCol, cols); }
    else { sqlite3_step( pStmt ); lua_pushboolean(L, !sqlite3_reset(pStmt) ); }

    return 1;
}
*/

static const struct luaL_Reg sql_funcs[] = {
    {"connect", connect},
    {"inmemory", inmemory},
    {"temp", temporary},
    {"backup", newBackup},
    {"version", version},
    {NULL, NULL}
};

static const struct luaL_Reg conn_meths[] = {
    {"__tostring", conn2string},
    {"__gc", conn_gc},
    {"prepare", prepareStmt},
    {"exec", exec},
    {"import", import},
    {"rows", newIter},
    {"sink", newSink},
    {NULL, NULL}
};

static const struct luaL_Reg stmt_meths[] = {
    {"__gc", stmt_gc},
    {"__tostring", stmt2string},
    {"__len", stmt_count},
    {NULL, NULL}
};

static const struct luaL_Reg backup_meths[] = {
    {"__gc", backup_gc},
    {NULL, NULL}
};

int luaopen_lsql (lua_State *L) {
    luaL_newmetatable(L, "caap.sqlite3.connection");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, conn_meths, 0);

    luaL_newmetatable(L, "caap.sqlite3.backup");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, backup_meths, 0);

    luaL_newmetatable(L, "caap.sqlite3.statement");
    lua_pushvalue(L, -1);
    lua_setfield(L, -1, "__index");
    luaL_setfuncs(L, stmt_meths, 0);

    // create library
    luaL_newlib(L, sql_funcs);
    return 1;
}
