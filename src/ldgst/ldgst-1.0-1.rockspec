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
	CMAKE_C_FLAGS	   = "-O2 -fPIC -Wall -pedantic",
    },
}

