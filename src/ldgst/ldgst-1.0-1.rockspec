package = "LDGST"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "OpenSSL - digests - wrapper"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_C_COMPILER   = "/usr/bin/clang",
	CMAKE_C_FLAGS	   = "-O2 -fPIC -Wall -pedantic",
	LUA_LIB		   = "/home/carlos/Lua/lib",
	LUA_INC 	   = "/home/carlos/Lua/include",
    },
}

