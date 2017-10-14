package = "LITK"
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
	litk = {
	    sources = {"litk.cpp", "itkImageIOBase.cxx", "itkImageIOFactory.cxx", "itkIOCommon.cxx", "itkLightProcessObject.cxx", "itkLightObject.cxx", "itkObject.cxx", "itkObjectFactoryBase.cxx"},
	    incdirs = {"$(ITK)/include/ITK-4.13"},
--	    libdirs = {"$(ITK)/lib" },
--	    libraries = {"ITKIOImageBase-4.13", "ITKCommon-4.13", "itkdouble-conversion-4.13"},
	}
    }
}

