#include "itkImage.h"
#include "itkVTKImageIO.h"
#include "itkSmartPointer.h"
#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"

#include "itkResampleImageFilter.h"
#include "itkLaplacianSharpeningImageFilter.h"

#include "itkImageRegistrationMethodv4.h"
#include "itkSyNImageRegistrationMethod.h"
#include "itkDisplacementFieldTransform.h"
#include "itkMeanSquaresImageToImageMetricv4.h"
#include "itkCorrelationImageToImageMetricv4.h"
#include "itkImageToImageMetricv4.h"
#include "itkMattesMutualInformationImageToImageMetricv4.h"
#include "itkImageMomentsCalculator.h"
#include "itkImageToHistogramFilter.h"
#include "itkHistogramMatchingImageFilter.h"
#include "itkIntensityWindowingImageFilter.h"

#include "itkAffineTransform.h"

#include "itkHistogramMatchingImageFilter.h"
#include "itkMinimumMaximumImageCalculator.h"

#include <lua.hpp>
#include <lauxlib.h>
#include <string.h>


#define catchMe(L) catch (itk::ExceptionObject &ex) { lua_pushnil(L); lua_pushfstring(L, "Exception found: %s\n", ex.what()); return 2; }


/*
template <class TImage>
typename TImage::Pointer MakeNewImage(typename TImage::Pointer image1, typename TImage::PixelType initval) {
  typedef itk::ImageRegionIteratorWithIndex<TImage> Iterator;
  typename TImage::Pointer varimage = AllocImage<TImage>(image1);
  Iterator vfIter2( varimage,  varimage->GetLargestPossibleRegion() );
  for(  vfIter2.GoToBegin(); !vfIter2.IsAtEnd(); ++vfIter2 )
    {
    if( initval >= 0 )
      {
      vfIter2.Set(initval);
      }
    else
      {
      vfIter2.Set(image1->GetPixel(vfIter2.GetIndex() ) );
      }
    }

  return varimage;
}
*/

template <typename ImageType>
typename ImageType::Pointer readImage(const char* fname) {
    if (fname == NULL) {
	typename ImageType::Pointer ret = NULL;
	return ret;
    }
  // Read the image files begin
  typedef itk::ImageFileReader<ImageType> ImageFileReader;

  typename ImageFileReader::Pointer reader = ImageFileReader::New();
  reader->SetFileName( fname );
  try
    {
    reader->Update();
    }
  catch( itk::ExceptionObject & e )
    {
    std::cerr << "Exception caught during image reference file reading " << std::endl;
    std::cerr << e << std::endl;
    return NULL;
    }

  typename ImageType::Pointer target = reader->GetOutput();
  return target;
}

template <typename ImageType>
typename ImageType::Pointer resampleImage(const char* fname, typename ImageType::Pointer model) {
    typedef itk::ResampleImageFilter<ImageType, ImageType, float> ResamplerType;
    typename ResamplerType::Pointer resampler = ResamplerType::New();

    typename ImageType::Pointer orig = readImage<ImageType>( fname );
    resampler->SetInput( orig );
    resampler->SetOutputParametersFromImage( model );
   try
    {
    resampler->Update();
    }
  catch( itk::ExceptionObject & e )
    {
    std::cerr << "Exception caught during image reference file reading " << std::endl;
    std::cerr << e << std::endl;
    return NULL;
    }

    typename ImageType::Pointer target = resampler->GetOutput();
    return target;
}
 

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
//	writer->SetUseCompression( true );
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

/*
template<typename MetricType>
template<typename ImageType>
typename MetricType::Pointer newmetric() {
    typedef itk::ANTSNeighborhoodCorrelationImageToImageMetricv4<ImageType, ImageType> CorrelationMetricType;
    typename CorrelationMetricType::Pointer correlationMetric = CorrelationMetricType::New();
    typename CorrelationMetricType::RadiusType radius;
    radius.Fill( radiusOption );
    correlationMetric->SetRadius( radius );
    correlationMetric->SetUseMovingImageGradientFilter( false );
    correlationMetric->SetUseFixedImageGradientFilter( false );
    return correlationMetric;

    typedef itk::MattesMutualInformationImageToImageMetricv4<ImageType, ImageType> MutualInformationMetricType;
    typename MutualInformationMetricType::Pointer mutualInformationMetric = MutualInformationMetricType::New();
    mutualInformationMetric->SetNumberOfHistogramBins( bins );
    mutualInformationMetric->SetUseMovingImageGradientFilter( false );
    mutualInformationMetric->SetUseFixedImageGradientFilter( false );

    return mutualInformationMetric;

    typedef itk::MeanSquaresImageToImageMetricv4<ImageType, ImageType> DemonsMetricType;
    typename DemonsMetricType::Pointer demonsMetric = DemonsMetricType::New();
    return demonsMetric;

    typedef itk::CorrelationImageToImageMetricv4<ImageType, ImageType> corrMetricType;
    typename corrMetricType::Pointer corrMetric = corrMetricType::New();
    return corrMetric;
}
*/

template<typename ImageType>
typename ImageType::Pointer preprocess(typename ImageType::Pointer inputImage, typename ImageType::PixelType lower, typename ImageType::PixelType upper, float lowerQuantile, float upperQuantile, typename ImageType::Pointer histogramMatch) {
    typedef itk::Statistics::ImageToHistogramFilter<ImageType>   HistogramFilterType;
    typedef typename HistogramFilterType::InputBooleanObjectType InputBooleanObjectType;
    typedef typename HistogramFilterType::HistogramSizeType      HistogramSizeType;

    HistogramSizeType histogramSize( 1 );
    histogramSize[0] = 256;

    typename InputBooleanObjectType::Pointer autoMinMaxInputObject = InputBooleanObjectType::New();
    autoMinMaxInputObject->Set( true );

    typename HistogramFilterType::Pointer histogramFilter = HistogramFilterType::New();
    histogramFilter->SetInput( inputImage );
    histogramFilter->SetAutoMinimumMaximumInput( autoMinMaxInputObject );
    histogramFilter->SetHistogramSize( histogramSize );
    histogramFilter->SetMarginalScale( 10.0 );
    histogramFilter->Update();

    float lowerFunction = histogramFilter->GetOutput()->Quantile( 0, lowerQuantile );
    float upperFunction = histogramFilter->GetOutput()->Quantile( 0, upperQuantile );

    typedef itk::IntensityWindowingImageFilter<ImageType, ImageType> IntensityWindowingImageFilterType;
    typename IntensityWindowingImageFilterType::Pointer windowingFilter = IntensityWindowingImageFilterType::New();
    windowingFilter->SetInput( inputImage );
    windowingFilter->SetWindowMinimum( lowerFunction );
    windowingFilter->SetWindowMaximum( upperFunction );
    windowingFilter->SetOutputMinimum( lower );
    windowingFilter->SetOutputMaximum( upper );
    windowingFilter->Update();

    typename ImageType::Pointer outputImage;
    if( histogramMatch ) {
	typedef itk::HistogramMatchingImageFilter<ImageType, ImageType> HistogramMatchingFilterType;
	typename HistogramMatchingFilterType::Pointer matchingFilter = HistogramMatchingFilterType::New();
	matchingFilter->SetSourceImage( windowingFilter->GetOutput() );
	matchingFilter->SetReferenceImage( histogramMatch );
	matchingFilter->SetNumberOfHistogramLevels( 256 );
	matchingFilter->SetNumberOfMatchPoints( 12 );
	matchingFilter->ThresholdAtMeanIntensityOn();
	matchingFilter->Update();

	outputImage = matchingFilter->GetOutput();
	outputImage->Update();

	typedef itk::MinimumMaximumImageCalculator<ImageType> CalculatorType;
	typename CalculatorType::Pointer calc = CalculatorType::New();
	calc->SetImage( inputImage );
	calc->ComputeMaximum();
	calc->ComputeMinimum();
	if( vnl_math_abs( calc->GetMaximum() - calc->GetMinimum() ) < 1.e-9 )
	    return histogramMatch;
    } else {
	outputImage = windowingFilter->GetOutput();
	outputImage->Update();
    }

    return outputImage;
}


template<typename TScalar>
int preprocessAnImage(lua_State *L, const char *path, const char *outpath, const char *match) {
    typedef itk::Image<TScalar, 3> ImageType;
    return writeImage<ImageType>(L, preprocess<ImageType>(readImage<ImageType>(path), 0, 1, 0.001, 0.999, readImage<ImageType>(match)), outpath);
}


#ifdef __cplusplus
extern "C" {
#endif


const char* checkpath(lua_State *L, int k) {
    lua_rawgeti(L, 1, k);
    return lua_tostring(L, -1);
}


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
    lua_pushinteger(L, imageIO->GetNumberOfComponents());
    lua_setfield(L, -2, "comps");
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

    // Dimensions
    lua_newtable(L);
    for(k=0; k<dims;) {
	lua_pushinteger(L, imageIO->GetDimensions(k));
	lua_rawseti(L, -2, ++k);
    }
    lua_setfield(L, -2, "bounds");

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


// preprocess an image by removing outliers (intensity peaks); is it similar to global thresholding from SPM?
static int preprocessImage(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const char *outpath = luaL_checkstring(L, 2);

    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(path, itk::ImageIOFactory::ReadMode);
    imageIO->SetFileName(path);
    imageIO->ReadImageInformation();
    itk::ImageIOBase::IOComponentType type = imageIO->GetComponentType();

    switch(type) {
	case itk::ImageIOBase::SHORT:   return preprocessAnImage<short>(L, path, outpath, lua_tostring(L, 3)); break;
	case itk::ImageIOBase::USHORT:  return preprocessAnImage<unsigned short>(L, path, outpath, lua_tostring(L, 3)); break;
	case itk::ImageIOBase::INT:	return preprocessAnImage<int>(L, path, outpath, lua_tostring(L, 3)); break;
	case itk::ImageIOBase::FLOAT:   return preprocessAnImage<float>(L, path, outpath, lua_tostring(L, 3)); break;
	case itk::ImageIOBase::DOUBLE:  return preprocessAnImage<double>(L, path, outpath, lua_tostring(L, 3)); break;
	default:
	    lua_pushnil(L);
	    lua_pushstring(L, "Error: Unsupported pixel type.\n");
	    return 2;
    }
}

/// average image series [normalize by average global mean value]
static int averageImages(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE); // images (paths)
    const int norm = lua_toboolean(L, 3); // normalize values
    const char *outpath = luaL_checkstring(L, 4); // output path name
    const int thresh = lua_tointeger(L, 5); // index of images for resampling

    const float numberofimages = (float)luaL_len(L, 1);
    unsigned int j = 1;

    typedef float PixelType; // pixel type is float by default
    typedef itk::Image<PixelType, 3> ImageType;
    typedef itk::ImageRegionIteratorWithIndex<ImageType> Iterator;
    typename ImageType::Pointer average;
    PixelType meanval = 0;

    average = readImage<ImageType>( checkpath(L, luaL_checkinteger(L, 2)) ); //index of image w maxSize
    average->FillBuffer( meanval );
    lua_pop(L, 1); // pop 'path' from stack

    for (j = 1; j < numberofimages; j++) {
	typename ImageType::Pointer img;
	if ((j-thresh) < 0)
	    img = resampleImage<ImageType>(checkpath(L, j), average);
	else
	    img = readImage<ImageType>(checkpath(L, j));
	lua_pop(L, 1); // pop 'path' from stack
	Iterator vfIter( img, img->GetLargestPossibleRegion() );
	unsigned long ct = 0;

	if (norm) {
	    meanval = 0;
	    for( vfIter.GoToBegin(); !vfIter.IsAtEnd(); ++vfIter, ++ct )
		meanval += img->GetPixel( vfIter.GetIndex() );
	    if (ct > 0)
		meanval /= (float)ct;
	    if (meanval <= 0)
		meanval = (1);
	}

	for( vfIter.GoToBegin(); !vfIter.IsAtEnd(); ++vfIter ) {
	    PixelType val = vfIter.Get();
	    if (norm)
		val /= meanval;
	    val = val / (float)numberofimages;
	    const PixelType & oldval = average->GetPixel(vfIter.GetIndex());
	    average->SetPixel(vfIter.GetIndex(), val+oldval);
	}
    }

    if (norm) {
	typedef itk::LaplacianSharpeningImageFilter<ImageType, ImageType> SharpeningFilter;
	typename SharpeningFilter::Pointer shFilter = SharpeningFilter::New();
	shFilter->SetInput(average);
	return  writeImage<ImageType>(L, shFilter->GetOutput(), outpath);
    }

    return writeImage<ImageType>(L, average, outpath);
}


//------------------------//

// AUXILIAR FNs

////////// Interface METHODs ///////////


// 3D Image Volume

//--------------WRITER-----------------//



//////////////////////////////

static const struct luaL_Reg itk_funcs[] = {
  {"info", getInfo},
  {"dcmSeries", dicomSeries},
  {"average", averageImages},
  {"preprocess", preprocessImage},
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


