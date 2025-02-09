/*
** $Id: lfunc.h,v 2.4.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in inlua.h
*/

#ifndef lfunc_h
#define lfunc_h


#include "lobject.h"


#define sizeCclosure(n)	(cast(int, sizeof(CClosure)) + \
                         cast(int, sizeof(TValue)*((n)-1)))

#define sizeLclosure(n)	(cast(int, sizeof(LClosure)) + \
                         cast(int, sizeof(TValue *)*((n)-1)))


INLUAI_FUNC Proto *luaF_newproto (inlua_State *L);
INLUAI_FUNC Closure *luaF_newCclosure (inlua_State *L, int nelems, Table *e);
INLUAI_FUNC Closure *luaF_newLclosure (inlua_State *L, int nelems, Table *e);
INLUAI_FUNC UpVal *luaF_newupval (inlua_State *L);
INLUAI_FUNC UpVal *luaF_findupval (inlua_State *L, StkId level);
INLUAI_FUNC void luaF_close (inlua_State *L, StkId level);
INLUAI_FUNC void luaF_freeproto (inlua_State *L, Proto *f);
INLUAI_FUNC void luaF_freeclosure (inlua_State *L, Closure *c);
INLUAI_FUNC void luaF_freeupval (inlua_State *L, UpVal *uv);
INLUAI_FUNC const char *luaF_getlocalname (const Proto *func, int local_number,
                                         int pc);


#endif
