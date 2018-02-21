#include "itkImage.h"
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

#ifdef __cplusplus
extern "C" {
#endif


#define regFuncs(T) {"T ## New", T ## New}, {"T ## Read", T ## Read}, {"T ## DCM", T ## DCM}
#define metaTable(T) luaL_newmetatable(L, "caap.itk.image. ## T"); lua_pushvalue(L, -1);lua_setfield(L, -1, "__index"); luaL_setfuncs(L, itk_ ## T ## _meths, 0)
#define interFuncs(T)  {"write", T ## Writer}, {"setRegions", T ## Regions}, {NULL, NULL}
#define catchMe(L) catch (itk::ExceptionObject &ex) { lua_pushnil(L); lua_pushfstring(L, "Exception found: %s\n", ex); return 2; }


#define newimg(L, IMAGE, T) (IMAGE *)lua_newuserdata(L, sizeof(IMAGE));luaL_getmetatable(L, "caap.itk.image. ## T");lua_setmetatable(L, -2)
#define checkimg(L, i, T) luaL_checkudata(L, i, "caap.itk.image. ## T")

#define newgen(L, T) (T *)lua_newuserdata(L, sizeof(T));luaL_getmetatable(L, "caap.itk.gdcm.namesGenerator");lua_setmetatable(L, -2)
#define checkgen(L, i, T) *(T *)luaL_checkudata(L, i, "caap.itk.gdcm.namesGenerator")


// GDCM

static int seriesReader(lua_State *L) {
    const char* dirname = luaL_checkstring(L, 1);
    typedef itk::GDCMSeriesFileNames NamesGeneratorType;
    NamesGeneratorType::Pointer *nameGenerator = newgen(L, NamesGeneratorType::Pointer);
    *nameGenerator = NamesGeneratorType::New();

    (*nameGenerator)->SetUseSeriesDetails( true );
    (*nameGenerator)->AddSeriesRestriction( "0008|0021" );
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

static int shortNew(lua_State *L) {
    typedef itk::Image<short, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, short);
    *img = ImageType::New();
    return 1;
}

static int shortRead(lua_State *L) {
    const char *fname = luaL_checkstring(L, 1);

    typedef itk::Image<short, 3> ImageType;
    typedef itk::ImageFileReader< ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( fname );
    reader->Update();

    ImageType::Pointer *img = newimg(L, ImageType::Pointer, short);
    *img = reader->GetOutput();
    return 1;
}

static int shortDCM(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    typedef itk::Image<short, 3> ImageType;
    typedef itk::ImageSeriesReader< ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    typedef itk::GDCMImageIO ImageIOType;
    ImageIOType::Pointer dicomIO = ImageIOType::New();

    reader->SetImageIO( dicomIO );

    try {
	reader->SetFileNames( fileNames );
	try {
	    reader->Update();
	    ImageType::Pointer *img = newimg(L, ImageType::Pointer, short);
	    *img = reader->GetOutput();
	    return 1;
	}
	catchMe(L)
    }
    catchMe(L)
}

//------------------------//

static int ushortNew(lua_State *L) {
    typedef itk::Image<unsigned short, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, ushort);
    *img = ImageType::New();
    return 1;
}

static int ushortRead(lua_State *L) {
    const char *fname = luaL_checkstring(L, 1);

    typedef itk::Image<unsigned short, 3> ImageType;
    typedef itk::ImageFileReader< ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( fname );
    reader->Update();

    ImageType::Pointer *img = newimg(L, ImageType::Pointer, ushort);
    *img = reader->GetOutput();
    return 1;
}

static int ushortDCM(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    typedef itk::Image<ushort, 3> ImageType;
    typedef itk::ImageSeriesReader< ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    typedef itk::GDCMImageIO ImageIOType;
    ImageIOType::Pointer dicomIO = ImageIOType::New();

    reader->SetImageIO( dicomIO );

    try {
	reader->SetFileNames( fileNames );
	try {
	    reader->Update();
	    ImageType::Pointer *img = newimg(L, ImageType::Pointer, ushort);
	    *img = reader->GetOutput();
	    return 1;
	}
	catchMe(L)
    }
    catchMe(L)
}

//------------------------//

static int intNew(lua_State *L) {
    typedef itk::Image<int, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, int);
    *img = ImageType::New();
    return 1;
}

static int intRead(lua_State *L) {
    const char *fname = luaL_checkstring(L, 1);

    typedef itk::Image<int, 3> ImageType;
    typedef itk::ImageFileReader< ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( fname );
    reader->Update();

    ImageType::Pointer *img = newimg(L, ImageType::Pointer, int);
    *img = reader->GetOutput();
    return 1;
}

static int intDCM(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    typedef itk::Image<int, 3> ImageType;
    typedef itk::ImageSeriesReader< ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    typedef itk::GDCMImageIO ImageIOType;
    ImageIOType::Pointer dicomIO = ImageIOType::New();

    reader->SetImageIO( dicomIO );

    try {
	reader->SetFileNames( fileNames );
	try {
	    reader->Update();
	    ImageType::Pointer *img = newimg(L, ImageType::Pointer, int);
	    *img = reader->GetOutput();
	    return 1;
	}
	catchMe(L)
    }
    catchMe(L)
}

//------------------------//

static int floatNew(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, float);
    *img = ImageType::New();
    return 1;
}

static int floatRead(lua_State *L) {
    const char *fname = luaL_checkstring(L, 1);

    typedef itk::Image<float, 3> ImageType;
    typedef itk::ImageFileReader< ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( fname );
    reader->Update();

    ImageType::Pointer *img = newimg(L, ImageType::Pointer, float);
    *img = reader->GetOutput();
    return 1;
}

static int floatDCM(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    typedef itk::Image<float, 3> ImageType;
    typedef itk::ImageSeriesReader< ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    typedef itk::GDCMImageIO ImageIOType;
    ImageIOType::Pointer dicomIO = ImageIOType::New();

    reader->SetImageIO( dicomIO );

    try {
	reader->SetFileNames( fileNames );
	try {
	    reader->Update();
	    ImageType::Pointer *img = newimg(L, ImageType::Pointer, float);
	    *img = reader->GetOutput();
	    return 1;
	}
	catchMe(L)
    }
    catchMe(L)
}

//------------------------//

static int doubleNew(lua_State *L) {
    typedef itk::Image<double, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, double);
    *img = ImageType::New();
    return 1;
}

static int doubleRead(lua_State *L) {
    const char *fname = luaL_checkstring(L, 1);

    typedef itk::Image<double, 3> ImageType;
    typedef itk::ImageFileReader< ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( fname );
    reader->Update();

    ImageType::Pointer *img = newimg(L, ImageType::Pointer, double);
    *img = reader->GetOutput();
    return 1;
}

static int doubleDCM(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    typedef itk::Image<double, 3> ImageType;
    typedef itk::ImageSeriesReader< ImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    typedef itk::GDCMImageIO ImageIOType;
    ImageIOType::Pointer dicomIO = ImageIOType::New();

    reader->SetImageIO( dicomIO );

    try {
	reader->SetFileNames( fileNames );
	try {
	    reader->Update();
	    ImageType::Pointer *img = newimg(L, ImageType::Pointer, double);
	    *img = reader->GetOutput();
	    return 1;
	}
	catchMe(L)
    }
    catchMe(L)
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

static int shortWriter(lua_State *L) {
    typedef itk::Image<short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, short);
    const char *fname = luaL_checkstring(L, 2);
    typedef itk::ImageFileWriter<ImageType> WriterType;
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( fname );
    writer->SetInput( img );
    writer->Update();
    return 1;
}

static int shortRegions(lua_State *L) {
    typedef itk::Image<short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, short);
    const int x = luaL_checkinteger(L, 2);
    const int y = luaL_checkinteger(L, 3);
    unsigned int dx = luaL_checkinteger(L, 4);
    unsigned int dy = luaL_checkinteger(L, 5);

    ImageType::IndexType start;
    ImageType::SizeType size;
    ImageType::RegionType region;
    start[0] = x; start[1] = y;
    size[0] = dx; size[2] = dy;
    region.SetIndex( start ); region.SetSize( size );

    img->SetRegions( region ); img->Allocate();
    return 1;
}

//----------------------------------//

static int ushortWriter(lua_State *L) {
    typedef itk::Image<unsigned short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, ushort);
    const char *fname = luaL_checkstring(L, 2);
    typedef itk::ImageFileWriter<ImageType> WriterType;
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( fname );
    writer->SetInput( img );
    writer->Update();
    return 1;
}

static int ushortRegions(lua_State *L) {
    typedef itk::Image<unsigned short, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, ushort);
    const int x = luaL_checkinteger(L, 2);
    const int y = luaL_checkinteger(L, 3);
    unsigned int dx = luaL_checkinteger(L, 4);
    unsigned int dy = luaL_checkinteger(L, 5);

    ImageType::IndexType start;
    ImageType::SizeType size;
    ImageType::RegionType region;
    start[0] = x; start[1] = y;
    size[0] = dx; size[2] = dy;
    region.SetIndex( start ); region.SetSize( size );

    img->SetRegions( region ); img->Allocate();
    return 1;
}

//----------------------------------//

static int intWriter(lua_State *L) {
    typedef itk::Image<int, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, int);
    const char *fname = luaL_checkstring(L, 2);
    typedef itk::ImageFileWriter<ImageType> WriterType;
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( fname );
    writer->SetInput( img );
    writer->Update();
    return 1;
}

static int intRegions(lua_State *L) {
    typedef itk::Image<int, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, int);
    const int x = luaL_checkinteger(L, 2);
    const int y = luaL_checkinteger(L, 3);
    unsigned int dx = luaL_checkinteger(L, 4);
    unsigned int dy = luaL_checkinteger(L, 5);

    ImageType::IndexType start;
    ImageType::SizeType size;
    ImageType::RegionType region;
    start[0] = x; start[1] = y;
    size[0] = dx; size[2] = dy;
    region.SetIndex( start ); region.SetSize( size );

    img->SetRegions( region ); img->Allocate();
    return 1;
}

//----------------------------------//

static int floatWriter(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, float);
    const char *fname = luaL_checkstring(L, 2);
    typedef itk::ImageFileWriter<ImageType> WriterType;
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( fname );
    writer->SetInput( img );
    writer->Update();
    return 1;
}

static int floatRegions(lua_State *L) {
    typedef itk::Image<float, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, float);
    const int x = luaL_checkinteger(L, 2);
    const int y = luaL_checkinteger(L, 3);
    unsigned int dx = luaL_checkinteger(L, 4);
    unsigned int dy = luaL_checkinteger(L, 5);

    ImageType::IndexType start;
    ImageType::SizeType size;
    ImageType::RegionType region;
    start[0] = x; start[1] = y;
    size[0] = dx; size[2] = dy;
    region.SetIndex( start ); region.SetSize( size );

    img->SetRegions( region ); img->Allocate();
    return 1;
}

//----------------------------------//

static int doubleWriter(lua_State *L) {
    typedef itk::Image<double, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, double);
    const char *fname = luaL_checkstring(L, 2);
    typedef itk::ImageFileWriter<ImageType> WriterType;
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( fname );
    writer->SetInput( img );
    writer->Update();
    return 1;
}

static int doubleRegions(lua_State *L) {
    typedef itk::Image<double, 3> ImageType;
    ImageType::Pointer img = *(ImageType::Pointer *)checkimg(L, 1, double);
    const int x = luaL_checkinteger(L, 2);
    const int y = luaL_checkinteger(L, 3);
    unsigned int dx = luaL_checkinteger(L, 4);
    unsigned int dy = luaL_checkinteger(L, 5);

    ImageType::IndexType start;
    ImageType::SizeType size;
    ImageType::RegionType region;
    start[0] = x; start[1] = y;
    size[0] = dx; size[2] = dy;
    region.SetIndex( start ); region.SetSize( size );

    img->SetRegions( region ); img->Allocate();
    return 1;
}


//////////////////////////////

static const struct luaL_Reg itk_funcs[] = {
  regFuncs(short),
  regFuncs(ushort),
  regFuncs(int),
  regFuncs(float),
  regFuncs(double),
  {"series", seriesReader},
  {"pixelType", getPixelType},
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
