package = "LXML"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "Tiny XML wrapper"
}

dependencies = {
    "lua >= 5.2"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_CXX_COMPILER   = "/usr/bin/clang++",
	CMAKE_CXX_FLAGS	   = "-O2 -fPIC -Wall -pedantic -std=c++11",
	ROCKS_LIB	   = "$ROCKS_LIB",
	LUA_INC 	   = "$LUA_INC",
    },
}

