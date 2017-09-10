#include "itkImage.h"
#include "itkImageIOBase.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageFileWriter.h"

#include <lua.hpp>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define seriesReader(PTYPE, DIMS) itk::ImageSeriesReader< itk::Image< PTYPE, DIMS > >::Pointer *reader = (itk::ImageSeriesReader< itk::Image< PTYPE, DIMS > >::Pointer *)lua_newuserdata(L, sizeof(itk::ImageSeriesReader< itk::Image< PTYPE, DIMS > >::Pointer)); luaL_getmetatable(L, "caap.itk.series"); lua_setmetatable(L, -2); *reader = itk::ImageSeriesReader< itk::Image< PTYPE, DIMS > >::New();

//  luaL_getmetatable(L, "caap.itk.series"); lua_setmetatable(L, -2); *reader = itk::ImageSeriesReader< itk::Image< PTYPE, DIMS > >::New();

static int newSeriesReader(lua_State *L) {

    const char *PixelType = luaL_checkstring(L, 1);


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

static int series2str(lua_State *L) {
//    cv::Mat *m = checkmat(L, 1);
    lua_pushfstring(L, "itkSeriesReader");
    return 1;
}

//////////////////////////////

static const struct luaL_Reg itk_funcs[] = {
  {"series", newSeriesReader},
  {NULL, NULL}
};

static const struct luaL_Reg series_meths[] = {
  {"__tostring", series2str},
  {NULL, NULL}
};

int luaopen_litk (lua_State *L) {
  luaL_newmetatable(L, "caap.itk.series");



  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, series_meths, 0);

  // create the library
  luaL_newlib(L, itk_funcs);
  return 1;
}

#ifdef __cplusplus
}
#endif





