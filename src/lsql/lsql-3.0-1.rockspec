package = "LSQL"
version = "3.0-1"

source = {
    url = "..."
}

description = {
    summary = "Sqlite wrapper"
}

dependencies = {
    "lua >= 5.2"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_C_COMPILER   = "/usr/bin/clang",
	CMAKE_C_FLAGS	   = "-O2 -fPIC -Wall -pedantic",
	ROCKS_LIB	   = "$HOME/Lua/lib/lua/5.3",
	LUA_INC 	   = "/usr/local/include/lua-5.3",
	SQL_LIB 	   = "/usr/local/lib",
	SQL_INC 	   = "/usr/local/include",
    },
}

