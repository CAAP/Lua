package = "LSQL"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "Sqlite wrapper"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "builtin",
    modules = {
	    lsql = {
            sources = { "lsql.c" },
	    libraries = { "sqlite3" },
	    libdirs = { "$(SQLITE)/lib" },
	    incdirs = { "$(SQLITE)/include" }
	    }
    }
}

