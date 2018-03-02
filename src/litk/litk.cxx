#include "itkImage.h"
#include "itkVTKImageIO.h"
#include "itkSmartPointer.h"
#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"

#include <gdcm/gdcmImage.h>
#include <gdcm/gdcmImageReader.h>

#include <lua.hpp>
#include <lauxlib.h>
#include <string.h>

#define catchMe(L) catch (itk::ExceptionObject &ex) { lua_pushnil(L); lua_pushfstring(L, "Exception found: %s\n", ex); return 2; }

template<typename TImage>
void deepCopy(typename TImage::Pointer input, typename TImage::Pointer &output) {
    output->SetRegions(input->GetLargestPossibleRegion());
    output->Allocate();
    itk::ImageRegionConstIterator<TImage> inputIterator(input, input->GetLargestPossibleRegion());
    itk::ImageRegionIterator<TImage> outputIterator(output, output->GetLargestPossibleRegion());
    while(!inputIterator.IsAtEnd()) {
	outputIterator.Set( inputIterator.Get() );
	++inputIterator; ++outputIterator;
    }
}

template<typename TImage>
void readImage(typename TImage::Pointer output, const char *fname) {
    typedef itk::ImageFileReader< TImage > ReaderType;
    typename ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( fname );

    const char *ext = strrchr(fname, '.');
    if (!(strcmp(ext, ".vtk") && strcmp(ext, ".VTK"))) {
	typedef itk::VTKImageIO ImageIOType;
	typename ImageIOType::Pointer vtkIO = ImageIOType::New();
	reader->SetImageIO( vtkIO );
    }

    reader->Update();
    typename TImage::Pointer ans = reader->GetOutput();
    deepCopy<TImage>(ans, output);
}

template<typename TImage>
void readDCM(typename TImage::Pointer output, std::vector<std::string> fileNames) {
    typedef itk::ImageSeriesReader<TImage> ReaderType;
    typename ReaderType::Pointer reader = ReaderType::New();
    typedef itk::GDCMImageIO ImageIOType;
    typename ImageIOType::Pointer dicomIO = ImageIOType::New();

    reader->SetImageIO( dicomIO );
    reader->SetFileNames( fileNames );
    reader->Update();
    typename TImage::Pointer ans = reader->GetOutput();
    deepCopy<TImage>(ans, output);
}

template<typename TImage>
void writeImage(typename TImage::Pointer input, const char* fname) {
    typedef itk::ImageFileWriter<TImage> WriterType;
    typename WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( fname );

    const char *ext = strrchr(fname, '.');
    if (!(strcmp(ext, ".vtk") && strcmp(ext, ".VTK"))) {
	typedef itk::VTKImageIO ImageIOType;
	typename ImageIOType::Pointer vtkIO = ImageIOType::New();
	writer->SetImageIO( vtkIO );
    }

    writer->SetInput( input );
    writer->Update();
}

/*
template<typename TImage>
void regions(typename TImage::Pointer img, const int x, const int y, unsigned int dx, unsigned int dy) {
    typename TImage::IndexType start = {{x, y}};
    typename TImage::SizeType size = {{dx, dy}};
    typename TImage::RegionType region(start, size);
    img->SetRegions( region );
    img->Allocate();
}
*/


#ifdef __cplusplus
extern "C" {
#endif


//  {"setRegions", T ## Regions}},
#define metaTable(T) luaL_newmetatable(L, "caap.itk.image." #T); lua_pushvalue(L, -1);lua_setfield(L, -1, "__index"); luaL_setfuncs(L, itk_ ## T ## _meths, 0)
#define interFuncs(T) {"read", T ## Reader}, {"dicom", T ## Dicom}, {"write", T ## Writer, {NULL, NULL}
#define getter(Method, L, io, v, k) io->Get ## Method (v);lua_pushstring(L, v);lua_rawseti(L, -2, k++)

#define newimg(L, P, T) (P *)lua_newuserdata(L, sizeof(P));luaL_getmetatable(L, "caap.itk.image." #T);lua_setmetatable(L, -2)
#define checkimg(L, i, T) luaL_checkudata(L, i, "caap.itk.image." #T)

#define newgen(L, T) (T *)lua_newuserdata(L, sizeof(T));luaL_getmetatable(L, "caap.itk.gdcm.namesGenerator");lua_setmetatable(L, -2)
#define checkgen(L, i, T) *(T *)luaL_checkudata(L, i, "caap.itk.gdcm.namesGenerator")


// Inspect IMAGEs

static int getInfo(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(path, itk::ImageIOFactory::ReadMode);
    imageIO->SetFileName(path);
    imageIO->ReadImageInformation();

    lua_newtable(L);
    lua_pushinteger(L, imageIO->GetNumberOfDimensions());
    lua_setfield(L, -2, "dims");
    lua_pushstring(L, imageIO->GetComponentTypeAsString(imageIO->GetComponentType()).c_str());
    lua_setfield(L, -2, "type");
    lua_pushstring(L, imageIO->GetPixelTypeAsString(imageIO->GetPixelType()).c_str());
    lua_setfield(L, -2, "pixel");
    return 1;
}


// GDCM

static int seriesReader(lua_State *L) {
    const char* dirname = luaL_checkstring(L, 1);
    typedef itk::GDCMSeriesFileNames NamesGeneratorType;
    NamesGeneratorType::Pointer *nameGenerator = newgen(L, NamesGeneratorType::Pointer);
    *nameGenerator = NamesGeneratorType::New();

    (*nameGenerator)->SetUseSeriesDetails( true );
    (*nameGenerator)->AddSeriesRestriction( "0008|0021" ); // Series Date
    (*nameGenerator)->RecursiveOn();
    (*nameGenerator)->SetInputDirectory( dirname );

    return 1;
}

static int getPixelType(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);

    gdcm::ImageReader reader;
    reader.SetFileName( filename );
    if ( !reader.Read() ) {
	lua_pushnil(L); lua_pushfstring(L, "ERROR: could not read %s \n", filename); return 2;
    }
    gdcm::Image & image = reader.GetImage();
    const gdcm::PixelFormat pf = image.GetPixelFormat();

    switch (pf) {
	case gdcm::PixelFormat::INT8: lua_pushstring(L, "char"); return 1;
	case gdcm::PixelFormat::UINT8: lua_pushstring(L, "uchar"); return 1;
	case gdcm::PixelFormat::INT16: lua_pushstring(L, "short"); return 1;
	case gdcm::PixelFormat::UINT16: lua_pushstring(L, "ushort"); return 1;
	case gdcm::PixelFormat::INT32: lua_pushstring(L, "int"); return 1;
	case gdcm::PixelFormat::FLOAT32: lua_pushstring(L, "float"); return 1;
	case gdcm::PixelFormat::FLOAT64: lua_pushstring(L, "double"); return 1;
	default: lua_pushnil(L); lua_pushfstring(L, "ERROR: cannot identify pixel format for file %s.\n", filename); return 2;
    }
}


//  IMAGE
//  itk::Image
//  n-dimensional regular sampling of data
//  The pixel type is arbitrary and specified upon instantiation.
//  The dimensionality must also be specified upon instantiation.

/////// 3D Image Volume

//------------------------//

// AUXILIAR FNs

/*
// Institution, Modality, PatientAge, PatientID, PatientName, PatientSex, StudyDate, StudyDescription, StudyID, RescaleSlope, RescaleIntercept, StudyInstanceUID, SeriesInstanceUID
void getInfo() {
    lua_newtable(L);
    int k = 1;
    char *v;

    getter(Institution, L, dicomIO, v, k);
    getter(Modality, L, dicomIO, v, k);
    getter(PatientAge, L, dicomIO, v, k);
    getter(PatientID, L, dicomIO, v, k);
    getter(PatientName, L, dicomIO, v, k);
    getter(PatientSex, L, dicomIO, v, k);
    getter(StudyDate, L, dicomIO, v, k);
    getter(StudyDescription, L, dicomIO, v, k);
    getter(StudyID, L, dicomIO, v, k);
    getter(RescaleSlope, L, dicomIO, v, k);
    getter(RescaleIntercept, L, dicomIO, v, k);
    getter(StudyInstanceUID, L, dicomIO, v, k);
    getter(SeriesInstanceUID, L, dicomIO, v, k);
}
*/

static int shortNew(lua_State *L) {
    typedef itk::Image<short, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, short);
    *img = ImageType::New();
    return 1;
}

static int ushortNew(lua_State *L) {
    typedef itk::Image<unsigned short, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, ushort);
    *img = ImageType::New();
    return 1;
}

static int intNew(lua_State *L) {
    typedef itk::Image<int, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, int);
    *img = ImageType::New();
    return 1;
}

static int floatNew(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, float);
    *img = ImageType::New();
    return 1;
}

static int doubleNew(lua_State *L) {
    typedef itk::Image<double, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, double);
    *img = ImageType::New();
    return 1;
}


////////// Interface METHODs ///////////

// AUXILIAR FNs

void fromVector(lua_State *L, const std::vector<std::string> v) {
    int i = 1;
    for(auto it = v.begin(); it != v.end(); ++it) {
	lua_pushstring(L, it->c_str() );
	lua_rawseti(L, -2, i++);
    }
}

// GDCM

static int addRestriction(lua_State *L) {
    typedef itk::GDCMSeriesFileNames NamesGeneratorType;
    NamesGeneratorType::Pointer nameGenerator = checkgen(L, 1, NamesGeneratorType::Pointer);
    const char *rest = luaL_checkstring(L, 2);
    nameGenerator->AddSeriesRestriction(rest);
    lua_pushboolean(L, 1);
    return 1;
}

static int getUIDs(lua_State *L) {
    typedef itk::GDCMSeriesFileNames NamesGeneratorType;
    NamesGeneratorType::Pointer nameGenerator = checkgen(L, 1, NamesGeneratorType::Pointer);
    if (nameGenerator.IsNull()) { luaL_error(L, "ERROR: NULL pointer for class GDCMSeriesFileNames.\n"); }

    try {
	typedef std::vector< std::string > SeriesIdContainer;
	const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();
	lua_newtable(L);
	fromVector(L, seriesUID);
	return 1;
    }
    catchMe(L)

}

static int getFiles(lua_State *L) {
    typedef itk::GDCMSeriesFileNames NamesGeneratorType;
    NamesGeneratorType::Pointer nameGenerator = checkgen(L, 1, NamesGeneratorType::Pointer);
    try {
	const char* seriesID = luaL_checkstring(L, 2);
	typedef std::vector< std::string > FileNamesContainer;
	FileNamesContainer fileNames = nameGenerator->GetFileNames( seriesID );
	lua_newtable(L);
	fromVector(L, fileNames);
	return 1;
    }
    catchMe(L)
}

// 3D Image Volume

static int shortReader(lua_State *L) {
    typedef itk::Image<short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, short);
    const char *fname = luaL_checkstring(L, 2);
    try {
	readImage<ImageType>(img, fname);
	return 1;
    }
    catchMe(L)
}

static int shortDicom(lua_State *L) {
    typedef itk::Image<short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, short);

    luaL_checktype(L, 2, LUA_TTABLE);
    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    try {
	readDCM<ImageType>(img, fileNames);
	return 1;
    }
    catchMe(L)
}

static int shortWriter(lua_State *L) {
    typedef itk::Image<short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, short);
    const char *fname = luaL_checkstring(L, 2);
    try {
	writeImage<ImageType>(img, fname);
	return 1;
    }
    catchMe(L)
}

/*
static int shortRegions(lua_State *L) {
    typedef itk::Image<short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, short);
    const int x = luaL_checkinteger(L, 2);
    const int y = luaL_checkinteger(L, 3);
    unsigned int dx = luaL_checkinteger(L, 4);
    unsigned int dy = luaL_checkinteger(L, 5);
    regions<ImageType>(img, x, y, dx, dy);
    return 1;
}


*/

//----------------------------------//

static int ushortReader(lua_State *L) {
    typedef itk::Image<unsigned short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, ushort);
    const char *fname = luaL_checkstring(L, 2);
    try {
	readImage<ImageType>(img, fname);
	return 1;
    }
    catchMe(L)
}

static int ushortDicom(lua_State *L) {
    typedef itk::Image<unsigned short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, ushort);

    luaL_checktype(L, 2, LUA_TTABLE);
    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    try {
	readDCM<ImageType>(img, fileNames);
	return 1;
    }
    catchMe(L)
}

static int ushortWriter(lua_State *L) {
    typedef itk::Image<unsigned short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, ushort);
    const char *fname = luaL_checkstring(L, 2);
    try {
	writeImage<ImageType>(img, fname);
	return 1;
    }
    catchMe(L)
}

/*
static int ushortRegions(lua_State *L) {
    typedef itk::Image<unsigned short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, ushort);
    const int x = luaL_checkinteger(L, 2);
    const int y = luaL_checkinteger(L, 3);
    unsigned int dx = luaL_checkinteger(L, 4);
    unsigned int dy = luaL_checkinteger(L, 5);
    regions<ImageType>(img, x, y, dx, dy);
    return 1;
}
*/

//----------------------------------//

static int intReader(lua_State *L) {
    typedef itk::Image<int, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, int);
    const char *fname = luaL_checkstring(L, 2);
    try {
	readImage<ImageType>(img, fname);
	return 1;
    }
    catchMe(L)
}

static int intDicom(lua_State *L) {
    typedef itk::Image<int, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, int);

    luaL_checktype(L, 2, LUA_TTABLE);
    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    try {
	readDCM<ImageType>(img, fileNames);
	return 1;
    }
    catchMe(L)
}

static int intWriter(lua_State *L) {
    typedef itk::Image<int, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, int);
    const char *fname = luaL_checkstring(L, 2);
    try {
	writeImage<ImageType>(img, fname);
	return 1;
    }
    catchMe(L)
}

/*
static int intRegions(lua_State *L) {
    typedef itk::Image<int, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, int);
    const int x = luaL_checkinteger(L, 2);
    const int y = luaL_checkinteger(L, 3);
    unsigned int dx = luaL_checkinteger(L, 4);
    unsigned int dy = luaL_checkinteger(L, 5);

    regions<ImageType>(img, x, y, dx, dy);
    return 1;
}


*/

//----------------------------------//

static int floatReader(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, float);
    const char *fname = luaL_checkstring(L, 2);
    try{
	readImage<ImageType>(img, fname);
	return 1;
    }
    catchMe(L)
}

static int floatDicom(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, float);

    luaL_checktype(L, 2, LUA_TTABLE);
    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    try {
	readDCM<ImageType>(img, fileNames);
	return 1;
    }
    catchMe(L)
}

static int floatWriter(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, float);
    const char *fname = luaL_checkstring(L, 2);
    try {
	writeImage<ImageType>(img, fname);
	return 1;
    }
    catchMe(L)
}

/*
static int floatRegions(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, float);
    const int x = luaL_checkinteger(L, 2);
    const int y = luaL_checkinteger(L, 3);
    unsigned int dx = luaL_checkinteger(L, 4);
    unsigned int dy = luaL_checkinteger(L, 5);

    regions<ImageType>(img, x, y, dx, dy);
    return 1;
}


*/

//----------------------------------//

static int doubleReader(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, float);
    const char *fname = luaL_checkstring(L, 2);
    try {
	readImage<ImageType>(img, fname);
	return 1;
    }
    catchMe(L)
}

static int doubleDicom(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, float);

    luaL_checktype(L, 2, LUA_TTABLE);
    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    try {
	readDCM<ImageType>(img, fileNames);
	return 1;
    }
    catchMe(L)
}

static int doubleWriter(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, float);
    const char *fname = luaL_checkstring(L, 2);
    try {
	writeImage<ImageType>(img, fname);
	return 1;
    }
    catchMe(L)
}

/*
static int doubleRegions(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, float);
    const int x = luaL_checkinteger(L, 2);
    const int y = luaL_checkinteger(L, 3);
    unsigned int dx = luaL_checkinteger(L, 4);
    unsigned int dy = luaL_checkinteger(L, 5);

    regions<ImageType>(img, x, y, dx, dy);
    return 1;
}


*/


//////////////////////////////
#define regFunc(T) {#T "New", T ## New}

static const struct luaL_Reg itk_funcs[] = {
  regFunc(short),
  regFunc(ushort),
  regFunc(int),
  regFunc(float),
  regFunc(double),
  {"series", seriesReader},
  {"pixelType", getPixelType},
  {"info", getInfo},
  {NULL, NULL}
};

static const struct luaL_Reg gdcm_meths[] = {
  {"restrict", addRestriction},
  {"uids", getUIDs},
  {"files", getFiles},
  {NULL, NULL}
};

/* *************************** */

static const struct luaL_Reg itk_short_meths[] = { interFuncs(short) };

static const struct luaL_Reg itk_ushort_meths[] = { interFuncs(ushort) };

static const struct luaL_Reg itk_int_meths[] = { interFuncs(int) };

static const struct luaL_Reg itk_float_meths[] = { interFuncs(float) };

static const struct luaL_Reg itk_double_meths[] = { interFuncs(double) };

////////////////////////////

int luaopen_litk (lua_State *L) {
  // short
  metaTable(short);
  // unsigned short
  metaTable(ushort);
  // int
  metaTable(int);
  // float
  metaTable(float);
  // double
  metaTable(double);
  // Names Generator
  luaL_newmetatable(L, "caap.itk.gdcm.namesGenerator");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, gdcm_meths, 0);

  // create the library
  luaL_newlib(L, itk_funcs);
  return 1;
}


#ifdef __cplusplus
}
#endif
