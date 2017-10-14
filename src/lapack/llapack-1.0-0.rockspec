package = "LLAPACK"
version = "1.0-0"

source = {
    url = "..."
}

description = {
    summary = "LAPACK wrapper"
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
    },
    modules = {
	llapack = {
	    sources = {"llapack.c"},
	    libraries = {"lapack", "lapacke"},
	}
    }
}

