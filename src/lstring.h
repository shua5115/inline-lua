/*
** $Id: lstring.h,v 1.43.1.1 2007/12/27 13:02:25 roberto Exp $
** String table (keep all strings handled by Lua)
** See Copyright Notice in inlua.h
*/

#ifndef lstring_h
#define lstring_h


#include "lgc.h"
#include "lobject.h"
#include "lstate.h"


#define sizestring(s)	(sizeof(union TString)+((s)->len+1)*sizeof(char))

#define sizeudata(u)	(sizeof(union Udata)+(u)->len)

#define luaS_new(L, s)	(luaS_newlstr(L, s, strlen(s)))
#define luaS_newliteral(L, s)	(luaS_newlstr(L, "" s, \
                                 (sizeof(s)/sizeof(char))-1))

#define luaS_fix(s)	l_setbit((s)->tsv.marked, FIXEDBIT)

INLUAI_FUNC void luaS_resize (inlua_State *L, int newsize);
INLUAI_FUNC Udata *luaS_newudata (inlua_State *L, size_t s, Table *e);
INLUAI_FUNC TString *luaS_newlstr (inlua_State *L, const char *str, size_t l);


#endif
