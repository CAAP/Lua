#include <opencv2/opencv.hpp>

#include <vector>

using namespace std;

#include <lua.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#define checkmat(L,i) *(cv::Mat **)luaL_checkudata(L, i, "caap.opencv.mat")
#define checkcont(L,i) *(cv::Mat **)luaL_checkudata(L, i, "caap.opencv.cont")
#define checkrect(L,i) (cv::Rect *)luaL_checkudata(L, i, "caap.opencv.rect")
#define checkrrect(L,i) (cv::RotatedRect *)luaL_checkudata(L, i, "caap.opencv.rrect")
#define newmat(L) (cv::Mat **)lua_newuserdata(L, sizeof(cv::Mat *)); luaL_getmetatable(L, "caap.opencv.mat"); lua_setmetatable(L, -2)
#define newcont(L) (cv::Mat **)lua_newuserdata(L, sizeof(cv::Mat *)); luaL_getmetatable(L, "caap.opencv.cont"); lua_setmetatable(L, -2)
#define newrect(L) (cv::Rect *)lua_newuserdata(L, sizeof(cv::Rect)); luaL_getmetatable(L, "caap.opencv.rect"); lua_setmetatable(L, -2)
#define newrrect(L) (cv::RotatedRect *)lua_newuserdata(L, sizeof(cv::RotatedRect)); luaL_getmetatable(L, "caap.opencv.rrect"); lua_setmetatable(L, -2)
#define check81(L, m) if ( (m->depth() > CV_8S) | (m->channels() > 1) ) luaL_error(L, "Only single channel and 8-bit images.")
#define check83(L, m) if ( (m->depth() > CV_8S) | (m->channels() != 3) ) luaL_error(L, "Only 8-bit 3-channel images.")

/*
static int cvtype(int pf) {
    switch (pf) {
	case gdcm::PixelFormat::UINT8: return CV_8U; // uchar
	case gdcm::PixelFormat::INT8: return CV_8S; // schar
	case gdcm::PixelFormat::UINT16: return CV_16U; // ushort
	case gdcm::PixelFormat::INT16: return CV_16S; // short
	case gdcm::PixelFormat::INT32: return CV_32S; // int
	case gdcm::PixelFormat::FLOAT32: return CV_32F; // float
	case gdcm::PixelFormat::FLOAT64: return CV_64F; // double
	default: return -1;
    }
}
*/

static int seti(lua_State *L, int i, const vector<cv::Point>& points) {
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(points, true);
    lua_rawseti(L, -2, i);
    return 0;
}

static int geti(lua_State *L, int i) {
    lua_rawgeti(L, -1, i);
    int ret = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return ret;
}

static int getfield(lua_State *L, const char *key) {
    lua_getfield(L, -1, key);
    int ret = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return ret;
}

static int zeros(lua_State *L, int rows, int cols) {
    cv::Mat ret = cv::Mat::zeros(rows, cols, CV_8UC1);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(ret);
    return 1;
}

static int ones(lua_State *L, int rows, int cols) {
    int factor = 1;
    if (lua_gettop(L) > 1)
	    factor = luaL_checkinteger(L, 2);
    cv::Mat ret = cv::Mat::ones(rows, cols, CV_8UC1) * factor;
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(ret);
    return 1;
}

static void byteOrFloat(cv::Mat *m, cv::Mat &target) {
    if ((m->depth() == CV_8U) | (m->depth() == CV_32F))
	target = cv::Mat(*m);
    else
	m->convertTo(target, CV_32F);
}

static void correctFormat(bool tobyte, cv::Mat *m, cv::Mat &target) {
    if (tobyte) 
	m->convertTo(target, CV_8U);
    else if(m->depth() != CV_8U)
	m->convertTo(target, CV_32F);
    else
	target = cv::Mat(*m); // header only
}

static const char* depth2str(int x) {
    switch (x) {
	case CV_8U : return "8U"; //uchar
	case CV_8S : return "8S"; //char
	case CV_16U : return "16U"; //ushort
	case CV_16S: return "16S"; //short
	case CV_32S: return "32S"; //int
	case CV_32F: return "32F"; //float
	case CV_64F: return "64F"; //double
	default: return NULL;
    }
}

/* ******************************** */

/* ************* MAT ** ********** */

static int streaming(lua_State *L) {
    cv::VideoCapture *vc = *(cv::VideoCapture **)lua_touserdata(L, lua_upvalueindex(1));
    cv::Mat frame;

    if (!(vc->isOpened()) || !(vc->read(frame)))
	return 0;

    lua_pushinteger(L, vc->get(CV_CAP_PROP_POS_FRAMES)); // MSEC
    cv::Mat **m = newmat(L);
    *m = new cv::Mat(frame);
    return 2;
}

static int videoCapture(lua_State *L) {
    const char *fname = luaL_checkstring(L, 1);

    cv::VideoCapture cap( fname );
    if (!cap.isOpened()) luaL_error(L, "Unable to open video file %s", fname);

    cv::VideoCapture **vcp = (cv::VideoCapture **)lua_newuserdata(L, sizeof(cv::VideoCapture *));
    *vcp = new cv::VideoCapture(cap);

    lua_pushcclosure(L, &streaming, 1); // VideoCapture

    return 1;
}

static int fromTable(lua_State *L) {
    int rows = luaL_checkinteger(L, 1);
    int cols = luaL_checkinteger(L, 2);
    int type = (luaL_checkstring(L, 3)[0] == 'd') ? CV_64F : CV_32S;
    luaL_checktype(L, 4, LUA_TTABLE);
    int N = luaL_len(L, 4);

    if (N != (rows*cols))
	luaL_error(L, "Number of (rows x cols) not equal to length of table.");

    cv::Mat ret;

    if (type == CV_64F) {
	double *data = (double *)lua_newuserdata(L, N*sizeof(double));
	for (unsigned int k = 0; k < N; k++) {
	    lua_rawgeti(L, 4, k+1);
	    data[k] = lua_tonumber(L, -1);
	    lua_pop(L, 1);
	}
	ret = cv::Mat(rows, cols, type, (void *)data);
    } else {
	int *data = (int *)lua_newuserdata(L, N*sizeof(int));
	for (unsigned int k = 0; k < N; k++) {
	    lua_rawgeti(L, 4, k+1);
	    data[k] = lua_tointeger(L, -1);
	    lua_pop(L, 1);
	}
	ret = cv::Mat(rows, cols, type, (void *)data);
    }

    cv::Mat **m = newmat(L);
    *m = new cv::Mat(ret);
    return 1;
}

static int rotationMatrix(lua_State *L) {
    float x = luaL_checknumber(L, 1);
    float y = luaL_checknumber(L, 2);
    double angle = luaL_checknumber(L, 3);
    cv::Point2f center(x, y);
    cv::Mat ret = cv::getRotationMatrix2D(center, angle, 1.0);
    cv::Mat **m = newmat(L);
    *m = new cv::Mat(ret);
    return 1;
}

static int openImage(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    int flag = CV_LOAD_IMAGE_ANYDEPTH;

    if (lua_gettop(L) > 1)
	    switch(luaL_checkstring(L, 2)[0]) {
		    case 'C': flag = CV_LOAD_IMAGE_COLOR; break;
		    case 'G': flag = CV_LOAD_IMAGE_GRAYSCALE; break;
		    case 'A': flag = -1; break;
	    }

    cv::Mat m = cv::imread(filename, flag); //cv::imread(filename, flag);
    if ( m.empty() ) {
	    lua_pushnil(L);
	    lua_pushfstring(L, "Error opening file: %s.", filename);
	    return 2;
    }

    cv::Mat **um = newmat(L);
    *um = new cv::Mat(m);
    return 1;
}

/*
static int fromGDCM(lua_State *L) {
    char *udata = (char *)luaL_checkudata(L, 1, "caap.gdcm.image");
    lua_getuservalue(L, 1);
    const unsigned int cols = getfield(L, "dimX");
    const unsigned int rows = getfield(L, "dimY");
    unsigned int type = cvtype( getfield(L, "type") );
    const int intercept = (int)getfield(L, "intercept");

    if (type == CV_16U) { type = CV_16S; }

    cv::Mat m = cv::Mat(rows, cols, type, (void *)udata);
    
    if ( m.empty() )
	    luaL_error(L, "Cannot read image from GDCM data buffer.");

    if ( intercept )
	    m = m + intercept;

    // XXX Add Slope Adjustments
    
//    if ( slope )
//	;


    cv::Mat **um = newmat(L);
//    if (type == CV_16U) { m.convertTo( m, CV_16S ); }
//    if ( intercept != 0 ) { m = m + intercept; }
    *um = new cv::Mat(m);

    return 1;
}
*/

//////////////////////////////


static int mat2zeros(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    return zeros(L, m->rows, m->cols);
}

static int mat2ones(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    return ones(L, m->rows, m->cols);
}

static int applyLUT(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    check81(L, m); // Only char images!!!
    luaL_checktype(L, 2, LUA_TTABLE);
    int k = luaL_len(L, 2);
    if (k != 256)
	luaL_error(L, "Second arguments must be a 256-table.");
    int lut[256];
    for (k=0; k<256; k++)
	lut[k] = geti(L, k+1);
    cv::Mat dst = cv::Mat::zeros( m->size(), CV_8UC1 );
    uchar *tgt = dst.data, *org = m->data;
    for (unsigned int k = 0; k < (m->rows * m->cols); k++)
	tgt[k] = lut[org[k]]; // alt: *tgt++ = lut[*org++];
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int applyOverlay(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Mat *over = checkmat(L, 2);
    cv::Mat rgb( m->size(), CV_8UC3 );

    cv::Mat two = m->clone();
    over->copyTo(two, *over);

    if (lua_gettop(L) == 2) {
	cv::Mat input[] = { *m, two};
	int from_to[] = { 0,0, 0,1, 1,2 };
	cv::mixChannels(input, 2, &rgb, 1, from_to, 3);
    } else {
	cv::Mat *over2 = checkmat(L, 3);
	cv::Mat tre = m->clone();
	over2->copyTo(tre, *over2);

	cv::Mat input[] = { *m, two, tre };
	cv::merge(input, 3, rgb);
    }


    cv::Mat **um = newmat(L);
    *um = new cv::Mat(rgb);
    return 1;
}

static int copyTo(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Mat dst;
    if (lua_gettop(L) > 1) {
	cv::Mat *mask = checkmat(L, 2);
	m->copyTo(dst, *mask);
    } else
	m->copyTo(dst);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int assignTo(lua_State *L) {
    cv::Mat *m1 = checkmat(L, 1);
    cv::Mat *m2 = checkmat(L, 2);
    if (lua_gettop(L) > 2) {
	cv::Mat *mask = checkmat(L, 3);
	m1->copyTo(*m2, *mask);
	return 0;
    }
    m1->copyTo(*m2);
    return 0;
}

static int bitwise(lua_State *L) {
    cv::Mat *m1 = checkmat(L, 1);
    cv::Mat *m2 = checkmat(L, 2);
    const char op = luaL_checkstring(L, 3)[0];

    cv::Mat dst;
    switch(op) {
	case 'A': cv::bitwise_and(*m1, *m2, dst); break;
	case 'O': cv::bitwise_or(*m1, *m2, dst); break;
	case 'N': cv::bitwise_not(*m1, dst); break;
	case 'X': cv::bitwise_xor(*m1, *m2, dst); break;
    }
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int add(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    double a = luaL_checknumber(L, 2);
    cv::Mat *mask = checkmat(L, 3);
    bool inplace = false;
    if (lua_gettop(L) > 3)
	    inplace = lua_toboolean(L, 4);

    if (inplace) {
	    cv::add(*m, cv::Scalar(a), *m, *mask);
	    return 0;
    } else {
	    cv::Mat dst;
	    cv::add(*m, cv::Scalar(a), dst, *mask);
	    cv::Mat **um = newmat(L);
	    *um = new cv::Mat(dst);
	    return 1;
    }
}

static int addWeighted(lua_State *L) {
    cv::Mat *m1 = checkmat(L, 1);
    cv::Mat *m2 = checkmat(L, 2);
    double alpha = luaL_checknumber(L, 3);
    cv::Mat dst;
    cv::addWeighted(*m1, alpha, *m2, (1-alpha), 0.0, dst);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int countNonZero(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    if (m->channels() > 1)
	luaL_error(L, "Only single channel images.");
    lua_pushinteger(L, cv::countNonZero(*m));
    return 1;
}

// can be changed to return different results for each channel
static int mean(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    if (lua_gettop(L) > 1) {
	    cv::Mat *mask = checkmat(L,2);
	    lua_pushnumber(L, cv::mean(*m, *mask)[0]);
    } else
	    lua_pushnumber(L, cv::mean(*m)[0]);
    return 1;
}

static int stdev(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Scalar mean, stdev;
    if (lua_gettop(L) > 1) {
	    cv::Mat *mask = checkmat(L,2);
	    cv::meanStdDev(*m, mean, stdev, *mask);
    } else
	    cv::meanStdDev(*m, mean, stdev);
    lua_pushnumber(L, mean[0]);
    lua_pushnumber(L, stdev[0]);
    return 2;
}

//////// PCA //////

static int doPCA(lua_State *L) {
    cv::Mat data, *m = checkmat(L, 1);
    int asrows = 1, nargs = lua_gettop(L);
    double var = 0.9;

    //if not orientation given then default to ROW
    if (nargs > 2)
	asrows = (luaL_checkstring(L, 3)[0] == 'c') ? 0 : 1;

    //if not # of components OR variance given then default to 2 components
    if (nargs > 3)
	var = luaL_checknumber(L, 4);

    if ((var <= 0) || (var > 1))
	luaL_error(L, "Variance cannot be greater than 1 nor less than 0.");

    // if not type float then convert to float
    if (m->depth() < CV_32F)
	m->convertTo(data, CV_32F);
    else
	data = *m;

    //if not mean value given then compute it
    if (nargs > 1) {
        cv::Mat *mean = checkmat(L, 2);
	cv::PCA pca(data, *mean, (asrows ? cv::PCA::DATA_AS_ROW : cv::PCA::DATA_AS_COL), var);
    }

    cv::PCA pca(data, cv::noArray(), (asrows ? cv::PCA::DATA_AS_ROW : cv::PCA::DATA_AS_COL), var);

    cv::Mat **eigvals = newmat(L);
    *eigvals = new cv::Mat(pca.eigenvalues);
    cv::Mat **eigvecs = newmat(L);
    *eigvecs = new cv::Mat(pca.eigenvectors);
    cv::Mat **mean = newmat(L);
    *mean = new cv::Mat(pca.mean);
    return 3;
}

//////////////

static int floodFill(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    int x = luaL_checkinteger(L, 2), y = luaL_checkinteger(L, 3);
    int lo = luaL_checkinteger(L, 4), up = luaL_checkinteger(L, 5);

    cv::Mat **um = newmat(L);
    *um = new cv::Mat(m->clone());

    cv::Rect ccomp;
    int flags = 4 + (255 << 8) + 0;
    cv::Point seed = cv::Point(x, y);
    int area = floodFill(**um, seed, cv::Scalar(255, 51, 51), &ccomp, cv::Scalar(lo, lo, lo), cv::Scalar(up, up, up), flags);

    lua_pushinteger(L, area);
    return 2;
}

static int equalizeHist(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    check81(L, m);
    cv::Mat dst;
    cv::equalizeHist(*m, dst);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int morphology(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    const char opt1 = luaL_checkstring(L, 2)[0];
    const char opt2 = luaL_checkstring(L, 3)[0];
    int iter = luaL_checkinteger(L, 4);
    int shape;
    switch(opt2) {
	case 'E': shape = cv::MORPH_ELLIPSE; break;
	case 'R': shape = cv::MORPH_RECT; break;
	case 'C': shape = cv::MORPH_CROSS; break;
    }
    cv::Mat element = cv::getStructuringElement(shape, cv::Size(iter*2+1, iter*2+1), cv::Point(iter, iter));
    cv::Mat dst;
    switch(opt1) {
	case 'E': cv::erode(*m, dst, element); break;
	case 'D': cv::dilate(*m, dst, element); break;
	case 'O': cv::morphologyEx(*m, dst, cv::MORPH_OPEN, element); break;
	case 'C': cv::morphologyEx(*m, dst, cv::MORPH_CLOSE, element); break;
	case 'T': cv::morphologyEx(*m, dst, cv::MORPH_TOPHAT, element); break;
	case 'B': cv::morphologyEx(*m, dst, cv::MORPH_BLACKHAT, element); break;
	case 'G': cv::morphologyEx(*m, dst, cv::MORPH_GRADIENT, element); break;
    }
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int median(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    int ksize = 3;
    if (lua_gettop(L) > 1)
	    ksize = luaL_checkinteger(L, 2);
    if ((ksize%2 == 0) | (ksize < 2))
	    luaL_error(L, "Aperture size must be odd and greater than 1.");
    if ((ksize > 5) & (m->depth() != CV_8U))
	    luaL_error(L, "For larger apertures, it can only be CV_8U.");
    cv::Mat dst;
    cv::medianBlur(*m, dst, ksize);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int gaussian(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Size ksize(3, 3);
    double sx = 0, sy = 0;

    if (lua_gettop(L)>1) {
	int k = luaL_checkinteger(L, 2);
	if (k%2 == 0)
	    luaL_error(L, "Kernel size must be odd.");
	ksize = cv::Size(k, k);
    }

    cv::Mat dst;
    cv::GaussianBlur(*m, dst, ksize, sx, sy, cv::BORDER_DEFAULT);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int laplacian(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);

    int ksize = 1;
    if (lua_gettop(L)> 1) {
	ksize = luaL_checkinteger(L, 2);
    }
    if ((ksize > 31) | (ksize%2 == 0))
	    luaL_error(L, "Kernel size must be odd and less than 31.");
    cv::Mat dst;
    cv::Laplacian(*m, dst, m->depth(), ksize);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int sobel(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    bool isx = true;
    int ksize = 3;
    
    if (lua_gettop(L) > 1) {
	    const char type = luaL_checkstring(L, 2)[0];
	    if (!((type == 'x') | (type == 'y')))
		    luaL_error(L, "Sobel derivative must be x- or y- direction only.");
	    if (type == 'y')
		    isx = false;
    }

    if (lua_gettop(L) > 2) {
	    ksize = luaL_checkinteger(L, 3);
    }

    cv::Mat ret;
    if (isx)
	    cv::Sobel(*m, ret, -1, 1, 0, ksize);
    else
	    cv::Sobel(*m, ret, -1, 0, 1, ksize);

    cv::Mat **um = newmat(L);
    *um = new cv::Mat(ret);
    return 1;
}

static int canny(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    check81(L, m);
    double thresh1 = luaL_checknumber(L, 2);
    double thresh2  = luaL_checknumber(L, 3);
    int aperture = 3;
    if (lua_gettop(L) > 3)
	    aperture = luaL_checkinteger(L, 4);
    if (thresh1 == thresh2)
	    luaL_error(L, "Threshold1 cannot be equal to threshold2.");
    if ((aperture < 1) | (aperture > 7) | (aperture%2 == 0))
	    luaL_error(L, "Aperture must be 1, 3, 5, or 7.");
    cv::Mat edges;
    cv::Canny(*m, edges, thresh1, thresh2, aperture);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(edges);
    return 1;
}

static int threshold(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    if( (m->channels()>1) | ((m->depth()>CV_8S) & (m->depth()<CV_32S)) )
	    luaL_error(L, "Must be single channel and 8/32-bit.");
    const char type = luaL_checkstring(L, 2)[0];

    int opts;
    switch(type) {
	case 'Z': opts = cv::THRESH_TOZERO; break;
	case 'B': opts = cv::THRESH_BINARY; break;
	case 'T': opts = cv::THRESH_TRUNC; break;
    }

    double thresh = -1.0;
    double maxv = 255.0;
    if (lua_gettop(L) > 2)
	thresh = luaL_checknumber(L, 3);
    else
	opts += cv::THRESH_OTSU;

    cv::Mat dst;
    cv::threshold(*m, dst, thresh, maxv, opts);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int minMax(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    double minv, maxv;
    if (lua_gettop(L) > 1) {
	    cv::Mat *mask = checkmat(L, 2);
	    cv::minMaxLoc(*m, &minv, &maxv, NULL, NULL, *mask);
    } else
	    cv::minMaxLoc(*m, &minv, &maxv);
    lua_pushnumber(L, minv);
    lua_pushnumber(L, maxv);
    return 2;
}

static int inRange(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Scalar lower = cv::Scalar( luaL_checknumber(L, 2) );
    cv::Scalar upper = cv::Scalar( luaL_checknumber(L, 3) );

    cv::Mat bw = cv::Mat::zeros( m->size(), CV_8UC1 );
    cv::inRange(*m, lower, upper, bw);

    cv::Mat **um = newmat(L);
    *um = new cv::Mat(bw);
    return 1;
}

// what about histogram object predefined in OpenCV XXX?
static int getHistogram(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);

    int histSize[] = {32};
    float range[] = {0, 256};
    const float* ranges[] = { range };
    bool uniform = true, accumulate = false;
    cv::Mat hist, tgt;

    bool tobyte = true;
    if (lua_gettop(L) > 3) // four args at least
	histSize[0] = luaL_checkinteger(L, 4);
    if (lua_gettop(L) > 2) { // three args at least
	range[0] = luaL_checknumber(L, 2);
	range[1] = luaL_checknumber(L, 3);
	tobyte = false;
    }
    correctFormat(tobyte, m, tgt);

    cv::Mat *mask;
    if (lua_gettop(L) > 4)
	mask = checkmat(L, 5);
    else
	mask = new cv::Mat();

    cv::calcHist( &tgt, 1, 0, *mask, hist, 1, histSize, ranges, uniform, accumulate );
    double sum = cv::sum(hist)[0];
    
    if (sum == 0)
	lua_pushnil(L);
    else {
	lua_newtable(L);
	int k = 1;
	cv::MatConstIterator_<float> it = hist.begin<float>(), it_end = hist.end<float>();
	for (; it != it_end; ++it) {
	    lua_pushinteger(L, *it);
//	    lua_pushnumber(L, *it/sum);
	    lua_rawseti(L, -2, k++);
	}
    }

    return 1;
}

static int watershed(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    check83(L, m);
    cv::Mat *priors = checkmat(L, 2); //32-bit single channel
    if (priors->depth() != CV_32S)
	    luaL_error(L, "32-bit single channel image expected.");
    cv::watershed(*m, *priors);
    return 1;
}

// MAYBE BETTER WRITTEN if each contour could write to the same image when called
static int drawContours(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    int k, cts = luaL_len(L, 2); // number of contours
    vector<vector<cv::Point> > contours = vector<vector<cv::Point> >(cts);
    for (k=1; k<= cts; k++) {
	lua_rawgeti(L, 2, k);
	cv::Mat *contour = checkcont(L, -1);
	contours[k-1] = *contour;
	lua_pop(L, 1);
    }

    cv::Mat dest;
    if (lua_gettop(L) > 2) {
        dest = cv::Mat::zeros( m->size(), CV_32S);
        for (k=0; k< cts; k++)
	    cv::drawContours( dest, contours, k, cv::Scalar(k+1), CV_FILLED, 8 );
    } else {
	dest = cv::Mat::zeros( m->size(), CV_8UC1);
	cv::drawContours( dest, contours, -1, cv::Scalar(255), CV_FILLED, 8 );
    }

    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dest);
    return 1;
}

static int cloneMe(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(m->clone());
    return 1;
}

static int gray2rgb(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Mat dst;
    cv::cvtColor(*m, dst, CV_GRAY2RGB);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int rgb2gray(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    check83(L, m);
    cv::Mat dst;
    cv::cvtColor(*m, dst, CV_RGB2GRAY);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int convert2byte(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Mat dst;
    double minv, maxv;
    if (lua_gettop(L) > 1) {
	    cv::Mat *mask = checkmat(L, 2);
	    cv::minMaxLoc(*m, &minv, &maxv, NULL, NULL, *mask);
    } else
	    cv::minMaxLoc(*m, &minv, &maxv);
    double alpha = 255.0/(maxv-minv);
    double beta = -1.0*minv*alpha;
    m->convertTo(dst, CV_8U, alpha, beta);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int convert2float(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Mat dst;
    m->convertTo(dst, CV_32F);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

// FIX: try converting S -> U; by i.e. type(m) % 2 == 0 then type(m)-1
static int convert2unsigned(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Mat dst;
    m->convertTo(dst, CV_8U);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int saveImage(lua_State *L) {
  cv::Mat *m = checkmat(L, 1);
  const char *filename = luaL_checkstring(L, 2);
  cv::imwrite(filename, *m);
  return 0;
}

static int release(lua_State *L) {
    cv::Mat *m = *(cv::Mat **)lua_touserdata(L, 1); //checkmat(L, 1);
    if (m != NULL) {
	m->release();
	m = NULL;
    }
    return 0;
}

static int mat2rect(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Rect *r = newrect(L);
    r->x = 0; r->y = 0; r->width = m->cols; r->height = m->rows;
    return 1;
}

static int mat2len(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    lua_pushinteger(L, m->rows * m->cols);
    return 1;
}

static int mat2str(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    lua_pushfstring(L, "ocvImage{ rows = %d, cols = %d, depth = '%s', channels = %d }", m->rows, m->cols, depth2str(m->depth()), m->channels());
    return 1;
}

static int mat2table(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    unsigned int rows = m->rows, cols = m->cols;
    int depth = m->depth();

    if (m->isContinuous()) { cols = cols * rows; rows = 1; }

    unsigned int k = 1;
    cv::Mat ret;
    lua_newtable(L);

    if (depth == CV_32F || depth == CV_64F) {
	m->convertTo(ret, CV_64F); // convert to Double
	double *p;
	for (unsigned int i = 0; i < rows; i++) {
	    p = ret.ptr<double>(i);
	    for (unsigned int j = 0; j < cols; j++) {
		lua_pushnumber(L, p[j]); // push Double
		lua_rawseti(L, 2, k++);
	    }
	}
    } else {
	m->convertTo(ret, CV_32S); // convert to Int
	int *p;
	for (unsigned int i = 0; i < rows; i++) {
	    p = ret.ptr<int>(i);
	    for (unsigned int j = 0; j < cols; j++) {
		lua_pushinteger(L, p[j]); // push Int
		lua_rawseti(L, 2, k++);
	    }
	}
    }

    return 1;
}

/* ****************************** */

/* ********* CONTOURS ********* */

static int findContours(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    check81(L, m);
    int mode = CV_RETR_CCOMP;

    if (lua_gettop(L)>1) {
	const char arg = luaL_checkstring(L, 2)[0];
	switch(arg) {
	    case 'E': mode = CV_RETR_EXTERNAL; break;
	    case 'L': mode = CV_RETR_LIST; break;
	    case 'C': mode = CV_RETR_CCOMP; break;
	    case 'T': mode = CV_RETR_TREE; break;
	}
    }
    
    vector<vector<cv::Point> > contours;
    vector<cv::Vec4i> hierarchy;
    cv::findContours( *m, contours, hierarchy, mode, CV_CHAIN_APPROX_SIMPLE );

    lua_newtable(L); // list of contour objects
    int i = 0, j = 1;
    for ( ; i >= 0; i = hierarchy[i][0] ) { // next index
	cv::Mat **um = newcont(L);
	*um = new cv::Mat(contours[i], true);
	lua_rawseti(L, -2, j++);
    }

    return 1;
}

static int properties(lua_State *L) {
    cv::Mat *m = checkcont(L, 1);
    const char opt = luaL_checkstring(L, 2)[0];
    cv::Rect dims;
    cv::RotatedRect ret;
    cv::Mat hull;
    switch(opt) {
	case 'A': lua_pushnumber(L, cv::contourArea(*m)); break;
	case 'P': lua_pushnumber(L, cv::arcLength(*m, true)); break;
	case 'S': cv::convexHull(*m, hull); lua_pushnumber(L, cv::contourArea(*m)/cv::contourArea(hull)); break;
	case 'R': dims = cv::boundingRect(*m); lua_pushnumber(L, (double)dims.width/dims.height); break;
	case 'C': lua_pushboolean(L, cv::isContourConvex(*m)); break;
	case 'M': if (m->rows < 5) {lua_pushnumber(L, -1); break;}
		  ret = cv::minAreaRect(*m);
		  break;
	case 'E': if (m->rows < 5) {lua_pushnumber(L, -1); break;}
		  ret = cv::fitEllipse(*m);
		  break;
    }

    if ((opt == 'M') | (opt == 'E')) {
	    cv::RotatedRect *r = newrrect(L);
	    r->size.width = ret.size.width; r->size.height = ret.size.height;
	    r->center.x = ret.center.x; r->center.y = ret.center.y;
	    r->angle = ret.angle;
    }
    return 1;
}

static int extrema(lua_State *L) {
    cv::Mat *m = checkcont(L, 1);

    cv::Mat xy[2];
    cv::split(*m, xy);

    double left, right;
    cv::minMaxLoc(xy[0], &left, &right);
    double top, bottom;
    cv::minMaxLoc(xy[1], &top, &bottom);

    lua_newtable(L);
    lua_pushnumber(L, left);
    lua_setfield(L, -2, "left");
    lua_pushnumber(L, right);
    lua_setfield(L, -2, "right");
    lua_pushnumber(L, top);
    lua_setfield(L, -2, "top");
    lua_pushnumber(L, bottom);
    lua_setfield(L, -2, "bottom");

    return 1;
}

static int getRoi(lua_State *L) {
    cv::Mat *box = checkcont(L, 1);
    cv::Rect br = cv::boundingRect(*box); // alt: minAreaRect()
    cv::Rect *r = newrect(L);
    r->x = br.x; r->y = br.y; r->width = br.width; r->height = br.height;
    return 1;
}

static int contourApprox(lua_State *L) {
    cv::Mat *m = checkcont(L, 1);
    double pct = luaL_checknumber(L, 2);
    double epsilon = pct*cv::arcLength(*m, true);
    cv::Mat a, **ret = newmat(L);
    cv::approxPolyDP(*m, a, epsilon, true);
    *ret = new cv::Mat(a);
    return 1;
}

static int convexHull(lua_State *L) {
    cv::Mat *c = checkcont(L, 1);
    cv::Mat hull;
    cv::convexHull(*c, hull);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(hull);
    return 1;
}

/* ****************************** */

/* *********** RECT ************ */

static int newRect(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    int x = getfield(L, "x");
    int y = getfield(L, "y");
    int w = getfield(L, "width");
    int h = getfield(L, "height");
    cv::Rect *r = newrect(L);
    r->x = x;
    r->y = y;
    r->width = w;
    r->height = h;
    return 1;
}

static int rect2zeros(lua_State *L) {
    cv::Rect *r = checkrect(L, 1);
    return zeros(L, r->height, r->width);
}

static int rect2ones(lua_State *L) {
    cv::Rect *r = checkrect(L, 1);
    return ones(L, r->height, r->width);
}

static int applyRoi(lua_State *L) {
    cv::Rect *r = checkrect(L, 1);
    cv::Mat *m = checkmat(L, 2);
    cv::Mat **ret = newmat(L);
    *ret = new cv::Mat(*m, *r);
    return 1;
}

static int grid(lua_State *L) {
    cv::Rect *r = checkrect(L, 1);
    int dx, dy;
    if (lua_gettop(L) > 2) {
	    dx = luaL_checkinteger(L, 2);
	    dy = luaL_checkinteger(L, 3);
    }
    else if (lua_gettop(L) > 1) {
	    dx = luaL_checkinteger(L, 2);
	    dy = dx;
    }
    lua_newtable(L);
    int x, y, k=1;
    for(y=0; y < r->height; y+=dy)
	for(x=0; x < r->width; x+=dx)
	{
	    cv::Rect *rr = newrect(L);
	    rr->x = x + r->x; rr->y = y + r->y; rr->width = dx; rr->height = dy;
	    lua_rawseti(L, -2, k++);
	}
    return 1;
}

static int dimensions(lua_State *L) {
    cv::Rect *r = checkrect(L, 1);
    lua_newtable(L);
    lua_pushinteger(L, r->width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, r->height);
    lua_setfield(L, -2, "height");
    return 1;
}

static int coordinates(lua_State *L) {
    cv::Rect *r = checkrect(L, 1);
    lua_newtable(L);
    lua_pushinteger(L, r->x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, r->y);
    lua_setfield(L, -2, "y");
    return 1;
}

static int rect2len(lua_State *L) {
    cv::Rect *r = checkrect(L, 1);
    lua_pushinteger(L, r->width * r->height);
    return 1;
}

static int rect2str(lua_State *L) {
    cv::Rect *r = checkrect(L, 1);
    lua_pushfstring(L, "ocvRect{ x = %d, y = %d, width = %d, height = %d }", r->x, r->y, r->width, r->height);
    return 1;
}

/* *********************************** */

/* ********** ROTATED-RECT *********** */

static int axes(lua_State *L) {
    cv::RotatedRect *rr = checkrrect(L, 1);
    lua_newtable(L);
    lua_pushinteger(L, rr->size.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, rr->size.height);
    lua_setfield(L, -2, "height");
    return 1;
}

static int rrect2str(lua_State *L) {
    cv::RotatedRect *r = checkrrect(L, 1);
    lua_pushfstring(L, "ocvRRect{ x = %f, y = %f, width = %f, height = %f, angle = %f }", r->center.x, r->center.y, r->size.width, r->size.height, r->angle);
    return 1;
}

static int drawEllipse(lua_State *L) {
    cv::RotatedRect *rr = checkrrect(L, 1);
    cv::Mat *m = checkmat(L, 2);

    cv::Mat dst;

    if (lua_gettop(L) > 2) {
	int w = luaL_checkinteger(L, 3);
	dst = cv::Mat(m->clone());
	cv::ellipse(dst, *rr, cv::Scalar(255,51,51), w > 0 ? w : 3, 8);
    } else {
	dst = cv::Mat::zeros( m->size(), CV_8UC1);
	cv::ellipse(dst, *rr, cv::Scalar(255,51,51), -1, 8);
    }

    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

/*
static int compareHist(lua_State *L) {
    cv::Mat *h1 = checkmat(L, 1);
    cv::Mat *h2 = checkmat(L, 2);
    if ((h1->cols > 1) | (h2->cols > 1))
	luaL_error(L, "Two histograms are required.");
    const char opt = luaL_checkstring(L, 3)[0];
    int method;
    switch(opt) {
	case 'C': method = CV_COMP_CORREL; break;
	case 'S' : method = CV_COMP_CHISQR; break;
	case 'B' : method = CV_COMP_BHATTACHARYYA; break;
	case 'I' : method = CV_COMP_INTERSECT; break;
    }
    lua_pushnumber(L, cv::compareHist(*h1, *h2, method));
    return 1;
}

static int humoments(lua_State *L) {
    cv::Mat *c1 = checkmat(L, 1);
    if (c1->cols > 1)
	luaL_error(L, "A contour is required.");
    lua_newtable(L);
    double hum[7];
    cv::HuMoments(cv::moments(*c1), hum);
    int k;
    for (k=0; k<7; k++) {
	lua_pushnumber(L, hum[k]);
	lua_rawseti(L, -2, k+1);
    }
    return 1;
}
*/
/*
static int matchShapes(lua_State *L) {
    cv::Mat *c1 = checkmat(L, 1);
    cv::Mat *c2 = checkmat(L, 2);
    if ((c1->cols > 1) | (c2->cols > 1))
	luaL_error(L, "Two contours are required.");
    int method = luaL_checkint(L, 3);
    switch(method) {
	case 1 : method = CV_CONTOURS_MATCH_I1; break;
	case 2 : method = CV_CONTOURS_MATCH_I2; break;
	case 3 : method = CV_CONTOURS_MATCH_I3; break;
    }
    lua_pushnumber(L, cv::matchShapes(*c1, *c2, method, 0.0));
    return 1;
}
*/
/*
static int calcBackProject(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Mat *hist = checkmat(L, 2);
    cv::Mat backp, tgt;
    float range[] = {0, 256};
    const float* ranges[] = { range };

    bool tobyte = true;
    if (lua_gettop(L) > 3) { // four args at least
	range[0] = luaL_checknumber(L, 3);
	range[1] = luaL_checknumber(L, 4);
	tobyte = false;
    }
    correctFormat(tobyte, m, tgt);
    
    cv::calcBackProject( &tgt, 1, 0, *hist, backp, ranges );
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(backp);
    return 1;
}
*/
/*
static int matchTemplate(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    cv::Mat *templ = checkmat(L,2);
    const char opt = luaL_checkstring(L, 3)[0];

    int method;
    switch( opt ) {
	case 'C': method = CV_TM_CCOEFF; break;
	case 'N': method = CV_TM_CCOEFF_NORMED; break;
    }

    cv::Mat tgt, tpl, ret;
    byteOrFloat(m, tgt);
    byteOrFloat(templ, tpl);

    cv::matchTemplate(tgt, tpl, ret, method);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(ret);
    return 1;
}

static int distanceTransform(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    check81(L, m);
    int mask = luaL_checkint(L, 2);
    if (!((mask == 0) | (mask == 3) | (mask == 5)))
	    luaL_error(L, "Mask must be 0, 3 or 5");
    cv::Mat dst;
    cv::distanceTransform( *m, dst, CV_DIST_L2, mask);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}
*/
/*
static int adaptiveThreshold(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    check81(L, m);

    int block = 11;
    if (lua_gettop(L) > 1) {
	block = luaL_checkint(L, 2);
    }

    cv::Mat dst;
    int maxv = 255, method = cv::ADAPTIVE_THRESH_GAUSSIAN_C, type = cv::THRESH_BINARY;
    double C = 2;
    cv::adaptiveThreshold(*m, dst, maxv, method, type, block, C);

    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}

static int bilateralFilter(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);
    check81(L, m);

    int diameter = 5;
    double sigma = 75;
    if (lua_gettop(L) > 2) {
	diameter = luaL_checkint(L, 2);
	sigma = luaL_checknumber(L, 3);
    }

    cv::Mat dst;
    cv::bilateralFilter(*m, dst, diameter, sigma, sigma);
    cv::Mat **um = newmat(L);
    *um = new cv::Mat(dst);
    return 1;
}
*/



static int showImage(lua_State *L) {
  cv::Mat *m = checkmat(L, 1);

  if ((m->cols == 1) | (m->rows == 1))
	  luaL_error(L, "Cannot display a single row/column matrix.");

  cv::namedWindow("Image", CV_GUI_NORMAL);
  cv::imshow("Image", *m);
  cv::waitKey(500);
  return 0;
}



static const struct luaL_Reg cv_funcs[] = {
  {"open", openImage},
//  {"openDCM", fromGDCM},
  {"openVideo", videoCapture},
  {"rect", newRect},
  {"rotation", rotationMatrix},
  {"PCA", doPCA},
  {"fromTable", fromTable},
  {NULL, NULL}
};

static const struct luaL_Reg rrect_meths[] = {
  {"__tostring", rrect2str},
  {"dims", axes},
  {"draw", drawEllipse},
  {NULL, NULL}
};

static const struct luaL_Reg rect_meths[] = {
  {"__tostring", rect2str},
  {"__len", rect2len},
  {"coords", coordinates},
  {"zeros", rect2zeros},
  {"ones", rect2ones},
  {"grid", grid},
  {"apply", applyRoi},
  {"dims", dimensions},
  {NULL, NULL}
};

static const struct luaL_Reg cont_meths[] = {
  {"__gc", release},
  {"property", properties},
  {"extrema", extrema},
  {"approximate", contourApprox},
//  {"moments", humoments},
  {"roi", getRoi},
  {NULL, NULL}
};

static const struct luaL_Reg mat_meths[] = {
  {"__tostring", mat2str},
  {"__gc", release},
  {"__len", mat2len},
  {"totable", mat2table},
  {"show", showImage},
  {"zeros", mat2zeros},
  {"ones", mat2ones},
  {"save", saveImage},
  {"range", inRange},
  {"torect", mat2rect},
  {"flood", floodFill},
  {"contours", findContours},
  {"draw", drawContours},
  {"chull", convexHull},
  {"nonzero", countNonZero},
  {"blur", gaussian},
  {"mean", mean},
  {"stdev", stdev},
  {"median", median},
  {"add", add},
  {"wsum", addWeighted},
  {"minmax", minMax},
  {"clone", cloneMe},
  {"histogram", getHistogram},
  {"lut", applyLUT},
//  {"distance", compareHist},
//  {"distancet", distanceTransform},
//  {"match", matchShapes},
//  {"backproject", calcBackProject},
//  {"adaptive", adaptiveThreshold},
  {"tobyte", convert2byte},
  {"threshold", threshold},
  {"overlay", applyOverlay},
  {"torgb", gray2rgb},
  {"tofloat",convert2float},
  {"togray", rgb2gray},
  {"unsigned", convert2unsigned},
  {"bitwise", bitwise},
  {"copy", copyTo},
  {"assign", assignTo},
  {"morphology", morphology},
  {"equalize", equalizeHist},
//  {"affine", warpAffine},
//  {"bilateral", bilateralFilter},
  {"laplacian", laplacian},
  {"sobel", sobel},
  {"canny", canny},
//  {"template", matchTemplate},
  {"watershed", watershed},
  {NULL, NULL}
};

int luaopen_locv (lua_State *L) {
  luaL_newmetatable(L, "caap.opencv.mat");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, mat_meths, 0);

  luaL_newmetatable(L, "caap.opencv.cont");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, cont_meths, 0);

  luaL_newmetatable(L, "caap.opencv.rect");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, rect_meths, 0);

  luaL_newmetatable(L, "caap.opencv.rrect");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, rrect_meths, 0);

  // create the library
  luaL_newlib(L, cv_funcs);
  return 1;
}

#ifdef __cplusplus
}
#endif

