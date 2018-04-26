#include "itkImage.h"
#include "itkVTKImageIO.h"
#include "itkSmartPointer.h"
#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkNiftiImageIO.h"

#include "itkResampleImageFilter.h"
#include "itkLaplacianSharpeningImageFilter.h"

#include "itkANTSNeighborhoodCorrelationImageToImageMetricv4.h"
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

#include "itkConjugateGradientLineSearchOptimizerv4.h"

#include "itkAffineTransform.h"
#include "itkEuler3DTransform.h"

#include "itkHistogramMatchingImageFilter.h"
#include "itkMinimumMaximumImageCalculator.h"

#include <lua.hpp>
#include <lauxlib.h>
#include <string.h>

#define catchMe(L) catch (itk::ExceptionObject &ex) { lua_pushnil(L); lua_pushfstring(L, "Exception found: %s\n", ex.what()); return 2; }

typedef float RealType;
typedef itk::ConjugateGradientLineSearchOptimizerv4 OptimizerType;

template<typename ImageType, unsigned int ImageDimension>
typename itk::Vector<RealType, ImageDimension> centerOfGravity(typename ImageType::Pointer inputImage) {
    typedef typename itk::ImageMomentsCalculator<ImageType> ImageCalculatorType;
    typename ImageCalculatorType::Pointer calc = ImageCalculatorType::New();
    calc->SetImage( inputImage );
    typename itk::Vector<RealType, ImageDimension> center;
    center.Fill(0);
    try {
	calc->Compute();
	center = calc->GetCenterOfGravity();
    } catch (itk::ExceptionObject &ex) {}
    return center;
}


template<typename ImageType, typename MetricType>
typename MetricType::Pointer newmetric(const char *mymetric, unsigned int radiusOrBins) {
    if (strcmp(mymetric, "cc") == 0) {
    	  typedef typename itk::ANTSNeighborhoodCorrelationImageToImageMetricv4<ImageType, ImageType> CorrelationMetricType;
    	  typename CorrelationMetricType::Pointer correlationMetric = CorrelationMetricType::New();
    	  typename CorrelationMetricType::RadiusType radius;
    	  radius.Fill( radiusOrBins );
    	  correlationMetric->SetRadius( radius );
    	  correlationMetric->SetUseMovingImageGradientFilter( false );
    	  correlationMetric->SetUseFixedImageGradientFilter( false );
    	  return correlationMetric;
    }
    else if (strcmp(mymetric, "mi") == 0) {
    	  typedef typename itk::MattesMutualInformationImageToImageMetricv4<ImageType, ImageType> MutualInformationMetricType;
    	  typename MutualInformationMetricType::Pointer mutualInformationMetric = MutualInformationMetricType::New();
    	  mutualInformationMetric->SetNumberOfHistogramBins( radiusOrBins );
    	  mutualInformationMetric->SetUseMovingImageGradientFilter( false );
    	  mutualInformationMetric->SetUseFixedImageGradientFilter( false );
    	  return mutualInformationMetric;
    }
    else if (strcmp(mymetric, "ms") == 0) {
    	  typedef typename itk::MeanSquaresImageToImageMetricv4<ImageType, ImageType> MeanSquaresMetricType;
    	  return MeanSquaresMetricType::New();
    }
    else if (strcmp(mymetric, "gc") == 0) {
    	  typedef typename itk::CorrelationImageToImageMetricv4<ImageType, ImageType> CorrMetricType;
    	  return CorrMetricType::New();
    } else {
    	  return ITK_NULLPTR;
    }
}

template <typename ImageType>
typename ImageType::Pointer readImage(const char* fname) {
    if (fname == NULL)
    	  return ITK_NULLPTR;

  // Read the image files begin
    typedef itk::ImageFileReader<ImageType> ImageFileReader;

  typename ImageFileReader::Pointer reader = ImageFileReader::New();
  reader->SetFileName( fname );
    const char *ext = strrchr(fname, '.');
    if (!(strcmp(ext, ".nii") && strcmp(ext, ".nii.gz"))) {
    	  typedef itk::NiftiImageIO ImageIOType;
    	  typename ImageIOType::Pointer niiIO = ImageIOType::New();
    	  reader->SetImageIO( niiIO );
    }

  try
    {
	reader->Update();
    }
  catch( itk::ExceptionObject & e )
    {
    std::cerr << "Exception caught during image reference file reading " << std::endl;
    std::cerr << e << std::endl;
	typename ImageType::Pointer ret;
	return ret;
    }

  return reader->GetOutput();
}

template <typename ImageType>
typename ImageType::Pointer resampleImage(typename ImageType::Pointer orig, typename ImageType::Pointer model) {
    typedef itk::ResampleImageFilter<ImageType, ImageType, RealType> ResamplerType;
    typename ResamplerType::Pointer resampler = ResamplerType::New();

//    typename ImageType::Pointer orig = readImage<ImageType>( fname );
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
	typename ImageType::Pointer ret;
	return ret;
    }

    return resampler->GetOutput();
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
    else if (!(strcmp(ext, "nii.") && strcmp(ext, ".nii.gz"))) {
    	  typedef itk::NiftiImageIO ImageIOType;
    	  typename ImageIOType::Pointer niiIO = ImageIOType::New();
    	  writer->SetImageIO( niiIO );
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
int convertImage(lua_State *L, const char *path, const char *outpath) {
    typedef itk::Image<TScalar, 3> ImageType;
    return writeImage<ImageType>(L, readImage<ImageType>(path), outpath);
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

    if (dims == 3) {  // Compute the center of Gravity
	typedef itk::Image<RealType, 3> ImageType;
	ImageType::Pointer image = readImage<ImageType>(path);
	itk::Vector<RealType, 3> center = centerOfGravity<ImageType, 3>(image);
	lua_newtable(L);
	for(k=0; k<3;) {
	    lua_pushnumber(L, center[k]);
	    lua_rawseti(L, -2, ++k);
	}
	lua_setfield(L, -2, "center");
    }

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
// (3) OutFilename,path [string]
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
    const char *match = lua_tostring(L, 3);

    typedef itk::Image<RealType, 3> ImageType;

    ImageType::Pointer ret = preprocess<ImageType>(readImage<ImageType>(path), 0, 1, 0.001, 0.999, readImage<ImageType>(match));
    return writeImage<ImageType>(L, ret, outpath);
}

static int vtk2nii(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    const char *outpath = luaL_checkstring(L, 2);

    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(path, itk::ImageIOFactory::ReadMode);
    imageIO->SetFileName(path);
    imageIO->ReadImageInformation();
    itk::ImageIOBase::IOComponentType type = imageIO->GetComponentType();

    switch(type) {
	case itk::ImageIOBase::SHORT:   return convertImage<short>(L, path, outpath);		break;
	case itk::ImageIOBase::USHORT:  return convertImage<unsigned short>(L, path, outpath);  break;
	case itk::ImageIOBase::INT:	return convertImage<int>(L, path, outpath); 		break;
	case itk::ImageIOBase::FLOAT:   return convertImage<float>(L, path, outpath); 		break;
	case itk::ImageIOBase::DOUBLE:  return convertImage<double>(L, path, outpath); 		break;
	default:
	    lua_pushnil(L);
	    lua_pushstring(L, "Error: Unsupported pixel type.\n");
	    return 2;
    }
}

/// average image series [normalize by average global mean value]
//
// input
// (1) A list of paths to indiviual images [table]
// (2) Index of Image with MaxSize [int]
// (3) Should values be normalied? [bool]
// (4) Output path name [string]
// (5) Index of images to be resampled [int]
static int averageImages(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE); // images (paths)
    const int norm = lua_toboolean(L, 3); // normalize values
    const char *outpath = luaL_checkstring(L, 4); // output path name
    const int resamp = lua_tointeger(L, 5); // index of images for resampling

    const float numberofimages = (float)luaL_len(L, 1);
    unsigned int j = 1;

    typedef itk::Image<RealType, 3> ImageType;
    typedef itk::ImageRegionIteratorWithIndex<ImageType> Iterator;
    ImageType::Pointer average;
    RealType meanval = 0;

    average = readImage<ImageType>( checkpath(L, luaL_checkinteger(L, 2)) ); //index of image w maxSize
    average->FillBuffer( meanval );
    lua_pop(L, 1); // pop 'path' from stack

    for (j = 1; j < numberofimages; j++) {
	ImageType::Pointer ret = readImage<ImageType>(checkpath(L, j));
	ImageType::Pointer img;
	if ((j-resamp) < 0)
	    img = resampleImage<ImageType>(ret, average);
	else
	    img = ret;
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
	    RealType val = vfIter.Get();
	    if (norm)
		val /= meanval;
	    val = val / (float)numberofimages;
	    const RealType & oldval = average->GetPixel(vfIter.GetIndex());
	    average->SetPixel(vfIter.GetIndex(), val+oldval);
	}
    }

    if (norm) {
	typedef itk::LaplacianSharpeningImageFilter<ImageType, ImageType> SharpeningFilter;
	SharpeningFilter::Pointer shFilter = SharpeningFilter::New();
	shFilter->SetInput(average);
	return  writeImage<ImageType>(L, shFilter->GetOutput(), outpath);
    }

    return writeImage<ImageType>(L, average, outpath);
}


/// Register moving image to fixed image
//
// input
// (1) Fixed image path [string]
// (2) Moving image path [string]
// (3) Metric to be applied [string]
// (4) Registration method [string]
// (5) Extra parameters [integer]
// (6) Should resampling be perfomed? [bool]
// (7) Output path name [string]
static int registerImage(lua_State *L) {
    const char *fixedPath 		= luaL_checkstring(L, 1);
    const char *movingPath 		= luaL_checkstring(L, 2);
    const char *outputPath 		= luaL_checkstring(L, 3);
    const char *metricStr 		= luaL_checkstring(L, 4);
    const unsigned int radiusOrBins 	= luaL_checkinteger(L, 5);
    const char *transform 		= luaL_checkstring(L, 6);

    const unsigned int ImageDimension = 3;
    typedef itk::Image<RealType, ImageDimension> ImageType;
    typedef itk::ImageToImageMetricv4<ImageType, ImageType> MetricType;
    typedef itk::AffineTransform<RealType, ImageDimension> AffineTransformType;
//    typedef itk::ImageRegistrationMethodv4<ImageType, ImageType, AffineTransformType> AffineRegistrationType;
    typedef itk::ConjugateGradientLineSearchOptimizerv4 OptimizerType;
    typedef itk::RegistrationParameterScalesFromPhysicalShift<MetricType> ScalesEstimatorType;
    typedef itk::Vector<RealType, ImageDimension> VectorType;

    float learningRate = 0.0; //  gradient step aka learning rate
    unsigned int estimateLearning = 1;
    unsigned int levels = 1; // it can be deduced from size(iterations)
    unsigned int niters = 20; // n of iterations per level / the optimizer

    float samplingPercentage = 1.0;

    // Read images
    ImageType::Pointer fixedImage = readImage<ImageType>( fixedPath );
    ImageType::Pointer movingImage = readImage<ImageType>( movingPath );

    // Initial estimates for registration using an Affine transform
    AffineTransformType::OffsetType offset;
    itk::Point<RealType, ImageDimension> center;
    {
	VectorType fixed_center = centerOfGravity<ImageType, ImageDimension>(fixedImage);
	VectorType moving_center = centerOfGravity<ImageType, ImageDimension>(movingImage);
	lua_newtable(L); // offset (-2) (-4)
	lua_newtable(L); // center (-1) (-3)
	unsigned int i;
	for(i=0; i<ImageDimension;){
	    RealType fixedCenter = fixed_center[i];
	    center[i] = fixedCenter;
	    offset[i] = moving_center[i] - fixedCenter;
	}
    }

    // Metric
    MetricType::Pointer metric = newmetric<ImageType, MetricType>(metricStr, radiusOrBins);
    metric->SetVirtualDomainFromImage( fixedImage );

    // Optimizer
    OptimizerType::Pointer optimizer = OptimizerType::New();
    optimizer->SetNumberOfIterations( niters );
    optimizer->SetMinimumConvergenceValue( 1.0e-7 );
    optimizer->SetConvergenceWindowSize( 10 );
    optimizer->SetLowerLimit( 0 );
    optimizer->SetUpperLimit( 2 );
    optimizer->SetEpsilon( 0.1 );
    optimizer->SetMaximumStepSizeInPhysicalUnits( learningRate );
    optimizer->SetDoEstimateLearningRateOnce( estimateLearning );
    optimizer->SetDoEstimateLearningRateAtEachIteration( !estimateLearning );

    // Normalizaing parameter space; useful during Optimization
    ScalesEstimatorType::Pointer scalesEstimator = ScalesEstimatorType::New();
    {
	scalesEstimator->SetMetric( metric );
	scalesEstimator->SetTransformForward( true );
//	optimizer->SetScalesEstimator( scalesEstimator ); // Employ Scales Estimator
    }

    if( strcmp(transform, "rigid") == 0 ) {
	typedef itk::Euler3DTransform<double> RigidTransformType;
	RigidTransformType::Pointer rigidTransform = RigidTransformType::New();
	typedef typename itk::ImageRegistrationMethodv4<ImageType, ImageType, RigidTransformType> RigidRegistrationType;
	typename RigidRegistrationType::Pointer rigidRegistration = RigidRegistrationType::New();

	rigidTransform->SetOffset( offset );
	rigidTransform->SetCenter( center );

	{
	    ImageType::Pointer preprocessFixedImage  = preprocess<ImageType>(fixedImage, 0, 1, 0.001, 0.999, ITK_NULLPTR);
	    ImageType::Pointer preprocessMovingImage = preprocess<ImageType>(movingImage, 0, 1, 0.001, 0.999, ITK_NULLPTR);
	    metric->SetFixedImage( preprocessFixedImage );
	    metric->SetVirtualDomainFromImage( preprocessFixedImage );
	    metric->SetMovingImage( preprocessMovingImage );
	    metric->SetMovingTransform( rigidTransform );
	    rigidRegistration->SetFixedImage( preprocessFixedImage );
	    rigidRegistration->SetMovingImage( preprocessMovingImage );
	    rigidRegistration->SetNumberOfLevels( levels );
	}

	{ // set shrink factors and smoothing sigmas / per level XXX REFACTOR because it is used for every registration type
	    RigidRegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
	    shrinkFactorsPerLevel.SetSize( 1 ); // shrink factors per level
	    shrinkFactorsPerLevel[0] = 1;
	    RigidRegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
	    smoothingSigmasPerLevel.SetSize( 1 ); // smoothing sigmas per level
	    smoothingSigmasPerLevel[0] = 0;
	    rigidRegistration->SetShrinkFactorsPerLevel( shrinkFactorsPerLevel );
	    rigidRegistration->SetSmoothingSigmasPerLevel( smoothingSigmasPerLevel );
	}


	{ // set metric sampling strategy XXX REFACTOR because it is used for every registration type
	    const char *samplingStrategy = "random";
	    RigidRegistrationType::MetricSamplingStrategyType metricSamplingStrategy = RigidRegistrationType::MetricSamplingStrategyType::NONE;
	    if( strcmp(samplingStrategy, "random") == 0 )
		metricSamplingStrategy = RigidRegistrationType::MetricSamplingStrategyType::RANDOM;
	    else if( strcmp(samplingStrategy, "regular") == 0 )
		metricSamplingStrategy = RigidRegistrationType::MetricSamplingStrategyType::REGULAR;
	    rigidRegistration->SetMetricSamplingStrategy( metricSamplingStrategy );
	}

	// Setting scales estimator for Optimzier & Initializing metric
	{
	    ScalesEstimatorType::ScalesType scales( rigidTransform->GetNumberOfParameters() );
	    MetricType::ParametersType newparams( rigidTransform->GetParameters() );
	    metric->SetParameters( newparams );
	    metric->Initialize();
	    scalesEstimator->SetMetric( metric );
	    scalesEstimator->EstimateScales( scales );
	    optimizer->SetScales( scales );
	}

	rigidRegistration->SetMetric( metric );
	rigidRegistration->SetMetricSamplingPercentage( samplingPercentage );
	rigidRegistration->SetOptimizer( optimizer );

	try {
	    rigidRegistration->Update();

	    typedef itk::ResampleImageFilter<ImageType, ImageType, double> ResamplerType;
	    ResamplerType::Pointer resampler = ResamplerType::New();
	    resampler->SetTransform( rigidRegistration->GetModifiableTransform() );
	    resampler->SetInput( movingImage );
	    resampler->SetOutputParametersFromImage( fixedImage );
	    resampler->SetDefaultPixelValue( 0 );
	    resampler->Update();

	    return writeImage<ImageType>(L, resampler->GetOutput(), outputPath);
	} catchMe(L)
    }

    lua_pushnil(L);
    return 1;
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
  {"register", registerImage},
  {"vtk2nii", vtk2nii},
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
template<typename TScalar>
int preprocessAnImage(lua_State *L, const char *path, const char *outpath, const char *match) {
    typedef itk::Image<TScalar, ImageDimension> ImageType;
    return writeImage<ImageType>(L, preprocess<ImageType>(readImage<ImageType>(path), 0, 1, 0.001, 0.999, readImage<ImageType>(match)), outpath);
}

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


/*
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



static int shortNew(lua_State *L) {
    typedef itk::Image<short, 3> ImageType;
    ImageType::Pointer *img = newimg(L, ImageType::Pointer, short);
    *img = ImageType::New();
    return 1;
}
*/

