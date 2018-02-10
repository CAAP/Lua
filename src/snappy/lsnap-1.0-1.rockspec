package = "LSNAP"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "Snappy wrapper"
}

dependencies = {
    "lua >= 5.2"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_CXX_COMPILER = "/usr/bin/clang++",
	CMAKE_CXX_FLAGS	   = "-O2 -fPIC -Wall -pedantic -std=c++11",
    },
}

