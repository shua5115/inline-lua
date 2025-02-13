/*
** $Id: inlua.h,v 1.218.1.7 2012/01/13 20:36:20 roberto Exp $
** Lua - An Extensible Extension Language
** Lua.org, PUC-Rio, Brazil (http://www.lua.org)
** See Copyright Notice at the end of this file
*/


#ifndef inlua_h
#define inlua_h

#include <stdarg.h>
#include <stddef.h>


#include "inluaconf.h"

#define INLUA_VERSION "Inlua 1.0"
#define INLUA_RELEASE "Inlua 1.0.0"
#define INLUA_VERSION_NUM 100
#define INLUA_LUA_VERSION	"Lua 5.1"
#define INLUA_LUA_RELEASE	"Lua 5.1.5"
#define INLUA_LUA_VERSION_NUM	501
#define INLUA_LUA_COPYRIGHT	"Copyright (C) 1994-2012 Lua.org, PUC-Rio"
#define INLUA_LUA_AUTHORS 	"R. Ierusalimschy, L. H. de Figueiredo & W. Celes"


/* mark for precompiled code (`<esc>Lua') */
#define	INLUA_SIGNATURE	"\033Lua"

/* option for multiple returns in `inlua_pcall' and `inlua_call' */
#define INLUA_MULTRET	(-1)


/*
** pseudo-indices
*/
#define INLUA_REGISTRYINDEX	(-10000)
#define INLUA_ENVIRONINDEX	(-10001)
#define INLUA_GLOBALSINDEX	(-10002)
#define inlua_upvalueindex(i)	(INLUA_GLOBALSINDEX-(i))


/* thread status; 0 is OK */
#define INLUA_YIELD	1
#define INLUA_ERRRUN	2
#define INLUA_ERRSYNTAX	3
#define INLUA_ERRMEM	4
#define INLUA_ERRERR	5


typedef struct inlua_State inlua_State;

typedef int (*inlua_CFunction) (inlua_State *L);


/*
** functions that read/write blocks when loading/dumping Lua chunks
*/
typedef const char * (*inlua_Reader) (inlua_State *L, void *ud, size_t *sz);

typedef int (*inlua_Writer) (inlua_State *L, const void* p, size_t sz, void* ud);


/*
** prototype for memory-allocation functions
*/
typedef void * (*inlua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);


/*
** basic types
*/
#define INLUA_TNONE		(-1)

#define INLUA_TNIL		0
#define INLUA_TBOOLEAN		1
#define INLUA_TLIGHTUSERDATA	2
#define INLUA_TNUMBER		3
#define INLUA_TSTRING		4
#define INLUA_TTABLE		5
#define INLUA_TFUNCTION		6
#define INLUA_TUSERDATA		7
#define INLUA_TTHREAD		8



/* minimum Lua stack available to a C function */
#define INLUA_MINSTACK	20


/*
** generic extra include file
*/
#if defined(INLUA_USER_H)
#include INLUA_USER_H
#endif


/* type of numbers in Lua */
typedef INLUA_NUMBER inlua_Number;


/* type for integer functions */
typedef INLUA_INTEGER inlua_Integer;



/*
** state manipulation
*/
INLUA_API inlua_State *(inlua_newstate) (inlua_Alloc f, void *ud);
INLUA_API void       (inlua_close) (inlua_State *L);
INLUA_API inlua_State *(inlua_newthread) (inlua_State *L);

INLUA_API inlua_CFunction (inlua_atpanic) (inlua_State *L, inlua_CFunction panicf);


/*
** basic stack manipulation
*/
INLUA_API int   (inlua_gettop) (inlua_State *L);
INLUA_API void  (inlua_settop) (inlua_State *L, int idx);
INLUA_API void  (inlua_pushvalue) (inlua_State *L, int idx);
INLUA_API void  (inlua_remove) (inlua_State *L, int idx);
INLUA_API void  (inlua_insert) (inlua_State *L, int idx);
INLUA_API void  (inlua_replace) (inlua_State *L, int idx);
INLUA_API int   (inlua_checkstack) (inlua_State *L, int sz);

INLUA_API void  (inlua_xmove) (inlua_State *from, inlua_State *to, int n);


/*
** access functions (stack -> C)
*/

INLUA_API int             (inlua_isnumber) (inlua_State *L, int idx);
INLUA_API int             (inlua_isstring) (inlua_State *L, int idx);
INLUA_API int             (inlua_iscfunction) (inlua_State *L, int idx);
INLUA_API int             (inlua_isuserdata) (inlua_State *L, int idx);
INLUA_API int             (inlua_type) (inlua_State *L, int idx);
INLUA_API const char     *(inlua_typename) (inlua_State *L, int tp);

INLUA_API int            (inlua_equal) (inlua_State *L, int idx1, int idx2);
INLUA_API int            (inlua_rawequal) (inlua_State *L, int idx1, int idx2);
INLUA_API int            (inlua_lessthan) (inlua_State *L, int idx1, int idx2);

INLUA_API inlua_Number      (inlua_tonumber) (inlua_State *L, int idx);
INLUA_API inlua_Integer     (inlua_tointeger) (inlua_State *L, int idx);
INLUA_API int             (inlua_toboolean) (inlua_State *L, int idx);
INLUA_API const char     *(inlua_tolstring) (inlua_State *L, int idx, size_t *len);
INLUA_API size_t          (inlua_objlen) (inlua_State *L, int idx);
INLUA_API inlua_CFunction   (inlua_tocfunction) (inlua_State *L, int idx);
INLUA_API void	       *(inlua_touserdata) (inlua_State *L, int idx);
INLUA_API inlua_State      *(inlua_tothread) (inlua_State *L, int idx);
INLUA_API const void     *(inlua_topointer) (inlua_State *L, int idx);


/*
** push functions (C -> stack)
*/
INLUA_API void  (inlua_pushnil) (inlua_State *L);
INLUA_API void  (inlua_pushnumber) (inlua_State *L, inlua_Number n);
INLUA_API void  (inlua_pushinteger) (inlua_State *L, inlua_Integer n);
INLUA_API void  (inlua_pushlstring) (inlua_State *L, const char *s, size_t l);
INLUA_API void  (inlua_pushstring) (inlua_State *L, const char *s);
INLUA_API const char *(inlua_pushvfstring) (inlua_State *L, const char *fmt,
                                                      va_list argp);
INLUA_API const char *(inlua_pushfstring) (inlua_State *L, const char *fmt, ...);
INLUA_API void  (inlua_pushcclosure) (inlua_State *L, inlua_CFunction fn, int n);
INLUA_API void  (inlua_pushboolean) (inlua_State *L, int b);
INLUA_API void  (inlua_pushlightuserdata) (inlua_State *L, void *p);
INLUA_API int   (inlua_pushthread) (inlua_State *L);


/*
** get functions (Lua -> stack)
*/
INLUA_API void  (inlua_gettable) (inlua_State *L, int idx);
INLUA_API void  (inlua_getfield) (inlua_State *L, int idx, const char *k);
INLUA_API void  (inlua_rawget) (inlua_State *L, int idx);
INLUA_API void  (inlua_rawgeti) (inlua_State *L, int idx, int n);
INLUA_API void  (inlua_createtable) (inlua_State *L, int narr, int nrec);
INLUA_API void *(inlua_newuserdata) (inlua_State *L, size_t sz);
INLUA_API int   (inlua_getmetatable) (inlua_State *L, int objindex);
INLUA_API void  (inlua_getfenv) (inlua_State *L, int idx);


/*
** set functions (stack -> Lua)
*/
INLUA_API void  (inlua_settable) (inlua_State *L, int idx);
INLUA_API void  (inlua_setfield) (inlua_State *L, int idx, const char *k);
INLUA_API void  (inlua_rawset) (inlua_State *L, int idx);
INLUA_API void  (inlua_rawseti) (inlua_State *L, int idx, int n);
INLUA_API int   (inlua_setmetatable) (inlua_State *L, int objindex);
INLUA_API int   (inlua_setfenv) (inlua_State *L, int idx);


/*
** `load' and `call' functions (load and run Lua code)
*/
INLUA_API void  (inlua_call) (inlua_State *L, int nargs, int nresults);
INLUA_API int   (inlua_pcall) (inlua_State *L, int nargs, int nresults, int errfunc);
INLUA_API int   (inlua_cpcall) (inlua_State *L, inlua_CFunction func, void *ud);
INLUA_API int   (inlua_load) (inlua_State *L, inlua_Reader reader, void *dt,
                                        const char *chunkname);

INLUA_API int (inlua_dump) (inlua_State *L, inlua_Writer writer, void *data);


/*
** coroutine functions
*/
INLUA_API int  (inlua_yield) (inlua_State *L, int nresults);
INLUA_API int  (inlua_resume) (inlua_State *L, int narg);
INLUA_API int  (inlua_status) (inlua_State *L);

/*
** garbage-collection function and options
*/

#define INLUA_GCSTOP		0
#define INLUA_GCRESTART		1
#define INLUA_GCCOLLECT		2
#define INLUA_GCCOUNT		3
#define INLUA_GCCOUNTB		4
#define INLUA_GCSTEP		5
#define INLUA_GCSETPAUSE		6
#define INLUA_GCSETSTEPMUL	7

INLUA_API int (inlua_gc) (inlua_State *L, int what, int data);


/*
** miscellaneous functions
*/

INLUA_API int   (inlua_error) (inlua_State *L);

INLUA_API int   (inlua_next) (inlua_State *L, int idx);

INLUA_API void  (inlua_concat) (inlua_State *L, int n);

INLUA_API inlua_Alloc (inlua_getallocf) (inlua_State *L, void **ud);
INLUA_API void inlua_setallocf (inlua_State *L, inlua_Alloc f, void *ud);



/* 
** ===============================================================
** some useful macros
** ===============================================================
*/

#define inlua_pop(L,n)		inlua_settop(L, -(n)-1)

#define inlua_newtable(L)		inlua_createtable(L, 0, 0)

#define inlua_register(L,n,f) (inlua_pushcfunction(L, (f)), inlua_setglobal(L, (n)))

#define inlua_pushcfunction(L,f)	inlua_pushcclosure(L, (f), 0)

#define inlua_strlen(L,i)		inlua_objlen(L, (i))

#define inlua_isfunction(L,n)	(inlua_type(L, (n)) == INLUA_TFUNCTION)
#define inlua_istable(L,n)	(inlua_type(L, (n)) == INLUA_TTABLE)
#define inlua_islightuserdata(L,n)	(inlua_type(L, (n)) == INLUA_TLIGHTUSERDATA)
#define inlua_isnil(L,n)		(inlua_type(L, (n)) == INLUA_TNIL)
#define inlua_isboolean(L,n)	(inlua_type(L, (n)) == INLUA_TBOOLEAN)
#define inlua_isthread(L,n)	(inlua_type(L, (n)) == INLUA_TTHREAD)
#define inlua_isnone(L,n)		(inlua_type(L, (n)) == INLUA_TNONE)
#define inlua_isnoneornil(L, n)	(inlua_type(L, (n)) <= 0)

#define inlua_pushliteral(L, s)	\
	inlua_pushlstring(L, "" s, (sizeof(s)/sizeof(char))-1)

#define inlua_setglobal(L,s)	inlua_setfield(L, INLUA_GLOBALSINDEX, (s))
#define inlua_getglobal(L,s)	inlua_getfield(L, INLUA_GLOBALSINDEX, (s))

#define inlua_tostring(L,i)	inlua_tolstring(L, (i), NULL)



/*
** compatibility macros and functions
*/

#define inlua_open()	inluaL_newstate()

#define inlua_getregistry(L)	inlua_pushvalue(L, INLUA_REGISTRYINDEX)

#define inlua_getgccount(L)	inlua_gc(L, INLUA_GCCOUNT, 0)

#define inlua_Chunkreader		inlua_Reader
#define inlua_Chunkwriter		inlua_Writer


/* hack */
INLUA_API void inlua_setlevel	(inlua_State *from, inlua_State *to);


/*
** {======================================================================
** Debug API
** =======================================================================
*/


/*
** Event codes
*/
#define INLUA_HOOKCALL	0
#define INLUA_HOOKRET	1
#define INLUA_HOOKLINE	2
#define INLUA_HOOKCOUNT	3
#define INLUA_HOOKTAILRET 4


/*
** Event masks
*/
#define INLUA_MASKCALL	(1 << INLUA_HOOKCALL)
#define INLUA_MASKRET	(1 << INLUA_HOOKRET)
#define INLUA_MASKLINE	(1 << INLUA_HOOKLINE)
#define INLUA_MASKCOUNT	(1 << INLUA_HOOKCOUNT)

typedef struct inlua_Debug inlua_Debug;  /* activation record */


/* Functions to be called by the debuger in specific events */
typedef void (*inlua_Hook) (inlua_State *L, inlua_Debug *ar);


INLUA_API int inlua_getstack (inlua_State *L, int level, inlua_Debug *ar);
INLUA_API int inlua_getinfo (inlua_State *L, const char *what, inlua_Debug *ar);
INLUA_API const char *inlua_getlocal (inlua_State *L, const inlua_Debug *ar, int n);
INLUA_API const char *inlua_setlocal (inlua_State *L, const inlua_Debug *ar, int n);
INLUA_API const char *inlua_getupvalue (inlua_State *L, int funcindex, int n);
INLUA_API const char *inlua_setupvalue (inlua_State *L, int funcindex, int n);

INLUA_API int inlua_sethook (inlua_State *L, inlua_Hook func, int mask, int count);
INLUA_API inlua_Hook inlua_gethook (inlua_State *L);
INLUA_API int inlua_gethookmask (inlua_State *L);
INLUA_API int inlua_gethookcount (inlua_State *L);


struct inlua_Debug {
  int event;
  const char *name;	/* (n) */
  const char *namewhat;	/* (n) `global', `local', `field', `method' */
  const char *what;	/* (S) `Lua', `C', `main', `tail' */
  const char *source;	/* (S) */
  int currentline;	/* (l) */
  int nups;		/* (u) number of upvalues */
  int linedefined;	/* (S) */
  int lastlinedefined;	/* (S) */
  char short_src[INLUA_IDSIZE]; /* (S) */
  /* private part */
  int i_ci;  /* active function */
};

/* }====================================================================== */


/******************************************************************************
* Copyright (C) 1994-2012 Lua.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/


#endif
