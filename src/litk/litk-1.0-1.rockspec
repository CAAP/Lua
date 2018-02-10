package = "LITK"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "ITK wrapper"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_CXX_COMPILER   = "/usr/bin/clang++",
	CMAKE_CXX_FLAGS	   = "-O2 -fPIC -Wall -pedantic",
    },
}

