/*
** $Id: ldblib.c,v 1.104.1.4 2009/08/04 18:50:18 roberto Exp $
** Interface from Lua to its debug API
** See Copyright Notice in inlua.h
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ldblib_c
#define INLUA_LIB

#include "inlua.h"

#include "inlauxlib.h"
#include "inlualib.h"



static int db_getregistry (inlua_State *L) {
  inlua_pushvalue(L, INLUA_REGISTRYINDEX);
  return 1;
}


static int db_getmetatable (inlua_State *L) {
  inluaL_checkany(L, 1);
  if (!inlua_getmetatable(L, 1)) {
    inlua_pushnil(L);  /* no metatable */
  }
  return 1;
}


static int db_setmetatable (inlua_State *L) {
  int t = inlua_type(L, 2);
  inluaL_argcheck(L, t == INLUA_TNIL || t == INLUA_TTABLE, 2,
                    "nil or table expected");
  inlua_settop(L, 2);
  inlua_pushboolean(L, inlua_setmetatable(L, 1));
  return 1;
}


static int db_getfenv (inlua_State *L) {
  inluaL_checkany(L, 1);
  inlua_getfenv(L, 1);
  return 1;
}


static int db_setfenv (inlua_State *L) {
  inluaL_checktype(L, 2, INLUA_TTABLE);
  inlua_settop(L, 2);
  if (inlua_setfenv(L, 1) == 0)
    inluaL_error(L, INLUA_QL("setfenv")
                  " cannot change environment of given object");
  return 1;
}


static void settabss (inlua_State *L, const char *i, const char *v) {
  inlua_pushstring(L, v);
  inlua_setfield(L, -2, i);
}


static void settabsi (inlua_State *L, const char *i, int v) {
  inlua_pushinteger(L, v);
  inlua_setfield(L, -2, i);
}


static inlua_State *getthread (inlua_State *L, int *arg) {
  if (inlua_isthread(L, 1)) {
    *arg = 1;
    return inlua_tothread(L, 1);
  }
  else {
    *arg = 0;
    return L;
  }
}


static void treatstackoption (inlua_State *L, inlua_State *L1, const char *fname) {
  if (L == L1) {
    inlua_pushvalue(L, -2);
    inlua_remove(L, -3);
  }
  else
    inlua_xmove(L1, L, 1);
  inlua_setfield(L, -2, fname);
}


static int db_getinfo (inlua_State *L) {
  inlua_Debug ar;
  int arg;
  inlua_State *L1 = getthread(L, &arg);
  const char *options = inluaL_optstring(L, arg+2, "flnSu");
  if (inlua_isnumber(L, arg+1)) {
    if (!inlua_getstack(L1, (int)inlua_tointeger(L, arg+1), &ar)) {
      inlua_pushnil(L);  /* level out of range */
      return 1;
    }
  }
  else if (inlua_isfunction(L, arg+1)) {
    inlua_pushfstring(L, ">%s", options);
    options = inlua_tostring(L, -1);
    inlua_pushvalue(L, arg+1);
    inlua_xmove(L, L1, 1);
  }
  else
    return inluaL_argerror(L, arg+1, "function or level expected");
  if (!inlua_getinfo(L1, options, &ar))
    return inluaL_argerror(L, arg+2, "invalid option");
  inlua_createtable(L, 0, 2);
  if (strchr(options, 'S')) {
    settabss(L, "source", ar.source);
    settabss(L, "short_src", ar.short_src);
    settabsi(L, "linedefined", ar.linedefined);
    settabsi(L, "lastlinedefined", ar.lastlinedefined);
    settabss(L, "what", ar.what);
  }
  if (strchr(options, 'l'))
    settabsi(L, "currentline", ar.currentline);
  if (strchr(options, 'u'))
    settabsi(L, "nups", ar.nups);
  if (strchr(options, 'n')) {
    settabss(L, "name", ar.name);
    settabss(L, "namewhat", ar.namewhat);
  }
  if (strchr(options, 'L'))
    treatstackoption(L, L1, "activelines");
  if (strchr(options, 'f'))
    treatstackoption(L, L1, "func");
  return 1;  /* return table */
}
    

static int db_getlocal (inlua_State *L) {
  int arg;
  inlua_State *L1 = getthread(L, &arg);
  inlua_Debug ar;
  const char *name;
  if (!inlua_getstack(L1, inluaL_checkint(L, arg+1), &ar))  /* out of range? */
    return inluaL_argerror(L, arg+1, "level out of range");
  name = inlua_getlocal(L1, &ar, inluaL_checkint(L, arg+2));
  if (name) {
    inlua_xmove(L1, L, 1);
    inlua_pushstring(L, name);
    inlua_pushvalue(L, -2);
    return 2;
  }
  else {
    inlua_pushnil(L);
    return 1;
  }
}


static int db_setlocal (inlua_State *L) {
  int arg;
  inlua_State *L1 = getthread(L, &arg);
  inlua_Debug ar;
  if (!inlua_getstack(L1, inluaL_checkint(L, arg+1), &ar))  /* out of range? */
    return inluaL_argerror(L, arg+1, "level out of range");
  inluaL_checkany(L, arg+3);
  inlua_settop(L, arg+3);
  inlua_xmove(L, L1, 1);
  inlua_pushstring(L, inlua_setlocal(L1, &ar, inluaL_checkint(L, arg+2)));
  return 1;
}


static int auxupvalue (inlua_State *L, int get) {
  const char *name;
  int n = inluaL_checkint(L, 2);
  inluaL_checktype(L, 1, INLUA_TFUNCTION);
  if (inlua_iscfunction(L, 1)) return 0;  /* cannot touch C upvalues from Lua */
  name = get ? inlua_getupvalue(L, 1, n) : inlua_setupvalue(L, 1, n);
  if (name == NULL) return 0;
  inlua_pushstring(L, name);
  inlua_insert(L, -(get+1));
  return get + 1;
}


static int db_getupvalue (inlua_State *L) {
  return auxupvalue(L, 1);
}


static int db_setupvalue (inlua_State *L) {
  inluaL_checkany(L, 3);
  return auxupvalue(L, 0);
}



static const char KEY_HOOK = 'h';


static void hookf (inlua_State *L, inlua_Debug *ar) {
  static const char *const hooknames[] =
    {"call", "return", "line", "count", "tail return"};
  inlua_pushlightuserdata(L, (void *)&KEY_HOOK);
  inlua_rawget(L, INLUA_REGISTRYINDEX);
  inlua_pushlightuserdata(L, L);
  inlua_rawget(L, -2);
  if (inlua_isfunction(L, -1)) {
    inlua_pushstring(L, hooknames[(int)ar->event]);
    if (ar->currentline >= 0)
      inlua_pushinteger(L, ar->currentline);
    else inlua_pushnil(L);
    inlua_assert(inlua_getinfo(L, "lS", ar));
    inlua_call(L, 2, 0);
  }
}


static int makemask (const char *smask, int count) {
  int mask = 0;
  if (strchr(smask, 'c')) mask |= INLUA_MASKCALL;
  if (strchr(smask, 'r')) mask |= INLUA_MASKRET;
  if (strchr(smask, 'l')) mask |= INLUA_MASKLINE;
  if (count > 0) mask |= INLUA_MASKCOUNT;
  return mask;
}


static char *unmakemask (int mask, char *smask) {
  int i = 0;
  if (mask & INLUA_MASKCALL) smask[i++] = 'c';
  if (mask & INLUA_MASKRET) smask[i++] = 'r';
  if (mask & INLUA_MASKLINE) smask[i++] = 'l';
  smask[i] = '\0';
  return smask;
}


static void gethooktable (inlua_State *L) {
  inlua_pushlightuserdata(L, (void *)&KEY_HOOK);
  inlua_rawget(L, INLUA_REGISTRYINDEX);
  if (!inlua_istable(L, -1)) {
    inlua_pop(L, 1);
    inlua_createtable(L, 0, 1);
    inlua_pushlightuserdata(L, (void *)&KEY_HOOK);
    inlua_pushvalue(L, -2);
    inlua_rawset(L, INLUA_REGISTRYINDEX);
  }
}


static int db_sethook (inlua_State *L) {
  int arg, mask, count;
  inlua_Hook func;
  inlua_State *L1 = getthread(L, &arg);
  if (inlua_isnoneornil(L, arg+1)) {
    inlua_settop(L, arg+1);
    func = NULL; mask = 0; count = 0;  /* turn off hooks */
  }
  else {
    const char *smask = inluaL_checkstring(L, arg+2);
    inluaL_checktype(L, arg+1, INLUA_TFUNCTION);
    count = inluaL_optint(L, arg+3, 0);
    func = hookf; mask = makemask(smask, count);
  }
  gethooktable(L);
  inlua_pushlightuserdata(L, L1);
  inlua_pushvalue(L, arg+1);
  inlua_rawset(L, -3);  /* set new hook */
  inlua_pop(L, 1);  /* remove hook table */
  inlua_sethook(L1, func, mask, count);  /* set hooks */
  return 0;
}


static int db_gethook (inlua_State *L) {
  int arg;
  inlua_State *L1 = getthread(L, &arg);
  char buff[5];
  int mask = inlua_gethookmask(L1);
  inlua_Hook hook = inlua_gethook(L1);
  if (hook != NULL && hook != hookf)  /* external hook? */
    inlua_pushliteral(L, "external hook");
  else {
    gethooktable(L);
    inlua_pushlightuserdata(L, L1);
    inlua_rawget(L, -2);   /* get hook */
    inlua_remove(L, -2);  /* remove hook table */
  }
  inlua_pushstring(L, unmakemask(mask, buff));
  inlua_pushinteger(L, inlua_gethookcount(L1));
  return 3;
}


static int db_debug (inlua_State *L) {
  for (;;) {
    char buffer[250];
    fputs("lua_debug> ", stderr);
    if (fgets(buffer, sizeof(buffer), stdin) == 0 ||
        strcmp(buffer, "cont\n") == 0)
      return 0;
    if (inluaL_loadbuffer(L, buffer, strlen(buffer), "=(debug command)") ||
        inlua_pcall(L, 0, 0, 0)) {
      fputs(inlua_tostring(L, -1), stderr);
      fputs("\n", stderr);
    }
    inlua_settop(L, 0);  /* remove eventual returns */
  }
}


#define LEVELS1	12	/* size of the first part of the stack */
#define LEVELS2	10	/* size of the second part of the stack */

static int db_errorfb (inlua_State *L) {
  int level;
  int firstpart = 1;  /* still before eventual `...' */
  int arg;
  inlua_State *L1 = getthread(L, &arg);
  inlua_Debug ar;
  if (inlua_isnumber(L, arg+2)) {
    level = (int)inlua_tointeger(L, arg+2);
    inlua_pop(L, 1);
  }
  else
    level = (L == L1) ? 1 : 0;  /* level 0 may be this own function */
  if (inlua_gettop(L) == arg)
    inlua_pushliteral(L, "");
  else if (!inlua_isstring(L, arg+1)) return 1;  /* message is not a string */
  else inlua_pushliteral(L, "\n");
  inlua_pushliteral(L, "stack traceback:");
  while (inlua_getstack(L1, level++, &ar)) {
    if (level > LEVELS1 && firstpart) {
      /* no more than `LEVELS2' more levels? */
      if (!inlua_getstack(L1, level+LEVELS2, &ar))
        level--;  /* keep going */
      else {
        inlua_pushliteral(L, "\n\t...");  /* too many levels */
        while (inlua_getstack(L1, level+LEVELS2, &ar))  /* find last levels */
          level++;
      }
      firstpart = 0;
      continue;
    }
    inlua_pushliteral(L, "\n\t");
    inlua_getinfo(L1, "Snl", &ar);
    inlua_pushfstring(L, "%s:", ar.short_src);
    if (ar.currentline > 0)
      inlua_pushfstring(L, "%d:", ar.currentline);
    if (*ar.namewhat != '\0')  /* is there a name? */
        inlua_pushfstring(L, " in function " INLUA_QS, ar.name);
    else {
      if (*ar.what == 'm')  /* main? */
        inlua_pushfstring(L, " in main chunk");
      else if (*ar.what == 'C' || *ar.what == 't')
        inlua_pushliteral(L, " ?");  /* C function or tail call */
      else
        inlua_pushfstring(L, " in function <%s:%d>",
                           ar.short_src, ar.linedefined);
    }
    inlua_concat(L, inlua_gettop(L) - arg);
  }
  inlua_concat(L, inlua_gettop(L) - arg);
  return 1;
}


static const inluaL_Reg dblib[] = {
  {"debug", db_debug},
  {"getfenv", db_getfenv},
  {"gethook", db_gethook},
  {"getinfo", db_getinfo},
  {"getlocal", db_getlocal},
  {"getregistry", db_getregistry},
  {"getmetatable", db_getmetatable},
  {"getupvalue", db_getupvalue},
  {"setfenv", db_setfenv},
  {"sethook", db_sethook},
  {"setlocal", db_setlocal},
  {"setmetatable", db_setmetatable},
  {"setupvalue", db_setupvalue},
  {"traceback", db_errorfb},
  {NULL, NULL}
};


INLUALIB_API int inluaopen_debug (inlua_State *L) {
  inluaL_register(L, INLUA_DBLIBNAME, dblib);
  return 1;
}

