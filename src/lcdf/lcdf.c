#include <lua.h>
#include <lauxlib.h>
#include <float.h>
#include <math.h>
#include "cdflib.h"

#define LBOUND (1.0e-20) /* ditto */
#define MAXITER (1000) /* used in dchisq */
#define SQRT2PI (2.506628274631) /* sqrt(2*pi) */

/* {=====================================================================
 *    Auxiliary
 * ======================================================================} */

/* Failsafe: execution shouldn't reach here (!), since most errors are checked
 * out by specific check_xxx routines; the only expected error is when status
 * == 10 */
static void check_status (lua_State *L, int status, lua_Number bound) {
  if (status == 1)
    luaL_error(L, "result lower than search bound: %f", bound);
  if (status == 2)
    luaL_error(L, "result higher than search bound: %f", bound);
  if (status < 0)
    luaL_error(L, "out of range on parameter %d: %f", -status, bound);
  if (status == 10)
    luaL_error(L, "error in cumgam: %d", status);
}

//* {=======   Normal   =======} */

static void check_norm (lua_State *L, int which, lua_Number x,
    lua_Number sd) {
  if (which == 2) luaL_argcheck(L, x >= 0 && x <= 1,  /* p */
      1, "out of range");
  luaL_argcheck(L, sd >= 0, 3, "non-negative value expected");
}

static int stat_dnorm (lua_State *L) {
  /* stack should contain x, and opt. mean and sd */
  lua_Number x = luaL_checknumber(L, 1);
  lua_Number mean = luaL_optnumber(L, 2, 0);
  lua_Number sd = luaL_optnumber(L, 3, 1);
  lua_Number d;
  check_norm(L, 1, x, sd);
  d = (x - mean) / sd;
  d = exp(-d*d / 2) / (SQRT2PI * sd);
  lua_pushnumber(L, d);
  return 1;
}

static int stat_pnorm (lua_State *L) {
  /* stack should contain x, and opt. mean and sd */
  lua_Number x = luaL_checknumber(L, 1);
  lua_Number mean = luaL_optnumber(L, 2, 0);
  lua_Number sd = luaL_optnumber(L, 3, 1);
  lua_Number p, q, bound;
  int which = 1;
  int status;
  check_norm(L, 1, x, sd);
  q = 1 - p;
  cdfnor(&which, &p, &q, &x, &mean, &sd, &status, &bound);
  check_status(L, status, bound);
  lua_pushnumber(L, p);
  return 1;
}

static int stat_qnorm (lua_State *L) {
  /* stack should contain p, and opt. mean and sd */
  lua_Number p = luaL_checknumber(L, 1);
  lua_Number mean = luaL_optnumber(L, 2, 0);
  lua_Number sd = luaL_optnumber(L, 3, 1);
  lua_Number x;
  check_norm(L, 2, p, sd);
  if (p == 0 || p == 1) x = (p == 0) ? -HUGE_VAL : HUGE_VAL;
  else {
    lua_Number q = 1 - p;
    lua_Number bound;
    int which = 2;
    int status;
    cdfnor(&which, &p, &q, &x, &mean, &sd, &status, &bound);
    check_status(L, status, bound);
  }
  lua_pushnumber(L, x);
  return 1;
}

/* {=======   t-Student   =======} */

static void check_t (lua_State *L, int which, lua_Number x,
    lua_Number df) {
  if (which==2) luaL_argcheck(L, x >= 0 && x <= 1,  /* p */
      1, "out of range");
  luaL_argcheck(L, df >= 0, 3, "non-negative value expected");
}

static int stat_dt (lua_State *L) {
  /* stack should contain x and df */
  lua_Number x = luaL_checknumber(L, 1);
  lua_Number df = luaL_checknumber(L, 2);
  lua_Number t = 0.5;
  lua_Number d;
  check_t(L, 1, x, df);
  d = df / 2;
  d = -dlnbet(&d, &t) - (df + 1) / 2 * log(1 + x * x / df);
  d = exp(d) / sqrt(df);
  lua_pushnumber(L, d);
  return 1;
}

static int stat_pt (lua_State *L) {
  /* stack should contain t and df */
  lua_Number t = luaL_checknumber(L, 1);
  lua_Number df = luaL_checknumber(L, 2);
  lua_Number p, q, bound;
  int which = 1;
  int status;
  check_t(L, 1, t, df);
  q = 1 - p;
  cdft(&which, &p, &q, &t, &df, &status, &bound);
  check_status(L, status, bound);
  lua_pushnumber(L, p);
  return 1;
}

static int stat_qt (lua_State *L) {
  /* stack should contain p and df */
  lua_Number p = luaL_checknumber(L, 1);
  lua_Number df = luaL_checknumber(L, 2);
  lua_Number t;
  check_t(L, 2, p, df);
  if (p == 0 || p == 1) t = (p == 0) ? -HUGE_VAL : HUGE_VAL;
  else {
    lua_Number q = 1 - p;
    lua_Number bound;
    int which = 2;
    int status;
    cdft(&which, &p, &q, &t, &df, &status, &bound);
    check_status(L, status, bound);
  }
  lua_pushnumber(L, t);
  return 1;
}

/* {=======   F-statistic   =======} */

static void check_f (lua_State *L, int which, lua_Number x,
    lua_Number dfn, lua_Number dfd) {
  luaL_argcheck(L, ((which == 1 && x >= 0)  /* x */
      || (which == 2 && (x >= 0 && x <= 1))), /* p */
      1, "out of range");
  luaL_argcheck(L, dfn >= 0, 2, "non-negative value expected");
  luaL_argcheck(L, dfd >= 0, 3, "non-negative value expected");
}

static int stat_df (lua_State *L) {
  /* stack should contain f, dfn, dfd */
  lua_Number f = luaL_checknumber(L, 1);
  lua_Number dfn = luaL_checknumber(L, 2);
  lua_Number dfd = luaL_checknumber(L, 3);
  lua_Number df1, df2, r, d;
  check_f(L, 1, f, dfn, dfd);
  df1 = dfn / 2;
  df2 = dfd / 2;
  r = dfn / dfd;
  d = df1 * log(r) + (df1 - 1) * log(f);
  d -= (df1 + df2) * log(1 + r * f);
  d -= dlnbet(&df1, &df2);
  lua_pushnumber(L, exp(d));
  return 1;
}

static int stat_pf (lua_State *L) {
  /* stack should contain f, dfn, dfd and opt. phonc */
  lua_Number f = luaL_checknumber(L, 1);
  lua_Number dfn = luaL_checknumber(L, 2);
  lua_Number dfd = luaL_checknumber(L, 3);
  lua_Number phonc = luaL_optnumber(L, 4, 0);
  lua_Number p, q, bound;
  int which = 1;
  int status;
  check_f(L, 1, f, dfn, dfd);
  if (phonc == 0) /* central? */
    cdff(&which, &p, &q, &f, &dfn, &dfd, &status, &bound);
  else /* non-central */
    cdffnc(&which, &p, &q, &f, &dfn, &dfd, &phonc, &status, &bound);
  check_status(L, status, bound);
  lua_pushnumber(L, p);
  return 1;
}

static int stat_qf (lua_State *L) {
  /* stack should contain p, dfn, dfd and opt. phonc */
  lua_Number p = luaL_checknumber(L, 1);
  lua_Number dfn = luaL_checknumber(L, 2);
  lua_Number dfd = luaL_checknumber(L, 3);
  lua_Number phonc = luaL_optnumber(L, 4, 0);
  lua_Number f;
  check_f(L, 2, p, dfn, dfd);
  if (p == 0 || p == 1) f = (p == 0) ? 0 : HUGE_VAL;
  else {
    lua_Number q = 1 - p;
    lua_Number bound;
    int which = 2;
    int status;
    if (phonc == 0) /* central? */
      cdff(&which, &p, &q, &f, &dfn, &dfd, &status, &bound);
    else /* non-central */
      cdffnc(&which, &p, &q, &f, &dfn, &dfd, &phonc, &status, &bound);
    check_status(L, status, bound);
  }
  lua_pushnumber(L, f);
  return 1;
}

/* {=======   Chi-square   =======} */

static void check_chisq (lua_State *L, int which, lua_Number x,
    lua_Number df, lua_Number pnonc) {
  luaL_argcheck(L, ((which == 1 && x >= 0)  /* x */
      || (which == 2 && (x >= 0 && x <= 1))), /* p */
      1, "out of range");
  if (pnonc == 0)
    luaL_argcheck(L, df > 0, 2, "positive value expected");
  else
    luaL_argcheck(L, df >= 0, 2, "non-negative value expected");
}

static int stat_dchisq (lua_State *L) {
  /* stack should contain x, df and opt. pnonc */
  lua_Number x = luaL_checknumber(L, 1);
  lua_Number df = luaL_checknumber(L, 2);
  lua_Number pnonc = luaL_optnumber(L, 3, 0);
  lua_Number d;
  check_chisq(L, 1, x, df, pnonc);
  /* compute central dchisq */
  d = df / 2;
  d = exp((d - 1) * log(x) - x / 2 - d * M_LN2 - dlngam(&d));
  /* compute non-central if that's the case */
  if (pnonc != 0) { /* non-central? */
    /* evaluate weighted series */
    int i;
    lua_Number t = d *= exp(-pnonc / 2); /* first term */
    for (i = 1; i < MAXITER && d > LBOUND && t > DBL_EPSILON * d; i++)
      d += t *= x * pnonc / (2 * i * (df + 2 * (i - 1)));
  }
  lua_pushnumber(L, d);
  return 1;
}

static int stat_pchisq (lua_State *L) {
  /* stack should contain x, df and opt. pnonc */
  lua_Number x = luaL_checknumber(L, 1);
  lua_Number df = luaL_checknumber(L, 2);
  lua_Number pnonc = luaL_optnumber(L, 3, 0);
  lua_Number p, q, bound;
  int which = 1;
  int status;
  check_chisq(L, 1, x, df, pnonc);
  if (pnonc == 0) /* central? */
    cdfchi(&which, &p, &q, &x, &df, &status, &bound);
  else /* non-central */
    cdfchn(&which, &p, &q, &x, &df, &pnonc, &status, &bound);
  check_status(L, status, bound);
  lua_pushnumber(L, p);
  return 1;
}

static int stat_qchisq (lua_State *L) {
  /* stack should contain p, df and opt. pnonc */
  lua_Number p = luaL_checknumber(L, 1);
  lua_Number df = luaL_checknumber(L, 2);
  lua_Number pnonc = luaL_optnumber(L, 3, 0);
  lua_Number x;
  check_chisq(L, 2, p, df, pnonc);
  if (p == 0 || p == 1) x = (p == 0) ? 0 : HUGE_VAL;
  else {
    lua_Number q = 1 - p;
    lua_Number bound;
    int which = 2;
    int status;
    if (pnonc == 0) /* central? */
      cdfchi(&which, &p, &q, &x, &df, &status, &bound);
    else /* non-central */
      cdfchn(&which, &p, &q, &x, &df, &pnonc, &status, &bound);
    check_status(L, status, bound);
  }
  lua_pushnumber(L, x);
  return 1;
}

static int fchoose (lua_Number n, lua_Number k) {
    lua_Number a = n - k + 1;
    lua_Number b = k + 1;
    return -dlnbet(&a, &b) - log(n + 1);
}

static int mathx_choose (lua_State *L) {
    lua_Number n = luaL_checknumber(L, 1);
    lua_Number k = luaL_checknumber(L, 2);
    lua_Number c;
    if (k < 0) c = 0;
    else if (k == 0) c = 1;
    /* k > 0 */
    else if (n < 0) c = exp(fchoose( k - n - 1, k));
    else if (n < k) c = 0;
    /* k <= n */
    else c = exp(fchoose(n, k));
    lua_pushnumber(L, c);
    return 1;
}

/* {=====================================================================
 *    Interface
 * ======================================================================} */

static const struct luaL_Reg cdf_funcs[] = {
    /* probability dists */
    {"dnorm", stat_dnorm},
    {"pnorm", stat_pnorm},
    {"qnorm", stat_qnorm},
    {"dstud", stat_dt},
    {"pstud", stat_pt},
    {"qstud", stat_qt},
    {"dfstat", stat_df},
    {"pfstat", stat_pf},
    {"qfstat", stat_qf},
    {"dchisq", stat_dchisq},
    {"pchisq", stat_pchisq},
    {"qchisq", stat_qchisq},
    {"choose", mathx_choose},
     {NULL, NULL}
};

int luaopen_lcdf (lua_State *L) {
    // create library
    luaL_newlib(L, cdf_funcs);
    return 1;
}

