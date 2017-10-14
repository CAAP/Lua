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
	LUA_LIB		   = "/home/carlos/Lua/lib",
	LUA_INC 	   = "/home/carlos/Lua/include",
	SQL_LIB 	   = "$(SQL)/lib",
	SQL_INC 	   = "$(SQL)/include",
    },
}

