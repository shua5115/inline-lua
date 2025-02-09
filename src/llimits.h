/*
** $Id: llimits.h,v 1.69.1.1 2007/12/27 13:02:25 roberto Exp $
** Limits, basic types, and some other `installation-dependent' definitions
** See Copyright Notice in inlua.h
*/

#ifndef llimits_h
#define llimits_h


#include <limits.h>
#include <stddef.h>


#include "inlua.h"


typedef INLUAI_UINT32 lu_int32;

typedef INLUAI_UMEM lu_mem;

typedef INLUAI_MEM l_mem;



/* chars used as small naturals (so that `char' is reserved for characters) */
typedef unsigned char lu_byte;


#define MAX_SIZET	((size_t)(~(size_t)0)-2)

#define MAX_LUMEM	((lu_mem)(~(lu_mem)0)-2)


#define MAX_INT (INT_MAX-2)  /* maximum value of an int (-2 for safety) */

/*
** conversion of pointer to integer
** this is for hashing only; there is no problem if the integer
** cannot hold the whole pointer value
*/
#define IntPoint(p)  ((unsigned int)(lu_mem)(p))



/* type to ensure maximum alignment */
typedef INLUAI_USER_ALIGNMENT_T L_Umaxalign;


/* result of a `usual argument conversion' over inlua_Number */
typedef INLUAI_UACNUMBER l_uacNumber;


/* internal assertions for in-house debugging */
#ifdef inlua_assert

#define check_exp(c,e)		(inlua_assert(c), (e))
#define api_check(l,e)		inlua_assert(e)

#else

#define inlua_assert(c)		((void)0)
#define check_exp(c,e)		(e)
#define api_check		inluai_apicheck

#endif


#ifndef UNUSED
#define UNUSED(x)	((void)(x))	/* to avoid warnings */
#endif


#ifndef cast
#define cast(t, exp)	((t)(exp))
#endif

#define cast_byte(i)	cast(lu_byte, (i))
#define cast_num(i)	cast(inlua_Number, (i))
#define cast_int(i)	cast(int, (i))



/*
** type for virtual-machine instructions
** must be an unsigned with (at least) 4 bytes (see details in lopcodes.h)
*/
typedef lu_int32 Instruction;



/* maximum stack for a Lua function */
#define MAXSTACK	250



/* minimum size for the string table (must be power of 2) */
#ifndef MINSTRTABSIZE
#define MINSTRTABSIZE	32
#endif


/* minimum size for string buffer */
#ifndef LUA_MINBUFFER
#define LUA_MINBUFFER	32
#endif


#ifndef lua_lock
#define lua_lock(L)     ((void) 0) 
#define lua_unlock(L)   ((void) 0)
#endif

#ifndef luai_threadyield
#define luai_threadyield(L)     {lua_unlock(L); lua_lock(L);}
#endif


/*
** macro to control inclusion of some hard tests on stack reallocation
*/ 
#ifndef HARDSTACKTESTS
#define condhardstacktests(x)	((void)0)
#else
#define condhardstacktests(x)	x
#endif

#endif
