/*
** $Id: lvm.h,v 2.5.1.1 2007/12/27 13:02:25 roberto Exp $
** Lua virtual machine
** See Copyright Notice in inlua.h
*/

#ifndef lvm_h
#define lvm_h


#include "ldo.h"
#include "lobject.h"
#include "ltm.h"


#define tostring(L,o) ((ttype(o) == INLUA_TSTRING) || (luaV_tostring(L, o)))

#define tonumber(o,n)	(ttype(o) == INLUA_TNUMBER || \
                         (((o) = luaV_tonumber(o,n)) != NULL))

#define equalobj(L,o1,o2) \
	(ttype(o1) == ttype(o2) && luaV_equalval(L, o1, o2))


INLUAI_FUNC int luaV_lessthan (inlua_State *L, const TValue *l, const TValue *r);
INLUAI_FUNC int luaV_equalval (inlua_State *L, const TValue *t1, const TValue *t2);
INLUAI_FUNC const TValue *luaV_tonumber (const TValue *obj, TValue *n);
INLUAI_FUNC int luaV_tostring (inlua_State *L, StkId obj);
INLUAI_FUNC void luaV_gettable (inlua_State *L, const TValue *t, TValue *key,
                                            StkId val);
INLUAI_FUNC void luaV_settable (inlua_State *L, const TValue *t, TValue *key,
                                            StkId val);
INLUAI_FUNC void luaV_execute (inlua_State *L, int nexeccalls);
INLUAI_FUNC void luaV_concat (inlua_State *L, int total, int last);

#endif
