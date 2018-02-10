#include <itkImage.h>
#include <itkImageFileReader.h>

#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageSeriesReader.h>
#include <itkImageFileWriter.h>

#include <lua.hpp>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif


/////  LIB //////

static int image3DUShort(lua_State *L) {
    typedef unsigned short PixelType;
    const unsigned int     Dimension = 3;
    typedef itk::Image< PixelType, Dimension >  ImageType;
    typedef itk::ImageSeriesReader< ImageType > ReaderType;

    return 1;
}


///// GDCM /////

static int seriesGDCM(lua_State *L) {
    const char *dirName = luaL_checkstring(L, 1);

    typedef itk::GDCMSeriesFileNames NamesGeneratorType;
    NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();

    nameGenerator->SetUseSeriesDetails(true);
    nameGenerator->AddSeriesRestriction("0008|0021");
    nameGenerator->SetGlobalWarningDisplay(false);
    nameGenerator->SetDirectory(dirName);

    lua_newtable(L);

    try {
	typedef std::vector< std::string > SeriesIdContainer;
	const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();
	SeriesIdContainer::const_iterator seriesItr = seriesUID.begin();
	SeriesIdContainer::const_iterator seriesEnd = seriesUID.end();

	int k = 0;
	while (seriesItr != seriesEnd) {
	    lua_pushstring(L, seriesItr[k++].c_str());
	    lua_rawseti(L, -2, k);
	}
    } catch (itk::ExceptionObject &ex) { luaL_error(L, "Error reading directory: %s.", dirName); }

    return 1;
}

////////////////////7

static int readInfo(lua_State *L) {
    const char *fname = luaL_checkstring(L, 1);

    typedef itk::ImageIOBase::IOComponentType ScalarPixelType;

    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(fname, itk::ImageIOFactory::ReadMode);
    if (!imageIO) luaL_error(L, "Could not create ImageIO for: %s", fname);

    imageIO->SetFileName( fname );
    imageIO->ReadImageInformation();
    const ScalarPixelType pixelType = imageIO->GetComponentType();
    const size_t numDims = imageIO->GetNumberOfDimensions();
    
    lua_newtable(L);
    lua_pushstring(L, imageIO->GetComponentTypeAsString(pixelType).c_str());
    lua_setfield(L, -2, "pixel");
    lua_pushinteger(L, numDims);
    lua_setfield(L, -2, "dimensions");
    lua_pushinteger(L, imageIO->GetComponentSize());
    lua_setfield(L, -2, "components");
    lua_pushstring(L, imageIO->GetPixelTypeAsString(imageIO->GetPixelType()).c_str());
    lua_setfield(L, -2, "image");
    return 1;
}

/*
const unsigned int Dimension

    // userdatum storing (SERIES) image reader Pointer
    switch (PixelType) {
	case "UCHAR": seriesReader(unsigned char, Dimension); // uchar
	case "CHAR": seriesReader(signed char, Dimension); // schar
	case "UINT16": seriesReader(unsigned short, Dimension); // ushort
	case "INT16": seriesReader(signed short, Dimension); // short
	case "INT32": seriesReader(int, Dimension); // int
	case "FLOAT32": seriesReader(float, Dimension); // float
	case "FLOAT64": seriesReader(double, Dimension); // double
    };
    return 1;
}
*/

//////////////////////////////

static const struct luaL_Reg itk_funcs[] = {
  {"info", readInfo},
  {"fromDCM", seriesGDCM},
  {NULL, NULL}
};

/*
static const struct luaL_Reg series_meths[] = {
  {"__tostring", series2str},
  {NULL, NULL}
};
*/

int luaopen_litk (lua_State *L) {
  // create the library
  luaL_newlib(L, itk_funcs);
  return 1;
}

#ifdef __cplusplus
}
#endif