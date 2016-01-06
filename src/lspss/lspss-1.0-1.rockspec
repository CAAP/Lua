package = "LSPSS"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "SPSS I/O Module wrapper"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "builtin",
    modules = {
	lspss = {
            sources = { "lspss.c" },
	    libraries = { "spssdio" },
	    libdirs = { "$(SPSS)/lib" },
	    incdirs = { "$(SPSS)/include" }
	}
    }
}

