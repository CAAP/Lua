package = "LMPACK"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "MSGPACK - C library - wrapper"
}

dependencies = {
    "lua >= 5.3"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_C_FLAGS	   = "-O2 -fPIC -Wall -pedantic",
    },
}

