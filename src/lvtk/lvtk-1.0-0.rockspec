package = "LVTK"
version = "1.0-0"

source = {
    url = "..."
}

description = {
    summary = "VTK wrapper"
}

dependencies = {
    "lua >= 5.2"
}

build = {
    type = "builtin",
    modules = {
	lvtk = {
	    sources = {"lvtk.cpp"},
	    incdirs = {"$(VTK)/include/vtk-8.0"},
	    libdirs = {"$(VTK)/lib" },
	    libraries = {"vtkImagingCore-8.0", "vtkIOCore-8.0", "vtkIOImage-8.0", "vtkIOXML-8.0", "vtkFiltersImaging-8.0"},
	}
    }
}

