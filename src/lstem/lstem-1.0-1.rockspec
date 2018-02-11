package = "LSTEM"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "STEMMING wrapper"
}

dependencies = {
    "lua >= 5.2"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_C_COMPILER   = "/usr/bin/clang",
	CMAKE_C_FLAGS	   = "-O2 -fPIC -Wall -pedantic",
    },
}

