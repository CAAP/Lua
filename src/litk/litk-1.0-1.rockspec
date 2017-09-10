package = "LGDCM"
version = "1.0-1"

source = {
    url = "..."
}

description = {
    summary = "ITK wrapper"
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "builtin",
    modules = {
	lgdcm = {
	    sources = {"anonymizer.cpp", "lgdcm.cpp"},
	    incdirs = {"$(GDCM)/include"},
	    libdirs = {"$(GDCM)/lib" },
	    libraries = {"gdcmCommon", "gdcmMSFF"},
	}
    }
}

