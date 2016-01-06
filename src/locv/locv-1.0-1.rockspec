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
    type = "builtin",
    modules = {
	locv = {
	    sources = {"locv.cpp"},
	    incdirs = {"$(OPENCV)/include", "$(GDCM)/include"},
	    libdirs = {"$(OPENCV)/lib"},
	    libraries = {"opencv_core", "opencv_imgcodecs", "opencv_imgproc"}
	}
    }
}

