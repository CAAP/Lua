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
    type = "builtin",
    modules = {
	ldgst = {
	    sources = {"ldgst.c"},
	    libraries = {"crypto"},
--	    incdirs = {"$(OPENSSL)/include"},
--	    libdirs = {"$(OPENSSL)/lib"},
	}
    }
}

