/*
** $Id: lmathlib.c,v 1.67.1.1 2007/12/27 13:02:25 roberto Exp $
** Standard mathematical library
** See Copyright Notice in inlua.h
*/


#include <stdlib.h>
#include <math.h>

#define lmathlib_c
#define INLUA_LIB

#include "inlua.h"

#include "inlauxlib.h"
#include "inlualib.h"


#undef PI
#define PI (3.14159265358979323846)
#define RADIANS_PER_DEGREE (PI/180.0)



static int math_abs (inlua_State *L) {
  inlua_pushnumber(L, fabs(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_sin (inlua_State *L) {
  inlua_pushnumber(L, sin(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_sinh (inlua_State *L) {
  inlua_pushnumber(L, sinh(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_cos (inlua_State *L) {
  inlua_pushnumber(L, cos(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_cosh (inlua_State *L) {
  inlua_pushnumber(L, cosh(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_tan (inlua_State *L) {
  inlua_pushnumber(L, tan(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_tanh (inlua_State *L) {
  inlua_pushnumber(L, tanh(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_asin (inlua_State *L) {
  inlua_pushnumber(L, asin(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_acos (inlua_State *L) {
  inlua_pushnumber(L, acos(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_atan (inlua_State *L) {
  inlua_pushnumber(L, atan(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_atan2 (inlua_State *L) {
  inlua_pushnumber(L, atan2(inluaL_checknumber(L, 1), inluaL_checknumber(L, 2)));
  return 1;
}

static int math_ceil (inlua_State *L) {
  inlua_pushnumber(L, ceil(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_floor (inlua_State *L) {
  inlua_pushnumber(L, floor(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_fmod (inlua_State *L) {
  inlua_pushnumber(L, fmod(inluaL_checknumber(L, 1), inluaL_checknumber(L, 2)));
  return 1;
}

static int math_modf (inlua_State *L) {
  double ip;
  double fp = modf(inluaL_checknumber(L, 1), &ip);
  inlua_pushnumber(L, ip);
  inlua_pushnumber(L, fp);
  return 2;
}

static int math_sqrt (inlua_State *L) {
  inlua_pushnumber(L, sqrt(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_pow (inlua_State *L) {
  inlua_pushnumber(L, pow(inluaL_checknumber(L, 1), inluaL_checknumber(L, 2)));
  return 1;
}

static int math_log (inlua_State *L) {
  inlua_pushnumber(L, log(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_log10 (inlua_State *L) {
  inlua_pushnumber(L, log10(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_exp (inlua_State *L) {
  inlua_pushnumber(L, exp(inluaL_checknumber(L, 1)));
  return 1;
}

static int math_deg (inlua_State *L) {
  inlua_pushnumber(L, inluaL_checknumber(L, 1)/RADIANS_PER_DEGREE);
  return 1;
}

static int math_rad (inlua_State *L) {
  inlua_pushnumber(L, inluaL_checknumber(L, 1)*RADIANS_PER_DEGREE);
  return 1;
}

static int math_frexp (inlua_State *L) {
  int e;
  inlua_pushnumber(L, frexp(inluaL_checknumber(L, 1), &e));
  inlua_pushinteger(L, e);
  return 2;
}

static int math_ldexp (inlua_State *L) {
  inlua_pushnumber(L, ldexp(inluaL_checknumber(L, 1), inluaL_checkint(L, 2)));
  return 1;
}



static int math_min (inlua_State *L) {
  int n = inlua_gettop(L);  /* number of arguments */
  inlua_Number dmin = inluaL_checknumber(L, 1);
  int i;
  for (i=2; i<=n; i++) {
    inlua_Number d = inluaL_checknumber(L, i);
    if (d < dmin)
      dmin = d;
  }
  inlua_pushnumber(L, dmin);
  return 1;
}


static int math_max (inlua_State *L) {
  int n = inlua_gettop(L);  /* number of arguments */
  inlua_Number dmax = inluaL_checknumber(L, 1);
  int i;
  for (i=2; i<=n; i++) {
    inlua_Number d = inluaL_checknumber(L, i);
    if (d > dmax)
      dmax = d;
  }
  inlua_pushnumber(L, dmax);
  return 1;
}


static int math_random (inlua_State *L) {
  /* the `%' avoids the (rare) case of r==1, and is needed also because on
     some systems (SunOS!) `rand()' may return a value larger than RAND_MAX */
  inlua_Number r = (inlua_Number)(rand()%RAND_MAX) / (inlua_Number)RAND_MAX;
  switch (inlua_gettop(L)) {  /* check number of arguments */
    case 0: {  /* no arguments */
      inlua_pushnumber(L, r);  /* Number between 0 and 1 */
      break;
    }
    case 1: {  /* only upper limit */
      int u = inluaL_checkint(L, 1);
      inluaL_argcheck(L, 1<=u, 1, "interval is empty");
      inlua_pushnumber(L, floor(r*u)+1);  /* int between 1 and `u' */
      break;
    }
    case 2: {  /* lower and upper limits */
      int l = inluaL_checkint(L, 1);
      int u = inluaL_checkint(L, 2);
      inluaL_argcheck(L, l<=u, 2, "interval is empty");
      inlua_pushnumber(L, floor(r*(u-l+1))+l);  /* int between `l' and `u' */
      break;
    }
    default: return inluaL_error(L, "wrong number of arguments");
  }
  return 1;
}


static int math_randomseed (inlua_State *L) {
  srand(inluaL_checkint(L, 1));
  return 0;
}


static const inluaL_Reg mathlib[] = {
  {"abs",   math_abs},
  {"acos",  math_acos},
  {"asin",  math_asin},
  {"atan2", math_atan2},
  {"atan",  math_atan},
  {"ceil",  math_ceil},
  {"cosh",   math_cosh},
  {"cos",   math_cos},
  {"deg",   math_deg},
  {"exp",   math_exp},
  {"floor", math_floor},
  {"fmod",   math_fmod},
  {"frexp", math_frexp},
  {"ldexp", math_ldexp},
  {"log10", math_log10},
  {"log",   math_log},
  {"max",   math_max},
  {"min",   math_min},
  {"modf",   math_modf},
  {"pow",   math_pow},
  {"rad",   math_rad},
  {"random",     math_random},
  {"randomseed", math_randomseed},
  {"sinh",   math_sinh},
  {"sin",   math_sin},
  {"sqrt",  math_sqrt},
  {"tanh",   math_tanh},
  {"tan",   math_tan},
  {NULL, NULL}
};


/*
** Open math library
*/
INLUALIB_API int inluaopen_math (inlua_State *L) {
  inluaL_register(L, INLUA_MATHLIBNAME, mathlib);
  inlua_pushnumber(L, PI);
  inlua_setfield(L, -2, "pi");
  inlua_pushnumber(L, HUGE_VAL);
  inlua_setfield(L, -2, "huge");
#if defined(INLUA_COMPAT_MOD)
  inlua_getfield(L, -1, "fmod");
  inlua_setfield(L, -2, "mod");
#endif
  return 1;
}

