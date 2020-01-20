package = "LSVM"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "LibSVM wrapper"
}

dependencies = {
    "lua >= 5.2"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_C_FLAGS	   = "-O2 -fPIC -Wall -pedantic",
	CMAKE_CXX_FLAGS	   = "-O2 -fPIC -Wall -pedantic"
    },
}

