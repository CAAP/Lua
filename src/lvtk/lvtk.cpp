#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageStencilData.h>
#include <vtkImageToImageStencil.h>
#include <vtkImageImport.h>
#include <vtkImageExport.h>
#include <vtkImageShiftScale.h>
#include <vtkImageThreshold.h>
#include <vtkImageThresholdConnectivity.h>
#include <vtkImageAccumulate.h>
#include <vtkImageHistogram.h>
#include <vtkImageHistogramStatistics.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkDICOMImageReader.h>
#include <vtkNIFTIImageReader.h>
#include <vtkNIFTIImageHeader.h>
#include <vtkNIFTIImageWriter.h>
#include <vtkImageFlip.h>
#include <vtkContourFilter.h>
#include <vtkIdTypeArray.h>

#include <stdio.h>

#include <lua.hpp>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define checkimage(L, i) *(vtkSmartPointer<vtkImageData> **)luaL_checkudata(L, i, "caap.vtk.image");
#define newimage(L) (vtkSmartPointer<vtkImageData> **)lua_newuserdata(L, sizeof(vtkSmartPointer<vtkImageData> *)); luaL_getmetatable(L, "caap.vtk.image"); lua_setmetatable(L, -2);
#define checkstencil(L, i) *(vtkSmartPointer<vtkImageStencilData> **)luaL_checkudata(L, i, "caap.vtk.stencil");
#define newstencil(L) (vtkSmartPointer<vtkImageStencilData> **)lua_newuserdata(L, sizeof(vtkSmartPointer<vtkImageStencilData> *)); luaL_getmetatable(L, "caap.vtk.stencil"); lua_setmetatable(L, -2);

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

void setUpvalue(lua_State *L, vtkSmartPointer<vtkImageData> img) {
    int *dims = img->GetDimensions(); // Is Dimensions truely what I need???
    const int type = img->GetScalarType(), N = dims[0]*dims[1]*dims[2];
    const char *tname = getName(type);
    // create UpValue
    lua_newtable(L);
    lua_pushinteger(L, dims[0]);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, dims[1]);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, dims[2]);
    lua_setfield(L, -2, "z");
    lua_pushinteger(L, N);
    lua_setfield(L, -2, "N");
    lua_pushinteger(L, type);
    lua_setfield(L, -2, "vtype");
    lua_pushstring(L, tname);
    lua_setfield(L, -2, "type");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "nifti"); // NIFTI
    lua_pushfstring(L, "VTK-Image{dims:{x: %d, y: %d, z: %d}, pixelType: %s}", dims[0], dims[1], dims[2], tname);
    lua_setfield(L, -2, "asstr");
    lua_setuservalue(L, -2); // append upvalue to userdatum
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

    vtkSmartPointer<vtkImageData> **img = newimage(L);
    *img = new vtkSmartPointer<vtkImageData>( imageImport->GetOutput() );
    setUpvalue(L, **img);
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

int getAxis(const char axis) {
    switch(axis) {
	case 'x': return 0;
	case 'y': return 1;
	case 'z': return 2;
    }
}

double getnum(lua_State *L, int idx, const char *lbl) {
    lua_getfield(L, idx, lbl);
    double ret = luaL_checknumber(L, -1);
    lua_pop(L, 1);
    return ret;
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

const char* rawstr(lua_State *L, const int idx, const int k) {
    lua_rawgeti(L, idx, k);
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
//    if (N != lua_len(L, 1)) luaL_error(L, "Mismatch: dimensions given vs. table size!"); XXX

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
    const char* fname = luaL_checkstring(L, 1);

    vtkSmartPointer<vtkDICOMImageReader> reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    reader->SetDirectoryName( fname );
    reader->Update();

    vtkSmartPointer<vtkImageData> **img = newimage(L);
    *img = new vtkSmartPointer<vtkImageData>( reader->GetOutput() );
    setUpvalue(L, **img);
    return 1;
}

static int readNII(lua_State *L) {
    const char* fname = luaL_checkstring(L, 1);

    vtkSmartPointer<vtkNIFTIImageReader> reader = vtkSmartPointer<vtkNIFTIImageReader>::New();
    reader->SetFileName( fname );
    reader->Update();

//    double inter = (double)reader->GetRescaleIntercept(), slope = (double)reader->GetRescaleSlope();

    vtkSmartPointer<vtkImageData> **img = newimage(L);
/*    if (inter || (slope != 1.0)) {
	vtkSmartPointer<vtkImageShiftScale> scale = vtkSmartPointer<vtkImageShiftScale>::New();
	scale->SetInputData( reader->GetOutput() );
	scale->SetShift( inter );
	scale->SetScale( slope );
	scale->ClampOverflowOn();
	scale->Update();
	*img = new vtkSmartPointer<vtkImageData>( scale->GetOutput() );

	vtkSmartPointer<vtkNIFTIImageHeader> header = reader->GetNIFTIHeader();
	header->SetSclInter( 0.0 ); header->SetSclSlope( 1.0 );
	
    } else*/
	*img = new vtkSmartPointer<vtkImageData>( reader->GetOutput() );
    setUpvalue(L, **img);

    lua_getuservalue(L, -1);
    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "nifti");
    vtkSmartPointer<vtkNIFTIImageReader> **pr = (vtkSmartPointer<vtkNIFTIImageReader> **)lua_newuserdata(L, sizeof(vtkSmartPointer<vtkNIFTIImageReader> *));
    *pr = new vtkSmartPointer<vtkNIFTIImageReader>( reader );
    lua_setfield(L, -2, "reader");
    lua_pop(L, 1);

    return 1;
}

///// INTERFACE //////

static int tostr(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "asstr");
    return 1;
}

static int size(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "N");
    return 1;
}

///// IMAGE-specific ////////

int niftiHeader(lua_State *L, vtkSmartPointer<vtkNIFTIImageWriter> writer, vtkSmartPointer<vtkNIFTIImageHeader> header) {
	lua_getfield(L, -1, "reader");
	vtkSmartPointer<vtkNIFTIImageReader> nii = **(vtkSmartPointer<vtkNIFTIImageReader> **)lua_touserdata(L, -1);
	lua_pop(L, 1);
	header->DeepCopy( nii->GetNIFTIHeader() );
	writer->SetQFac( nii->GetQFac() );
	writer->SetTimeDimension( nii->GetTimeDimension() );
	writer->SetTimeSpacing( nii->GetTimeSpacing() );
	if (nii->GetRescaleSlope()) writer->SetRescaleSlope( nii->GetRescaleSlope() );
	if (nii->GetRescaleIntercept()) writer->SetRescaleIntercept( nii->GetRescaleIntercept() );
	writer->SetQFormMatrix( nii->GetQFormMatrix() );
	if (nii->GetSFormMatrix())
	    writer->SetSFormMatrix( nii->GetSFormMatrix() );
	else
	    writer->SetSFormMatrix( nii->GetQFormMatrix() );
	return 0;
}

static int writeNII(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;
    const char *fname = luaL_checkstring(L, 2);
    const char *desc = luaL_checkstring(L, 3);

    vtkSmartPointer<vtkNIFTIImageWriter> writer = vtkSmartPointer<vtkNIFTIImageWriter>::New();
    vtkSmartPointer<vtkNIFTIImageHeader> header = writer->GetNIFTIHeader();

    writer->SetFileName( fname );
    header->SetDescrip( desc );

    lua_getuservalue(L, 1);
    int isnii = getint(L, -1, "nifti");
    if (isnii) niftiHeader(L, writer, header);
    lua_pop(L, 1);

#if VTK_MAJOR_VERSION <= 5
    writer->SetInputConnection( img->GetProducerPort() );
#else
    writer->SetInputData( img );
#endif

    writer->Write();
    lua_pushboolean(L, 1);
    return 1;
}

static int writeVTI(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;
    const char *fname = luaL_checkstring(L, 2);

    vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
    writer->SetFileName( fname );

// REFACTOR DUPLICATE CODE FOLLOWS XXX
#if VTK_MAJOR_VERSION <= 5
    writer->SetInputConnection( img->GetProducerPort() );
#else
    writer->SetInputData( img );
#endif

    writer->Write();
    lua_pushboolean(L, 1);
    return 1;
}

// FALTA AGREGAR background-image
static int stencil(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;
    const char thre = luaL_checkstring(L, 2)[0];
    double low, upp;

    vtkSmartPointer<vtkImageToImageStencil> ste = vtkSmartPointer<vtkImageToImageStencil>::New();
    ste->SetInputData( img );
//    ste->SetBackgroundValue( 0.0 );

    switch (thre) {
	case 'L':
	case 'l': low = luaL_checknumber(L, 3); ste->ThresholdByLower( low ); break;
	case 'U':
	case 'u': upp = luaL_checknumber(L, 3); ste->ThresholdByUpper( upp ); break;
	case 'B':
	case 'b': low = luaL_checknumber(L, 3); upp = luaL_checknumber(L, 4); ste->ThresholdBetween( low, upp ); break;
    }
    ste->Update();

    vtkSmartPointer<vtkImageStencilData> **ret = newstencil(L);
    *ret = new vtkSmartPointer<vtkImageStencilData>( ste->GetOutput() );

    lua_newtable(L);
    lua_pushstring(L, "VTK-Stencil");
    lua_setfield(L, -2, "asstr");
    lua_setuservalue(L, -2); // append upvalue to userdatum

    return 1;
}

/*
static int connectedThreshold(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;

    vtkSmartPointer<vtkImageThresholdConnectivity> conn = vtkSmartPointer<vtkImageThresholdConnectivity>::New();
    conn->SetInputData( img );

    conn->SetNeighborhoodRadius();

    conn->Update();

    conn->GetOutput();

}
*/

// FALTA AGREGAR
static int threshold(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;

    const char thre = luaL_checkstring(L, 2)[0];

    double low, upp;
//    double lower = luaL_checknumber(L, 2);
//    double upper = luaL_checknumber(L, 3);

    vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
    threshold->SetInputData( img );

    threshold->ReplaceInOn(); // Replace the pixel in range with InValue
    threshold->SetInValue(0);
//    threshold->ReplaceOutOn(); // Replace the pixel out of range with OutValue
//    threshold->SetOutValue();

// Refactor this, already done previously by Stencil
    switch (thre) {
	case 'L':
	case 'l': low = luaL_checknumber(L, 3); threshold->ThresholdByLower( low ); break;
	case 'U':
	case 'u': upp = luaL_checknumber(L, 3); threshold->ThresholdByUpper( upp ); break;
	case 'B':
	case 'b': low = luaL_checknumber(L, 3); upp = luaL_checknumber(L, 4); threshold->ThresholdBetween( low, upp ); break;
    }
    threshold->Update();

    vtkSmartPointer<vtkImageData> **img2 = newimage(L);
    *img2 = new vtkSmartPointer<vtkImageData>( threshold->GetOutput() );
    lua_getuservalue(L, 1);
    lua_setuservalue(L, -2);

    return 1;
}

// XXX ADD number of bins or automatic, origing or 0, spacing or 1
static int histogram(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;
    const int N = luaL_checkinteger(L, 2);

    vtkSmartPointer<vtkImageHistogram> histo = vtkSmartPointer<vtkImageHistogram>::New();
    histo->SetInputData( img );
    histo->AutomaticBinningOn();
    histo->SetMaximumNumberOfBins( N );

    if (lua_gettop(L) == 3) {
	vtkSmartPointer<vtkImageStencilData> *ste = checkstencil(L, 3);
	histo->SetStencilData( *ste );
    }

    histo->Update();

    lua_newtable(L);

    vtkIdTypeArray *h = histo->GetHistogram();
    int k, bins = h->GetNumberOfTuples();
    double *data = (double *)lua_newuserdata(L, bins*sizeof(double));
    for (k=0; k<bins; k++) data[k] = h->GetValue(k);

    lua_setfield(L, -2, "data");
    lua_pushinteger(L, histo->GetTotal());
    lua_setfield(L, -2, "total");
    lua_pushinteger(L, histo->GetNumberOfBins());
    lua_setfield(L, -2, "bins");
    lua_pushnumber(L, histo->GetBinSpacing());
    lua_setfield(L, -2, "delta");
    lua_pushnumber(L, histo->GetBinOrigin());
    lua_setfield(L, -2, "origin");

    return 1;
}

static int accumulate(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;

    vtkSmartPointer<vtkImageAccumulate> histo = vtkSmartPointer<vtkImageAccumulate>::New();
    histo->SetInputData( img );

    if (lua_gettop(L) == 2) {
	vtkSmartPointer<vtkImageStencilData> *ste = checkstencil(L, 2);
	histo->SetStencilData( *ste );
    }
    histo->Update();

    lua_newtable(L);
    lua_pushnumber(L, histo->GetMin()[0]);
    lua_setfield(L, -2, "min");
    lua_pushnumber(L, histo->GetMax()[0]);
    lua_setfield(L, -2, "max");
    lua_pushnumber(L, histo->GetMean()[0]);
    lua_setfield(L, -2, "mean");
    lua_pushnumber(L, histo->GetStandardDeviation()[0]);
    lua_setfield(L, -2, "sd");
    lua_pushinteger(L, histo->GetVoxelCount());
    lua_setfield(L, -2, "count");

    histo->IgnoreZeroOn();
    histo->Update();
    lua_pushinteger(L, histo->GetVoxelCount());
    lua_setfield(L, -2, "nonzero");

    return 1;
}

static int minmax(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;

    double valuesRange[2] = {0.0, 0.0};
    img->GetScalarRange(valuesRange);

    lua_pushnumber(L, valuesRange[0]);
    lua_pushnumber(L, valuesRange[1]);
    return 2;
}

static int export2array(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;
    lua_getuservalue(L, 1);
    const int N = getint(L, -1, "N");
    const int type = getint(L, -1, "vtype"); //VTK-specific type

    void *data; //XXX create header file w/info for Array Metatable XXX URGENT!!!

    switch (type) {
	case VTK_DOUBLE: data = (void *)lua_newuserdata(L, N*sizeof(double)); break;
	case VTK_FLOAT: data = (void *)lua_newuserdata(L, N*sizeof(float)); break;
	case VTK_SHORT: data = (void *)lua_newuserdata(L, N*sizeof(short)); break;
	case VTK_INT: data = (void *)lua_newuserdata(L, N*sizeof(int)); break;
	case VTK_UNSIGNED_SHORT: data = (void *)lua_newuserdata(L, N*sizeof(unsigned short)); break;
	case VTK_UNSIGNED_CHAR: data = (void *)lua_newuserdata(L, N*sizeof(unsigned char)); break;
    }

    vtkSmartPointer<vtkImageExport> exporter = vtkSmartPointer<vtkImageExport>::New();
#if VTK_MAJOR_VERSION <= 5
    exporter->SetInput( img );
#else
    exporter->SetInputData( img );
#endif
    exporter->ImageLowerLeftOn(); // Whatsit??? XXX
    exporter->Update();
    exporter->Export( data ); // void*

    // create UpValue
    lua_pushvalue(L, -2); // duplicate uservalue -> upvalue to ImageData userdatum
    lua_pushfstring(L, "VTK-Array{size: %d, pixelType: %s}", N, getName(type));
    lua_setfield(L, -1, "asstr");
    lua_setuservalue(L, -2); // append upvalue to Array userdatum

    return 1;
}

// FALTA AGREGAR
static int accumulator(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;
    lua_getuservalue(L, 1);

    vtkSmartPointer<vtkImageAccumulate> imageAccumulate = vtkSmartPointer<vtkImageAccumulate>::New();

#if VTK_MAJOR_VERSION <= 5
    imageAccumulate->SetInputConnection(img->GetProducerPort());
#else
    imageAccumulate->SetInputData(img);
#endif
    imageAccumulate->SetComponentExtent(0, 255, 0, 0, 0, 0);
    imageAccumulate->SetComponentOrigin(0, 0, 0);
    imageAccumulate->SetComponentSpacing(1, 0, 0);
    imageAccumulate->Update();
}

// https://lorensen.github.io/VTKExamples/site/Cxx/Images/Flip
static int flip(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;
    char axis = luaL_checkstring(L, 2)[0];

    vtkSmartPointer<vtkImageFlip> filter = vtkSmartPointer<vtkImageFlip>::New();
    filter->SetFilteredAxis( getAxis(axis) );
// REFACTOR DUPLICATE CODE XXX
#if VTK_MAJOR_VERSION <= 5
    filter->SetInputConnection( img->GetProducerPort() );
#else
    filter->SetInputData( img );
#endif
    filter->Update();

    vtkSmartPointer<vtkImageData> **img2 = newimage(L);
    *img2 = new vtkSmartPointer<vtkImageData>( filter->GetOutput() );
    setUpvalue(L, **img2);
    return 1;
}

static int img2gc(lua_State *L) {
    vtkSmartPointer<vtkImageData> *img = checkimage(L, 1);
    if (img != NULL)
	img = NULL;
    return 0;
}

//////// STENCIL ////////

static int ste2gc(lua_State *L) {
    vtkSmartPointer<vtkImageStencilData> *img = checkstencil(L, 1);
    if (img != NULL)
	img = NULL;
    return 0;
}

/////////// LIBS ///////////////

static const struct luaL_Reg vtk_funcs[] = {
  {"fromTable", fromTable},
  {"readDicom", readDICOM},
  {"readNifti", readNII },
  {NULL, NULL}
};

static const struct luaL_Reg img_meths[] = {
  {"__tostring", tostr},
  {"__len", size},
  {"__gc", img2gc},
  {"tovti", writeVTI},
  {"tonii", writeNII},
  {"minmax", minmax},
  {"stats", accumulate},
  {"histo", histogram},
//  {"hstats", hstats},
  {"toarray", export2array},
  {"threshold", threshold},
  {"stencil", stencil},
//  {"contours", contours},
  {"flip", flip},
  {NULL, NULL}
};

static const struct luaL_Reg ste_meths[] = {
  {"__tostring", tostr},
  {"__gc", ste2gc},
  {NULL, NULL}
};

///////////////////////////// 

int luaopen_lvtk (lua_State *L) {
////////////////
  luaL_newmetatable(L, "caap.vtk.image");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, img_meths, 0);

  luaL_newmetatable(L, "caap.vtk.stencil");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, ste_meths, 0);
///////////
  luaL_newlib(L, vtk_funcs);
  return 1;
}

#ifdef __cplusplus
}
#endif

/*
static int contours(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;

    vtkSmartPointer<vtkContourFilter> contours = vtkSmartPointer<vtkContourFilter>::New();
    contours->SetInputData( img );
    contours->SetValue(0, 4000);
    contours->Update();

    vtkSmartPointer<vtkPolyData> contour = contours->GetOutput();


    vtkSmartPointer<vtkImageData> **img2 = newimage(L);
    *img2 = new vtkSmartPointer<vtkImageData>( contours->GetOutput() );
    lua_getuservalue(L, 1);
    lua_setuservalue(L, -2);

    return 1;
}
*/



/*
static int hstats(lua_State *L) {
    vtkSmartPointer<vtkImageData> img, *pi = checkimage(L, 1);
    img = *pi;

    vtkSmartPointer<vtkImageHistogramStatistics> stats = vtkSmartPointer<vtkImageHistogramStatistics>::New();
    stats->SetInputData( img );
    stats->Update();

    // view range
    double range[2];
    stats->GetAutoRange( range );

    lua_newtable(L);
    lua_pushnumber(L, stats->GetMinimum());
    lua_setfield(L, -2, "min");
    lua_pushnumber(L, stats->GetMaximum());
    lua_setfield(L, -2, "max");
    lua_pushnumber(L, stats->GetMedian());
    lua_setfield(L, -2, "median");
    lua_pushnumber(L, stats->GetStandardDeviation());
    lua_setfield(L, -2, "stdev");
    lua_pushnumber(L, range[0]); 
    lua_setfield(L, -2, "minRange");
    lua_pushnumber(L, range[1]);
    lua_setfield(L, -2, "maxRange");

    return 1;
}
*/


