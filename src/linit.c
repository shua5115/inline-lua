/*
** $Id: linit.c,v 1.14.1.1 2007/12/27 13:02:25 roberto Exp $
** Initialization of libraries for lua.c
** See Copyright Notice in inlua.h
*/


#define linit_c
#define INLUA_LIB

#include "inlua.h"

#include "inlualib.h"
#include "inlauxlib.h"


static const inluaL_Reg lualibs[] = {
  {"", inluaopen_base},
  {INLUA_LOADLIBNAME, inluaopen_package},
  {INLUA_TABLIBNAME, inluaopen_table},
  {INLUA_IOLIBNAME, inluaopen_io},
  {INLUA_OSLIBNAME, inluaopen_os},
  {INLUA_STRLIBNAME, inluaopen_string},
  {INLUA_MATHLIBNAME, inluaopen_math},
  {INLUA_DBLIBNAME, inluaopen_debug},
  {NULL, NULL}
};


INLUALIB_API void inluaL_openlibs (inlua_State *L) {
  const inluaL_Reg *lib = lualibs;
  for (; lib->func; lib++) {
    inlua_pushcfunction(L, lib->func);
    inlua_pushstring(L, lib->name);
    inlua_call(L, 1, 0);
  }
}

