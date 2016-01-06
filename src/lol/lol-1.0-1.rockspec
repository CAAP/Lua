package = "LOL"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "Test wrapper"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "builtin",
    modules = {
	lol = {
	    sources = {"lol.cpp"},
	    incdirs = {"$(LIBOCV)/include"},
	    libdirs = {"$(LIBOCV)/lib"},
	    libraries = {"opencv_core", "opencv_imgcodecs"}
	}
    }
}

