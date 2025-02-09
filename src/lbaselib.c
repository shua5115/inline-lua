/*
** $Id: lbaselib.c,v 1.191.1.6 2008/02/14 16:46:22 roberto Exp $
** Basic library
** See Copyright Notice in inlua.h
*/



#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define lbaselib_c
#define INLUA_LIB

#include "inlua.h"

#include "inlauxlib.h"
#include "inlualib.h"




/*
** If your system does not support `stdout', you can just remove this function.
** If you need, you can define your own `print' function, following this
** model but changing `fputs' to put the strings at a proper place
** (a console window or a log file, for instance).
*/
static int luaB_print (inlua_State *L) {
  int n = inlua_gettop(L);  /* number of arguments */
  int i;
  inlua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    inlua_pushvalue(L, -1);  /* function to be called */
    inlua_pushvalue(L, i);   /* value to print */
    inlua_call(L, 1, 1);
    s = inlua_tostring(L, -1);  /* get result */
    if (s == NULL)
      return inluaL_error(L, INLUA_QL("tostring") " must return a string to "
                           INLUA_QL("print"));
    if (i>1) fputs("\t", stdout);
    fputs(s, stdout);
    inlua_pop(L, 1);  /* pop result */
  }
  fputs("\n", stdout);
  return 0;
}


static int luaB_tonumber (inlua_State *L) {
  int base = inluaL_optint(L, 2, 10);
  if (base == 10) {  /* standard conversion */
    inluaL_checkany(L, 1);
    if (inlua_isnumber(L, 1)) {
      inlua_pushnumber(L, inlua_tonumber(L, 1));
      return 1;
    }
  }
  else {
    const char *s1 = inluaL_checkstring(L, 1);
    char *s2;
    unsigned long n;
    inluaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
    n = strtoul(s1, &s2, base);
    if (s1 != s2) {  /* at least one valid digit? */
      while (isspace((unsigned char)(*s2))) s2++;  /* skip trailing spaces */
      if (*s2 == '\0') {  /* no invalid trailing characters? */
        inlua_pushnumber(L, (inlua_Number)n);
        return 1;
      }
    }
  }
  inlua_pushnil(L);  /* else not a number */
  return 1;
}


static int luaB_error (inlua_State *L) {
  int level = inluaL_optint(L, 2, 1);
  inlua_settop(L, 1);
  if (inlua_isstring(L, 1) && level > 0) {  /* add extra information? */
    inluaL_where(L, level);
    inlua_pushvalue(L, 1);
    inlua_concat(L, 2);
  }
  return inlua_error(L);
}


static int luaB_getmetatable (inlua_State *L) {
  inluaL_checkany(L, 1);
  if (!inlua_getmetatable(L, 1)) {
    inlua_pushnil(L);
    return 1;  /* no metatable */
  }
  inluaL_getmetafield(L, 1, "__metatable");
  return 1;  /* returns either __metatable field (if present) or metatable */
}


static int luaB_setmetatable (inlua_State *L) {
  int t = inlua_type(L, 2);
  inluaL_checktype(L, 1, INLUA_TTABLE);
  inluaL_argcheck(L, t == INLUA_TNIL || t == INLUA_TTABLE, 2,
                    "nil or table expected");
  if (inluaL_getmetafield(L, 1, "__metatable"))
    inluaL_error(L, "cannot change a protected metatable");
  inlua_settop(L, 2);
  inlua_setmetatable(L, 1);
  return 1;
}


static void getfunc (inlua_State *L, int opt) {
  if (inlua_isfunction(L, 1)) inlua_pushvalue(L, 1);
  else {
    inlua_Debug ar;
    int level = opt ? inluaL_optint(L, 1, 1) : inluaL_checkint(L, 1);
    inluaL_argcheck(L, level >= 0, 1, "level must be non-negative");
    if (inlua_getstack(L, level, &ar) == 0)
      inluaL_argerror(L, 1, "invalid level");
    inlua_getinfo(L, "f", &ar);
    if (inlua_isnil(L, -1))
      inluaL_error(L, "no function environment for tail call at level %d",
                    level);
  }
}


static int luaB_getfenv (inlua_State *L) {
  getfunc(L, 1);
  if (inlua_iscfunction(L, -1))  /* is a C function? */
    inlua_pushvalue(L, INLUA_GLOBALSINDEX);  /* return the thread's global env. */
  else
    inlua_getfenv(L, -1);
  return 1;
}


static int luaB_setfenv (inlua_State *L) {
  inluaL_checktype(L, 2, INLUA_TTABLE);
  getfunc(L, 0);
  inlua_pushvalue(L, 2);
  if (inlua_isnumber(L, 1) && inlua_tonumber(L, 1) == 0) {
    /* change environment of current thread */
    inlua_pushthread(L);
    inlua_insert(L, -2);
    inlua_setfenv(L, -2);
    return 0;
  }
  else if (inlua_iscfunction(L, -2) || inlua_setfenv(L, -2) == 0)
    inluaL_error(L,
          INLUA_QL("setfenv") " cannot change environment of given object");
  return 1;
}


static int luaB_rawequal (inlua_State *L) {
  inluaL_checkany(L, 1);
  inluaL_checkany(L, 2);
  inlua_pushboolean(L, inlua_rawequal(L, 1, 2));
  return 1;
}


static int luaB_rawget (inlua_State *L) {
  inluaL_checktype(L, 1, INLUA_TTABLE);
  inluaL_checkany(L, 2);
  inlua_settop(L, 2);
  inlua_rawget(L, 1);
  return 1;
}

static int luaB_rawset (inlua_State *L) {
  inluaL_checktype(L, 1, INLUA_TTABLE);
  inluaL_checkany(L, 2);
  inluaL_checkany(L, 3);
  inlua_settop(L, 3);
  inlua_rawset(L, 1);
  return 1;
}


static int luaB_gcinfo (inlua_State *L) {
  inlua_pushinteger(L, inlua_getgccount(L));
  return 1;
}


static int luaB_collectgarbage (inlua_State *L) {
  static const char *const opts[] = {"stop", "restart", "collect",
    "count", "step", "setpause", "setstepmul", NULL};
  static const int optsnum[] = {INLUA_GCSTOP, INLUA_GCRESTART, INLUA_GCCOLLECT,
    INLUA_GCCOUNT, INLUA_GCSTEP, INLUA_GCSETPAUSE, INLUA_GCSETSTEPMUL};
  int o = inluaL_checkoption(L, 1, "collect", opts);
  int ex = inluaL_optint(L, 2, 0);
  int res = inlua_gc(L, optsnum[o], ex);
  switch (optsnum[o]) {
    case INLUA_GCCOUNT: {
      int b = inlua_gc(L, INLUA_GCCOUNTB, 0);
      inlua_pushnumber(L, res + ((inlua_Number)b/1024));
      return 1;
    }
    case INLUA_GCSTEP: {
      inlua_pushboolean(L, res);
      return 1;
    }
    default: {
      inlua_pushnumber(L, res);
      return 1;
    }
  }
}


static int luaB_type (inlua_State *L) {
  inluaL_checkany(L, 1);
  inlua_pushstring(L, inluaL_typename(L, 1));
  return 1;
}


static int luaB_next (inlua_State *L) {
  inluaL_checktype(L, 1, INLUA_TTABLE);
  inlua_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (inlua_next(L, 1))
    return 2;
  else {
    inlua_pushnil(L);
    return 1;
  }
}


static int luaB_pairs (inlua_State *L) {
  inluaL_checktype(L, 1, INLUA_TTABLE);
  inlua_pushvalue(L, inlua_upvalueindex(1));  /* return generator, */
  inlua_pushvalue(L, 1);  /* state, */
  inlua_pushnil(L);  /* and initial value */
  return 3;
}


static int ipairsaux (inlua_State *L) {
  int i = inluaL_checkint(L, 2);
  inluaL_checktype(L, 1, INLUA_TTABLE);
  i++;  /* next value */
  inlua_pushinteger(L, i);
  inlua_rawgeti(L, 1, i);
  return (inlua_isnil(L, -1)) ? 0 : 2;
}


static int luaB_ipairs (inlua_State *L) {
  inluaL_checktype(L, 1, INLUA_TTABLE);
  inlua_pushvalue(L, inlua_upvalueindex(1));  /* return generator, */
  inlua_pushvalue(L, 1);  /* state, */
  inlua_pushinteger(L, 0);  /* and initial value */
  return 3;
}


static int load_aux (inlua_State *L, int status) {
  if (status == 0)  /* OK? */
    return 1;
  else {
    inlua_pushnil(L);
    inlua_insert(L, -2);  /* put before error message */
    return 2;  /* return nil plus error message */
  }
}


static int luaB_loadstring (inlua_State *L) {
  size_t l;
  const char *s = inluaL_checklstring(L, 1, &l);
  const char *chunkname = inluaL_optstring(L, 2, s);
  return load_aux(L, inluaL_loadbuffer(L, s, l, chunkname));
}


static int luaB_loadfile (inlua_State *L) {
  const char *fname = inluaL_optstring(L, 1, NULL);
  return load_aux(L, inluaL_loadfile(L, fname));
}


/*
** Reader for generic `load' function: `inlua_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader (inlua_State *L, void *ud, size_t *size) {
  (void)ud;  /* to avoid warnings */
  inluaL_checkstack(L, 2, "too many nested functions");
  inlua_pushvalue(L, 1);  /* get function */
  inlua_call(L, 0, 1);  /* call it */
  if (inlua_isnil(L, -1)) {
    *size = 0;
    return NULL;
  }
  else if (inlua_isstring(L, -1)) {
    inlua_replace(L, 3);  /* save string in a reserved stack slot */
    return inlua_tolstring(L, 3, size);
  }
  else inluaL_error(L, "reader function must return a string");
  return NULL;  /* to avoid warnings */
}


static int luaB_load (inlua_State *L) {
  int status;
  const char *cname = inluaL_optstring(L, 2, "=(load)");
  inluaL_checktype(L, 1, INLUA_TFUNCTION);
  inlua_settop(L, 3);  /* function, eventual name, plus one reserved slot */
  status = inlua_load(L, generic_reader, NULL, cname);
  return load_aux(L, status);
}


static int luaB_dofile (inlua_State *L) {
  const char *fname = inluaL_optstring(L, 1, NULL);
  int n = inlua_gettop(L);
  if (inluaL_loadfile(L, fname) != 0) inlua_error(L);
  inlua_call(L, 0, INLUA_MULTRET);
  return inlua_gettop(L) - n;
}


static int luaB_assert (inlua_State *L) {
  inluaL_checkany(L, 1);
  if (!inlua_toboolean(L, 1))
    return inluaL_error(L, "%s", inluaL_optstring(L, 2, "assertion failed!"));
  return inlua_gettop(L);
}


static int luaB_unpack (inlua_State *L) {
  int i, e, n;
  inluaL_checktype(L, 1, INLUA_TTABLE);
  i = inluaL_optint(L, 2, 1);
  e = inluaL_opt(L, inluaL_checkint, 3, inluaL_getn(L, 1));
  if (i > e) return 0;  /* empty range */
  n = e - i + 1;  /* number of elements */
  if (n <= 0 || !inlua_checkstack(L, n))  /* n <= 0 means arith. overflow */
    return inluaL_error(L, "too many results to unpack");
  inlua_rawgeti(L, 1, i);  /* push arg[i] (avoiding overflow problems) */
  while (i++ < e)  /* push arg[i + 1...e] */
    inlua_rawgeti(L, 1, i);
  return n;
}


static int luaB_select (inlua_State *L) {
  int n = inlua_gettop(L);
  if (inlua_type(L, 1) == INLUA_TSTRING && *inlua_tostring(L, 1) == '#') {
    inlua_pushinteger(L, n-1);
    return 1;
  }
  else {
    int i = inluaL_checkint(L, 1);
    if (i < 0) i = n + i;
    else if (i > n) i = n;
    inluaL_argcheck(L, 1 <= i, 1, "index out of range");
    return n - i;
  }
}


static int luaB_pcall (inlua_State *L) {
  int status;
  inluaL_checkany(L, 1);
  status = inlua_pcall(L, inlua_gettop(L) - 1, INLUA_MULTRET, 0);
  inlua_pushboolean(L, (status == 0));
  inlua_insert(L, 1);
  return inlua_gettop(L);  /* return status + all results */
}


static int luaB_xpcall (inlua_State *L) {
  int status;
  inluaL_checkany(L, 2);
  inlua_settop(L, 2);
  inlua_insert(L, 1);  /* put error function under function to be called */
  status = inlua_pcall(L, 0, INLUA_MULTRET, 1);
  inlua_pushboolean(L, (status == 0));
  inlua_replace(L, 1);
  return inlua_gettop(L);  /* return status + all results */
}


static int luaB_tostring (inlua_State *L) {
  inluaL_checkany(L, 1);
  if (inluaL_callmeta(L, 1, "__tostring"))  /* is there a metafield? */
    return 1;  /* use its value */
  switch (inlua_type(L, 1)) {
    case INLUA_TNUMBER:
      inlua_pushstring(L, inlua_tostring(L, 1));
      break;
    case INLUA_TSTRING:
      inlua_pushvalue(L, 1);
      break;
    case INLUA_TBOOLEAN:
      inlua_pushstring(L, (inlua_toboolean(L, 1) ? "true" : "false"));
      break;
    case INLUA_TNIL:
      inlua_pushliteral(L, "nil");
      break;
    default:
      inlua_pushfstring(L, "%s: %p", inluaL_typename(L, 1), inlua_topointer(L, 1));
      break;
  }
  return 1;
}


static int luaB_newproxy (inlua_State *L) {
  inlua_settop(L, 1);
  inlua_newuserdata(L, 0);  /* create proxy */
  if (inlua_toboolean(L, 1) == 0)
    return 1;  /* no metatable */
  else if (inlua_isboolean(L, 1)) {
    inlua_newtable(L);  /* create a new metatable `m' ... */
    inlua_pushvalue(L, -1);  /* ... and mark `m' as a valid metatable */
    inlua_pushboolean(L, 1);
    inlua_rawset(L, inlua_upvalueindex(1));  /* weaktable[m] = true */
  }
  else {
    int validproxy = 0;  /* to check if weaktable[metatable(u)] == true */
    if (inlua_getmetatable(L, 1)) {
      inlua_rawget(L, inlua_upvalueindex(1));
      validproxy = inlua_toboolean(L, -1);
      inlua_pop(L, 1);  /* remove value */
    }
    inluaL_argcheck(L, validproxy, 1, "boolean or proxy expected");
    inlua_getmetatable(L, 1);  /* metatable is valid; get it */
  }
  inlua_setmetatable(L, 2);
  return 1;
}


static const inluaL_Reg base_funcs[] = {
  {"assert", luaB_assert},
  {"collectgarbage", luaB_collectgarbage},
  {"dofile", luaB_dofile},
  {"error", luaB_error},
  {"gcinfo", luaB_gcinfo},
  {"getfenv", luaB_getfenv},
  {"getmetatable", luaB_getmetatable},
  {"loadfile", luaB_loadfile},
  {"load", luaB_load},
  {"loadstring", luaB_loadstring},
  {"next", luaB_next},
  {"pcall", luaB_pcall},
  {"print", luaB_print},
  {"rawequal", luaB_rawequal},
  {"rawget", luaB_rawget},
  {"rawset", luaB_rawset},
  {"select", luaB_select},
  {"setfenv", luaB_setfenv},
  {"setmetatable", luaB_setmetatable},
  {"tonumber", luaB_tonumber},
  {"tostring", luaB_tostring},
  {"type", luaB_type},
  {"unpack", luaB_unpack},
  {"xpcall", luaB_xpcall},
  {NULL, NULL}
};


/*
** {======================================================
** Coroutine library
** =======================================================
*/

#define CO_RUN	0	/* running */
#define CO_SUS	1	/* suspended */
#define CO_NOR	2	/* 'normal' (it resumed another coroutine) */
#define CO_DEAD	3

static const char *const statnames[] =
    {"running", "suspended", "normal", "dead"};

static int costatus (inlua_State *L, inlua_State *co) {
  if (L == co) return CO_RUN;
  switch (inlua_status(co)) {
    case INLUA_YIELD:
      return CO_SUS;
    case 0: {
      inlua_Debug ar;
      if (inlua_getstack(co, 0, &ar) > 0)  /* does it have frames? */
        return CO_NOR;  /* it is running */
      else if (inlua_gettop(co) == 0)
          return CO_DEAD;
      else
        return CO_SUS;  /* initial state */
    }
    default:  /* some error occured */
      return CO_DEAD;
  }
}


static int luaB_costatus (inlua_State *L) {
  inlua_State *co = inlua_tothread(L, 1);
  inluaL_argcheck(L, co, 1, "coroutine expected");
  inlua_pushstring(L, statnames[costatus(L, co)]);
  return 1;
}


static int auxresume (inlua_State *L, inlua_State *co, int narg) {
  int status = costatus(L, co);
  if (!inlua_checkstack(co, narg))
    inluaL_error(L, "too many arguments to resume");
  if (status != CO_SUS) {
    inlua_pushfstring(L, "cannot resume %s coroutine", statnames[status]);
    return -1;  /* error flag */
  }
  inlua_xmove(L, co, narg);
  inlua_setlevel(L, co);
  status = inlua_resume(co, narg);
  if (status == 0 || status == INLUA_YIELD) {
    int nres = inlua_gettop(co);
    if (!inlua_checkstack(L, nres + 1))
      inluaL_error(L, "too many results to resume");
    inlua_xmove(co, L, nres);  /* move yielded values */
    return nres;
  }
  else {
    inlua_xmove(co, L, 1);  /* move error message */
    return -1;  /* error flag */
  }
}


static int luaB_coresume (inlua_State *L) {
  inlua_State *co = inlua_tothread(L, 1);
  int r;
  inluaL_argcheck(L, co, 1, "coroutine expected");
  r = auxresume(L, co, inlua_gettop(L) - 1);
  if (r < 0) {
    inlua_pushboolean(L, 0);
    inlua_insert(L, -2);
    return 2;  /* return false + error message */
  }
  else {
    inlua_pushboolean(L, 1);
    inlua_insert(L, -(r + 1));
    return r + 1;  /* return true + `resume' returns */
  }
}


static int luaB_auxwrap (inlua_State *L) {
  inlua_State *co = inlua_tothread(L, inlua_upvalueindex(1));
  int r = auxresume(L, co, inlua_gettop(L));
  if (r < 0) {
    if (inlua_isstring(L, -1)) {  /* error object is a string? */
      inluaL_where(L, 1);  /* add extra info */
      inlua_insert(L, -2);
      inlua_concat(L, 2);
    }
    inlua_error(L);  /* propagate error */
  }
  return r;
}


static int luaB_cocreate (inlua_State *L) {
  inlua_State *NL = inlua_newthread(L);
  inluaL_argcheck(L, inlua_isfunction(L, 1) && !inlua_iscfunction(L, 1), 1,
    "Lua function expected");
  inlua_pushvalue(L, 1);  /* move function to top */
  inlua_xmove(L, NL, 1);  /* move function from L to NL */
  return 1;
}


static int luaB_cowrap (inlua_State *L) {
  luaB_cocreate(L);
  inlua_pushcclosure(L, luaB_auxwrap, 1);
  return 1;
}


static int luaB_yield (inlua_State *L) {
  return inlua_yield(L, inlua_gettop(L));
}


static int luaB_corunning (inlua_State *L) {
  if (inlua_pushthread(L))
    inlua_pushnil(L);  /* main thread is not a coroutine */
  return 1;
}


static const inluaL_Reg co_funcs[] = {
  {"create", luaB_cocreate},
  {"resume", luaB_coresume},
  {"running", luaB_corunning},
  {"status", luaB_costatus},
  {"wrap", luaB_cowrap},
  {"yield", luaB_yield},
  {NULL, NULL}
};

/* }====================================================== */


static void auxopen (inlua_State *L, const char *name,
                     inlua_CFunction f, inlua_CFunction u) {
  inlua_pushcfunction(L, u);
  inlua_pushcclosure(L, f, 1);
  inlua_setfield(L, -2, name);
}


static void base_open (inlua_State *L) {
  /* set global _G */
  inlua_pushvalue(L, INLUA_GLOBALSINDEX);
  inlua_setglobal(L, "_G");
  /* open lib into global table */
  inluaL_register(L, "_G", base_funcs);
  inlua_pushliteral(L, INLUA_VERSION);
  inlua_setglobal(L, "_VERSION");  /* set global _VERSION */
  /* `ipairs' and `pairs' need auxiliary functions as upvalues */
  auxopen(L, "ipairs", luaB_ipairs, ipairsaux);
  auxopen(L, "pairs", luaB_pairs, luaB_next);
  /* `newproxy' needs a weaktable as upvalue */
  inlua_createtable(L, 0, 1);  /* new table `w' */
  inlua_pushvalue(L, -1);  /* `w' will be its own metatable */
  inlua_setmetatable(L, -2);
  inlua_pushliteral(L, "kv");
  inlua_setfield(L, -2, "__mode");  /* metatable(w).__mode = "kv" */
  inlua_pushcclosure(L, luaB_newproxy, 1);
  inlua_setglobal(L, "newproxy");  /* set global `newproxy' */
}


INLUALIB_API int inluaopen_base (inlua_State *L) {
  base_open(L);
  inluaL_register(L, INLUA_COLIBNAME, co_funcs);
  return 2;
}

