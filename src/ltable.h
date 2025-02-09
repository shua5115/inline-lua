/*
** $Id: ltable.h,v 2.10.1.1 2007/12/27 13:02:25 roberto Exp $
** Lua tables (hash)
** See Copyright Notice in inlua.h
*/

#ifndef ltable_h
#define ltable_h

#include "lobject.h"


#define gnode(t,i)	(&(t)->node[i])
#define gkey(n)		(&(n)->i_key.nk)
#define gval(n)		(&(n)->i_val)
#define gnext(n)	((n)->i_key.nk.next)

#define key2tval(n)	(&(n)->i_key.tvk)


INLUAI_FUNC const TValue *luaH_getnum (Table *t, int key);
INLUAI_FUNC TValue *luaH_setnum (inlua_State *L, Table *t, int key);
INLUAI_FUNC const TValue *luaH_getstr (Table *t, TString *key);
INLUAI_FUNC TValue *luaH_setstr (inlua_State *L, Table *t, TString *key);
INLUAI_FUNC const TValue *luaH_get (Table *t, const TValue *key);
INLUAI_FUNC TValue *luaH_set (inlua_State *L, Table *t, const TValue *key);
INLUAI_FUNC Table *luaH_new (inlua_State *L, int narray, int lnhash);
INLUAI_FUNC void luaH_resizearray (inlua_State *L, Table *t, int nasize);
INLUAI_FUNC void luaH_free (inlua_State *L, Table *t);
INLUAI_FUNC int luaH_next (inlua_State *L, Table *t, StkId key);
INLUAI_FUNC int luaH_getn (Table *t);


#if defined(LUA_DEBUG)
INLUAI_FUNC Node *luaH_mainposition (const Table *t, const TValue *key);
INLUAI_FUNC int luaH_isdummy (Node *n);
#endif


#endif
