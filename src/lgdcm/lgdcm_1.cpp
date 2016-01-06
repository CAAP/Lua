#include <gdcm-2.4/gdcmImageReader.h>
#include <gdcm-2.4/gdcmImage.h>
#include <gdcm-2.4/gdcmUIDGenerator.h>
#include <gdcm-2.4/gdcmDataSetHelper.h>
#include <gdcm-2.4/gdcmGlobal.h>
#include <gdcm-2.4/gdcmVR.h>
#include <gdcm-2.4/gdcmScanner.h>
#include <gdcm-2.4/gdcmDefs.h>
#include <gdcm-2.4/gdcmStringFilter.h>
#include <gdcm-2.4/gdcmType.h>
#include <gdcm-2.4/gdcmWriter.h>

#include <lua.hpp>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define checkimage(L,i) (char *)luaL_checkudata(L, i, "caap.gdcm.image")

static gdcm::Tag BasicApplicationLevelConfidentialityProfileAttributes[] = {
//    Attribute Name                                Tag
/*    Instance Creator UID                      */ gdcm::Tag(0x0008,0x0014),
/*    SOP Instance UID                          */ gdcm::Tag(0x0008,0x0018),
/*    Accession Number                          */ gdcm::Tag(0x0008,0x0050),
/*    Institution Name                          */ gdcm::Tag(0x0008,0x0080),
/*    Institution Address                       */ gdcm::Tag(0x0008,0x0081),
/*    Referring Physician's Name                */ gdcm::Tag(0x0008,0x0090),
/*    Referring Physician's Address             */ gdcm::Tag(0x0008,0x0092),
/*    Referring Physician's Telephone Numbers   */ gdcm::Tag(0x0008,0x0094),
/*    Station Name                              */ gdcm::Tag(0x0008,0x1010),
/*    Study Description                         */ gdcm::Tag(0x0008,0x1030),
/*    Series Description                        */ gdcm::Tag(0x0008,0x103E),
/*    Institutional Department Name             */ gdcm::Tag(0x0008,0x1040),
/*    Physician(s) of Record                    */ gdcm::Tag(0x0008,0x1048),
/*    Performing Physicians' Name               */ gdcm::Tag(0x0008,0x1050),
/*    Name of Physician(s) Reading Study        */ gdcm::Tag(0x0008,0x1060),
/*    Operators' Name                           */ gdcm::Tag(0x0008,0x1070),
/*    Admitting Diagnoses Description           */ gdcm::Tag(0x0008,0x1080),
/*    Referenced SOP Instance UID               */ gdcm::Tag(0x0008,0x1155),
/*    Derivation Description                    */ gdcm::Tag(0x0008,0x2111),
/*    Patient's Name                            */ gdcm::Tag(0x0010,0x0010),
/*    Patient ID                                */ gdcm::Tag(0x0010,0x0020),
/*    Patient's Birth Date                      */ gdcm::Tag(0x0010,0x0030),
/*    Patient's Birth Time                      */ gdcm::Tag(0x0010,0x0032),
/*    Patient's Sex                             */ gdcm::Tag(0x0010,0x0040),
/*    Other Patient Ids                         */ gdcm::Tag(0x0010,0x1000),
/*    Other Patient Names                       */ gdcm::Tag(0x0010,0x1001),
/*    Patient's Age                             */ gdcm::Tag(0x0010,0x1010),
/*    Patient's Size                            */ gdcm::Tag(0x0010,0x1020),
/*    Patient's Weight                          */ gdcm::Tag(0x0010,0x1030),
/*    Medical Record Locator                    */ gdcm::Tag(0x0010,0x1090),
/*    Ethnic Group                              */ gdcm::Tag(0x0010,0x2160),
/*    Occupation                                */ gdcm::Tag(0x0010,0x2180),
/*    Additional Patient's History              */ gdcm::Tag(0x0010,0x21B0),
/*    Patient Comments                          */ gdcm::Tag(0x0010,0x4000),
/*    Device Serial Number                      */ gdcm::Tag(0x0018,0x1000),
/*    Protocol Name                             */ gdcm::Tag(0x0018,0x1030),
/*    Study Instance UID                        */ gdcm::Tag(0x0020,0x000D),
/*    Series Instance UID                       */ gdcm::Tag(0x0020,0x000E),
/*    Study ID                                  */ gdcm::Tag(0x0020,0x0010),
/*    Frame of Reference UID                    */ gdcm::Tag(0x0020,0x0052),
/*    Synchronization Frame of Reference UID    */ gdcm::Tag(0x0020,0x0200),
/*    Image Comments                            */ gdcm::Tag(0x0020,0x4000),
/*    Request Attributes Sequence               */ gdcm::Tag(0x0040,0x0275),
/*    UID                                       */ gdcm::Tag(0x0040,0xA124),
/*    Content Sequence                          */ gdcm::Tag(0x0040,0xA730),
/*    Storage Media File-set UID                */ gdcm::Tag(0x0088,0x0140),
/*    Referenced Frame of Reference UID         */ gdcm::Tag(0x3006,0x0024),
/*    Related Frame of Reference UID            */ gdcm::Tag(0x3006,0x00C2)
};

static const gdcm::Tag SpecialTypeTags[] = {
/*   Patient's Name          */ gdcm::Tag(0x0010,0x0010),
/*   Patient ID              */ gdcm::Tag(0x0010,0x0020),
/*   Study ID                */ gdcm::Tag(0x0020,0x0010),
/*   Series Number           */ gdcm::Tag(0x0020,0x0011)
};

static const gdcm::Global &g = gdcm::GlobalInstance;

static const gdcm::Dicts &dicts = g.GetDicts();

static const gdcm::Defs &defs = g.GetDefs();

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
		hexs[j++] = lua_tounsigned(L, -1);
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
	lua_pushstring(L, strm.str().c_str());
	lua_pushnumber(L, lua_tonumber(L, -1));
	lua_remove(L, -2);
	lua_setfield(L, -2, "intercept");
}

// PUBLIC FUNCTIONS

static int isValidTag(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    unsigned int j[2], valid=0;

    gettag(L, j);
    static const gdcm::Global &g = gdcm::GlobalInstance;
    static const gdcm::Dicts &dicts = g.GetDicts();
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

    getIntercept(L, filename);

    lua_pushinteger(L, stype);
    lua_setfield(L, -2, "type");
    lua_pushfstring(L, "GDCM-Image{file: %s, dims:{x: %d, y: %d}, pixel: %s, intercept: %f}", filename, dimension[0], dimension[1], pixel2str(stype), intercept);
    lua_setfield(L, -2, "asstring");
    lua_setuservalue(L, -2);
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

static int read(lua_State *L) {
    Tags *ptags = (Tags *)luaL_checkudata(L, 1, "caap.gdcm.reader");
    const char *fname = luaL_checkstring(L, 2);

    int N = ptags->size;
    gdcm::Reader reader;
    reader.SetFileName( fname );
    if ( reader.ReadUpToTag( ptags->tags[N-1] ) ) {
	const gdcm::DataSet & ds = reader.GetFile().GetDataSet();
	const gdcm::FileMetaInformation & header = reader.GetFile().GetHeader();
	gdcm::StringFilter sf;
	sf.SetFile( reader.GetFile() );
        // create result table
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
	    lua_pushstring(L, s.c_str());
	    if ( lua_isnumber(L, -1) == 1 ) {
		lua_pushnumber(L, lua_tonumber(L, -1));
		lua_remove(L, -2);
	    }
	    lua_rawseti(L, -2, k+1);
	}
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

static const unsigned int TagSize = sizeof( gdcm::Tag );

static const unsigned int nSIDs = sizeof(SpecialTypeTags) / TagSize;

static const unsigned int nIDs = sizeof(BasicApplicationLevelConfidentialityProfileAttributes) / TagSize;

static int isSpecial ( gdcm::Tag const &tag ) {
    return std::binary_search( SpecialTypeTags, SpecialTypeTags + nSIDs, tag );
}

//static int removeTag( gdcm::Tag const &t, gdcm::DataSet const &ds ) {
//    return ds.FindDataElement( t ) && ds.Remove( t );
//}

static int replaceData( gdcm::Tag const &t, gdcm::DataSet &ds, const gdcm::DictEntry &entry, char const *str, gdcm::VL::Type const size ) {
    gdcm::DataElement de ( t );
    de.SetVR( ds.FindDataElement(t) ? ds.GetDataElement(t).GetVR() : entry.GetVR() );
    de.SetByteValue( str, size );
    ds.Replace( de );
    return 1;
}

static int emptyData( gdcm::Tag const &t, gdcm::DataSet &ds ) {
    gdcm::DataElement de( t );
    de.SetVR( gdcm::VR::SQ );
    ds.Replace( de );
    return 1;
}

static int lreplace( gdcm::Tag const &t, const char *value, gdcm::VL const & vl, gdcm::DataSet &ds ) {
    if ( t.GetGroup() < 0x0008 ) return 0;

    if ( t.IsPrivate() && vl == 0 && ds.FindDataElement( t ) ) {
	gdcm::DataElement de ( ds.GetDataElement( t ) );
	if ( de.GetVR() != gdcm::VR::INVALID ) {
	    if ( de.GetVR() == gdcm::VR::SQ && *value == 0 )
		{ return emptyData( t, ds ); } // Only allowed operation for a Private Tag
	    else { return 0; } //Trying to replace value of a Private Tag
	}
	de.SetByteValue( "", 0 );
	ds.Insert( de );
	return 1;
    } else {
	const gdcm::DictEntry &entry = dicts.GetDictEntry( t );

	if ( entry.GetVR() == gdcm::VR::INVALID || entry.GetVR() == gdcm::VR::UN )
	    { return 0; }

	if ( entry.GetVR() == gdcm::VR::SQ && vl == 0 && value && *value == 0 )
	    { return emptyData( t, ds ); }

	else if ( entry.GetVR() && gdcm::VR::VRBINARY && vl == 0 )
		{ return replaceData( t, ds, entry, "", 0 ); }

	else { // ASCII
	    if ( value ) {
		std::string padded( value, vl);
		if ( vl.IsOdd() && (entry.GetVR() != gdcm::VR::UI) ) { padded += " "; }
		return replaceData( t, ds, entry, padded.c_str(), (gdcm::VL::Type)padded.size() );
	    }
	}
    }

    return 0;
}

static int replaceTag( gdcm::Tag const &t, const char *value, gdcm::DataSet &ds ) {
    gdcm::VL::Type len = (value == NULL) ? 0 : (gdcm::VL::Type)std::strlen( value );
    return lreplace( t, value, len, ds );
}

static int empty( gdcm::Tag const &t, gdcm::DataSet &ds ) {
    return lreplace(t, "", 0, ds);
}

static int canEmpty( gdcm::Tag const &tag, const gdcm::Type type ) {
    return !( type == gdcm::Type::T1 || type == gdcm::Type::T1C ) && ( type == gdcm::Type::UNKNOWN ) || !isSpecial( tag );
}

static int isVRUI( gdcm::Tag const &tag ) {
    return dicts.GetDictEntry( tag ).GetVR() == gdcm::VR::UI;
}

static int BasicProtection ( gdcm::Tag const &tag, const gdcm::Type type, gdcm::DataSet &ds ) {
    gdcm::DataElement de = ds.GetDataElement( tag );

    if ( canEmpty(tag, type) ) {
	de.Empty();
	ds.Replace( de );
	return 1;
    }

    gdcm::UIDGenerator uid;
    if ( isVRUI(tag) ) {
	const gdcm::ByteValue *bv;
	const char *oldUID = ( !de.IsEmpty() && (bv = de.GetByteValue()) ) ? bv->GetPointer() : "";
	// anonymize UID XXX
	const char *newUID = uid.Generate();
	de.SetByteValue( newUID, (uint32_t)strlen(newUID) );
    } else {
	const char *dummy = "";
	//dummy = ( dummy = gdcm::DummyValueGenerator::Generate( "" ) )  "";
	// anonymize non-UID XXX
	de.SetByteValue( dummy, (uint32_t)strlen(dummy) );
    }
    ds.Replace( de );
    return 1;
}

static int recurseDataSet ( gdcm::DataSet &ds, gdcm::File &F ) {
    if ( ds.IsEmpty() ) {return 0;}

    static const gdcm::Tag *start = BasicApplicationLevelConfidentialityProfileAttributes;
    static const gdcm::Tag *ptr,*end = start + nIDs;
    const gdcm::IOD& iod = defs.GetIODFromFile( F );

    for(ptr = start; ptr != end; ++ptr) {
	const gdcm::Tag &tag = *ptr;
	if ( ds.FindDataElement( tag ) ) {
	    BasicProtection( tag, iod.GetTypeFromTag(defs, tag), ds );
	}
    }

    gdcm::DataSet::ConstIterator it = ds.Begin();
    for (; it != ds.End() ;) {
	gdcm::DataElement de = *it++;
	gdcm::VR vr = gdcm::DataSetHelper::ComputeVR( F, ds, de.GetTag() );
	gdcm::SmartPointer<gdcm::SequenceOfItems> sqi = 0;

	if ( vr == gdcm::VR::SQ && (sqi = de.GetValueAsSQ()) ) {
	    de.SetValue( *sqi );
	    de.SetVLToUndefined();
	    gdcm::SequenceOfItems::SizeType i;
	    for (i = 1; i <= sqi->GetNumberOfItems(); i++ ) {
		gdcm::Item &item = sqi->GetItem( i );
		gdcm::DataSet &nested = item.GetNestedDataSet();
		recurseDataSet( nested, F );
	    }
	}

	ds.Replace( de ); // REPLACE
    }
    return 1;
}

static int basicAnonymizer( gdcm::File *file ) {

    gdcm::DataSet &ds = file->GetDataSet();

    // sanity checks
    static const gdcm::Tag Att1 = gdcm::Tag(0x0400, 0x0500), Att2 = gdcm::Tag(0x0012, 0x0062), Att3 = gdcm::Tag(0x0012, 0x0063);
    if ( ds.FindDataElement( Att1 ) || ds.FindDataElement( Att2 ) || ds.FindDataElement( Att3 ) ) { return 0; }

    recurseDataSet( ds, *file );

    replaceTag( gdcm::Tag(0x0012, 0x0062), "YES", ds );
    replaceTag( gdcm::Tag(0x0012, 0x0063), "BASIC APPLICATION LEVEL CONFIDENTIALITY PROFILE", ds );
    
    return 1;
}

static int deidentify( lua_State *L ) {
    const char *filename = luaL_checkstring(L, 1);
    const char *outfilename = luaL_checkstring(L, 2);

    gdcm::Reader reader;
    reader.SetFileName( filename );
    if ( !reader.Read() ) {
	    lua_pushnil(L); lua_pushfstring(L, "Could not read: %s", filename);
	    return 2;
    }

    gdcm::File &file = reader.GetFile();
    gdcm::MediaStorage ms;
    ms.SetFromFile(file);
/*    static const gdcm::Global &g = gdcm::GlobalInstance;
    static const gdcm::Dicts &dicts = g.GetDicts();
    static const gdcm::Defs &defs = g.GetDefs();
    if ( gdcm::Defs::GetIODNameFromMediaStorage(ms) == NULL ) {
	lua_pushnil(L); lua_pushfstring(L, "The Media Storage Type of your file is not supported: %s", ms);
	return 2;
    }
*/
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

static const struct luaL_Reg dcm_funcs[] = {
    {"open", imageReader},
    {"reader", newReader},
    {"scanner", scanner},
    {"valid", isValidTag},
    {"anonymize", deidentify},
    {NULL, NULL}
};

static const struct luaL_Reg img_meths[] = {
    {"__tostring", img2string},
    {"__len", img_len},
    {"x", deltax},
    {"y", deltay},
    {"intercept", intercept},
    {NULL, NULL}
};

static const struct luaL_Reg rdr_meths[] = {
    {"__len", rdr_len},
    {"__tostring", rdr2string},
    {"read", read},
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

