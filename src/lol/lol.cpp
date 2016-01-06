#include <opencv2/opencv.hpp>

using namespace std;

#include <lua.hpp>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define checkmat(L,i) *(cv::Mat **)luaL_checkudata(L, i, "caap.opencv.mat")
#define newmat(L) (cv::Mat **)lua_newuserdata(L, sizeof(cv::Mat *)); luaL_getmetatable(L, "caap.opencv.mat"); lua_setmetatable(L, -2)
#define check81(L, m) if ( (m->depth() > CV_8S) | (m->channels() > 1) ) luaL_error(L, "Only single channel and 8-bit images.")
#define check83(L, m) if ( (m->depth() > CV_8S) | (m->channels() != 3) ) luaL_error(L, "Only 8-bit 3-channel images.")

static int openImage(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    int flag = CV_LOAD_IMAGE_ANYDEPTH;

    if (lua_gettop(L) > 1)
	    switch(luaL_checkstring(L, 2)[0]) {
		    case 'C': flag = CV_LOAD_IMAGE_COLOR; break;
		    case 'G': flag = CV_LOAD_IMAGE_GRAYSCALE; break;
		    case 'A': flag = -1; break;
	    }

    cv::Mat m = cv::imread(filename); //cv::imread(filename, flag);
    if ( m.empty() ) {
	    lua_pushnil(L);
	    return 1;
    }

    cv::Mat **um = newmat(L);
    *um = new cv::Mat(m);

    return 1;
}

/*
static int saveImage(lua_State *L) {
  cv::Mat *m = checkmat(L, 1);
  const char *filename = luaL_checkstring(L, 2);
  vector<int> params;
  cv::imwrite(filename, *m, params);
  return 0;
}
*/

static int release(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    if (m != NULL) {
	m->release();
	m=NULL;
    }
    return 0;
}

static const struct luaL_Reg cv_funcs[] = {
  {"open", openImage},
  {NULL, NULL}
};

static const struct luaL_Reg mat_meths[] = {
  {"__gc", release},
//  {"save", saveImage},
  {NULL, NULL}
};

int luaopen_lol (lua_State *L) {
  luaL_newmetatable(L, "caap.opencv.mat");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, mat_meths, 0);

  // create the library
  luaL_newlib(L, cv_funcs);
  return 1;
}

#ifdef __cplusplus
}
#endif

