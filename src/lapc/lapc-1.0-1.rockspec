package = "LAPC"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "apCluster wrapper"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "builtin",
    modules = {
	lapc = {
	    sources = {"lapc.c"},
	    libraries = {"apcluster"},
	    incdirs = {"$(APC)"},
	    libdirs = {"$(APC)"}
	}
    }
}

