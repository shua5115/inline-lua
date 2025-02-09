/*
** $Id: lauxlib.c,v 1.159.1.3 2008/01/21 13:20:51 roberto Exp $
** Auxiliary functions for building Lua libraries
** See Copyright Notice in inlua.h
*/


#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* This file uses only the official API of Lua.
** Any function declared here could be written as an application function.
*/

#define lauxlib_c
#define INLUA_LIB

#include "inlua.h"

#include "inlauxlib.h"


#define FREELIST_REF	0	/* free list of references */


/* convert a stack index to positive */
#define abs_index(L, i)		((i) > 0 || (i) <= INLUA_REGISTRYINDEX ? (i) : \
					inlua_gettop(L) + (i) + 1)


/*
** {======================================================
** Error-report functions
** =======================================================
*/


INLUALIB_API int inluaL_argerror (inlua_State *L, int narg, const char *extramsg) {
  inlua_Debug ar;
  if (!inlua_getstack(L, 0, &ar))  /* no stack frame? */
    return inluaL_error(L, "bad argument #%d (%s)", narg, extramsg);
  inlua_getinfo(L, "n", &ar);
  if (strcmp(ar.namewhat, "method") == 0) {
    narg--;  /* do not count `self' */
    if (narg == 0)  /* error is in the self argument itself? */
      return inluaL_error(L, "calling " INLUA_QS " on bad self (%s)",
                           ar.name, extramsg);
  }
  if (ar.name == NULL)
    ar.name = "?";
  return inluaL_error(L, "bad argument #%d to " INLUA_QS " (%s)",
                        narg, ar.name, extramsg);
}


INLUALIB_API int inluaL_typerror (inlua_State *L, int narg, const char *tname) {
  const char *msg = inlua_pushfstring(L, "%s expected, got %s",
                                    tname, inluaL_typename(L, narg));
  return inluaL_argerror(L, narg, msg);
}


static void tag_error (inlua_State *L, int narg, int tag) {
  inluaL_typerror(L, narg, inlua_typename(L, tag));
}


INLUALIB_API void inluaL_where (inlua_State *L, int level) {
  inlua_Debug ar;
  if (inlua_getstack(L, level, &ar)) {  /* check function at level */
    inlua_getinfo(L, "Sl", &ar);  /* get info about it */
    if (ar.currentline > 0) {  /* is there info? */
      inlua_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
      return;
    }
  }
  inlua_pushliteral(L, "");  /* else, no information available... */
}


INLUALIB_API int inluaL_error (inlua_State *L, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  inluaL_where(L, 1);
  inlua_pushvfstring(L, fmt, argp);
  va_end(argp);
  inlua_concat(L, 2);
  return inlua_error(L);
}

/* }====================================================== */


INLUALIB_API int inluaL_checkoption (inlua_State *L, int narg, const char *def,
                                 const char *const lst[]) {
  const char *name = (def) ? inluaL_optstring(L, narg, def) :
                             inluaL_checkstring(L, narg);
  int i;
  for (i=0; lst[i]; i++)
    if (strcmp(lst[i], name) == 0)
      return i;
  return inluaL_argerror(L, narg,
                       inlua_pushfstring(L, "invalid option " INLUA_QS, name));
}


INLUALIB_API int inluaL_newmetatable (inlua_State *L, const char *tname) {
  inlua_getfield(L, INLUA_REGISTRYINDEX, tname);  /* get registry.name */
  if (!inlua_isnil(L, -1))  /* name already in use? */
    return 0;  /* leave previous value on top, but return 0 */
  inlua_pop(L, 1);
  inlua_newtable(L);  /* create metatable */
  inlua_pushvalue(L, -1);
  inlua_setfield(L, INLUA_REGISTRYINDEX, tname);  /* registry.name = metatable */
  return 1;
}


INLUALIB_API void *inluaL_checkudata (inlua_State *L, int ud, const char *tname) {
  void *p = inlua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (inlua_getmetatable(L, ud)) {  /* does it have a metatable? */
      inlua_getfield(L, INLUA_REGISTRYINDEX, tname);  /* get correct metatable */
      if (inlua_rawequal(L, -1, -2)) {  /* does it have the correct mt? */
        inlua_pop(L, 2);  /* remove both metatables */
        return p;
      }
    }
  }
  inluaL_typerror(L, ud, tname);  /* else error */
  return NULL;  /* to avoid warnings */
}


INLUALIB_API void inluaL_checkstack (inlua_State *L, int space, const char *mes) {
  if (!inlua_checkstack(L, space))
    inluaL_error(L, "stack overflow (%s)", mes);
}


INLUALIB_API void inluaL_checktype (inlua_State *L, int narg, int t) {
  if (inlua_type(L, narg) != t)
    tag_error(L, narg, t);
}


INLUALIB_API void inluaL_checkany (inlua_State *L, int narg) {
  if (inlua_type(L, narg) == INLUA_TNONE)
    inluaL_argerror(L, narg, "value expected");
}


INLUALIB_API const char *inluaL_checklstring (inlua_State *L, int narg, size_t *len) {
  const char *s = inlua_tolstring(L, narg, len);
  if (!s) tag_error(L, narg, INLUA_TSTRING);
  return s;
}


INLUALIB_API const char *inluaL_optlstring (inlua_State *L, int narg,
                                        const char *def, size_t *len) {
  if (inlua_isnoneornil(L, narg)) {
    if (len)
      *len = (def ? strlen(def) : 0);
    return def;
  }
  else return inluaL_checklstring(L, narg, len);
}


INLUALIB_API inlua_Number inluaL_checknumber (inlua_State *L, int narg) {
  inlua_Number d = inlua_tonumber(L, narg);
  if (d == 0 && !inlua_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, INLUA_TNUMBER);
  return d;
}


INLUALIB_API inlua_Number inluaL_optnumber (inlua_State *L, int narg, inlua_Number def) {
  return inluaL_opt(L, inluaL_checknumber, narg, def);
}


INLUALIB_API inlua_Integer inluaL_checkinteger (inlua_State *L, int narg) {
  inlua_Integer d = inlua_tointeger(L, narg);
  if (d == 0 && !inlua_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, INLUA_TNUMBER);
  return d;
}


INLUALIB_API inlua_Integer inluaL_optinteger (inlua_State *L, int narg,
                                                      inlua_Integer def) {
  return inluaL_opt(L, inluaL_checkinteger, narg, def);
}


INLUALIB_API int inluaL_getmetafield (inlua_State *L, int obj, const char *event) {
  if (!inlua_getmetatable(L, obj))  /* no metatable? */
    return 0;
  inlua_pushstring(L, event);
  inlua_rawget(L, -2);
  if (inlua_isnil(L, -1)) {
    inlua_pop(L, 2);  /* remove metatable and metafield */
    return 0;
  }
  else {
    inlua_remove(L, -2);  /* remove only metatable */
    return 1;
  }
}


INLUALIB_API int inluaL_callmeta (inlua_State *L, int obj, const char *event) {
  obj = abs_index(L, obj);
  if (!inluaL_getmetafield(L, obj, event))  /* no metafield? */
    return 0;
  inlua_pushvalue(L, obj);
  inlua_call(L, 1, 1);
  return 1;
}


INLUALIB_API void (inluaL_register) (inlua_State *L, const char *libname,
                                const inluaL_Reg *l) {
  inluaI_openlib(L, libname, l, 0);
}


static int libsize (const inluaL_Reg *l) {
  int size = 0;
  for (; l->name; l++) size++;
  return size;
}


INLUALIB_API void inluaI_openlib (inlua_State *L, const char *libname,
                              const inluaL_Reg *l, int nup) {
  if (libname) {
    int size = libsize(l);
    /* check whether lib already exists */
    inluaL_findtable(L, INLUA_REGISTRYINDEX, "_LOADED", 1);
    inlua_getfield(L, -1, libname);  /* get _LOADED[libname] */
    if (!inlua_istable(L, -1)) {  /* not found? */
      inlua_pop(L, 1);  /* remove previous result */
      /* try global variable (and create one if it does not exist) */
      if (inluaL_findtable(L, INLUA_GLOBALSINDEX, libname, size) != NULL)
        inluaL_error(L, "name conflict for module " INLUA_QS, libname);
      inlua_pushvalue(L, -1);
      inlua_setfield(L, -3, libname);  /* _LOADED[libname] = new table */
    }
    inlua_remove(L, -2);  /* remove _LOADED table */
    inlua_insert(L, -(nup+1));  /* move library table to below upvalues */
  }
  for (; l->name; l++) {
    int i;
    for (i=0; i<nup; i++)  /* copy upvalues to the top */
      inlua_pushvalue(L, -nup);
    inlua_pushcclosure(L, l->func, nup);
    inlua_setfield(L, -(nup+2), l->name);
  }
  inlua_pop(L, nup);  /* remove upvalues */
}



/*
** {======================================================
** getn-setn: size for arrays
** =======================================================
*/

#if defined(INLUA_COMPAT_GETN)

static int checkint (inlua_State *L, int topop) {
  int n = (inlua_type(L, -1) == INLUA_TNUMBER) ? inlua_tointeger(L, -1) : -1;
  inlua_pop(L, topop);
  return n;
}


static void getsizes (inlua_State *L) {
  inlua_getfield(L, INLUA_REGISTRYINDEX, "LUA_SIZES");
  if (inlua_isnil(L, -1)) {  /* no `size' table? */
    inlua_pop(L, 1);  /* remove nil */
    inlua_newtable(L);  /* create it */
    inlua_pushvalue(L, -1);  /* `size' will be its own metatable */
    inlua_setmetatable(L, -2);
    inlua_pushliteral(L, "kv");
    inlua_setfield(L, -2, "__mode");  /* metatable(N).__mode = "kv" */
    inlua_pushvalue(L, -1);
    inlua_setfield(L, INLUA_REGISTRYINDEX, "LUA_SIZES");  /* store in register */
  }
}


INLUALIB_API void inluaL_setn (inlua_State *L, int t, int n) {
  t = abs_index(L, t);
  inlua_pushliteral(L, "n");
  inlua_rawget(L, t);
  if (checkint(L, 1) >= 0) {  /* is there a numeric field `n'? */
    inlua_pushliteral(L, "n");  /* use it */
    inlua_pushinteger(L, n);
    inlua_rawset(L, t);
  }
  else {  /* use `sizes' */
    getsizes(L);
    inlua_pushvalue(L, t);
    inlua_pushinteger(L, n);
    inlua_rawset(L, -3);  /* sizes[t] = n */
    inlua_pop(L, 1);  /* remove `sizes' */
  }
}


INLUALIB_API int inluaL_getn (inlua_State *L, int t) {
  int n;
  t = abs_index(L, t);
  inlua_pushliteral(L, "n");  /* try t.n */
  inlua_rawget(L, t);
  if ((n = checkint(L, 1)) >= 0) return n;
  getsizes(L);  /* else try sizes[t] */
  inlua_pushvalue(L, t);
  inlua_rawget(L, -2);
  if ((n = checkint(L, 2)) >= 0) return n;
  return (int)inlua_objlen(L, t);
}

#endif

/* }====================================================== */



INLUALIB_API const char *inluaL_gsub (inlua_State *L, const char *s, const char *p,
                                                               const char *r) {
  const char *wild;
  size_t l = strlen(p);
  inluaL_Buffer b;
  inluaL_buffinit(L, &b);
  while ((wild = strstr(s, p)) != NULL) {
    inluaL_addlstring(&b, s, wild - s);  /* push prefix */
    inluaL_addstring(&b, r);  /* push replacement in place of pattern */
    s = wild + l;  /* continue after `p' */
  }
  inluaL_addstring(&b, s);  /* push last suffix */
  inluaL_pushresult(&b);
  return inlua_tostring(L, -1);
}


INLUALIB_API const char *inluaL_findtable (inlua_State *L, int idx,
                                       const char *fname, int szhint) {
  const char *e;
  inlua_pushvalue(L, idx);
  do {
    e = strchr(fname, '.');
    if (e == NULL) e = fname + strlen(fname);
    inlua_pushlstring(L, fname, e - fname);
    inlua_rawget(L, -2);
    if (inlua_isnil(L, -1)) {  /* no such field? */
      inlua_pop(L, 1);  /* remove this nil */
      inlua_createtable(L, 0, (*e == '.' ? 1 : szhint)); /* new table for field */
      inlua_pushlstring(L, fname, e - fname);
      inlua_pushvalue(L, -2);
      inlua_settable(L, -4);  /* set new table into field */
    }
    else if (!inlua_istable(L, -1)) {  /* field has a non-table value? */
      inlua_pop(L, 2);  /* remove table and value */
      return fname;  /* return problematic part of the name */
    }
    inlua_remove(L, -2);  /* remove previous table */
    fname = e + 1;
  } while (*e == '.');
  return NULL;
}



/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/


#define bufflen(B)	((B)->p - (B)->buffer)
#define bufffree(B)	((size_t)(INLUAL_BUFFERSIZE - bufflen(B)))

#define LIMIT	(INLUA_MINSTACK/2)


static int emptybuffer (inluaL_Buffer *B) {
  size_t l = bufflen(B);
  if (l == 0) return 0;  /* put nothing on stack */
  else {
    inlua_pushlstring(B->L, B->buffer, l);
    B->p = B->buffer;
    B->lvl++;
    return 1;
  }
}


static void adjuststack (inluaL_Buffer *B) {
  if (B->lvl > 1) {
    inlua_State *L = B->L;
    int toget = 1;  /* number of levels to concat */
    size_t toplen = inlua_strlen(L, -1);
    do {
      size_t l = inlua_strlen(L, -(toget+1));
      if (B->lvl - toget + 1 >= LIMIT || toplen > l) {
        toplen += l;
        toget++;
      }
      else break;
    } while (toget < B->lvl);
    inlua_concat(L, toget);
    B->lvl = B->lvl - toget + 1;
  }
}


INLUALIB_API char *inluaL_prepbuffer (inluaL_Buffer *B) {
  if (emptybuffer(B))
    adjuststack(B);
  return B->buffer;
}


INLUALIB_API void inluaL_addlstring (inluaL_Buffer *B, const char *s, size_t l) {
  while (l--)
    inluaL_addchar(B, *s++);
}


INLUALIB_API void inluaL_addstring (inluaL_Buffer *B, const char *s) {
  inluaL_addlstring(B, s, strlen(s));
}


INLUALIB_API void inluaL_pushresult (inluaL_Buffer *B) {
  emptybuffer(B);
  inlua_concat(B->L, B->lvl);
  B->lvl = 1;
}


INLUALIB_API void inluaL_addvalue (inluaL_Buffer *B) {
  inlua_State *L = B->L;
  size_t vl;
  const char *s = inlua_tolstring(L, -1, &vl);
  if (vl <= bufffree(B)) {  /* fit into buffer? */
    memcpy(B->p, s, vl);  /* put it there */
    B->p += vl;
    inlua_pop(L, 1);  /* remove from stack */
  }
  else {
    if (emptybuffer(B))
      inlua_insert(L, -2);  /* put buffer before new value */
    B->lvl++;  /* add new value into B stack */
    adjuststack(B);
  }
}


INLUALIB_API void inluaL_buffinit (inlua_State *L, inluaL_Buffer *B) {
  B->L = L;
  B->p = B->buffer;
  B->lvl = 0;
}

/* }====================================================== */


INLUALIB_API int inluaL_ref (inlua_State *L, int t) {
  int ref;
  t = abs_index(L, t);
  if (inlua_isnil(L, -1)) {
    inlua_pop(L, 1);  /* remove from stack */
    return INLUA_REFNIL;  /* `nil' has a unique fixed reference */
  }
  inlua_rawgeti(L, t, FREELIST_REF);  /* get first free element */
  ref = (int)inlua_tointeger(L, -1);  /* ref = t[FREELIST_REF] */
  inlua_pop(L, 1);  /* remove it from stack */
  if (ref != 0) {  /* any free element? */
    inlua_rawgeti(L, t, ref);  /* remove it from list */
    inlua_rawseti(L, t, FREELIST_REF);  /* (t[FREELIST_REF] = t[ref]) */
  }
  else {  /* no free elements */
    ref = (int)inlua_objlen(L, t);
    ref++;  /* create new reference */
  }
  inlua_rawseti(L, t, ref);
  return ref;
}


INLUALIB_API void inluaL_unref (inlua_State *L, int t, int ref) {
  if (ref >= 0) {
    t = abs_index(L, t);
    inlua_rawgeti(L, t, FREELIST_REF);
    inlua_rawseti(L, t, ref);  /* t[ref] = t[FREELIST_REF] */
    inlua_pushinteger(L, ref);
    inlua_rawseti(L, t, FREELIST_REF);  /* t[FREELIST_REF] = ref */
  }
}



/*
** {======================================================
** Load functions
** =======================================================
*/

typedef struct LoadF {
  int extraline;
  FILE *f;
  char buff[INLUAL_BUFFERSIZE];
} LoadF;


static const char *getF (inlua_State *L, void *ud, size_t *size) {
  LoadF *lf = (LoadF *)ud;
  (void)L;
  if (lf->extraline) {
    lf->extraline = 0;
    *size = 1;
    return "\n";
  }
  if (feof(lf->f)) return NULL;
  *size = fread(lf->buff, 1, sizeof(lf->buff), lf->f);
  return (*size > 0) ? lf->buff : NULL;
}


static int errfile (inlua_State *L, const char *what, int fnameindex) {
  const char *serr = strerror(errno);
  const char *filename = inlua_tostring(L, fnameindex) + 1;
  inlua_pushfstring(L, "cannot %s %s: %s", what, filename, serr);
  inlua_remove(L, fnameindex);
  return INLUA_ERRFILE;
}


INLUALIB_API int inluaL_loadfile (inlua_State *L, const char *filename) {
  LoadF lf;
  int status, readstatus;
  int c;
  int fnameindex = inlua_gettop(L) + 1;  /* index of filename on the stack */
  lf.extraline = 0;
  if (filename == NULL) {
    inlua_pushliteral(L, "=stdin");
    lf.f = stdin;
  }
  else {
    inlua_pushfstring(L, "@%s", filename);
    lf.f = fopen(filename, "r");
    if (lf.f == NULL) return errfile(L, "open", fnameindex);
  }
  c = getc(lf.f);
  if (c == '#') {  /* Unix exec. file? */
    lf.extraline = 1;
    while ((c = getc(lf.f)) != EOF && c != '\n') ;  /* skip first line */
    if (c == '\n') c = getc(lf.f);
  }
  if (c == INLUA_SIGNATURE[0] && filename) {  /* binary file? */
    lf.f = freopen(filename, "rb", lf.f);  /* reopen in binary mode */
    if (lf.f == NULL) return errfile(L, "reopen", fnameindex);
    /* skip eventual `#!...' */
   while ((c = getc(lf.f)) != EOF && c != INLUA_SIGNATURE[0]) ;
    lf.extraline = 0;
  }
  ungetc(c, lf.f);
  status = inlua_load(L, getF, &lf, inlua_tostring(L, -1));
  readstatus = ferror(lf.f);
  if (filename) fclose(lf.f);  /* close file (even in case of errors) */
  if (readstatus) {
    inlua_settop(L, fnameindex);  /* ignore results from `inlua_load' */
    return errfile(L, "read", fnameindex);
  }
  inlua_remove(L, fnameindex);
  return status;
}


typedef struct LoadS {
  const char *s;
  size_t size;
} LoadS;


static const char *getS (inlua_State *L, void *ud, size_t *size) {
  LoadS *ls = (LoadS *)ud;
  (void)L;
  if (ls->size == 0) return NULL;
  *size = ls->size;
  ls->size = 0;
  return ls->s;
}


INLUALIB_API int inluaL_loadbuffer (inlua_State *L, const char *buff, size_t size,
                                const char *name) {
  LoadS ls;
  ls.s = buff;
  ls.size = size;
  return inlua_load(L, getS, &ls, name);
}


INLUALIB_API int (inluaL_loadstring) (inlua_State *L, const char *s) {
  return inluaL_loadbuffer(L, s, strlen(s), s);
}



/* }====================================================== */


static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud;
  (void)osize;
  if (nsize == 0) {
    free(ptr);
    return NULL;
  }
  else
    return realloc(ptr, nsize);
}


static int panic (inlua_State *L) {
  (void)L;  /* to avoid warnings */
  fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n",
                   inlua_tostring(L, -1));
  return 0;
}


INLUALIB_API inlua_State *inluaL_newstate (void) {
  inlua_State *L = inlua_newstate(l_alloc, NULL);
  if (L) inlua_atpanic(L, &panic);
  return L;
}

