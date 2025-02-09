/*
** $Id: loslib.c,v 1.19.1.3 2008/01/18 16:38:18 roberto Exp $
** Standard Operating System library
** See Copyright Notice in inlua.h
*/


#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define inloslib_c
#define INLUA_LIB

#include "inlua.h"

#include "inlauxlib.h"
#include "inlualib.h"


static int os_pushresult (inlua_State *L, int i, const char *filename) {
  int en = errno;  /* calls to Lua API may change this value */
  if (i) {
    inlua_pushboolean(L, 1);
    return 1;
  }
  else {
    inlua_pushnil(L);
    inlua_pushfstring(L, "%s: %s", filename, strerror(en));
    inlua_pushinteger(L, en);
    return 3;
  }
}


static int os_execute (inlua_State *L) {
  inlua_pushinteger(L, system(inluaL_optstring(L, 1, NULL)));
  return 1;
}


static int os_remove (inlua_State *L) {
  const char *filename = inluaL_checkstring(L, 1);
  return os_pushresult(L, remove(filename) == 0, filename);
}


static int os_rename (inlua_State *L) {
  const char *fromname = inluaL_checkstring(L, 1);
  const char *toname = inluaL_checkstring(L, 2);
  return os_pushresult(L, rename(fromname, toname) == 0, fromname);
}


static int os_tmpname (inlua_State *L) {
  char buff[INLUA_TMPNAMBUFSIZE];
  int err;
  inlua_tmpnam(buff, err);
  if (err)
    return inluaL_error(L, "unable to generate a unique filename");
  inlua_pushstring(L, buff);
  return 1;
}


static int os_getenv (inlua_State *L) {
  inlua_pushstring(L, getenv(inluaL_checkstring(L, 1)));  /* if NULL push nil */
  return 1;
}


static int os_clock (inlua_State *L) {
  inlua_pushnumber(L, ((inlua_Number)clock())/(inlua_Number)CLOCKS_PER_SEC);
  return 1;
}


/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

static void setfield (inlua_State *L, const char *key, int value) {
  inlua_pushinteger(L, value);
  inlua_setfield(L, -2, key);
}

static void setboolfield (inlua_State *L, const char *key, int value) {
  if (value < 0)  /* undefined? */
    return;  /* does not set field */
  inlua_pushboolean(L, value);
  inlua_setfield(L, -2, key);
}

static int getboolfield (inlua_State *L, const char *key) {
  int res;
  inlua_getfield(L, -1, key);
  res = inlua_isnil(L, -1) ? -1 : inlua_toboolean(L, -1);
  inlua_pop(L, 1);
  return res;
}


static int getfield (inlua_State *L, const char *key, int d) {
  int res;
  inlua_getfield(L, -1, key);
  if (inlua_isnumber(L, -1))
    res = (int)inlua_tointeger(L, -1);
  else {
    if (d < 0)
      return inluaL_error(L, "field " INLUA_QS " missing in date table", key);
    res = d;
  }
  inlua_pop(L, 1);
  return res;
}


static int os_date (inlua_State *L) {
  const char *s = inluaL_optstring(L, 1, "%c");
  time_t t = inluaL_opt(L, (time_t)inluaL_checknumber, 2, time(NULL));
  struct tm *stm;
  if (*s == '!') {  /* UTC? */
    stm = gmtime(&t);
    s++;  /* skip `!' */
  }
  else
    stm = localtime(&t);
  if (stm == NULL)  /* invalid date? */
    inlua_pushnil(L);
  else if (strcmp(s, "*t") == 0) {
    inlua_createtable(L, 0, 9);  /* 9 = number of fields */
    setfield(L, "sec", stm->tm_sec);
    setfield(L, "min", stm->tm_min);
    setfield(L, "hour", stm->tm_hour);
    setfield(L, "day", stm->tm_mday);
    setfield(L, "month", stm->tm_mon+1);
    setfield(L, "year", stm->tm_year+1900);
    setfield(L, "wday", stm->tm_wday+1);
    setfield(L, "yday", stm->tm_yday+1);
    setboolfield(L, "isdst", stm->tm_isdst);
  }
  else {
    char cc[3];
    inluaL_Buffer b;
    cc[0] = '%'; cc[2] = '\0';
    inluaL_buffinit(L, &b);
    for (; *s; s++) {
      if (*s != '%' || *(s + 1) == '\0')  /* no conversion specifier? */
        inluaL_addchar(&b, *s);
      else {
        size_t reslen;
        char buff[200];  /* should be big enough for any conversion result */
        cc[1] = *(++s);
        reslen = strftime(buff, sizeof(buff), cc, stm);
        inluaL_addlstring(&b, buff, reslen);
      }
    }
    inluaL_pushresult(&b);
  }
  return 1;
}


static int os_time (inlua_State *L) {
  time_t t;
  if (inlua_isnoneornil(L, 1))  /* called without args? */
    t = time(NULL);  /* get current time */
  else {
    struct tm ts;
    inluaL_checktype(L, 1, INLUA_TTABLE);
    inlua_settop(L, 1);  /* make sure table is at the top */
    ts.tm_sec = getfield(L, "sec", 0);
    ts.tm_min = getfield(L, "min", 0);
    ts.tm_hour = getfield(L, "hour", 12);
    ts.tm_mday = getfield(L, "day", -1);
    ts.tm_mon = getfield(L, "month", -1) - 1;
    ts.tm_year = getfield(L, "year", -1) - 1900;
    ts.tm_isdst = getboolfield(L, "isdst");
    t = mktime(&ts);
  }
  if (t == (time_t)(-1))
    inlua_pushnil(L);
  else
    inlua_pushnumber(L, (inlua_Number)t);
  return 1;
}


static int os_difftime (inlua_State *L) {
  inlua_pushnumber(L, difftime((time_t)(inluaL_checknumber(L, 1)),
                             (time_t)(inluaL_optnumber(L, 2, 0))));
  return 1;
}

/* }====================================================== */


static int os_setlocale (inlua_State *L) {
  static const int cat[] = {LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY,
                      LC_NUMERIC, LC_TIME};
  static const char *const catnames[] = {"all", "collate", "ctype", "monetary",
     "numeric", "time", NULL};
  const char *l = inluaL_optstring(L, 1, NULL);
  int op = inluaL_checkoption(L, 2, "all", catnames);
  inlua_pushstring(L, setlocale(cat[op], l));
  return 1;
}


static int os_exit (inlua_State *L) {
  exit(inluaL_optint(L, 1, EXIT_SUCCESS));
}

static const inluaL_Reg syslib[] = {
  {"clock",     os_clock},
  {"date",      os_date},
  {"difftime",  os_difftime},
  {"execute",   os_execute},
  {"exit",      os_exit},
  {"getenv",    os_getenv},
  {"remove",    os_remove},
  {"rename",    os_rename},
  {"setlocale", os_setlocale},
  {"time",      os_time},
  {"tmpname",   os_tmpname},
  {NULL, NULL}
};

/* }====================================================== */



INLUALIB_API int inluaopen_os (inlua_State *L) {
  inluaL_register(L, INLUA_OSLIBNAME, syslib);
  return 1;
}

