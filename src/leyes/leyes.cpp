#include <opencv2/opencv.hpp>

#include <queue>

using namespace std;

#include <lua.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

#define checkmat(L,i) *(cv::Mat **)luaL_checkudata(L, i, "caap.opencv.mat")

cv::Mat double calcularGradiente(const cv::Mat &mat) {
    cv::Mat out(mat.rows, mat.cols, CV_64F);
    for (int y = 0; y < mat.rows; ++y) {
	const uchar *Mr = mat.ptr<uchar>(y);
	double *Or = out.ptr<double>(y);
	Or[0] = Mr[1] - Mr[0];
	for (int x = 1; x < mat.cols - 1; ++x)
	    Or[x] = (Mr[x+1] - Mr[x-1])/2.0;
	Or[mat.cols-1] = Mr[mat.cols-1] - Mr[mat.cols-2];
    }

    return out;
}

cv::Mat norm(const cv::Mat &matX, const cv::Mat &matY) {
    cv::Mat norma(matX.rows, matX.cols, CV_64F);
    for (int y = 0; y < matX.rows; ++y) {
	const double *Xr = matX.ptr<double>(y), *Yr = matY.ptr<double>(y);
	double *Mr = norma.ptr<double>(y);
	for (int x = 0; x < matX.cols; ++x) {
	    double gX = Xr[x], gY = Yr[x];
	    Mr[x] = sqrt((gX*gX) + (gY*gY));
	}
    }
    return norma;
}

void centersFormula(int x, int y, const cv::Mat &weighted, double gx, double gy, cv::Mat &out) {
    const float divisor = 5.0;
    for (int cy = 0; cy < out.rows; ++cy) {
	double *Or = out.ptr<double>(cy);
	const unsigned char *Wr = weighted.ptr<unsigned char>(cy);
	for (int cx = 0; cx < out.cols; ++cx) {
	    if (x == cx && y == cy)
		    continue;
	    double dx = x - cx;
	    double dy = y - cy;
	    double magnitude = sqrt((dx*dx) + (dy*dy));
	    dx /= magnitude;
	    dy /= magnitude;
	    double dotproduct = dx*gx + dy*gy;
	    Or[cx] += dotproduct * dotproduct * (Wr[cx]/divisor);
//		Or[cx] += dotproduct * dotproduct;
	}
    }
}

int pushp(const cv::Point &np, const cv::Mat &mat) {
    return np.x >= 0 && np.x < mat.cols && np.y >=0 && np.y < mat.rows;
}

cv::Mat floodKillEdges(cv::Mat &mat) {
    rectangle(mat, cv::Rect(0,0,mat.cols,mat.rows), 255);

    cv::Mat mask(mat.rows, mat.cols, CV_8U, 255);
    queue<cv::Point> toDo;
    toDo.push(cv::Point(0,0));
    while (!toDo.empty()) {
	cv::Point p = toDo.front();
	toDo.pop();
	if (mat.at<float>(p) == 0.0f)
	    continue;

	cv::Point np(p.x + 1, p.y);
	if (pushp(np, mat)) toDo.push(np);
	np.x = p.x - 1; np.y = p.y;
	if (pushp(np, mat)) toDo.push(np);
	np.x = p.x; np.y = p.y + 1;
	if (pushp(np, mat)) toDo.push(np);
	np.x = p.x; np.y = p.y - 1;
	if (pushp(np, mat)) toDo.push(np);
	mat.at<float>(p) = 0.0f;
	mask.at<uchar>(p) = 0;
    }
    return mask;
}

double calc_threshold(const cv::Mat &mat, double stdDevF) {
    cv::Scalar stdDevG, meanG;
    cv::meanStdDev(mat, meanG, stdDevG);
    double stdDev = stdDevG[0] / sqrt(mat.rows * mat.cols);
    return stdDevF * stdDev + meanG[0];
}

static int eyesCenter(lua_State *L) {
    cv::Mat *m = checkmat(L, 1);

    float eyeWidth = 50.0;
    const int mythresh = 1;
    const int blur = 5;

    cv::Mat eye, weighted, out;
    cv::resize(*m, eye, cv::Size(eyeWidth, eyeWidth*(m->rows)/(m->cols)));
    cv::Mat gradX calcularGradiente(eye);
    cv::Mat gradY calcularGradiente(eye.t()).t();
    cv::Mat norma = norm(gradX, gradY);
    double threshold = calc_threshold(norma, mythresh);

    for (int y = 0; y < eye.rows; ++y) {
	double *Xr = gradX.ptr<double>(y), *Yr = gradY.ptr<double>(y);
	const double *Mr = norma.ptr<double>(y);
	for (int x = 0; x < eye.cols; ++x) {
	    double gX = Xr[x], gY = Yr[x];
	    double mm = Mr[x];
	    if (mm > threshold) {
		Xr[x] = gX/mm;
		Yr[x] = gY/mm;
	    } else {
		Xr[x] = 0.0;
		Yr[x] = 0.0;
	    }
	}
    }

    cv::GaussianBlur(eye, weighted, cv::Size(blur, blur), 0, 0);

    for (int y = 0; y < weighted.rows; ++y) {
	unsigned char *row = weighted.ptr<unsigned char>(y);
	for (int x = 0; x < weighted.cols; ++x)
	    rows[x] = (255 - row[x]);
    }

    cv::Mat outSum = cv::Mat::zeros(eye.rows, eye.cols, CV_64F);

    for (int y = 0; y < weighted.rows; ++y) {
	const double *Xr = gradX.ptr<double>(y), *Yr = gradY.ptr<double>(y);
	for (int x = 0; x < weighted.cols; ++x) {
	    double gX = Xr[x], gY = Yr[x];
	    if (gX == 0.0 && gY == 0.0)
		    continue;

	    centersFormula(x, y, weighted, gX, gY, outSum);
	}
    }

    double ngrads = weighted.cols * weighted.rows;
    outSum.converTo(out, CV_32F, 1.0/ngrads);
    cv::Point maxP;
    double maxVal;
    cv::minMaxLoc(out, NULL, &maxVal, NULL, &maxP);

    const double postThreshold = 0.97;
    cv::Mat floodClone;
    double floodThresh = maxVal * postThreshold;
    cv::threshold(out, floodClone, floodThresh, 0.0f, cv::THRESH_TOZERO);
    cv::Mat mask = floodKillEdges(floodClone);
    cv::minMaxLoc(out, NULL, &maxVal, NULL, &maxP, mask);

    float ratio = (eyeWidth)/(m->cols);
    int x = round(maxP.x / ratio);
    int y = round(maxP.y / ratio);

    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

static const struct luaL_Reg eyes_funcs[] = {
  {"eyes", eyesCenter},
  {NULL, NULL}
};

int luaopen_leyes (lua_State *L) {
  // create the library
  luaL_newlib(L, eyes_funcs);
  return 1;
}

#ifdef __cplusplus
}
#endif
