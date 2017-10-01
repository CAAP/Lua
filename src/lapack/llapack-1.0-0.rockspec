package = "LLAPACK"
version = "1.0-0"

source = {
    url = "..."
}

description = {
    summary = "LAPACK wrapper"
}

dependencies = {
    "lua >= 5.2"
}

build = {
    type = "builtin",
    modules = {
	llapack = {
	    sources = {"llapack.c"},
	    libraries = {"lapack", "lapacke"},
	}
    }
}

