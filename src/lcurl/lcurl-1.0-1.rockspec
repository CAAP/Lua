package = "LCURL"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "cURL wrapper"
}

dependencies = {
    "lua >= 5.2"
}

build = {
    type = "builtin",
    modules = {
	lcurl = {
            sources = { "lcurl.c" },
	    incdirs = { "/usr/local/include" },
	    libdirs = { "/usr/local/lib" },
	    libraries = { "curl" },
	}
    }
}

