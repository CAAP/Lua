package = "LEYES"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "OpenCV find EYES method"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_CXX_FLAGS	   = "-O2 -fPIC -Wall -pedantic",
    },
}

