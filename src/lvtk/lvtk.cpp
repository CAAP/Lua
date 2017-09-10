#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkDICOMImageReader.h>
#include <vtkImageFlip.h>

#include <lua.hpp>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define checkimage(L, i) (vtkSmartPointer<vtkImageData>*)luaL_checkudata(L, i, "caap.vtk.image");
#define newimage(L) (vtkSmartPointer<vtkImageData>*)lua_newuserdata(L, sizeof(vtkSmartPointer<vtkImageData>)); luaL_getmetatable(L, "caap.vtk.image"); lua_setmetatable(L, -2);

///////////  AUX  ///////////////

int getType(const char *type) {
    if (!strcmp(type, "double"))
	return VTK_DOUBLE;
    if (!strcmp(type, "float"))
	return VTK_FLOAT;
    if (!strcmp(type, "int"))
	return VTK_INT;
    if (!strcmp(type, "short"))
	return VTK_SHORT;
    if (!strcmp(type, "uint"))
	return VTK_UNSIGNED_INT;
    if (!strcmp(type, "uchar"))
	return VTK_UNSIGNED_CHAR;
}

const char* getName(int type) {
    switch(type) {
	case VTK_DOUBLE: return "double";
	case VTK_FLOAT: return "float";
	case VTK_INT: return "int";
	case VTK_SHORT: return "short";
	case VTK_UNSIGNED_INT: return "uint";
	case VTK_UNSIGNED_CHAR: return "uchar";
    }
}

//https://lorensen.github.io/VTKExamples/site/Cxx/Images/ImageImport/
// Convert a c-style image to a vtkImageData
int fromArray(lua_State *L, const int x, const int y, const int z, const int type, void *cImage) {
    vtkSmartPointer<vtkImageImport> imageImport = vtkSmartPointer<vtkImageImport>::New();
    imageImport->SetDataSpacing(1, 1, 1);
    imageImport->SetDataOrigin(0, 0, 0);
    imageImport->SetWholeExtent(0, x-1, 0, y-1, 0, 0);
    imageImport->SetDataExtentToWholeExtent();

    imageImport->SetDataScalarType( type );

    imageImport->SetNumberOfScalarComponents(z);
    imageImport->SetImportVoidPointer(cImage); // void* ptr
    imageImport->Update();

    vtkSmartPointer<vtkImageData> *img = newimage(L);
    *img = imageImport->GetOutput();
    return 1;
}

static void fillArray(lua_State *L, void *array, const int N) {
    for(unsigned int k = 0; k < N; k++) {
	lua_rawgeti(L, 1, k+1);
#ifdef VTKFLOAT
	((double *) array)[k] = lua_tonumber(L, -1);
#else
	((int *) array)[k] = lua_tointeger(L, -1);
#endif
	lua_pop(L, 1);
    }
}

int getint(lua_State *L, int idx, const char *lbl) {
    lua_getfield(L, idx, lbl);
    int ret = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return ret;
}

const char* getstr(lua_State *L, const int idx, const char *lbl) {
    lua_getfield(L, idx, lbl);
    const char* ret = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    return ret;
}

////////////////////////////////////

static int fromTable(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TTABLE);

    const int x = getint(L, 2, "x");
    const int y = getint(L, 2, "y");
    const int z = getint(L, 2, "z");
    const char *ctype = getstr(L, 2, "type");

    const int N = x*y*z;
    if (N != lua_len(L, 1)) luaL_error(L, "Mismatch: dimensions given vs. table size!");

    const int type = getType(ctype);

    void *data;
    switch (type) {
#define VTKFLOAT 1
	case VTK_DOUBLE: data = (void *)lua_newuserdata(L, N*sizeof(double)); break;
	case VTK_FLOAT: data = (void *)lua_newuserdata(L, N*sizeof(float)); break;
#undef VTKFLOAT
	case VTK_SHORT: data = (void *)lua_newuserdata(L, N*sizeof(short)); break;
	case VTK_INT: data = (void *)lua_newuserdata(L, N*sizeof(int)); break;
	case VTK_UNSIGNED_SHORT: data = (void *)lua_newuserdata(L, N*sizeof(unsigned short)); break;
	case VTK_UNSIGNED_CHAR: data = (void *)lua_newuserdata(L, N*sizeof(unsigned char)); break;
    }

    return fromArray(L, x, y, z, type, data);
}

// https://lorensen.github.io/VTKExamples/site/Cxx/IO/ReadDICOMSeries
// Read all DICOM files in a specified directory.
static int readDICOM(lua_State *L) {
    vtkSmartPointer<vtkDICOMImageReader> reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    reader->SetDirectoryName();
    reader->Update();

    vtkSmartPointer<vtkImageData> *img = newimage(L);
    *img = reader->GetOutput();
    return 1;
}

///// IMAGE-specific ////////

static int writeVTI(lua_State *L) {
    vtkSmartPointer<vtkImageData> *img = checkimage(L, 1);
    const char *fname = luaL_checkstring(L, 2);

    vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
    writer->SetFileName( fname );
#if VTK_MAJOR_VERSION <= 5
    writer->SetInputConnection( img->GetProducerPort() );
#else
    writer->SetInputData( *img );
#endif
    writer->Write();
    lua_pushboolean(L, 1);
    return 1;
}

static int export2array(lua_State *L) {
    vtkSmartPointer<vtkImageData> *img = checkimage(L, 1);

/*  CHECK tHIS OUT XXX  */
    int dims[3] = {0 , 0, 0};
    img->GetDimensions( dims ); // Is Dimensions truely what I need???

    const int type = img->GetScalarType(), N = dims[0]*dims[1]*dims[2];
    void *data; //XXX create header file w/info for Array Metatable XXX URGENT!!!

    switch (type) {
	case VTK_DOUBLE: data = (void *)lua_newuserdata(L, N*sizeof(double)); break;
	case VTK_FLOAT: data = (void *)lua_newuserdata(L, N*sizeof(float)); break;
	case VTK_SHORT: data = (void *)lua_newuserdata(L, N*sizeof(short)); break;
	case VTK_INT: data = (void *)lua_newuserdata(L, N*sizeof(int)); break;
	case VTK_UNSIGNED_SHORT: data = (void *)lua_newuserdata(L, N*sizeof(unsigned short)); break;
	case VTK_UNSIGNED_CHAR: data = (void *)lua_newuserdata(L, N*sizeof(unsigned char)); break;
    }

    vtkSmartPointer<vtkImageExporter> exprter = vtkSmartPointer<vtkImageExporter>::New();
#if VTK_MAJOR_VERSION <= 5
    exporter->SetInput( *img );
#else
    exporter->SetInputData( *img );
#endif
    exporter->ImageLowerLeftOn(); // Whatsit??? XXX
    exporter->Update();
    exporter->Export( data ); // void*

    return 1;
}

int getAxis(const char axis) {
    switch(axis) {
	case 'x': return 0;
	case 'y': return 1;
	case 'z': return 2;
    }
}

// https://lorensen.github.io/VTKExamples/site/Cxx/Images/Flip
static int flip(lua_State *L) {
    vtkSmartPointer<vtkImageData> *img = checkimage(L, 1);
    char axis = luaL_checkstring(L, 2)[0];

    vtkSmartPointer<vtkImageFlip> filter = vtkSmartPointer<vtkImageFlip>::New();
    filter->SetFilteredAxis( getAxis(axis) );
#if VTK_MAJOR_VERSION <= 5
    filter->SetInputConnection( img->GetProducerPort() );
#else
    filter->SetInputData( *img );
#endif
    filter->Update();

    vtkSmartPointer<vtkImageData> *img2 = newimage(L);
    *img2 = filter->GetOutput();
    return 1;
}

static int img2str(lua_State *L) {
    vtkSmartPointer<vtkImageData> *img = checkimage(L, 1);
    int dims[3] = {0 , 0, 0}; // REPEATED CODE; should be refactored XXX
    img->GetDimensions( dims ); // Is Dimensions truely what I need???
    const int N = dims[0]*dims[1]*dims[2];
    lua_pushfstring(L, "vtkImageData{size=%d, type=%s, }", getName(N, img->GetScalarType()));
    return 1;
}

static int img2gc(lua_State *L) {
    vtkSmartPointer<vtkImageData> *img = checkimage(L, 1);
    if (img != NULL)
	img = NULL;
    return 0;
}

/////////// LIBS ///////////////

static const struct luaL_Reg vtk_funcs[] = {
  {"fromTable", fromTable},
  {"readDICOM", readDICOM},
  {NULL, NULL}
};

static const struct luaL_Reg img_meths[] = {
  {"__tostring", img2str},
  {"__gc", img2gc},
  {"tovti", writeVTI},
  {"toarray", export2array},
  {"flip", flip},
  {NULL, NULL}
};

///////////////////////////// 

int luaopen_lvtk (lua_State *L) {
////////////////
  luaL_newmetatable(L, "caap.vtk.image");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, img_meths, 0);
///////////
  luaL_newlib(L, vtk_funcs);
  return 1;
}

#ifdef __cplusplus
}
#endif
