package = "LGDCM"
version = "2.0-1"

source = {
    url = "..."
}

description = {
    summary = "GDCM wrapper"
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

