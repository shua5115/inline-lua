/*
** $Id: inlualib.h,v 1.36.1.1 2007/12/27 13:02:25 roberto Exp $
** Lua standard libraries
** See Copyright Notice in inlua.h
*/


#ifndef inlualib_h
#define inlualib_h

#include "inlua.h"


/* Key to file-handle type */
#define INLUA_FILEHANDLE		"FILE*"


#define INLUA_COLIBNAME	"coroutine"
INLUALIB_API int (inluaopen_base) (inlua_State *L);

#define INLUA_TABLIBNAME	"table"
INLUALIB_API int (inluaopen_table) (inlua_State *L);

#define INLUA_IOLIBNAME	"io"
INLUALIB_API int (inluaopen_io) (inlua_State *L);

#define INLUA_OSLIBNAME	"os"
INLUALIB_API int (inluaopen_os) (inlua_State *L);

#define INLUA_STRLIBNAME	"string"
INLUALIB_API int (inluaopen_string) (inlua_State *L);

#define INLUA_MATHLIBNAME	"math"
INLUALIB_API int (inluaopen_math) (inlua_State *L);

#define INLUA_DBLIBNAME	"debug"
INLUALIB_API int (inluaopen_debug) (inlua_State *L);

#define INLUA_LOADLIBNAME	"package"
INLUALIB_API int (inluaopen_package) (inlua_State *L);


/* open all previous libraries */
INLUALIB_API void (inluaL_openlibs) (inlua_State *L); 



#ifndef inlua_assert
#define inlua_assert(x)	((void)0)
#endif


#endif
