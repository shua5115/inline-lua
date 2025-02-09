/*
** $Id: inlauxlib.h,v 1.88.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions for building Lua libraries
** See Copyright Notice in inlua.h
*/


#ifndef inlauxlib_h
#define inlauxlib_h


#include <stddef.h>
#include <stdio.h>

#include "inlua.h"


#if defined(INLUA_COMPAT_GETN)
INLUALIB_API int (inluaL_getn) (inlua_State *L, int t);
INLUALIB_API void (inluaL_setn) (inlua_State *L, int t, int n);
#else
#define inluaL_getn(L,i)          ((int)inlua_objlen(L, i))
#define inluaL_setn(L,i,j)        ((void)0)  /* no op! */
#endif

#if defined(INLUA_COMPAT_OPENLIB)
#define inluaI_openlib	inluaL_openlib
#endif


/* extra error code for `luaL_load' */
#define INLUA_ERRFILE     (INLUA_ERRERR+1)


typedef struct inluaL_Reg {
  const char *name;
  inlua_CFunction func;
} inluaL_Reg;



INLUALIB_API void (inluaI_openlib) (inlua_State *L, const char *libname,
                                const inluaL_Reg *l, int nup);
INLUALIB_API void (inluaL_register) (inlua_State *L, const char *libname,
                                const inluaL_Reg *l);
INLUALIB_API int (inluaL_getmetafield) (inlua_State *L, int obj, const char *e);
INLUALIB_API int (inluaL_callmeta) (inlua_State *L, int obj, const char *e);
INLUALIB_API int (inluaL_typerror) (inlua_State *L, int narg, const char *tname);
INLUALIB_API int (inluaL_argerror) (inlua_State *L, int numarg, const char *extramsg);
INLUALIB_API const char *(inluaL_checklstring) (inlua_State *L, int numArg,
                                                          size_t *l);
INLUALIB_API const char *(inluaL_optlstring) (inlua_State *L, int numArg,
                                          const char *def, size_t *l);
INLUALIB_API inlua_Number (inluaL_checknumber) (inlua_State *L, int numArg);
INLUALIB_API inlua_Number (inluaL_optnumber) (inlua_State *L, int nArg, inlua_Number def);

INLUALIB_API inlua_Integer (inluaL_checkinteger) (inlua_State *L, int numArg);
INLUALIB_API inlua_Integer (inluaL_optinteger) (inlua_State *L, int nArg,
                                          inlua_Integer def);

INLUALIB_API void (inluaL_checkstack) (inlua_State *L, int sz, const char *msg);
INLUALIB_API void (inluaL_checktype) (inlua_State *L, int narg, int t);
INLUALIB_API void (inluaL_checkany) (inlua_State *L, int narg);

INLUALIB_API int   (inluaL_newmetatable) (inlua_State *L, const char *tname);
INLUALIB_API void *(inluaL_checkudata) (inlua_State *L, int ud, const char *tname);

INLUALIB_API void (inluaL_where) (inlua_State *L, int lvl);
INLUALIB_API int (inluaL_error) (inlua_State *L, const char *fmt, ...);

INLUALIB_API int (inluaL_checkoption) (inlua_State *L, int narg, const char *def,
                                   const char *const lst[]);

INLUALIB_API int (inluaL_ref) (inlua_State *L, int t);
INLUALIB_API void (inluaL_unref) (inlua_State *L, int t, int ref);

INLUALIB_API int (inluaL_loadfile) (inlua_State *L, const char *filename);
INLUALIB_API int (inluaL_loadbuffer) (inlua_State *L, const char *buff, size_t sz,
                                  const char *name);
INLUALIB_API int (inluaL_loadstring) (inlua_State *L, const char *s);

INLUALIB_API inlua_State *(inluaL_newstate) (void);


INLUALIB_API const char *(inluaL_gsub) (inlua_State *L, const char *s, const char *p,
                                                  const char *r);

INLUALIB_API const char *(inluaL_findtable) (inlua_State *L, int idx,
                                         const char *fname, int szhint);




/*
** ===============================================================
** some useful macros
** ===============================================================
*/

#define inluaL_argcheck(L, cond,numarg,extramsg)	\
		((void)((cond) || inluaL_argerror(L, (numarg), (extramsg))))
#define inluaL_checkstring(L,n)	(inluaL_checklstring(L, (n), NULL))
#define inluaL_optstring(L,n,d)	(inluaL_optlstring(L, (n), (d), NULL))
#define inluaL_checkint(L,n)	((int)inluaL_checkinteger(L, (n)))
#define inluaL_optint(L,n,d)	((int)inluaL_optinteger(L, (n), (d)))
#define inluaL_checklong(L,n)	((long)inluaL_checkinteger(L, (n)))
#define inluaL_optlong(L,n,d)	((long)inluaL_optinteger(L, (n), (d)))

#define inluaL_typename(L,i)	inlua_typename(L, inlua_type(L,(i)))

#define inluaL_dofile(L, fn) \
	(inluaL_loadfile(L, fn) || inlua_pcall(L, 0, INLUA_MULTRET, 0))

#define inluaL_dostring(L, s) \
	(inluaL_loadstring(L, s) || inlua_pcall(L, 0, INLUA_MULTRET, 0))

#define inluaL_getmetatable(L,n)	(inlua_getfield(L, INLUA_REGISTRYINDEX, (n)))

#define inluaL_opt(L,f,n,d)	(inlua_isnoneornil(L,(n)) ? (d) : f(L,(n)))

/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/



typedef struct inluaL_Buffer {
  char *p;			/* current position in buffer */
  int lvl;  /* number of strings in the stack (level) */
  inlua_State *L;
  char buffer[INLUAL_BUFFERSIZE];
} inluaL_Buffer;

#define inluaL_addchar(B,c) \
  ((void)((B)->p < ((B)->buffer+INLUAL_BUFFERSIZE) || inluaL_prepbuffer(B)), \
   (*(B)->p++ = (char)(c)))

/* compatibility only */
#define inluaL_putchar(B,c)	inluaL_addchar(B,c)

#define inluaL_addsize(B,n)	((B)->p += (n))

INLUALIB_API void (inluaL_buffinit) (inlua_State *L, inluaL_Buffer *B);
INLUALIB_API char *(inluaL_prepbuffer) (inluaL_Buffer *B);
INLUALIB_API void (inluaL_addlstring) (inluaL_Buffer *B, const char *s, size_t l);
INLUALIB_API void (inluaL_addstring) (inluaL_Buffer *B, const char *s);
INLUALIB_API void (inluaL_addvalue) (inluaL_Buffer *B);
INLUALIB_API void (inluaL_pushresult) (inluaL_Buffer *B);


/* }====================================================== */


/* compatibility with ref system */

/* pre-defined references */
#define INLUA_NOREF       (-2)
#define INLUA_REFNIL      (-1)

#define inlua_ref(L,lock) ((lock) ? inluaL_ref(L, INLUA_REGISTRYINDEX) : \
      (inlua_pushstring(L, "unlocked references are obsolete"), inlua_error(L), 0))

#define inlua_unref(L,ref)        inluaL_unref(L, INLUA_REGISTRYINDEX, (ref))

#define inlua_getref(L,ref)       inlua_rawgeti(L, INLUA_REGISTRYINDEX, (ref))


#define inluaL_reg	inluaL_Reg

#endif


