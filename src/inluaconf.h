/*
** $Id: inluaconf.h,v 1.82.1.7 2008/02/11 16:25:08 roberto Exp $
** Configuration file for Lua
** See Copyright Notice in inlua.h
*/


#ifndef inlconfig_h
#define inlconfig_h

#include <limits.h>
#include <stddef.h>


/*
** ==================================================================
** Search for "@@" to find all configurable definitions.
** ===================================================================
*/


/*
@@ INLUA_ANSI controls the use of non-ansi features.
** CHANGE it (define it) if you want Lua to avoid the use of any
** non-ansi feature or library.
*/
#if defined(__STRICT_ANSI__)
#define INLUA_ANSI
#endif


#if !defined(INLUA_ANSI) && defined(_WIN32)
#define INLUA_WIN
#endif

#if defined(INLUA_USE_LINUX)
#define INLUA_USE_POSIX
#define INLUA_USE_DLOPEN		/* needs an extra library: -ldl */
#define INLUA_USE_READLINE	/* needs some extra libraries */
#endif

#if defined(INLUA_USE_MACOSX)
#define INLUA_USE_POSIX
#define INLUA_DL_DYLD		/* does not need extra library */
#endif



/*
@@ INLUA_USE_POSIX includes all functionallity listed as X/Open System
@* Interfaces Extension (XSI).
** CHANGE it (define it) if your system is XSI compatible.
*/
#if defined(INLUA_USE_POSIX)
#define INLUA_USE_MKSTEMP
#define INLUA_USE_ISATTY
#define INLUA_USE_POPEN
#define INLUA_USE_ULONGJMP
#endif


/*
@@ INLUA_PATH and INLUA_CPATH are the names of the environment variables that
@* Lua check to set its paths.
@@ INLUA_INIT is the name of the environment variable that Lua
@* checks for initialization code.
** CHANGE them if you want different names.
*/
#define INLUA_PATH        "INLUA_PATH"
#define INLUA_CPATH       "INLUA_CPATH"
#define INLUA_INIT	"INLUA_INIT"


/*
@@ INLUA_PATH_DEFAULT is the default path that Lua uses to look for
@* Lua libraries.
@@ INLUA_CPATH_DEFAULT is the default path that Lua uses to look for
@* C libraries.
** CHANGE them if your machine has a non-conventional directory
** hierarchy or if you want to install your libraries in
** non-conventional directories.
*/
#if defined(_WIN32)
/*
** In Windows, any exclamation mark ('!') in the path is replaced by the
** path of the directory of the executable file of the current process.
*/
#define INLUA_LDIR	"!\\lua\\"
#define INLUA_CDIR	"!\\"
#define INLUA_PATH_DEFAULT  \
		".\\?.lua;"  INLUA_LDIR"?.lua;"  INLUA_LDIR"?\\init.lua;" \
		             INLUA_CDIR"?.lua;"  INLUA_CDIR"?\\init.lua"
#define INLUA_CPATH_DEFAULT \
	".\\?.dll;"  INLUA_CDIR"?.dll;" INLUA_CDIR"loadall.dll"

#else
#define INLUA_ROOT	"/usr/local/"
#define INLUA_LDIR	INLUA_ROOT "share/lua/5.1/"
#define INLUA_CDIR	INLUA_ROOT "lib/lua/5.1/"
#define INLUA_PATH_DEFAULT  \
		"./?.lua;"  INLUA_LDIR"?.lua;"  INLUA_LDIR"?/init.lua;" \
		            INLUA_CDIR"?.lua;"  INLUA_CDIR"?/init.lua"
#define INLUA_CPATH_DEFAULT \
	"./?.so;"  INLUA_CDIR"?.so;" INLUA_CDIR"loadall.so"
#endif


/*
@@ INLUA_DIRSEP is the directory separator (for submodules).
** CHANGE it if your machine does not use "/" as the directory separator
** and is not Windows. (On Windows Lua automatically uses "\".)
*/
#if defined(_WIN32)
#define INLUA_DIRSEP	"\\"
#else
#define INLUA_DIRSEP	"/"
#endif


/*
@@ INLUA_PATHSEP is the character that separates templates in a path.
@@ INLUA_PATH_MARK is the string that marks the substitution points in a
@* template.
@@ INLUA_EXECDIR in a Windows path is replaced by the executable's
@* directory.
@@ INLUA_IGMARK is a mark to ignore all before it when bulding the
@* luaopen_ function name.
** CHANGE them if for some reason your system cannot use those
** characters. (E.g., if one of those characters is a common character
** in file/directory names.) Probably you do not need to change them.
*/
#define INLUA_PATHSEP	";"
#define INLUA_PATH_MARK	"?"
#define INLUA_EXECDIR	"!"
#define INLUA_IGMARK	"-"


/*
@@ INLUA_INTEGER is the integral type used by inlua_pushinteger/inlua_tointeger.
** CHANGE that if ptrdiff_t is not adequate on your machine. (On most
** machines, ptrdiff_t gives a good choice between int or long.)
*/
#define INLUA_INTEGER	ptrdiff_t


/*
@@ INLUA_API is a mark for all core API functions.
@@ INLUALIB_API is a mark for all standard library functions.
** CHANGE them if you need to define those functions in some special way.
** For instance, if you want to create one Windows DLL with the core and
** the libraries, you may want to use the following definition (define
** INLUA_BUILD_AS_DLL to get it).
*/
#if defined(INLUA_BUILD_AS_DLL)

#if defined(INLUA_CORE) || defined(INLUA_LIB)
#define INLUA_API __declspec(dllexport)
#else
#define INLUA_API __declspec(dllimport)
#endif

#else

#define INLUA_API		extern

#endif

/* more often than not the libs go together with the core */
#define INLUALIB_API	INLUA_API


/*
@@ INLUAI_FUNC is a mark for all extern functions that are not to be
@* exported to outside modules.
@@ INLUAI_DATA is a mark for all extern (const) variables that are not to
@* be exported to outside modules.
** CHANGE them if you need to mark them in some special way. Elf/gcc
** (versions 3.2 and later) mark them as "hidden" to optimize access
** when Lua is compiled as a shared library.
*/
#if defined(inluaall_c)
#define INLUAI_FUNC	static
#define INLUAI_DATA	/* empty */

#elif defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && \
      defined(__ELF__)
#define INLUAI_FUNC	__attribute__((visibility("hidden"))) extern
#define INLUAI_DATA	INLUAI_FUNC

#else
#define INLUAI_FUNC	extern
#define INLUAI_DATA	extern
#endif



/*
@@ INLUA_QL describes how error messages quote program elements.
** CHANGE it if you want a different appearance.
*/
#define INLUA_QL(x)	"'" x "'"
#define INLUA_QS		INLUA_QL("%s")


/*
@@ INLUA_IDSIZE gives the maximum size for the description of the source
@* of a function in debug information.
** CHANGE it if you want a different size.
*/
#define INLUA_IDSIZE	60


/*
** {==================================================================
** Stand-alone configuration
** ===================================================================
*/

#if defined(inlua_c) || defined(inluaall_c)

/*
@@ inlua_stdin_is_tty detects whether the standard input is a 'tty' (that
@* is, whether we're running lua interactively).
** CHANGE it if you have a better definition for non-POSIX/non-Windows
** systems.
*/
#if defined(INLUA_USE_ISATTY)
#include <unistd.h>
#define inlua_stdin_is_tty()	isatty(0)
#elif defined(INLUA_WIN)
#include <io.h>
#include <stdio.h>
#define inlua_stdin_is_tty()	_isatty(_fileno(stdin))
#else
#define inlua_stdin_is_tty()	1  /* assume stdin is a tty */
#endif


/*
@@ INLUA_PROMPT is the default prompt used by stand-alone Lua.
@@ INLUA_PROMPT2 is the default continuation prompt used by stand-alone Lua.
** CHANGE them if you want different prompts. (You can also change the
** prompts dynamically, assigning to globals _PROMPT/_PROMPT2.)
*/
#define INLUA_PROMPT		"> "
#define INLUA_PROMPT2		">> "


/*
@@ INLUA_PROGNAME is the default name for the stand-alone Lua program.
** CHANGE it if your stand-alone interpreter has a different name and
** your system is not able to detect that name automatically.
*/
#define INLUA_PROGNAME		"lua"


/*
@@ INLUA_MAXINPUT is the maximum length for an input line in the
@* stand-alone interpreter.
** CHANGE it if you need longer lines.
*/
#define INLUA_MAXINPUT	512


/*
@@ inlua_readline defines how to show a prompt and then read a line from
@* the standard input.
@@ inlua_saveline defines how to "save" a read line in a "history".
@@ inlua_freeline defines how to free a line read by inlua_readline.
** CHANGE them if you want to improve this functionality (e.g., by using
** GNU readline and history facilities).
*/
#if defined(INLUA_USE_READLINE)
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#define inlua_readline(L,b,p)	((void)L, ((b)=readline(p)) != NULL)
#define inlua_saveline(L,idx) \
	if (inlua_strlen(L,idx) > 0)  /* non-empty line? */ \
	  add_history(inlua_tostring(L, idx));  /* add it to history */
#define inlua_freeline(L,b)	((void)L, free(b))
#else
#define inlua_readline(L,b,p)	\
	((void)L, fputs(p, stdout), fflush(stdout),  /* show prompt */ \
	fgets(b, INLUA_MAXINPUT, stdin) != NULL)  /* get line */
#define inlua_saveline(L,idx)	{ (void)L; (void)idx; }
#define inlua_freeline(L,b)	{ (void)L; (void)b; }
#endif

#endif

/* }================================================================== */


/*
@@ INLUAI_GCPAUSE defines the default pause between garbage-collector cycles
@* as a percentage.
** CHANGE it if you want the GC to run faster or slower (higher values
** mean larger pauses which mean slower collection.) You can also change
** this value dynamically.
*/
#define INLUAI_GCPAUSE	200  /* 200% (wait memory to double before next GC) */


/*
@@ INLUAI_GCMUL defines the default speed of garbage collection relative to
@* memory allocation as a percentage.
** CHANGE it if you want to change the granularity of the garbage
** collection. (Higher values mean coarser collections. 0 represents
** infinity, where each step performs a full collection.) You can also
** change this value dynamically.
*/
#define INLUAI_GCMUL	200 /* GC runs 'twice the speed' of memory allocation */



/*
@@ INLUA_COMPAT_GETN controls compatibility with old getn behavior.
** CHANGE it (define it) if you want exact compatibility with the
** behavior of setn/getn in Lua 5.0.
*/
#undef INLUA_COMPAT_GETN

/*
@@ INLUA_COMPAT_LOADLIB controls compatibility about global loadlib.
** CHANGE it to undefined as soon as you do not need a global 'loadlib'
** function (the function is still available as 'package.loadlib').
*/
#undef INLUA_COMPAT_LOADLIB

/*
@@ INLUA_COMPAT_VARARG controls compatibility with old vararg feature.
** CHANGE it to undefined as soon as your programs use only '...' to
** access vararg parameters (instead of the old 'arg' table).
*/
#undef INLUA_COMPAT_VARARG

/*
@@ INLUA_COMPAT_MOD controls compatibility with old math.mod function.
** CHANGE it to undefined as soon as your programs use 'math.fmod' or
** the new '%' operator instead of 'math.mod'.
*/
#undef INLUA_COMPAT_MOD

/*
@@ INLUA_COMPAT_LSTR controls compatibility with old long string nesting
@* facility.
** CHANGE it to 2 if you want the old behaviour, or undefine it to turn
** off the advisory error when nesting [[...]].
*/
#undef INLUA_COMPAT_LSTR

/*
@@ INLUA_COMPAT_GFIND controls compatibility with old 'string.gfind' name.
** CHANGE it to undefined as soon as you rename 'string.gfind' to
** 'string.gmatch'.
*/
#undef INLUA_COMPAT_GFIND

/*
@@ INLUA_COMPAT_OPENLIB controls compatibility with old 'inluaL_openlib'
@* behavior.
** CHANGE it to undefined as soon as you replace to 'inluaL_register'
** your uses of 'inluaL_openlib'
*/
#undef INLUA_COMPAT_OPENLIB



/*
@@ inluai_apicheck is the assert macro used by the Lua-C API.
** CHANGE inluai_apicheck if you want Lua to perform some checks in the
** parameters it gets from API calls. This may slow down the interpreter
** a bit, but may be quite useful when debugging C code that interfaces
** with Lua. A useful redefinition is to use assert.h.
*/
#if defined(INLUA_USE_APICHECK)
#include <assert.h>
#define inluai_apicheck(L,o)	{ (void)L; assert(o); }
#else
#define inluai_apicheck(L,o)	{ (void)L; }
#endif


/*
@@ INLUAI_BITSINT defines the number of bits in an int.
** CHANGE here if Lua cannot automatically detect the number of bits of
** your machine. Probably you do not need to change this.
*/
/* avoid overflows in comparison */
#if INT_MAX-20 < 32760
#define INLUAI_BITSINT	16
#elif INT_MAX > 2147483640L
/* int has at least 32 bits */
#define INLUAI_BITSINT	32
#else
#error "you must define INLUAI_BITSINT with number of bits in an integer"
#endif


/*
@@ INLUAI_UINT32 is an unsigned integer with at least 32 bits.
@@ INLUAI_INT32 is an signed integer with at least 32 bits.
@@ INLUAI_UMEM is an unsigned integer big enough to count the total
@* memory used by Lua.
@@ INLUAI_MEM is a signed integer big enough to count the total memory
@* used by Lua.
** CHANGE here if for some weird reason the default definitions are not
** good enough for your machine. (The definitions in the 'else'
** part always works, but may waste space on machines with 64-bit
** longs.) Probably you do not need to change this.
*/
#if INLUAI_BITSINT >= 32
#define INLUAI_UINT32	unsigned int
#define INLUAI_INT32	int
#define INLUAI_MAXINT32	INT_MAX
#define INLUAI_UMEM	size_t
#define INLUAI_MEM	ptrdiff_t
#else
/* 16-bit ints */
#define INLUAI_UINT32	unsigned long
#define INLUAI_INT32	long
#define INLUAI_MAXINT32	LONG_MAX
#define INLUAI_UMEM	unsigned long
#define INLUAI_MEM	long
#endif


/*
@@ INLUAI_MAXCALLS limits the number of nested calls.
** CHANGE it if you need really deep recursive calls. This limit is
** arbitrary; its only purpose is to stop infinite recursion before
** exhausting memory.
*/
#define INLUAI_MAXCALLS	20000


/*
@@ INLUAI_MAXCSTACK limits the number of Lua stack slots that a C function
@* can use.
** CHANGE it if you need lots of (Lua) stack space for your C
** functions. This limit is arbitrary; its only purpose is to stop C
** functions to consume unlimited stack space. (must be smaller than
** -INLUA_REGISTRYINDEX)
*/
#define INLUAI_MAXCSTACK	8000



/*
** {==================================================================
** CHANGE (to smaller values) the following definitions if your system
** has a small C stack. (Or you may want to change them to larger
** values if your system has a large C stack and these limits are
** too rigid for you.) Some of these constants control the size of
** stack-allocated arrays used by the compiler or the interpreter, while
** others limit the maximum number of recursive calls that the compiler
** or the interpreter can perform. Values too large may cause a C stack
** overflow for some forms of deep constructs.
** ===================================================================
*/


/*
@@ INLUAI_MAXCCALLS is the maximum depth for nested C calls (short) and
@* syntactical nested non-terminals in a program.
*/
#define INLUAI_MAXCCALLS		200


/*
@@ INLUAI_MAXVARS is the maximum number of local variables per function
@* (must be smaller than 250).
*/
#define INLUAI_MAXVARS		200


/*
@@ INLUAI_MAXUPVALUES is the maximum number of upvalues per function
@* (must be smaller than 250).
*/
#define INLUAI_MAXUPVALUES	60


/*
@@ INLUAL_BUFFERSIZE is the buffer size used by the lauxlib buffer system.
*/
#define INLUAL_BUFFERSIZE		BUFSIZ

/* }================================================================== */




/*
** {==================================================================
@@ INLUA_NUMBER is the type of numbers in Lua.
** CHANGE the following definitions only if you want to build Lua
** with a number type different from double. You may also need to
** change inlua_number2int & inlua_number2integer.
** ===================================================================
*/

#define INLUA_NUMBER_DOUBLE
#define INLUA_NUMBER	double

/*
@@ INLUAI_UACNUMBER is the result of an 'usual argument conversion'
@* over a number.
*/
#define INLUAI_UACNUMBER	double


/*
@@ INLUA_NUMBER_SCAN is the format for reading numbers.
@@ INLUA_NUMBER_FMT is the format for writing numbers.
@@ inlua_number2str converts a number to a string.
@@ INLUAI_MAXNUMBER2STR is maximum size of previous conversion.
@@ inlua_str2number converts a string to a number.
*/
#define INLUA_NUMBER_SCAN		"%lf"
#define INLUA_NUMBER_FMT		"%.14g"
#define inlua_number2str(s,n)	sprintf((s), INLUA_NUMBER_FMT, (n))
#define INLUAI_MAXNUMBER2STR	32 /* 16 digits, sign, point, and \0 */
#define inlua_str2number(s,p)	strtod((s), (p))


/*
@@ The luai_num* macros define the primitive operations over numbers.
*/
#if defined(INLUA_CORE)
#include <math.h>
#define inluai_numadd(a,b)	((a)+(b))
#define inluai_numsub(a,b)	((a)-(b))
#define inluai_nummul(a,b)	((a)*(b))
#define inluai_numdiv(a,b)	((a)/(b))
#define inluai_nummod(a,b)	((a) - floor((a)/(b))*(b))
#define inluai_numpow(a,b)	(pow(a,b))
#define inluai_numunm(a)		(-(a))
#define inluai_numeq(a,b)		((a)==(b))
#define inluai_numlt(a,b)		((a)<(b))
#define inluai_numle(a,b)		((a)<=(b))
#define inluai_numisnan(a)	(!inluai_numeq((a), (a)))
#endif


/*
@@ inlua_number2int is a macro to convert inlua_Number to int.
@@ inlua_number2integer is a macro to convert inlua_Number to inlua_Integer.
** CHANGE them if you know a faster way to convert a inlua_Number to
** int (with any rounding method and without throwing errors) in your
** system. In Pentium machines, a naive typecast from double to int
** in C is extremely slow, so any alternative is worth trying.
*/

/* On a Pentium, resort to a trick */
#if defined(INLUA_NUMBER_DOUBLE) && !defined(INLUA_ANSI) && !defined(__SSE2__) && \
    (defined(__i386) || defined (_M_IX86) || defined(__i386__))

/* On a Microsoft compiler, use assembler */
#if defined(_MSC_VER)

#define inlua_number2int(i,d)   __asm fld d   __asm fistp i
#define inlua_number2integer(i,n)		inlua_number2int(i, n)

/* the next trick should work on any Pentium, but sometimes clashes
   with a DirectX idiosyncrasy */
#else

union luai_Cast { double l_d; long l_l; };
#define inlua_number2int(i,d) \
  { volatile union luai_Cast u; u.l_d = (d) + 6755399441055744.0; (i) = u.l_l; }
#define inlua_number2integer(i,n)		inlua_number2int(i, n)

#endif


/* this option always works, but may be slow */
#else
#define inlua_number2int(i,d)	((i)=(int)(d))
#define inlua_number2integer(i,d)	((i)=(inlua_Integer)(d))

#endif

/* }================================================================== */


/*
@@ INLUAI_USER_ALIGNMENT_T is a type that requires maximum alignment.
** CHANGE it if your system requires alignments larger than double. (For
** instance, if your system supports long doubles and they must be
** aligned in 16-byte boundaries, then you should add long double in the
** union.) Probably you do not need to change this.
*/
#define INLUAI_USER_ALIGNMENT_T	union { double u; void *s; long l; }


/*
@@ INLUAI_THROW/INLUAI_TRY define how Lua does exception handling.
** CHANGE them if you prefer to use longjmp/setjmp even with C++
** or if want/don't to use _longjmp/_setjmp instead of regular
** longjmp/setjmp. By default, Lua handles errors with exceptions when
** compiling as C++ code, with _longjmp/_setjmp when asked to use them,
** and with longjmp/setjmp otherwise.
*/
#if defined(__cplusplus)
/* C++ exceptions */
#define INLUAI_THROW(L,c)	throw(c)
#define INLUAI_TRY(L,c,a)	try { a } catch(...) \
	{ if ((c)->status == 0) (c)->status = -1; }
#define inluai_jmpbuf	int  /* dummy variable */

#elif defined(INLUA_USE_ULONGJMP)
/* in Unix, try _longjmp/_setjmp (more efficient) */
#define INLUAI_THROW(L,c)	_longjmp((c)->b, 1)
#define INLUAI_TRY(L,c,a)	if (_setjmp((c)->b) == 0) { a }
#define inluai_jmpbuf	jmp_buf

#else
/* default handling with long jumps */
#define INLUAI_THROW(L,c)	longjmp((c)->b, 1)
#define INLUAI_TRY(L,c,a)	if (setjmp((c)->b) == 0) { a }
#define inluai_jmpbuf	jmp_buf

#endif


/*
@@ INLUA_MAXCAPTURES is the maximum number of captures that a pattern
@* can do during pattern-matching.
** CHANGE it if you need more captures. This limit is arbitrary.
*/
#define INLUA_MAXCAPTURES		32


/*
@@ inlua_tmpnam is the function that the OS library uses to create a
@* temporary name.
@@ INLUA_TMPNAMBUFSIZE is the maximum size of a name created by inlua_tmpnam.
** CHANGE them if you have an alternative to tmpnam (which is considered
** insecure) or if you want the original tmpnam anyway.  By default, Lua
** uses tmpnam except when POSIX is available, where it uses mkstemp.
*/
#if defined(inloslib_c) || defined(inluaall_c)

#if defined(INLUA_USE_MKSTEMP)
#include <unistd.h>
#define INLUA_TMPNAMBUFSIZE	32
#define inlua_tmpnam(b,e)	{ \
	strcpy(b, "/tmp/lua_XXXXXX"); \
	e = mkstemp(b); \
	if (e != -1) close(e); \
	e = (e == -1); }

#else
#define INLUA_TMPNAMBUFSIZE	L_tmpnam
#define inlua_tmpnam(b,e)		{ e = (tmpnam(b) == NULL); }
#endif

#endif


/*
@@ inlua_popen spawns a new process connected to the current one through
@* the file streams.
** CHANGE it if you have a way to implement it in your system.
*/
#if defined(INLUA_USE_POPEN)

#define inlua_popen(L,c,m)	((void)L, fflush(NULL), popen(c,m))
#define inlua_pclose(L,file)	((void)L, (pclose(file) != -1))

#elif defined(INLUA_WIN)

#define inlua_popen(L,c,m)	((void)L, _popen(c,m))
#define inlua_pclose(L,file)	((void)L, (_pclose(file) != -1))

#else

#define inlua_popen(L,c,m)	((void)((void)c, m),  \
		inluaL_error(L, INLUA_QL("popen") " not supported"), (FILE*)0)
#define inlua_pclose(L,file)		((void)((void)L, file), 0)

#endif

/*
@@ LUA_DL_* define which dynamic-library system Lua should use.
** CHANGE here if Lua has problems choosing the appropriate
** dynamic-library system for your platform (either Windows' DLL, Mac's
** dyld, or Unix's dlopen). If your system is some kind of Unix, there
** is a good chance that it has dlopen, so INLUA_DL_DLOPEN will work for
** it.  To use dlopen you also need to adapt the src/Makefile (probably
** adding -ldl to the linker options), so Lua does not select it
** automatically.  (When you change the makefile to add -ldl, you must
** also add -DINLUA_USE_DLOPEN.)
** If you do not want any kind of dynamic library, undefine all these
** options.
** By default, _WIN32 gets INLUA_DL_DLL and MAC OS X gets INLUA_DL_DYLD.
*/
#if defined(INLUA_USE_DLOPEN)
#define INLUA_DL_DLOPEN
#endif

#if defined(INLUA_WIN)
#define INLUA_DL_DLL
#endif


/*
@@ INLUAI_EXTRASPACE allows you to add user-specific data in a inlua_State
@* (the data goes just *before* the inlua_State pointer).
** CHANGE (define) this if you really need that. This value must be
** a multiple of the maximum alignment required for your machine.
*/
#define INLUAI_EXTRASPACE		0


/*
@@ luai_userstate* allow user-specific actions on threads.
** CHANGE them if you defined INLUAI_EXTRASPACE and need to do something
** extra when a thread is created/deleted/resumed/yielded.
*/
#define inluai_userstateopen(L)		((void)L)
#define inluai_userstateclose(L)		((void)L)
#define inluai_userstatethread(L,L1)	((void)L)
#define inluai_userstatefree(L)		((void)L)
#define inluai_userstateresume(L,n)	((void)L)
#define inluai_userstateyield(L,n)	((void)L)


/*
@@ INLUA_INTFRMLEN is the length modifier for integer conversions
@* in 'string.format'.
@@ INLUA_INTFRM_T is the integer type correspoding to the previous length
@* modifier.
** CHANGE them if your system supports long long or does not support long.
*/

#if defined(INLUA_USELONGLONG)

#define INLUA_INTFRMLEN		"ll"
#define INLUA_INTFRM_T		long long

#else

#define INLUA_INTFRMLEN		"l"
#define INLUA_INTFRM_T		long

#endif



/* =================================================================== */

/*
** Local configuration. You can use this space to add your redefinitions
** without modifying the main part of the file.
*/



#endif

