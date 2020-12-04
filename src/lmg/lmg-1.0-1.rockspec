package = "LMG"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "Mongoose web API wrapper"
}

dependencies = {
    "lua >= 5.2"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_C_FLAGS	   = "-O2 -fPIC -W -Wall -pedantic -DMG_ENABLE_SSL",
    },
}

