/*
** $Id: ldo.h,v 2.7.1.1 2007/12/27 13:02:25 roberto Exp $
** Stack and Call structure of Lua
** See Copyright Notice in inlua.h
*/

#ifndef ldo_h
#define ldo_h


#include "lobject.h"
#include "lstate.h"
#include "lzio.h"


#define luaD_checkstack(L,n)	\
  if ((char *)L->stack_last - (char *)L->top <= (n)*(int)sizeof(TValue)) \
    luaD_growstack(L, n); \
  else condhardstacktests(luaD_reallocstack(L, L->stacksize - EXTRA_STACK - 1));


#define incr_top(L) {luaD_checkstack(L,1); L->top++;}

#define savestack(L,p)		((char *)(p) - (char *)L->stack)
#define restorestack(L,n)	((TValue *)((char *)L->stack + (n)))

#define saveci(L,p)		((char *)(p) - (char *)L->base_ci)
#define restoreci(L,n)		((CallInfo *)((char *)L->base_ci + (n)))


/* results from luaD_precall */
#define PCRLUA		0	/* initiated a call to a Lua function */
#define PCRC		1	/* did a call to a C function */
#define PCRYIELD	2	/* C funtion yielded */


/* type of protected functions, to be ran by `runprotected' */
typedef void (*Pfunc) (inlua_State *L, void *ud);

INLUAI_FUNC int luaD_protectedparser (inlua_State *L, ZIO *z, const char *name);
INLUAI_FUNC void luaD_callhook (inlua_State *L, int event, int line);
INLUAI_FUNC int luaD_precall (inlua_State *L, StkId func, int nresults);
INLUAI_FUNC void luaD_call (inlua_State *L, StkId func, int nResults);
INLUAI_FUNC int luaD_pcall (inlua_State *L, Pfunc func, void *u,
                                        ptrdiff_t oldtop, ptrdiff_t ef);
INLUAI_FUNC int luaD_poscall (inlua_State *L, StkId firstResult);
INLUAI_FUNC void luaD_reallocCI (inlua_State *L, int newsize);
INLUAI_FUNC void luaD_reallocstack (inlua_State *L, int newsize);
INLUAI_FUNC void luaD_growstack (inlua_State *L, int n);

INLUAI_FUNC void luaD_throw (inlua_State *L, int errcode);
INLUAI_FUNC int luaD_rawrunprotected (inlua_State *L, Pfunc f, void *ud);

INLUAI_FUNC void luaD_seterrorobj (inlua_State *L, int errcode, StkId oldtop);

#endif

