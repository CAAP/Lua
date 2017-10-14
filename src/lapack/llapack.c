#include <lapacke/lapacke.h>

#include <lua.h>
#include <lauxlib.h>

#include <stdio.h>

#define checkvector(L, i) (double *)luaL_checkudata(L, i, "caap.lapack.vector");
#define newvector(L, n) (double *)lua_newuserdata(L, n*sizeof(double)); luaL_getmetatable(L, "caap.lapack.vector"); lua_setmetatable(L, -2);
#define checkmatrix(L, i) (double *)luaL_checkudata(L, i, "caap.lapack.matrix");
#define newmatrix(L, n) (double *)lua_newuserdata(L, n*sizeof(double)); luaL_getmetatable(L, "caap.lapack.matrix"); lua_setmetatable(L, -2);

// https://software.intel.com/en-us/mkl-developer-reference-c-matrix-layout-for-lapack-routines
// https://software.intel.com/en-us/mkl-developer-reference-c-matrix-storage-schemes-for-lapack-routines
//
// LAYOUT
//      COLUMN MAJOR: { a_11 a_21 a_31 a_41 ... }
//      ROW MAJOR: { a_11 a_12 a_13 a_14 ... }
//
// LEADING DIMENSION
// 	COLUMN-MAJOR: M number of ROWs
// 	ROW-MAJOR: N number of COLUMNs
//
// OFFSET
// 	COLUMN-MAJOR: i_0 + j_0*ld(M)
// 	ROW-MAJOR: i_0*ld(N) + j_0
//
// INDEX k(i,j)
//	COLUMN-MAJOR: i-1 + (j-1)*ld(M)
//	ROW_MAJOR: (i-1)*ld(N) + j-1
//
// PACKED STORAGE
// Only one part od the matrix is necessary: the UPPER or LOWER triangle
// when the matrix is upper triangular, lower triangular, symmetric or Hermitian
//

///////////  AUX  ///////////////

void print_vector(const char *desc, const lapack_int n, const double *vec) {
    lapack_int j;
    printf( "\n %s \n", desc );
    for (j = 0; j<n; j++)
	printf( "%6.2f\n", vec[j] );
    printf( "\n" );
}

void print_matrix_rowmajor(const char *desc, const lapack_int m, const lapack_int n, const double *mat, const lapack_int ldm) {
    lapack_int i,j;
    printf( "\n %s\n", desc );

    for (i = 0; i<m; i++) {
	for (j = 0; j<n; j++)
	    printf( " %6.2f", mat[i*ldm+j] );
	printf( "\n" );
    }
}

void print_matrix_colmajor(const char *desc, const lapack_int m, const lapack_int n, const double *mat, const lapack_int ldm) {
    lapack_int i,j;
    printf( "\n %s\n", desc );

    for (i = 0; i<m; i++) {
	for (j = 0; j<n; j++)
	    printf( " %6.2f", mat[i+j*ldm] );
	printf( "\n" );
    }
}

void checkinfo(lua_State *L, const lapack_int info) {
    switch(info) {
	case 0: return;
	case -1011: luaL_error(L, "Memory allocation error!");
	default: luaL_error(L, "Illegal value for parameter %d", info);
    }
}

int getint(lua_State *L, int idx, const char *lbl) {
    lua_getfield(L, idx, lbl);
    int ret = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return ret;
}

const char* getstr(lua_State *L, const int idx, const char *lbl) {
    lua_getfield(L, idx, lbl);
    const char* ret = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    return ret;
}

void setUpvalue(lua_State *L, const int M, const int N, const int layout) {
    int isrows = (layout == LAPACK_ROW_MAJOR) ? 1 : 0;
    // create UpValue
    lua_newtable(L);
    lua_pushinteger(L, M);
    lua_setfield(L, -2, "M");
    lua_pushinteger(L, N);
    lua_setfield(L, -2, "N");
    lua_pushinteger(L, (isrows ? N : M));
    lua_setfield(L, -2, "LD");
    lua_pushinteger(L, layout);
    lua_setfield(L, -2, "layout");
    lua_pushfstring(L, "LAPACK-Matrix{rows: %d, cols: %d, %s}", M, N, (isrows ? "ROW-wise" : "COLUMN-wise"));
    lua_setfield(L, -2, "asstr");
    lua_setuservalue(L, -2); // append upvalue to userdatum
}

void vecSetUpvalue(lua_State *L, const int N) {
    // create UpValue
    lua_newtable(L);
    lua_pushinteger(L, N);
    lua_setfield(L, -2, "N");
    lua_pushfstring(L, "LAPACK-Vector{cols: %d}", N);
    lua_setfield(L, -2, "asstr");
    lua_setuservalue(L, -2); // append upvalue to userdatum
}

static void fillArray(lua_State *L, double *array, const lapack_int N) {
    unsigned int k;
    for (k = 0; k < N; k++) {
	lua_rawgeti(L, 1, k+1);
	array[k] = lua_tonumber(L, -1);
	lua_pop(L, 1);
    }
}


///////// INTERFACE /////////

static int tostr(lua_State *L) {
    lua_getuservalue(L, 1);
    lua_getfield(L, -1, "asstr");
    return 1;
}

static int transpose(lua_State *L) {
    lua_getuservalue(L, 1);
    lapack_int M = getint(L, -1, "M"), N = getint(L, -1, "N");
    int layout = getint(L, -1, "layout");
    lua_pop(L, 1);

    layout = (layout == LAPACK_ROW_MAJOR) ? LAPACK_COL_MAJOR : LAPACK_ROW_MAJOR;
    setUpvalue(L, N, M, layout);
    return tostr(L);
}

/////////// LAPACK ////////
// check whether it's symmetric
static int fromTable(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    const lapack_int M = getint(L, 1, "M"), N = getint(L, 1, "N");

    double *data = newmatrix(L, (M*N))
    setUpvalue(L, M, N, LAPACK_ROW_MAJOR);
    fillArray(L, data, M*N);
    return 1;
}

static int fromArray(lua_State *L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);
    const lapack_int M = luaL_checkinteger(L, 2);
    const int k = lua_gettop(L);

    lua_pushvalue(L, 1);

    if (k == 3) {
	const lapack_int N = luaL_checkinteger(L, 3);
	luaL_getmetatable(L, "caap.lapack.matrix"); lua_setmetatable(L, -2);
	setUpvalue(L, M, N, LAPACK_ROW_MAJOR);
    } else
	luaL_getmetatable(L, "caap.lapack.vector"); lua_setmetatable(L, -2);
	vecSetUpvalue(L, M);

    return 1;
}

//////////  VECTOR ///////////////

static int displayVector(lua_State *L) {
    double *vec = checkvector(L, 1);

    lua_getuservalue(L, 1);
    const lapack_int N = getint(L, -1, "N");
    const char *desc = getstr(L, -1, "asstr");
    lua_pop(L, 1);

    print_vector(desc, N, vec);
    return 0;
}

static int vec2gc(lua_State *L) {
    double *vec = checkvector(L, 1);
    if (vec != NULL)
	vec = NULL;
    return 0;
}


////////// SYMMETRIC /////////

// 	'M', 'm' largest absolute value
// 	'1', '0', 'o' 1-norm, maximum column sum
// 	'I', 'i' infinity norm, maximum row sum
// 	'F', 'f' Froenius norm, SR-SSQ 
// https://software.intel.com/en-us/mkl-developer-reference-c-lange
/*
static int normSym(lua_State *L) {
    const double *mat = checkmatrix(L, 1);
    const char n = luaL_checkstring(L, 2)[0];
    const int N = getint(L, -1, "N"), layout = getint(L, -1, "layout");
    const char uplo = (char)getint(L, -1, "uplo");
    const lapack_int LD = getint(L, -1, "LD");

    double ret = LAPACKE_dlansy(layout, n, uplo, N, mat, LD);
    lua_pushnumber(L, ret);
    return 1;
}

// https://software.intel.com/en-us/mkl-developer-reference-c-syevr
static int eigenvalue(lua_State *L) {
    double *mat = checkmatrix(L, 1);
    const int vecs = lua_toboolean(L, 2);

    lua_getuservalue(L, 1);
    lapack_int N = getint(L, -1, "N"), LD = getint(L, -1, "LD");
    const int layout = getint(L, -1, "layout");
    char uplo = (char)getint(L, -1, "uplo");
    lua_pop(L, 1);

    char jobz = vecs ? 'V' : 'N';
    char range = 'A';
    lapack_int i1 = 0, i2 = 0, *m = &N;
    if (lua_gettop(L) == 4) {
	range = 'I';
	i1 = luaL_checkinteger(L, 3);
	i2 = luaL_checkinteger(L, 4);
	*m = (i2-i1+1);
    }
    lapack_int *isuppz = lua_newuserdata(L, 2*(*m)*sizeof(lapack_int));
    double *w = newvector(L, N);
    vecSetUpvalue(L, N);
    double *z = newmatrix(L, (N*(*m)));
    lapack_int ldz = (layout == LAPACK_ROW_MAJOR) ? (*m) : N;
    setUpvalue(L, N, (*m), layout);

    lapack_int info = LAPACKE_dsyevr(layout, jobz, range, uplo, N, mat, LD, 0, 0, i1, i2, 1e-4, m, w, z, ldz, isuppz);
    checkinfo(L, info);

    return 2;
}
*/

//////// MATRIX //////////

static int displayMatrix(lua_State *L) {
    double *mat = checkmatrix(L, 1);

    lua_getuservalue(L, 1);
    const lapack_int M = getint(L, -1, "M"), N = getint(L, -1, "N"), LD = getint(L, -1, "LD");
    const int layout = getint(L, -1, "layout");
    const char *desc = getstr(L, -1, "asstr");
    lua_pop(L, 1);

    if (layout == LAPACK_ROW_MAJOR)
	print_matrix_rowmajor(desc, M, N, mat, LD);
    else
	print_matrix_colmajor(desc, M, N, mat, LD);

    return 0;
}

// norm
// 	'M', 'm' largest absolute value
// 	'1', '0', 'o' 1-norm, maximum column sum
// 	'I', 'i' infinity norm, maximum row sum
// 	'F', 'f' Frobenius norm, SR-SSQ 
// https://software.intel.com/en-us/mkl-developer-reference-c-lange
static int normMat(lua_State *L) {
    double *mat = checkmatrix(L, 1);
    const char n = luaL_checkstring(L, 2)[0];

    lua_getuservalue(L, 1);
    const int M = getint(L, -1, "M"), N = getint(L, -1, "N"), layout = getint(L, -1, "layout");
    const lapack_int LD = getint(L, -1, "LD");
    lua_pop(L, 1);

    double ret = LAPACKE_dlange(layout, n, M, N, mat, LD);
    lua_pushnumber(L, ret);
    return 1;
}

//////// Bi-diagonal /////////
// BIDIAGONAL
// Let A be a M-by-N general real matrix
// if m >= n, B is upper bidiagonal, if m < n, B is lower bidiagonal
// if m >= n, Q has n terms, P has n-1 terms, if m < n, Q has m-1 terms, P has m terms

static int bidiagonal(lua_State *L) {
    double *mat = checkmatrix(L, 1);

    lua_getuservalue(L, 1);
    const lapack_int M = getint(L, -1, "M"), N = getint(L, -1, "N"), LD = getint(L, -1, "LD"), layout = getint(L, -1, "layout");
    lua_pop(L, 1);

// Define type
    const int islower = (M < N) ? 1 : 0;
    const lapack_int min0 = islower ? M : N;
    const lapack_int min1 = min0-1;

    double *d = newvector(L, min0);
    vecSetUpvalue(L, min0);
    double *e = newvector(L, min1);
    vecSetUpvalue(L, min1);
    double *tauq = newvector(L, min0);
    double *taup = newvector(L, min0);

    int info = LAPACKE_dgebrd(layout, M, N, mat, LD, d, e, tauq, taup);
    checkinfo(L, info);

    lua_pop(L, 2); // tauq, taup

    return 2;
}

/////// SVD /////

static int svd(lua_State *L) {
    double *mat = checkmatrix(L, 1);
    const int dac = (int)lua_toboolean(L, 2); // Divide and conquer algorithm

    lua_getuservalue(L, 1);
    const lapack_int M = getint(L, -1, "M"), N = getint(L, -1, "N"), LD = getint(L, -1, "LD"), layout = getint(L, -1, "layout");
    lua_pop(L, 1);

    const int min0 = (M < N) ? M : N;
    char jobz = 'S';
    lapack_int ldu = 1, ldvt = 1;
    double *u = NULL, *vt = NULL;
    double *superb = newvector(L, (min0-2));
    double *s = newvector(L, min0);
    vecSetUpvalue(L, min0);

    ldu = (layout == LAPACK_ROW_MAJOR) ? min0 : M;
    u = newmatrix(L, (M*min0));
    setUpvalue(L, M, min0, layout);
    ldvt = (layout == LAPACK_ROW_MAJOR) ? N : min0;
    vt = newmatrix(L, (min0*N));
    setUpvalue(L, min0, N, layout);

    int info;
    if (dac) { info = LAPACKE_dgesdd(layout, jobz, M, N, mat, LD, s, u, ldu, vt, ldvt); }
    else { info = LAPACKE_dgesvd(layout, jobz, jobz, M, N, mat, LD, s, u, ldu, vt, ldvt, superb); }
    if (info>0)
	luaL_error(L,"Failed to compute singular value %d", info);
    else
	checkinfo(L, info);

    return 3;
}

static int mat2gc(lua_State *L) {
    double *mat = checkmatrix(L, 1);
    if (mat != NULL)
	mat = NULL;
    return 0;
}

/////////// LIBS ///////////////

static const struct luaL_Reg lapack_funcs[] = {
  {"fromTable", fromTable},
  {"fromArray", fromArray},
  {NULL, NULL}
};

static const struct luaL_Reg mat_meths[] = {
  {"__tostring", tostr},
  {"__gc", mat2gc},
//  {"__len", size},
  {"display", displayMatrix},
  {"t", transpose},
  {"norm", normMat},
  {"bidiagonal", bidiagonal},
  {"svd", svd},
  {NULL, NULL}
};

static const struct luaL_Reg vec_meths[] = {
  {"__tostring", tostr},
//  {"__len", size},
  {"display", displayVector},
  {"__gc", vec2gc},
  {NULL, NULL}
};

///////////////////////////// 

int luaopen_llapack (lua_State *L) {
////////////////
  luaL_newmetatable(L, "caap.lapack.matrix");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, mat_meths, 0);
///////////
  luaL_newmetatable(L, "caap.lapack.vector");
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");
  luaL_setfuncs(L, vec_meths, 0);
//
  luaL_newlib(L, lapack_funcs);
  return 1;
}




/*
static int svd(lua_State *L) {
    double *mat = checkmatrix(L, 1);
    const lapack_int ncvt = luaL_checkinteger(L, 2), nru = lua_checkinteger(L, 3);
    const int dac = lua_toboolean(L, 4); // Divide and conquer algorithm

// Bidiagonal matrix
    const int islower = (M < N) ? 1 : 0;
    const lapack_int min0 = islower ? M : N;

    double *q = NULL, *pt = NULL, *d = newvector(L, min0), *e = newvector(L, min0 - 1);
//    if (islower) { min0, min1 = min1, min0; } // upper or lower bidiagonal XXX ERROR
    double *tauq = newvector(L, min0), *taup = newvector(L, min0);

    int info = LAPACKE_dgebrd(layout, M, N, mat, LD, d, e, tauq, taup);
    checkinfo(info);

// Recreate orthogonal matrix, if NEEDED
    if (nru > 0)
	info = LAPACKE_dorgbr(layout, 'Q', M, min0, N, mat, LD, tauq);
    if (ncvt > 0)
	info = LAPACKE_dorgbr(layout, 'P', min0, N, M, mat, LD, taup);

    const char uplo = lower ? 'L' : 'U';
    const char compq = ((ncvt == 0)&&(nru == 0)) ? 'N' : 'I';

// ASSERT: ncvt & nru >= min0 WHEN Divide&Conquer is selected
    if (dac == 1) {
	ncvt = (ncvt < min0) ? min0 : ncvt;
	nru = (nru < min0) ? min0 : nru;
    }
    const lapack_int ldvt = min0, ldu = nru; // COLUMN-wise

    double *vt = newmatrix(L, min0*ncvt);
    setUpvalue(L, min0, ncvt, LAPACK_COLUMN_MAJOR);
    double *u = newmatrix(L, nru*min0);
    setUpvalue(L, nrvt, min0, LAPACK_COLUMN_MAJOR);

// https://software.intel.com/en-us/mkl-developer-reference-c-bdsqr
// https://software.intel.com/en-us/mkl-developer-reference-c-bdsdc
    if (dac == 1)
	info = LAPACKE_dbdsdc(layout, uplo, compq, min0, d, e, u, ldu, vt, ldvt, NULL, NULL);
    else
	info = LAPACKE_dbdsqr(layout, uplo, min0, ncvt, nru, 0, d, e, vt, ldvt, u, ldu, NULL, 1);

// QR-related info is missing check website XXX

    if (info>0)
	luaL_error(L,"Failed to compute singular value %d", info);
    else
	checkinfo(info);


}

//////  Linear least-squres  ////////////

// DGELS using row-major order
static int dgels(lua_State *L) {
    lapack_int info, m, n, lda, ldb, nrhs;

    // General minimization
    info = LAPACKE_dgels(LAPACK_ROW_MAJOR, 'N', m, n, nrhs, a, lda, b, ldb);

    checkinfo(info);


}
*/
