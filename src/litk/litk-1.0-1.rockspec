package = "LITK"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "ITK library wrapper"
}

dependencies = {
    "lua >= 5.3"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_CXX_FLAGS	     = "-O2 -fPIC -Wall -pedantic",
    },
}

