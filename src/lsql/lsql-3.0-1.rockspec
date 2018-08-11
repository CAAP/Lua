package = "LSQL"
version = "3.0-1"

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
    type = "cmake",
    variables = {
	CMAKE_C_FLAGS	   = "-O2 -fPIC -Wall -pedantic",
    },
}

