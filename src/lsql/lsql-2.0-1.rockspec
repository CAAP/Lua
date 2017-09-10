package = "LSQL"
version = "2.0-1"

source = {
    url = "..."
}

description = {
    summary = "Sqlite wrapper"
}

dependencies = {
    "lua >= 5.2"
}

build = {
    type = "builtin",
    modules = {
	lsql = {
            sources = { "lsql.c" },
	    incdirs = {"$(SQL)/include"},
	    libdirs = {"$(SQL)/lib" },
	    libraries = { "sqlite3" },
	}
    }
}

