package = "LCDF"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "Cummulative Distribution Functions"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "builtin",
    modules = {
	lcdf = {
	    sources = {"dcdflib.c", "ipmpar.c", "lcdf.c"}
	}
    }
}

