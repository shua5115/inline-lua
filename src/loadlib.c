/*
** $Id: loadlib.c,v 1.52.1.4 2009/09/09 13:17:16 roberto Exp $
** Dynamic library loader for Lua
** See Copyright Notice in inlua.h
**
** This module contains an implementation of loadlib for Unix systems
** that have dlfcn, an implementation for Darwin (Mac OS X), an
** implementation for Windows, and a stub for other systems.
*/


#include <stdlib.h>
#include <string.h>


#define loadlib_c
#define INLUA_LIB

#include "inlua.h"

#include "inlauxlib.h"
#include "inlualib.h"


/* prefix for open functions in C libraries */
#define LUA_POF		"luaopen_"

/* separator for open functions in C libraries */
#define LUA_OFSEP	"_"


#define LIBPREFIX	"LOADLIB: "

#define POF		LUA_POF
#define LIB_FAIL	"open"


/* error codes for ll_loadfunc */
#define ERRLIB		1
#define ERRFUNC		2

#define setprogdir(L)		((void)0)


static void ll_unloadlib (void *lib);
static void *ll_load (inlua_State *L, const char *path);
static inlua_CFunction ll_sym (inlua_State *L, void *lib, const char *sym);



#if defined(INLUA_DL_DLOPEN)
/*
** {========================================================================
** This is an implementation of loadlib based on the dlfcn interface.
** The dlfcn interface is available in Linux, SunOS, Solaris, IRIX, FreeBSD,
** NetBSD, AIX 4.2, HPUX 11, and  probably most other Unix flavors, at least
** as an emulation layer on top of native functions.
** =========================================================================
*/

#include <dlfcn.h>

static void ll_unloadlib (void *lib) {
  dlclose(lib);
}


static void *ll_load (inlua_State *L, const char *path) {
  void *lib = dlopen(path, RTLD_NOW);
  if (lib == NULL) inlua_pushstring(L, dlerror());
  return lib;
}


static inlua_CFunction ll_sym (inlua_State *L, void *lib, const char *sym) {
  inlua_CFunction f = (inlua_CFunction)dlsym(lib, sym);
  if (f == NULL) inlua_pushstring(L, dlerror());
  return f;
}

/* }====================================================== */



#elif defined(INLUA_DL_DLL)
/*
** {======================================================================
** This is an implementation of loadlib for Windows using native functions.
** =======================================================================
*/

#include <windows.h>


#undef setprogdir

static void setprogdir (inlua_State *L) {
  char buff[MAX_PATH + 1];
  char *lb;
  DWORD nsize = sizeof(buff)/sizeof(char);
  DWORD n = GetModuleFileNameA(NULL, buff, nsize);
  if (n == 0 || n == nsize || (lb = strrchr(buff, '\\')) == NULL)
    inluaL_error(L, "unable to get ModuleFileName");
  else {
    *lb = '\0';
    inluaL_gsub(L, inlua_tostring(L, -1), INLUA_EXECDIR, buff);
    inlua_remove(L, -2);  /* remove original string */
  }
}


static void pusherror (inlua_State *L) {
  int error = GetLastError();
  char buffer[128];
  if (FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, error, 0, buffer, sizeof(buffer), NULL))
    inlua_pushstring(L, buffer);
  else
    inlua_pushfstring(L, "system error %d\n", error);
}

static void ll_unloadlib (void *lib) {
  FreeLibrary((HINSTANCE)lib);
}


static void *ll_load (inlua_State *L, const char *path) {
  HINSTANCE lib = LoadLibraryA(path);
  if (lib == NULL) pusherror(L);
  return lib;
}


static inlua_CFunction ll_sym (inlua_State *L, void *lib, const char *sym) {
  inlua_CFunction f = (inlua_CFunction)GetProcAddress((HINSTANCE)lib, sym);
  if (f == NULL) pusherror(L);
  return f;
}

/* }====================================================== */



#elif defined(INLUA_DL_DYLD)
/*
** {======================================================================
** Native Mac OS X / Darwin Implementation
** =======================================================================
*/

#include <mach-o/dyld.h>


/* Mac appends a `_' before C function names */
#undef POF
#define POF	"_" LUA_POF


static void pusherror (inlua_State *L) {
  const char *err_str;
  const char *err_file;
  NSLinkEditErrors err;
  int err_num;
  NSLinkEditError(&err, &err_num, &err_file, &err_str);
  inlua_pushstring(L, err_str);
}


static const char *errorfromcode (NSObjectFileImageReturnCode ret) {
  switch (ret) {
    case NSObjectFileImageInappropriateFile:
      return "file is not a bundle";
    case NSObjectFileImageArch:
      return "library is for wrong CPU type";
    case NSObjectFileImageFormat:
      return "bad format";
    case NSObjectFileImageAccess:
      return "cannot access file";
    case NSObjectFileImageFailure:
    default:
      return "unable to load library";
  }
}


static void ll_unloadlib (void *lib) {
  NSUnLinkModule((NSModule)lib, NSUNLINKMODULE_OPTION_RESET_LAZY_REFERENCES);
}


static void *ll_load (inlua_State *L, const char *path) {
  NSObjectFileImage img;
  NSObjectFileImageReturnCode ret;
  /* this would be a rare case, but prevents crashing if it happens */
  if(!_dyld_present()) {
    inlua_pushliteral(L, "dyld not present");
    return NULL;
  }
  ret = NSCreateObjectFileImageFromFile(path, &img);
  if (ret == NSObjectFileImageSuccess) {
    NSModule mod = NSLinkModule(img, path, NSLINKMODULE_OPTION_PRIVATE |
                       NSLINKMODULE_OPTION_RETURN_ON_ERROR);
    NSDestroyObjectFileImage(img);
    if (mod == NULL) pusherror(L);
    return mod;
  }
  inlua_pushstring(L, errorfromcode(ret));
  return NULL;
}


static inlua_CFunction ll_sym (inlua_State *L, void *lib, const char *sym) {
  NSSymbol nss = NSLookupSymbolInModule((NSModule)lib, sym);
  if (nss == NULL) {
    inlua_pushfstring(L, "symbol " INLUA_QS " not found", sym);
    return NULL;
  }
  return (inlua_CFunction)NSAddressOfSymbol(nss);
}

/* }====================================================== */



#else
/*
** {======================================================
** Fallback for other systems
** =======================================================
*/

#undef LIB_FAIL
#define LIB_FAIL	"absent"


#define DLMSG	"dynamic libraries not enabled; check your Lua installation"


static void ll_unloadlib (void *lib) {
  (void)lib;  /* to avoid warnings */
}


static void *ll_load (inlua_State *L, const char *path) {
  (void)path;  /* to avoid warnings */
  inlua_pushliteral(L, DLMSG);
  return NULL;
}


static inlua_CFunction ll_sym (inlua_State *L, void *lib, const char *sym) {
  (void)lib; (void)sym;  /* to avoid warnings */
  inlua_pushliteral(L, DLMSG);
  return NULL;
}

/* }====================================================== */
#endif



static void **ll_register (inlua_State *L, const char *path) {
  void **plib;
  inlua_pushfstring(L, "%s%s", LIBPREFIX, path);
  inlua_gettable(L, INLUA_REGISTRYINDEX);  /* check library in registry? */
  if (!inlua_isnil(L, -1))  /* is there an entry? */
    plib = (void **)inlua_touserdata(L, -1);
  else {  /* no entry yet; create one */
    inlua_pop(L, 1);
    plib = (void **)inlua_newuserdata(L, sizeof(const void *));
    *plib = NULL;
    inluaL_getmetatable(L, "_LOADLIB");
    inlua_setmetatable(L, -2);
    inlua_pushfstring(L, "%s%s", LIBPREFIX, path);
    inlua_pushvalue(L, -2);
    inlua_settable(L, INLUA_REGISTRYINDEX);
  }
  return plib;
}


/*
** __gc tag method: calls library's `ll_unloadlib' function with the lib
** handle
*/
static int gctm (inlua_State *L) {
  void **lib = (void **)inluaL_checkudata(L, 1, "_LOADLIB");
  if (*lib) ll_unloadlib(*lib);
  *lib = NULL;  /* mark library as closed */
  return 0;
}


static int ll_loadfunc (inlua_State *L, const char *path, const char *sym) {
  void **reg = ll_register(L, path);
  if (*reg == NULL) *reg = ll_load(L, path);
  if (*reg == NULL)
    return ERRLIB;  /* unable to load library */
  else {
    inlua_CFunction f = ll_sym(L, *reg, sym);
    if (f == NULL)
      return ERRFUNC;  /* unable to find function */
    inlua_pushcfunction(L, f);
    return 0;  /* return function */
  }
}


static int ll_loadlib (inlua_State *L) {
  const char *path = inluaL_checkstring(L, 1);
  const char *init = inluaL_checkstring(L, 2);
  int stat = ll_loadfunc(L, path, init);
  if (stat == 0)  /* no errors? */
    return 1;  /* return the loaded function */
  else {  /* error; error message is on stack top */
    inlua_pushnil(L);
    inlua_insert(L, -2);
    inlua_pushstring(L, (stat == ERRLIB) ?  LIB_FAIL : "init");
    return 3;  /* return nil, error message, and where */
  }
}



/*
** {======================================================
** 'require' function
** =======================================================
*/


static int readable (const char *filename) {
  FILE *f = fopen(filename, "r");  /* try to open file */
  if (f == NULL) return 0;  /* open failed */
  fclose(f);
  return 1;
}


static const char *pushnexttemplate (inlua_State *L, const char *path) {
  const char *l;
  while (*path == *INLUA_PATHSEP) path++;  /* skip separators */
  if (*path == '\0') return NULL;  /* no more templates */
  l = strchr(path, *INLUA_PATHSEP);  /* find next separator */
  if (l == NULL) l = path + strlen(path);
  inlua_pushlstring(L, path, l - path);  /* template */
  return l;
}


static const char *findfile (inlua_State *L, const char *name,
                                           const char *pname) {
  const char *path;
  name = inluaL_gsub(L, name, ".", INLUA_DIRSEP);
  inlua_getfield(L, INLUA_ENVIRONINDEX, pname);
  path = inlua_tostring(L, -1);
  if (path == NULL)
    inluaL_error(L, INLUA_QL("package.%s") " must be a string", pname);
  inlua_pushliteral(L, "");  /* error accumulator */
  while ((path = pushnexttemplate(L, path)) != NULL) {
    const char *filename;
    filename = inluaL_gsub(L, inlua_tostring(L, -1), INLUA_PATH_MARK, name);
    inlua_remove(L, -2);  /* remove path template */
    if (readable(filename))  /* does file exist and is readable? */
      return filename;  /* return that file name */
    inlua_pushfstring(L, "\n\tno file " INLUA_QS, filename);
    inlua_remove(L, -2);  /* remove file name */
    inlua_concat(L, 2);  /* add entry to possible error message */
  }
  return NULL;  /* not found */
}


static void loaderror (inlua_State *L, const char *filename) {
  inluaL_error(L, "error loading module " INLUA_QS " from file " INLUA_QS ":\n\t%s",
                inlua_tostring(L, 1), filename, inlua_tostring(L, -1));
}


static int loader_Lua (inlua_State *L) {
  const char *filename;
  const char *name = inluaL_checkstring(L, 1);
  filename = findfile(L, name, "path");
  if (filename == NULL) return 1;  /* library not found in this path */
  if (inluaL_loadfile(L, filename) != 0)
    loaderror(L, filename);
  return 1;  /* library loaded successfully */
}


static const char *mkfuncname (inlua_State *L, const char *modname) {
  const char *funcname;
  const char *mark = strchr(modname, *INLUA_IGMARK);
  if (mark) modname = mark + 1;
  funcname = inluaL_gsub(L, modname, ".", LUA_OFSEP);
  funcname = inlua_pushfstring(L, POF"%s", funcname);
  inlua_remove(L, -2);  /* remove 'gsub' result */
  return funcname;
}


static int loader_C (inlua_State *L) {
  const char *funcname;
  const char *name = inluaL_checkstring(L, 1);
  const char *filename = findfile(L, name, "cpath");
  if (filename == NULL) return 1;  /* library not found in this path */
  funcname = mkfuncname(L, name);
  if (ll_loadfunc(L, filename, funcname) != 0)
    loaderror(L, filename);
  return 1;  /* library loaded successfully */
}


static int loader_Croot (inlua_State *L) {
  const char *funcname;
  const char *filename;
  const char *name = inluaL_checkstring(L, 1);
  const char *p = strchr(name, '.');
  int stat;
  if (p == NULL) return 0;  /* is root */
  inlua_pushlstring(L, name, p - name);
  filename = findfile(L, inlua_tostring(L, -1), "cpath");
  if (filename == NULL) return 1;  /* root not found */
  funcname = mkfuncname(L, name);
  if ((stat = ll_loadfunc(L, filename, funcname)) != 0) {
    if (stat != ERRFUNC) loaderror(L, filename);  /* real error */
    inlua_pushfstring(L, "\n\tno module " INLUA_QS " in file " INLUA_QS,
                       name, filename);
    return 1;  /* function not found */
  }
  return 1;
}


static int loader_preload (inlua_State *L) {
  const char *name = inluaL_checkstring(L, 1);
  inlua_getfield(L, INLUA_ENVIRONINDEX, "preload");
  if (!inlua_istable(L, -1))
    inluaL_error(L, INLUA_QL("package.preload") " must be a table");
  inlua_getfield(L, -1, name);
  if (inlua_isnil(L, -1))  /* not found? */
    inlua_pushfstring(L, "\n\tno field package.preload['%s']", name);
  return 1;
}


static const int sentinel_ = 0;
#define sentinel	((void *)&sentinel_)


static int ll_require (inlua_State *L) {
  const char *name = inluaL_checkstring(L, 1);
  int i;
  inlua_settop(L, 1);  /* _LOADED table will be at index 2 */
  inlua_getfield(L, INLUA_REGISTRYINDEX, "_LOADED");
  inlua_getfield(L, 2, name);
  if (inlua_toboolean(L, -1)) {  /* is it there? */
    if (inlua_touserdata(L, -1) == sentinel)  /* check loops */
      inluaL_error(L, "loop or previous error loading module " INLUA_QS, name);
    return 1;  /* package is already loaded */
  }
  /* else must load it; iterate over available loaders */
  inlua_getfield(L, INLUA_ENVIRONINDEX, "loaders");
  if (!inlua_istable(L, -1))
    inluaL_error(L, INLUA_QL("package.loaders") " must be a table");
  inlua_pushliteral(L, "");  /* error message accumulator */
  for (i=1; ; i++) {
    inlua_rawgeti(L, -2, i);  /* get a loader */
    if (inlua_isnil(L, -1))
      inluaL_error(L, "module " INLUA_QS " not found:%s",
                    name, inlua_tostring(L, -2));
    inlua_pushstring(L, name);
    inlua_call(L, 1, 1);  /* call it */
    if (inlua_isfunction(L, -1))  /* did it find module? */
      break;  /* module loaded successfully */
    else if (inlua_isstring(L, -1))  /* loader returned error message? */
      inlua_concat(L, 2);  /* accumulate it */
    else
      inlua_pop(L, 1);
  }
  inlua_pushlightuserdata(L, sentinel);
  inlua_setfield(L, 2, name);  /* _LOADED[name] = sentinel */
  inlua_pushstring(L, name);  /* pass name as argument to module */
  inlua_call(L, 1, 1);  /* run loaded module */
  if (!inlua_isnil(L, -1))  /* non-nil return? */
    inlua_setfield(L, 2, name);  /* _LOADED[name] = returned value */
  inlua_getfield(L, 2, name);
  if (inlua_touserdata(L, -1) == sentinel) {   /* module did not set a value? */
    inlua_pushboolean(L, 1);  /* use true as result */
    inlua_pushvalue(L, -1);  /* extra copy to be returned */
    inlua_setfield(L, 2, name);  /* _LOADED[name] = true */
  }
  return 1;
}

/* }====================================================== */



/*
** {======================================================
** 'module' function
** =======================================================
*/
  

static void setfenv (inlua_State *L) {
  inlua_Debug ar;
  if (inlua_getstack(L, 1, &ar) == 0 ||
      inlua_getinfo(L, "f", &ar) == 0 ||  /* get calling function */
      inlua_iscfunction(L, -1))
    inluaL_error(L, INLUA_QL("module") " not called from a Lua function");
  inlua_pushvalue(L, -2);
  inlua_setfenv(L, -2);
  inlua_pop(L, 1);
}


static void dooptions (inlua_State *L, int n) {
  int i;
  for (i = 2; i <= n; i++) {
    inlua_pushvalue(L, i);  /* get option (a function) */
    inlua_pushvalue(L, -2);  /* module */
    inlua_call(L, 1, 0);
  }
}


static void modinit (inlua_State *L, const char *modname) {
  const char *dot;
  inlua_pushvalue(L, -1);
  inlua_setfield(L, -2, "_M");  /* module._M = module */
  inlua_pushstring(L, modname);
  inlua_setfield(L, -2, "_NAME");
  dot = strrchr(modname, '.');  /* look for last dot in module name */
  if (dot == NULL) dot = modname;
  else dot++;
  /* set _PACKAGE as package name (full module name minus last part) */
  inlua_pushlstring(L, modname, dot - modname);
  inlua_setfield(L, -2, "_PACKAGE");
}


static int ll_module (inlua_State *L) {
  const char *modname = inluaL_checkstring(L, 1);
  int loaded = inlua_gettop(L) + 1;  /* index of _LOADED table */
  inlua_getfield(L, INLUA_REGISTRYINDEX, "_LOADED");
  inlua_getfield(L, loaded, modname);  /* get _LOADED[modname] */
  if (!inlua_istable(L, -1)) {  /* not found? */
    inlua_pop(L, 1);  /* remove previous result */
    /* try global variable (and create one if it does not exist) */
    if (inluaL_findtable(L, INLUA_GLOBALSINDEX, modname, 1) != NULL)
      return inluaL_error(L, "name conflict for module " INLUA_QS, modname);
    inlua_pushvalue(L, -1);
    inlua_setfield(L, loaded, modname);  /* _LOADED[modname] = new table */
  }
  /* check whether table already has a _NAME field */
  inlua_getfield(L, -1, "_NAME");
  if (!inlua_isnil(L, -1))  /* is table an initialized module? */
    inlua_pop(L, 1);
  else {  /* no; initialize it */
    inlua_pop(L, 1);
    modinit(L, modname);
  }
  inlua_pushvalue(L, -1);
  setfenv(L);
  dooptions(L, loaded - 1);
  return 0;
}


static int ll_seeall (inlua_State *L) {
  inluaL_checktype(L, 1, INLUA_TTABLE);
  if (!inlua_getmetatable(L, 1)) {
    inlua_createtable(L, 0, 1); /* create new metatable */
    inlua_pushvalue(L, -1);
    inlua_setmetatable(L, 1);
  }
  inlua_pushvalue(L, INLUA_GLOBALSINDEX);
  inlua_setfield(L, -2, "__index");  /* mt.__index = _G */
  return 0;
}


/* }====================================================== */



/* auxiliary mark (for internal use) */
#define AUXMARK		"\1"

static void setpath (inlua_State *L, const char *fieldname, const char *envname,
                                   const char *def) {
  const char *path = getenv(envname);
  if (path == NULL)  /* no environment variable? */
    inlua_pushstring(L, def);  /* use default */
  else {
    /* replace ";;" by ";AUXMARK;" and then AUXMARK by default path */
    path = inluaL_gsub(L, path, INLUA_PATHSEP INLUA_PATHSEP,
                              INLUA_PATHSEP AUXMARK INLUA_PATHSEP);
    inluaL_gsub(L, path, AUXMARK, def);
    inlua_remove(L, -2);
  }
  setprogdir(L);
  inlua_setfield(L, -2, fieldname);
}


static const inluaL_Reg pk_funcs[] = {
  {"loadlib", ll_loadlib},
  {"seeall", ll_seeall},
  {NULL, NULL}
};


static const inluaL_Reg ll_funcs[] = {
  {"module", ll_module},
  {"require", ll_require},
  {NULL, NULL}
};


static const inlua_CFunction loaders[] =
  {loader_preload, loader_Lua, loader_C, loader_Croot, NULL};


INLUALIB_API int inluaopen_package (inlua_State *L) {
  int i;
  /* create new type _LOADLIB */
  inluaL_newmetatable(L, "_LOADLIB");
  inlua_pushcfunction(L, gctm);
  inlua_setfield(L, -2, "__gc");
  /* create `package' table */
  inluaL_register(L, INLUA_LOADLIBNAME, pk_funcs);
#if defined(INLUA_COMPAT_LOADLIB) 
  inlua_getfield(L, -1, "loadlib");
  inlua_setfield(L, INLUA_GLOBALSINDEX, "loadlib");
#endif
  inlua_pushvalue(L, -1);
  inlua_replace(L, INLUA_ENVIRONINDEX);
  /* create `loaders' table */
  inlua_createtable(L, sizeof(loaders)/sizeof(loaders[0]) - 1, 0);
  /* fill it with pre-defined loaders */
  for (i=0; loaders[i] != NULL; i++) {
    inlua_pushcfunction(L, loaders[i]);
    inlua_rawseti(L, -2, i+1);
  }
  inlua_setfield(L, -2, "loaders");  /* put it in field `loaders' */
  setpath(L, "path", INLUA_PATH, INLUA_PATH_DEFAULT);  /* set field `path' */
  setpath(L, "cpath", INLUA_CPATH, INLUA_CPATH_DEFAULT); /* set field `cpath' */
  /* store config information */
  inlua_pushliteral(L, INLUA_DIRSEP "\n" INLUA_PATHSEP "\n" INLUA_PATH_MARK "\n"
                     INLUA_EXECDIR "\n" INLUA_IGMARK);
  inlua_setfield(L, -2, "config");
  /* set field `loaded' */
  inluaL_findtable(L, INLUA_REGISTRYINDEX, "_LOADED", 2);
  inlua_setfield(L, -2, "loaded");
  /* set field `preload' */
  inlua_newtable(L);
  inlua_setfield(L, -2, "preload");
  inlua_pushvalue(L, INLUA_GLOBALSINDEX);
  inluaL_register(L, NULL, ll_funcs);  /* open lib into global table */
  inlua_pop(L, 1);
  return 1;  /* return 'package' table */
}

