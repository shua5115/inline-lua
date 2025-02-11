/*
** $Id: ldebug.h,v 2.3.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions from Debug Interface module
** See Copyright Notice in inlua.h
*/

#ifndef ldebug_h
#define ldebug_h


#include "lstate.h"


#define pcRel(pc, p)	(cast(int, (pc) - (p)->code) - 1)

#define lgetline(f,pc)	(((f)->lineinfo) ? (f)->lineinfo[pc] : 0)

#define resethookcount(L)	(L->hookcount = L->basehookcount)


INLUAI_FUNC void luaG_typeerror (inlua_State *L, const TValue *o,
                                             const char *opname);
INLUAI_FUNC void luaG_concaterror (inlua_State *L, StkId p1, StkId p2);
INLUAI_FUNC void luaG_aritherror (inlua_State *L, const TValue *p1,
                                              const TValue *p2);
INLUAI_FUNC int luaG_ordererror (inlua_State *L, const TValue *p1,
                                             const TValue *p2);
INLUAI_FUNC void luaG_runerror (inlua_State *L, const char *fmt, ...);
INLUAI_FUNC void luaG_errormsg (inlua_State *L);
INLUAI_FUNC int luaG_checkcode (const Proto *pt);
INLUAI_FUNC int luaG_checkopenop (Instruction i);

#endif
