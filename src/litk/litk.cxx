#include "itkImage.h"
#include "itkVTKImageIO.h"
#include "itkSmartPointer.h"
#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"


#include <lua.hpp>
#include <lauxlib.h>
#include <string.h>


#define catchMe(L) catch (itk::ExceptionObject &ex) { lua_pushnil(L); lua_pushfstring(L, "Exception found: %s\n", ex); return 2; }


template<typename TImage>
int writeImage(lua_State *L, typename TImage::Pointer input, const char* fname) {
    typedef itk::ImageFileWriter<TImage> WriterType;
    typename WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( fname );

    const char *ext = strrchr(fname, '.');
    if (!(strcmp(ext, ".vtk") && strcmp(ext, ".VTK"))) {
	typedef itk::VTKImageIO ImageIOType;
	typename ImageIOType::Pointer vtkIO = ImageIOType::New();
	writer->SetImageIO( vtkIO );
    }

    try {
	writer->SetInput( input );
	writer->Update();
    } catchMe(L)

    lua_pushboolean(L, 1);
    return 1;
}

template<typename TScalar>
int imageIOGDCM(lua_State *L, std::vector<std::string> fileNames, const char *path) {
    typedef itk::Image<TScalar, 3> TImage;
    typedef itk::ImageSeriesReader<TImage> ReaderType;
    typename ReaderType::Pointer reader = ReaderType::New();

    typedef itk::GDCMImageIO ImageIOType;
    typename ImageIOType::Pointer dicomIO = ImageIOType::New();

    try {
	reader->SetImageIO( dicomIO );
	reader->SetFileNames( fileNames );
	reader->Update();
    } catchMe(L)

    return writeImage<TImage>(L, reader->GetOutput(), path);
}


#ifdef __cplusplus
extern "C" {
#endif


// Inspect IMAGEs

static int getInfo(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(path, itk::ImageIOFactory::ReadMode);
    imageIO->SetFileName(path);
    imageIO->ReadImageInformation();
    unsigned int k, dims = imageIO->GetNumberOfDimensions();
    itk::ImageIOBase::IOComponentType type = imageIO->GetComponentType();

    lua_newtable(L);
    lua_pushinteger(L, dims);
    lua_setfield(L, -2, "dims");
    lua_pushstring(L, imageIO->GetComponentTypeAsString(type).c_str());
    lua_setfield(L, -2, "type");
    lua_pushinteger(L, type);
    lua_setfield(L, -2, "ctype");
    lua_pushstring(L, imageIO->GetPixelTypeAsString(imageIO->GetPixelType()).c_str());
    lua_setfield(L, -2, "pixel");
    lua_pushstring(L, imageIO->GetByteOrderAsString(imageIO->GetByteOrder()).c_str());
    lua_setfield(L, -2, "order"); // Little/Big Endian
    lua_pushinteger(L, imageIO->GetImageSizeInBytes());
    lua_setfield(L, -2, "size");

    // Origin, spacing & [direction XXX]
    lua_newtable(L); // origin (-5) | (-3)
    lua_newtable(L); // spacing (-4) | (-2)
//    lua_newtable(L); // direction (-3) | (-1)
    for(k=0; k<dims;) {
	const double sp = imageIO->GetSpacing(k);
	lua_pushnumber(L, imageIO->GetOrigin(k)); // (-2)
	lua_pushnumber(L, sp); // (-1)
	lua_rawseti(L, -3, ++k);
	lua_rawseti(L, -3, k);
    }
    lua_setfield(L, -3, "spacing");
    lua_setfield(L, -2, "origin");

    return 1;
}

//  IMAGE
//  itk::Image
//  n-dimensional regular sampling of data
//  The pixel type is arbitrary and specified upon instantiation.
//  The dimensionality must also be specified upon instantiation.

/////// 3D Image Volume

// input
// (1) ComponentType [int]
// (2) FilesInSeries [table]
// (3) Filename,path [string]
static int dicomSeries(lua_State *L) {
    const char *path = luaL_checkstring(L, 3); 
    luaL_checktype(L, 2, LUA_TTABLE);
    typedef std::vector< std::string > FileNamesContainer;
    FileNamesContainer fileNames;
    int i, k = luaL_len(L, 2);
    for (i=0; i<k;) {
	lua_rawgeti(L, 2, ++i);
    	fileNames.push_back( lua_tostring(L, -1) );
	lua_pop(L, 1);
    }

    switch((itk::ImageIOBase::IOComponentType)luaL_checkinteger(L, 1)) {
	case itk::ImageIOBase::SHORT:   return imageIOGDCM<short>(L, fileNames, path); 		break;
	case itk::ImageIOBase::USHORT:  return imageIOGDCM<unsigned short>(L, fileNames, path);	break;
	case itk::ImageIOBase::INT:	return imageIOGDCM<int>(L, fileNames, path);		break;
	case itk::ImageIOBase::FLOAT:   return imageIOGDCM<float>(L, fileNames, path); 		break;
	case itk::ImageIOBase::DOUBLE:  return imageIOGDCM<double>(L, fileNames, path);		break;
	default:
	    lua_pushnil(L);
	    lua_pushstring(L, "Error: Unsupported pixel type.\n");
	    return 2;
    }
}

//------------------------//

// AUXILIAR FNs

////////// Interface METHODs ///////////

#define checkgen(L, i, T) *(T *)luaL_checkudata(L, i, "caap.itk.gdcm.namesGenerator")

// 3D Image Volume

//--------------WRITER-----------------//



//////////////////////////////

static const struct luaL_Reg itk_funcs[] = {
  {"info", getInfo},
  {"series", dicomSeries},
  {NULL, NULL}
};


////////////////////////////

int luaopen_litk (lua_State *L) {
  //  IMAGE
  luaL_newmetatable(L, "caap.itk.image");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
//  luaL_setfuncs(L, itk_funcs, 0);

  // create the library
  luaL_newlib(L, itk_funcs);
  return 1;
}


#ifdef __cplusplus
}
#endif



/*

// input
// (1) ComponentType [int]
static int creteImage(lua_State *L) {
    switch((itk::ImageIOBase)luaL_checkinteger(L, 1)) {
	case itk::ImageIOBase::SHORT:   return newImage<short>(L); 		break;
	case itk::ImageIOBase::USHORT:  return newImage<unsigned short>(L); 	break;
	case itk::ImageIOBase::INT:	return newImage<int>(L);		break;
	case itk::ImageIOBase::FLOAT:   return newImage<float>(L); 		break;
	case itk::ImageIOBase::DOUBLE:  return newImage<double>(L); 		break;
	default:
	    lua_pushnil(L);
	    lua_pushstring(L, "Error: Unsupported pixel type.\n");
	    return 2;
    }
}



template<typename TScalar>
int newImage(lua_State *L) {
    typedef itk::Image<TScalar, 3> ImageType;
    typename ImageType::Pointer PIType;
    typename PIType *img = (PIType *)lua_newuserdata(L, sizeof(PIType));
    luaL_getmetatable(L, "caap.itk.image");
    lua_setmetatable(L, -2);
    *img = ImageType::New();
    return 1;
}




template<typename TScalar>
int imageIOGDCM(lua_State *L, std::vector<std::string> fileNames) {
    typedef itk::Image<TScalar, 3> TImage;
    typedef itk::ImageSeriesReader<TImage> ReaderType;
    typedef ReaderType::Pointer PRType;
    typename PRType *rdr = (PRType *)lua_newuserdata(L, sizeof(PRType));
    luaL_getmetatable(L, "caap.itk.imageio");
    // closure
    lua_setmetatable(L, -2);
    *rdr = ReaderType::New();
    typename PRType reader = *rdr;

    typedef itk::GDCMImageIO ImageIOType;
    typename ImageIOType::Pointer dicomIO = ImageIOType::New();

    try {
	reader->SetImageIO( dicomIO );
	reader->SetFileNames( fileNames );
	reader->Update();
    } catchMe(L)

    return 1;
}

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
*/

/*
template<typename TImage>
void regions(typename TImage::Pointer img, const int x, const int y, unsigned int dx, unsigned int dy) {
    typename TImage::IndexType start = {{x, y}};
    typename TImage::SizeType size = {{dx, dy}};
    typename TImage::RegionType region(start, size);
    img->SetRegions( region );
    img->Allocate();
}

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
}


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




*/


