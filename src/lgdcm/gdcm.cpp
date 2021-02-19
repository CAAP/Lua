#include <gdcmImageReader.h>
#include <gdcmImage.h>
#include <gdcmScanner.h>
#include <gdcmStringFilter.h>

#include <lua.hpp>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define checkimage(L,i) (char *)luaL_checkudata(L, i, "caap.gdcm.image")

static const char* pixel2str(int pf) {
    switch (pf) {
	    case gdcm::PixelFormat::UINT8: return "UINT8"; // uchar
	    case gdcm::PixelFormat::INT8: return "INT8"; // schar
	    case gdcm::PixelFormat::UINT16: return "UINT16"; // ushort
	    case gdcm::PixelFormat::INT16: return "INT16"; // short
	    case gdcm::PixelFormat::INT32: return "INT32"; // int
	    case gdcm::PixelFormat::FLOAT32: return "FLOAT32"; // float
	    case gdcm::PixelFormat::FLOAT64: return "FLOAT64"; // double
	default: return NULL;
    }
}

static void gettag(lua_State *L, unsigned int *hexs) {
	int j;
	luaL_checktype(L, -1, LUA_TTABLE);
	if (luaL_len(L, -1) < 2)
		luaL_error(L, "A Tag needs two numbers.");
	for (j=0; j<2;) {
		lua_rawgeti(L, -1, j+1);
		hexs[j++] = lua_tounsigned(L, -1);
		lua_pop(L, 1);
	}
}

static int getHeader(lua_State *L, const char *filename, gdcm::Tag *tags, int N) {
    gdcm::Reader reader;
    reader.SetFileName( filename );
    if( !reader.Read() )
	    luaL_error(L, "Fail to read file.");
    const gdcm::DataSet & ds = reader.GetFile().GetDataSet();
    const gdcm::FileMetaInformation & header = reader.GetFile().GetHeader();
    gdcm::StringFilter sf;
    sf.SetFile( reader.GetFile() );

    // create result table
    lua_newtable(L);
    lua_pushstring(L, filename); // add filename
    lua_rawseti(L, -2, 1);
    int k, w = 0;
    for (k=0; k < N;) {
	    gdcm::Tag tag( tags[k++] );
	    std::string s = "";
	    if ( tag.GetGroup() == 0x2 && header.FindDataElement( tag ))
		    s = sf.ToString( header.GetDataElement( tag ).GetTag() );
	    else if ( ds.FindDataElement( tag ) )
		    s = sf.ToString( ds.GetDataElement( tag ).GetTag() );
	    else
		    w++; // missing value
	    lua_pushstring(L, s.c_str());
	    if ( lua_isnumber(L, -1) == 1 ) {
		    lua_pushnumber(L, lua_tonumber(L, -1));
		    lua_remove(L, -2);
	    }
	    lua_rawseti(L, -2, k+1);
    }

    return w;
}

static void getIntercept(lua_State *L, const char *filename) {
	gdcm::Tag intercept(0x0028, 0x1052);
	std::stringstream strm;
	gdcm::Reader reader;
	reader.SetFileName( filename );
	reader.Read();
	gdcm::File &file = reader.GetFile();
	gdcm::DataSet &ds = file.GetDataSet();
	strm.str("");
	if ( ds.FindDataElement( intercept ) ) {
		ds.GetDataElement( intercept ).GetValue().Print( strm );
	}
	lua_pushstring(L, strm.str().c_str());
	lua_pushnumber(L, lua_tonumber(L, -1));
	lua_remove(L, -2);
	lua_setfield(L, -2, "intercept");
}

static int imageReader(lua_State *L){
    const char *filename = luaL_checkstring(L, 1);

    gdcm::ImageReader reader;
    reader.SetFileName( filename );
    if ( !reader.Read() ) {
	    lua_pushnil(L);
	    return 1;
    }
    gdcm::Image & image = reader.GetImage();

    const unsigned int* dimension = image.GetDimensions();
    const double* spacing = image.GetSpacing();
    const double intercept = image.GetIntercept();
    const unsigned int dims = image.GetNumberOfDimensions();
    const gdcm::PixelFormat pf = image.GetPixelFormat();
    const int stype = pf.GetScalarType();
    const unsigned long length = image.GetBufferLength();
    

    // userdatum storing the (GDCM) image data
    char *udata = (char *)lua_newuserdata(L, (size_t)length);
    luaL_getmetatable(L, "caap.gdcm.image");
    lua_setmetatable(L, -2);
    // store only the image buffer
    image.GetBuffer(udata);
    // create upvalue
    lua_newtable(L);
    lua_pushvalue(L, 1);
    lua_setfield(L, -2, "filename");
    lua_pushinteger(L, length);
    lua_setfield(L, -2, "length");
    lua_pushinteger(L, dimension[0]);
    lua_setfield(L, -2, "dimX"); 
    lua_pushinteger(L, dimension[1]);
    lua_setfield(L, -2, "dimY");
    lua_pushinteger(L, dims);
    lua_setfield(L, -2, "dimensions");
    getIntercept(L, filename);
    lua_pushnumber(L, spacing[0]);
    lua_setfield(L, -2, "deltaX");
    lua_pushnumber(L, spacing[1]);
    lua_setfield(L, -2, "deltaY");
    lua_pushinteger(L, stype);
    lua_setfield(L, -2, "type");
//    lua_pushinteger(L, pixel2mat(stype));
//    lua_setfield(L, -2, "cvtype");
    lua_pushfstring(L, "GDCM-Image{file: %s, dims(%d) [x: %d, y: %d], spacing [x: %f, y: %f], pixel: %s}", filename, dims, dimension[0], dimension[1], spacing[0], spacing[1], pixel2str(stype));
    lua_setfield(L, -2, "asstring");
    lua_setuservalue(L, -2);
    return 1;
}

/*
static int addTag( Tag const & t ) {
    static const Global &g = GlobalInstance;
    static const Dicts &dicts = g.GetDicts();
    const DictEntry &entry = dicts.GetDictEntry( t );
    if( entry.GetVR() & VR::VRASCII ) { Tags.insert( t );
    if( entry.GetVR() & VR::VRBINARY ) { Tags.insert( t ) } ;
}
*/

static int readImageHeader(lua_State *L) {
    luaL_checktype(L, 2, LUA_TTABLE);

    int k, N = luaL_len(L, 2);
    unsigned int j[2];
    gdcm::Tag tags[N];
    for (k=0; k < N; k++) {
	    lua_rawgeti(L, 2, k+1);
	    gettag(L, j);
	    lua_pop(L, 1);
	    tags[k] = gdcm::Tag(j[0], j[1]);
    }

    lua_pushinteger(L, getHeader(L, luaL_checkstring(L, 1), tags, N)); // number of missing values

    return 2;
}

/*
static int newIter (lua_State *L) {
    gdcm::Scanner scn;
    gdcm::Directory d;
    const char* dirname = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE); // table containing dicom tags

    d.Load( dirname, true ); // Load files from directory; recursively

    int k, N = luaL_len(L, 2); // N is the number of tags
    unsigned int j[2];
    gdcm::Tag tags[N];
    for (k=0; k < N; k++) {
	    lua_rawgeti(L, 2, k+1);
	    gettag(L, j);
	    lua_pop(L, 1);
	    tags[k] = gdcm::Tag(j[0], j[1]);
	    scn.AddTag( tags[k] );
    }

    if (scn.Scan( d.GetFilenames() )) {
	    lua_newtable(L);
	    int m = 1; // Number of files
	    gdcm::Directory::FilenamesType files = scn.GetKeys();
	    for (gdcm::Directory::FilenamesType::const_iterator file = files.begin(); file != files.end(); ++file) {
		    lua_newtable(L);
		    const char *fname = file->c_str();
		    lua_pushstring(L, fname);
		    lua_rawseti(L, -2, 1);
		    for (k=0; k<N; k++) {
			    lua_pushstring(L, scn.GetValue(fname, tags[k]));
			    if (lua_isnumber(L, -1) == 1) {
				    lua_pushnumber(L, lua_tonumber(L, -1));
				    lua_remove(L, -2);
			    }
			    if (lua_isnoneornil(L, -1) == 1) {
				    lua_pop(L, 1);
				    lua_pushstring(L, "");
			    }
			    lua_rawseti(L, -2, k+2);
		    }
		    lua_rawseti(L, -2, m++); 
	    }
    } else
	    luaL_error(L, "Unable to read directory.");

    return 1;
}
*/

static int scanner(lua_State *L) {
    gdcm::Scanner scn;
    gdcm::Directory d;
    const char* dirname = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE); // table containing dicom tags
    bool recurse = (1 == lua_toboolean(L, 3)); // whether recursion is performed

    d.Load( dirname, recurse ); // Load files from directory; recursively

    int k, N = luaL_len(L, 2); // N is the number of tags
    unsigned int j[2];
    gdcm::Tag tags[N];
    for (k=0; k < N; k++) {
	    lua_rawgeti(L, 2, k+1);
	    gettag(L, j);
	    lua_pop(L, 1);
	    tags[k] = gdcm::Tag(j[0], j[1]);
	    scn.AddTag( tags[k] );
    }

    if (!scn.Scan( d.GetFilenames() )) {lua_pushnil(L); lua_pushfstring(L, "Unable to scan directory: %s.\n", dirname); return 2;}

    gdcm::Directory::FilenamesType files = scn.GetKeys(); // it is a vector structure

    if ( !files.empty() ) {
	    lua_newtable(L);
	    int m = 1; // Number of files
	    for (gdcm::Directory::FilenamesType::const_iterator file = files.begin(); file != files.end(); ++file) {
		    lua_newtable(L);
		    const char *fname = file->c_str();
		    lua_pushstring(L, fname);
		    lua_rawseti(L, -2, 1);
		    for (k=0; k<N; k++) {
			    lua_pushstring(L, scn.GetValue(fname, tags[k]));
			    if (lua_isnumber(L, -1) == 1) {
				    lua_pushnumber(L, lua_tonumber(L, -1));
				    lua_remove(L, -2);
			    }
			    if (lua_isnoneornil(L, -1) == 1) {
				    lua_pop(L, 1);
				    lua_pushstring(L, "");
			    }
			    lua_rawseti(L, -2, k+2);
		    }
		    lua_rawseti(L, -2, m++); 
	    }
    } else
	    {lua_pushnil(L); lua_pushfstring(L, "Unable to find DCM files in directory: %s.\n", dirname); return 2;}; //luaL_error(L, "Unable to read directory.");
    return 1;
}

static int dcm2string(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "asstring");
    return 1;
}

static int dcmsize(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "length");
    return 1;
}

static int deltax(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "deltaX");
    return 1;
}

static int deltay(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "deltaY");
    return 1;
}

static int intercept(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "intercept");
    return 1;
}

static const struct luaL_Reg dcm_funcs[] = {
    {"open", imageReader},
    {"scanner", scanner},
    {"header", readImageHeader},
    {NULL, NULL}
};

static const struct luaL_Reg dcm_meths[] = {
    {"__tostring", dcm2string},
    {"__len", dcmsize},
    {"x", deltax},
    {"y", deltay},
    {"intercept", intercept},
    {NULL, NULL}
};

int luaopen_lgdcm (lua_State *L) {
 // GDCM-Image
 luaL_newmetatable(L, "caap.gdcm.image");
 lua_pushvalue(L, -1); // duplicate metatable
 lua_setfield(L, -2, "__index");
 luaL_setfuncs(L, dcm_meths, 0);

 /* create the library */
  luaL_newlib(L, dcm_funcs);
  return 1;
}

#ifdef __cplusplus
}
#endif

