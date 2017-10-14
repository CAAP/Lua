package = "LOCV"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "OpenCV wrapper"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "cmake",
    variables = {
	CMAKE_CXX_COMPILER   = "/usr/bin/clang++",
	CMAKE_CXX_FLAGS	   = "-O2 -fPIC -Wall -pedantic",
	CMAKE_LINKER	   = "/usr/bin/ld",
	LUA_LIB		   = "/home/carlos/Lua/lib",
	LUA_INC 	   = "/home/carlos/Lua/include",
	OCV_LIB 	   = "$(OCV)/lib",
	OCV_INC 	   = "$(OCV)/include",
	DCM_INC		   = "$(GDCM)/include"
    },
}

