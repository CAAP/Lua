#include <gdcm/gdcmGlobal.h>
#include <gdcm/gdcmStringFilter.h>
#include <gdcm/gdcmSequenceOfItems.h>
#include <gdcm/gdcmExplicitDataElement.h>
#include <gdcm/gdcmDataSetHelper.h>
#include <gdcm/gdcmUIDGenerator.h>
#include <gdcm/gdcmAttribute.h>
#include <gdcm/gdcmDummyValueGenerator.h>
#include <gdcm/gdcmDicts.h>
#include <gdcm/gdcmType.h>
#include <gdcm/gdcmDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "anon.h"

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
/*    Series Number  ***       			*/ gdcm::Tag(0x0020,0x0011),
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

static const unsigned int TagSize = sizeof( gdcm::Tag );

static const unsigned int nSIDs = sizeof(SpecialTypeTags) / TagSize;

static const unsigned int nIDs = sizeof(BasicApplicationLevelConfidentialityProfileAttributes) / TagSize;

static int isSpecial ( gdcm::Tag const &tag ) {
    return std::binary_search( SpecialTypeTags, SpecialTypeTags + nSIDs, tag );
}

/*
static int removeTag( gdcm::Tag const &t, gdcm::DataSet const &ds ) {
    return ds.FindDataElement( t ) && ds.Remove( t );
} */

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

/* UNUSED fun XXX
static int empty( gdcm::Tag const &t, gdcm::DataSet &ds ) {
    return lreplace(t, "", 0, ds);
} */

static int canEmpty( gdcm::Tag const &tag, const gdcm::Type type ) {
    switch( type ) {
	case gdcm::Type::T1 : case gdcm::Type::T1C : return 0;
	case gdcm::Type::UNKNOWN : return 1;
	default: return !isSpecial( tag );
    }
}

static int isVRUI( gdcm::Tag const &tag ) {
    return dicts.GetDictEntry( tag ).GetVR() == gdcm::VR::UI;
}

static int basicProtection ( gdcm::Tag const &tag, const gdcm::Type type, gdcm::DataSet &ds ) {
    gdcm::DataElement de = ds.GetDataElement( tag );

    if ( canEmpty(tag, type) ) {
	de.Empty();
	ds.Replace( de );
	return 1;
    }

    gdcm::UIDGenerator uid;
    if ( isVRUI(tag) ) {
//	const gdcm::ByteValue *bv; // NOT USED XXX
//	const char *oldUID = ( !de.IsEmpty() && (bv = de.GetByteValue()) ) ? bv->GetPointer() : ""; // NOT USED XXX
	// anonymize UID
	const char *newUID = "DUMMY-UID"; //uid.Generate();
	de.SetByteValue( newUID, (uint32_t)strlen(newUID) );
    } else {
	const char *dummy = "DUMMY";
	//dummy = ( dummy = gdcm::DummyValueGenerator::Generate( "" ) )  "";
	// anonymize non-UID
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
	    basicProtection( tag, iod.GetTypeFromTag(defs, tag), ds );
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

int basicAnonymizer( gdcm::File *file ) {
    gdcm::DataSet &ds = file->GetDataSet();

    // sanity checks
    static const gdcm::Tag Att1 = gdcm::Tag(0x0400, 0x0500), Att2 = gdcm::Tag(0x0012, 0x0062), Att3 = gdcm::Tag(0x0012, 0x0063);
    if ( ds.FindDataElement( Att1 ) || ds.FindDataElement( Att2 ) || ds.FindDataElement( Att3 ) ) { return 0; }

    recurseDataSet( ds, *file );

    replaceTag( gdcm::Tag(0x0012, 0x0062), "YES", ds );
    replaceTag( gdcm::Tag(0x0012, 0x0063), "BASIC APPLICATION LEVEL CONFIDENTIALITY PROFILE", ds );
    
    return 1;
}

unsigned int nAttributes() {
    return nIDs;
}

void getAttributes( gdcm::Tag *ptag) {
    static const gdcm::Tag *start = BasicApplicationLevelConfidentialityProfileAttributes;
    static const gdcm::Tag *ptr,*end = start + nIDs;

    for(ptr = start; ptr != end; ++ptr) {
	*ptag++ = *ptr;
    }
}

#ifdef __cplusplus
}
#endif

