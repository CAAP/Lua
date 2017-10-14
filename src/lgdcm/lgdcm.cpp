#include <gdcm-2.9/gdcmImageReader.h>
#include <gdcm-2.9/gdcmImage.h>
#include <gdcm-2.9/gdcmGlobal.h>
#include <gdcm-2.9/gdcmDicts.h>
#include <gdcm-2.9/gdcmStringFilter.h>
#include <gdcm-2.9/gdcmVR.h>
#include <gdcm-2.9/gdcmScanner.h>
#include <gdcm-2.9/gdcmDefs.h>
#include <gdcm-2.9/gdcmWriter.h>

#include <lua.hpp>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "anon.h"

#define checkimage(L,i) (char *)luaL_checkudata(L, i, "caap.gdcm.image")

typedef struct gdcmtags {
    int size;
    gdcm::Tag tags[1];
} Tags;

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
		hexs[j++] = ((unsigned int)lua_tointeger(L, -1)); // lua_tounsigned
		lua_pop(L, 1);
	}
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
	if (!lua_stringtonumber(L, strm.str().c_str()))
	    lua_pushinteger(L, 0);
/*	lua_pushstring(L, strm.str().c_str());
	lua_pushinteger(L, lua_tointeger(L, -1)); // lua_pushnumber ... lua_tonumber
	lua_remove(L, -2); */
	lua_setfield(L, -2, "intercept");

}

static int initializeXML() {
    const char* xmlpath = getenv("GDCM_RESOURCES");
    if ( xmlpath && g.Prepend( xmlpath ) && g.LoadResourcesFiles() )
	return 1;
    return 0;
}

// PUBLIC FUNCTIONS

static int isValidTag(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    unsigned int j[2], valid=0;

    gettag(L, j);
    const gdcm::DictEntry &entry = dicts.GetDictEntry( gdcm::Tag(j[0], j[1]) );
    if( entry.GetVR() & gdcm::VR::VRASCII ) { valid = 1; }
    if( entry.GetVR() & gdcm::VR::VRBINARY ) { valid = 1; }
    lua_pushboolean(L, valid);
    return 1;
}

// IMAGE

static int imageReader(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);

    gdcm::ImageReader reader;
    reader.SetFileName( filename );
    if ( !reader.Read() ) {
	    lua_pushnil(L); lua_pushfstring(L, "Could not read: %s", filename);
	    return 2;
    }
    gdcm::Image & image = reader.GetImage();

    const unsigned int* dimension = image.GetDimensions();
    const gdcm::PixelFormat pf = image.GetPixelFormat();
    const int stype = pf.GetScalarType();
    const unsigned long length = image.GetBufferLength();
    const double intercept = image.GetIntercept();

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

    getIntercept(L, filename); // add intercept field with initial value or zero

    lua_pushinteger(L, stype);
    lua_setfield(L, -2, "type");
    lua_pushfstring(L, "GDCM-Image{file: %s, dims:{x: %d, y: %d}, pixel: %s, intercept: %f}", filename, dimension[0], dimension[1], pixel2str(stype), intercept);
    lua_setfield(L, -2, "asstring");
    lua_setuservalue(L, -2); // append upvalue to userdatum
    return 1; // returns userdatum: caap.gdcm.image
}

static int dimx(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "dimX");
    return 1;
}

static int dimy(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "dimY");
    return 1;
}

static int intercept(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "intercept");
    return 1;
}

static int img2string(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "asstring");
    return 1;
}

static int img_len(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "length");
    return 1;
}

//READER

static int confidentialProfile(lua_State *L) {
    unsigned int N = nAttributes(); // number of dicom tags
    Tags *ptags = (Tags *)lua_newuserdata(L, sizeof( Tags ) +  (N - 1)*sizeof(gdcm::Tag));
    luaL_getmetatable(L, "caap.gdcm.reader");
    lua_setmetatable(L, -2);

    ptags->size = N;
    gdcm::Tag *ptag = ptags->tags;
    getAttributes( ptag );

    return 1;
}

static int newReader(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE); // table containing dicom tags

    int N = luaL_len(L, 1); // number of dicom tags
    Tags *ptags = (Tags *)lua_newuserdata(L, sizeof( Tags ) +  (N - 1)*sizeof(gdcm::Tag));
    luaL_getmetatable(L, "caap.gdcm.reader");
    lua_setmetatable(L, -2);

    ptags->size = N;
    gdcm::Tag *ptag = ptags->tags;
    unsigned int k, j[2];
    for(k=0; k<N;) {
	lua_rawgeti(L, 1, ++k);
	gettag(L, j);
	lua_pop(L, 1);
	*ptag++ = gdcm::Tag(j[0], j[1]);
    }

    return 1;
}

static int readData(lua_State *L) {
    Tags *ptags = (Tags *)luaL_checkudata(L, 1, "caap.gdcm.reader");
    const char *fname = luaL_checkstring(L, 2);

    unsigned int N = ptags->size;
    gdcm::Reader reader;
    reader.SetFileName( fname );
    if ( reader.ReadUpToTag( ptags->tags[N-1] ) ) {
	const gdcm::DataSet & ds = reader.GetFile().GetDataSet();
	const gdcm::FileMetaInformation & header = reader.GetFile().GetHeader();
	gdcm::StringFilter sf;
	sf.SetFile( reader.GetFile() );
        // create result TABLE
	lua_newtable(L);
	lua_pushstring(L, fname); // add filename
	lua_rawseti(L, -2, 1);
	gdcm::Tag *ptag = ptags->tags;
	unsigned int k, w = 0;
	for (k=0; k < N;) {
	    gdcm::Tag tag( ptag[k++] );
	    std::string s = "";
	    if ( tag.GetGroup() == 0x2 && header.FindDataElement( tag ))
		s = sf.ToString( header.GetDataElement( tag ).GetTag() );
	    else if ( ds.FindDataElement( tag ) )
		s = sf.ToString( ds.GetDataElement( tag ).GetTag() );
	    else
		w++; // missing value
	    // ADD value/empty string
	    if (!lua_stringtonumber(L, s.c_str()))
		lua_pushstring(L, s.c_str());
/*	    lua_pushstring(L, s.c_str());
	    if ( lua_isnumber(L, -1) == 1 ) {
		lua_pushnumber(L, lua_tonumber(L, -1));
		lua_remove(L, -2);
	    } */
	    lua_rawseti(L, -2, k+1);
	}
// mising values could be added to TABLE
        return 1;
    }
    else { lua_pushnil(L); lua_pushstring(L, "Error reading file."); return 2; }
}

static int isValid(lua_State *L) {
    Tags *ptags = (Tags *)luaL_checkudata(L, 1, "caap.gdcm.reader");
    const char *fname = luaL_checkstring(L, 2);

    int N = ptags->size;
    gdcm::Reader reader;
    reader.SetFileName( fname );
    lua_pushboolean( L, reader.ReadUpToTag( ptags->tags[N-1] ) );
    return 1;
}

static int rdr_len(lua_State *L) {
    Tags *ptags = (Tags *)luaL_checkudata(L, 1, "caap.gdcm.reader");
    lua_pushinteger( L, ptags->size );
    return 1;
}

static int rdr2string(lua_State *L) {
    lua_pushstring(L, "GDCM-Reader");
    return 1;
}

// ANONYMIZER

static int deidentify( lua_State *L ) {
    const char *filename = luaL_checkstring(L, 1);
    const char *outfilename = luaL_checkstring(L, 2);

    if ( !initializeXML() ) {
	lua_pushnil(L); lua_pushfstring(L, "Error loading GDCM Resources path.\n");
	return 2;
    }

    gdcm::Reader reader;
    reader.SetFileName( filename );
    if ( !reader.Read() ) {
	    lua_pushnil(L); lua_pushfstring(L, "Could not read: %s", filename);
	    return 2;
    }

    gdcm::File &file = reader.GetFile();
    gdcm::MediaStorage ms;
    ms.SetFromFile(file);
    if ( gdcm::Defs::GetIODNameFromMediaStorage(ms) == NULL ) {
	lua_pushnil(L); lua_pushfstring(L, "The Media Storage Type of your file is not supported: %s", ms.GetString());
	return 2;
    }

    basicAnonymizer( &file );// ANON STUFF

    gdcm::FileMetaInformation &fmi = file.GetHeader();
    fmi.Clear();

    gdcm::Writer writer;
    writer.SetFileName( outfilename );
    writer.SetFile( file ); // INPUT file

    if ( (strcmp(filename, outfilename) != 0) && !writer.Write() ) {
	lua_pushnil(L); lua_pushfstring(L, "Could not write: %s", outfilename);
	return 2;
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// SCANNER

static int scanner(lua_State *L) {
    gdcm::Scanner scn;
    gdcm::Directory d;
    const char* dirname = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE); // table containing dicom tags
    bool recurse = (1 == lua_toboolean(L, 3)); // whether recursion is performed

    d.Load( dirname, recurse ); // Load files from directory; recursively

    int k, N = luaL_len(L, 2); // N is the number of tags
    unsigned int j[2];
    gdcm::Tag *tags = (gdcm::Tag *)lua_newuserdata(L, N*sizeof(gdcm::Tag));
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
			    const char *value = scn.GetValue(fname, tags[k]); // maybe change to Mappings as suggested in Doc!
			    if(!lua_stringtonumber(L, value))
				lua_pushstring(L, (value ? value : ""));
/*			    lua_pushstring(L, scn.GetValue(fname, tags[k]));
			    if (lua_isnumber(L, -1) == 1) {
				    lua_pushnumber(L, lua_tonumber(L, -1));
				    lua_remove(L, -2);
			    }
			    if (lua_isnoneornil(L, -1) == 1) {
				    lua_pop(L, 1);
				    lua_pushstring(L, "");
			    } */
			    lua_rawseti(L, -2, k+2);
		    }
		    lua_rawseti(L, -2, m++); 
	    }
    } else
	    {lua_pushnil(L); lua_pushfstring(L, "Unable to find DCM files in directory: %s.\n", dirname); return 2;}; //luaL_error(L, "Unable to read directory.");
    return 1;
}

static const struct luaL_Reg dcm_funcs[] = {
    {"open", imageReader},
    {"reader", newReader},
    {"profile", confidentialProfile},
    {"scanner", scanner},
    {"valid", isValidTag},
    {"anonymize", deidentify},
    {NULL, NULL}
};

static const struct luaL_Reg img_meths[] = {
    {"__tostring", img2string},
    {"__len", img_len},
    {"x", dimx},
    {"y", dimy},
    {"intercept", intercept},
    {NULL, NULL}
};

static const struct luaL_Reg rdr_meths[] = {
    {"__len", rdr_len},
    {"__tostring", rdr2string},
    {"read", readData},
    {"valid", isValid},
    {NULL, NULL}
};

int luaopen_lgdcm (lua_State *L) {
 // GDCM-Image
 luaL_newmetatable(L, "caap.gdcm.image");
 lua_pushvalue(L, -1); // duplicate metatable
 lua_setfield(L, -1, "__index");
 luaL_setfuncs(L, img_meths, 0);

 // GDCM-Reader
 luaL_newmetatable(L, "caap.gdcm.reader");
 lua_pushvalue(L, -1);
 lua_setfield(L, -1, "__index");
 luaL_setfuncs(L, rdr_meths, 0);

 /* create the library */
  luaL_newlib(L, dcm_funcs);
  return 1;
}

#ifdef __cplusplus
}
#endif

/*
static int IODName(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);

    gdcm::Reader reader;
    reader.SetFileName( filename );
    if ( !reader.Read() ) {
	    lua_pushnil(L); lua_pushfstring(L, "Could not read: %s", filename);
	    return 2;
    }
    gdcm::File &file = reader.GetFile();

    if ( !initializeXML() ) {
	lua_pushboolean(L, 0);
	lua_pushfstring(L, "Error loading GDCM Resources path.\n");
	return 2;
    }

    gdcm::MediaStorage ms;
    ms.SetFromFile( file );
    const gdcm::IODs &iods = defs.GetIODs();
    const gdcm::Modules &modules = defs.GetModules();
    const gdcm::Macros &macros = defs.GetMacros();
    const char *iodname = gdcm::Defs::GetIODNameFromMediaStorage( ms );
    if( !iodname ) { lua_pushnil(L); return 1; }
    const gdcm::IOD &iod = iods.GetIOD( iodname );

    gdcm::IOD::SizeType niods = iod.GetNumberOfIODs();
    bool v = true;
    unsigned int idx;
    for (idx = 0; idx < niods; idx++) {
	const gdcm::IODEntry &iodentry = iod.GetIODEntry(idx);
	const char *ref = iodentry.GetRef();
	gdcm::Usage::UsageType ut = iodentry.GetUsageType();
	const gdcm::Module &module = modules.GetModule( ref );
	v = v && module.Verify( file.GetDataSet(), ut );
    }
    lua_pushboolean(L, v);

    return 1;
}
*/

