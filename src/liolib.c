/*
** $Id: liolib.c,v 2.73.1.4 2010/05/14 15:33:51 roberto Exp $
** Standard I/O (and system) library
** See Copyright Notice in inlua.h
*/


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define liolib_c
#define INLUA_LIB

#include "inlua.h"

#include "inlauxlib.h"
#include "inlualib.h"

#if defined(INLUA_USE_POSIX)
#include <unistd.h>
#include <paths.h>
#elif defined(INLUA_WIN)
#include <windows.h>
#include <fcntl.h>

// NOTE: handle is closed by this function.
FILE* Handle2Stream(HANDLE h, const char *mode) {
  int fd = _open_osfhandle((intptr_t)h, _O_BINARY);
  if (fd < 0) return NULL;
  return _fdopen(fd, mode);
}

#endif

#define IO_INPUT	1
#define IO_OUTPUT	2


static const char *const fnames[] = {"input", "output"};


static int pushresult (inlua_State *L, int i, const char *filename) {
  int en = errno;  /* calls to Lua API may change this value */
  if (i) {
    inlua_pushboolean(L, 1);
    return 1;
  }
  else {
    inlua_pushnil(L);
    if (filename)
      inlua_pushfstring(L, "%s: %s", filename, strerror(en));
    else
      inlua_pushfstring(L, "%s", strerror(en));
    inlua_pushinteger(L, en);
    return 3;
  }
}


static void fileerror (inlua_State *L, int arg, const char *filename) {
  inlua_pushfstring(L, "%s: %s", filename, strerror(errno));
  inluaL_argerror(L, arg, inlua_tostring(L, -1));
}


#define tofilep(L)	((FILE **)inluaL_checkudata(L, 1, INLUA_FILEHANDLE))


static int io_type (inlua_State *L) {
  void *ud;
  inluaL_checkany(L, 1);
  ud = inlua_touserdata(L, 1);
  inlua_getfield(L, INLUA_REGISTRYINDEX, INLUA_FILEHANDLE);
  if (ud == NULL || !inlua_getmetatable(L, 1) || !inlua_rawequal(L, -2, -1))
    inlua_pushnil(L);  /* not a file */
  else if (*((FILE **)ud) == NULL)
    inlua_pushliteral(L, "closed file");
  else
    inlua_pushliteral(L, "file");
  return 1;
}


static FILE *tofile (inlua_State *L) {
  FILE **f = tofilep(L);
  if (*f == NULL)
    inluaL_error(L, "attempt to use a closed file");
  return *f;
}



/*
** When creating file handles, always creates a `closed' file handle
** before opening the actual file; so, if there is a memory error, the
** file is not left opened.
*/
static FILE **newfile (inlua_State *L) {
  FILE **pf = (FILE **)inlua_newuserdata(L, sizeof(FILE *));
  *pf = NULL;  /* file handle is currently `closed' */
  inluaL_getmetatable(L, INLUA_FILEHANDLE);
  inlua_setmetatable(L, -2);
  return pf;
}


/*
** function to (not) close the standard files stdin, stdout, and stderr
*/
static int io_noclose (inlua_State *L) {
  inlua_pushnil(L);
  inlua_pushliteral(L, "cannot close standard file");
  return 2;
}


/*
** function to close 'popen' files
*/
static int io_pclose (inlua_State *L) {
  FILE **p = tofilep(L);
  int ok = inlua_pclose(L, *p);
  *p = NULL;
  return pushresult(L, ok, NULL);
}


/*
** function to close regular files
*/
static int io_fclose (inlua_State *L) {
  FILE **p = tofilep(L);
  int ok = (fclose(*p) == 0);
  *p = NULL;
  return pushresult(L, ok, NULL);
}


static int aux_close (inlua_State *L) {
  inlua_getfenv(L, 1);
  inlua_getfield(L, -1, "__close");
  return (inlua_tocfunction(L, -1))(L);
}


static int io_close (inlua_State *L) {
  if (inlua_isnone(L, 1))
    inlua_rawgeti(L, INLUA_ENVIRONINDEX, IO_OUTPUT);
  tofile(L);  /* make sure argument is a file */
  return aux_close(L);
}


static int io_gc (inlua_State *L) {
  FILE *f = *tofilep(L);
  /* ignore closed files */
  if (f != NULL)
    aux_close(L);
  return 0;
}


static int io_tostring (inlua_State *L) {
  FILE *f = *tofilep(L);
  if (f == NULL)
    inlua_pushliteral(L, "file (closed)");
  else
    inlua_pushfstring(L, "file (%p)", f);
  return 1;
}


static int io_open (inlua_State *L) {
  const char *filename = inluaL_checkstring(L, 1);
  const char *mode = inluaL_optstring(L, 2, "r");
  FILE **pf = newfile(L);
  *pf = fopen(filename, mode);
  return (*pf == NULL) ? pushresult(L, 0, filename) : 1;
}


/*
** this function has a separated environment, which defines the
** correct __close for 'popen' files
*/
static int io_popen (inlua_State *L) {
  const char *filename = inluaL_checkstring(L, 1);
  const char *mode = inluaL_optstring(L, 2, "r");
  FILE **pf = newfile(L);
  *pf = inlua_popen(L, filename, mode);
  return (*pf == NULL) ? pushresult(L, 0, filename) : 1;
}


static int io_subprocess (inlua_State *L) {
  size_t filename_len = 0;
  const char *filename = inluaL_checklstring(L, 1, &filename_len);
#if defined(INLUA_USE_POSIX)
  int cpid;
  int cin[2];
  int cout[2];
  int cerr[2];
  FILE **istream;
  FILE **ostream;
  FILE **estream;

  if (0 != pipe(cin)) {
    inlua_pushnil(L);
    inlua_pushnil(L);
    return 2+pushresult(L, 0, NULL);
  }
  if (0 != pipe(cout)) {
    (void)close(cin[0]);
    (void)close(cin[1]);
    inlua_pushnil(L);
    inlua_pushnil(L);
    return 2+pushresult(L, 0, NULL);
  }
  if (0 != pipe(cerr)) {
    (void)close(cin[0]);
    (void)close(cin[1]);
    (void)close(cout[0]);
    (void)close(cout[1]);
    inlua_pushnil(L);
    inlua_pushnil(L);
    return 2+pushresult(L, 0, NULL);
  }
  cpid = fork();
  if (cpid == 0) {
    // child process
    if (0 > dup2(cin[0], STDIN_FILENO) || 0 > dup2(cout[1], STDOUT_FILENO) || 0 > dup2(cerr[1], STDERR_FILENO)) {
      _Exit(127);
    }
    (void)close(cin[0]); (void)close(cin[1]); (void)close(cout[0]); (void)close(cout[1]); (void)close(cerr[0]); (void)close(cerr[1]);
    execl(_PATH_BSHELL, "sh", "-c", filename, NULL);
    _Exit(127);
  }

  (void)close(cin[0]);
  (void)close(cout[1]);
  (void)close(cerr[1]);

  ostream = newfile(L);
  *ostream = fdopen(cout[0], "r");
  if (NULL == *ostream) {
    (void)close(cout[0]);
    (void)close(cin[1]);
    (void)close(cerr[0]);
    inlua_pushnil(L);
    inlua_pushnil(L);
    return 2+pushresult(L, 0, NULL);
  }
  istream = newfile(L);
  *istream = fdopen(cin[1], "w");
  if (NULL == *istream) {
    (void)fclose(*ostream);
    (void)close(cin[1]);
    (void)close(cerr[0]);
    inlua_pushnil(L);
    inlua_pushnil(L);
    return 2+pushresult(L, 0, NULL);
  }
  estream = newfile(L);
  *estream = fdopen(cerr[0], "r");
  if (NULL == *estream) {
    (void)fclose(*ostream);
    (void)fclose(*istream);
    (void)close(cerr[0]);
    inlua_pushnil(L);
    inlua_pushnil(L);
    return 2+pushresult(L, 0, NULL);
  }
  return 3;
#elif defined(INLUA_WIN)
  HANDLE cin_rd;
  HANDLE cin_wr;
  HANDLE cout_rd;
  HANDLE cout_wr;
  HANDLE cerr_rd;
  HANDLE cerr_wr;
  FILE **istream;
  FILE **ostream;
  FILE **estream;
  SECURITY_ATTRIBUTES sa = {0};
  PROCESS_INFORMATION pi = {0};
  STARTUPINFO si = {0};
  int success;

  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = 1;
  sa.lpSecurityDescriptor = NULL;

  if (!CreatePipe(&cout_rd, &cout_wr, &sa, 0)) {
    inlua_pushnil(L);
    inlua_pushnil(L);
    inlua_pushnil(L);
    inlua_pushstring(L, "failed to create pipe");
    return 4;
  }

  if (!SetHandleInformation(cout_rd, HANDLE_FLAG_INHERIT, 0) || !CreatePipe(&cin_rd, &cin_wr, &sa, 0)) {
    CloseHandle(cout_rd);
    CloseHandle(cout_wr);
    inlua_pushnil(L);
    inlua_pushnil(L);
    inlua_pushnil(L);
    inlua_pushstring(L, "failed to create pipe");
    return 4;
  }

  if (!SetHandleInformation(cin_wr, HANDLE_FLAG_INHERIT, 0) || !CreatePipe(&cerr_rd, &cerr_wr, &sa, 0)) {
    CloseHandle(cout_rd);
    CloseHandle(cout_wr);
    CloseHandle(cin_rd);
    CloseHandle(cin_wr);
    inlua_pushnil(L);
    inlua_pushnil(L);
    inlua_pushnil(L);
    inlua_pushstring(L, "failed to create pipe");
    return 4;
  }

  if (!SetHandleInformation(cerr_rd, HANDLE_FLAG_INHERIT, 0)) {
    CloseHandle(cout_rd);
    CloseHandle(cout_wr);
    CloseHandle(cin_rd);
    CloseHandle(cin_wr);
    CloseHandle(cerr_rd);
    CloseHandle(cerr_wr);
    inlua_pushnil(L);
    inlua_pushnil(L);
    inlua_pushnil(L);
    inlua_pushstring(L, "failed to create pipe");
    return 4;
  }

  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = cout_wr;
  si.hStdInput = cin_rd;
  si.hStdError = cerr_wr;

  success = CreateProcess(NULL,
    filename, // command to run
    NULL, // default process security
    NULL, // default thread security
    TRUE, // inherit handles
    0, // flags
    NULL, // default environment
    NULL, // inherit cwd
    &si,
    &pi
  );
  if (!success) {
    inlua_pushnil(L);
    inlua_pushnil(L);
    inlua_pushnil(L);
    inlua_pushstring(L, "failed to run command");
    return 4;
  }
  // close child's handles
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(cout_wr);
  CloseHandle(cin_rd);
  CloseHandle(cerr_wr);

  ostream = newfile(L);
  *ostream = Handle2Stream(cout_rd, "r");
  if (NULL == *ostream) {
    CloseHandle(cout_rd);
    CloseHandle(cin_wr);
    CloseHandle(cerr_rd);
    inlua_pushnil(L);
    inlua_pushnil(L);
    return 2+pushresult(L, 0, NULL);
  }
  istream = newfile(L);
  *istream = Handle2Stream(cin_wr, "w");
  if (NULL == *istream) {
    fclose(*ostream);
    CloseHandle(cin_wr);
    inlua_pushnil(L);
    inlua_pushnil(L);
    return 2+pushresult(L, 0, NULL);
  }
  estream = newfile(L);
  *estream = Handle2Stream(cerr_rd, "r");
  if (NULL == *estream) {
    fclose(*ostream);
    fclose(*istream);
    inlua_pushnil(L);
    inlua_pushnil(L);
    return 2+pushresult(L, 0, NULL);
  }
  return 3;
#else
  (void) filename;
  inlua_pushnil(L);
  inlua_pushnil(L);
  inlua_pushstring(L, INLUA_QL("subprocess") " not supported");
  return 3;
#endif
}


static int io_tmpfile (inlua_State *L) {
  FILE **pf = newfile(L);
  *pf = tmpfile();
  return (*pf == NULL) ? pushresult(L, 0, NULL) : 1;
}


static FILE *getiofile (inlua_State *L, int findex) {
  FILE *f;
  inlua_rawgeti(L, INLUA_ENVIRONINDEX, findex);
  f = *(FILE **)inlua_touserdata(L, -1);
  if (f == NULL)
    inluaL_error(L, "standard %s file is closed", fnames[findex - 1]);
  return f;
}


static int g_iofile (inlua_State *L, int f, const char *mode) {
  if (!inlua_isnoneornil(L, 1)) {
    const char *filename = inlua_tostring(L, 1);
    if (filename) {
      FILE **pf = newfile(L);
      *pf = fopen(filename, mode);
      if (*pf == NULL)
        fileerror(L, 1, filename);
    }
    else {
      tofile(L);  /* check that it's a valid file handle */
      inlua_pushvalue(L, 1);
    }
    inlua_rawseti(L, INLUA_ENVIRONINDEX, f);
  }
  /* return current value */
  inlua_rawgeti(L, INLUA_ENVIRONINDEX, f);
  return 1;
}


static int io_input (inlua_State *L) {
  return g_iofile(L, IO_INPUT, "r");
}


static int io_output (inlua_State *L) {
  return g_iofile(L, IO_OUTPUT, "w");
}


static int io_readline (inlua_State *L);


static void aux_lines (inlua_State *L, int idx, int toclose) {
  inlua_pushvalue(L, idx);
  inlua_pushboolean(L, toclose);  /* close/not close file when finished */
  inlua_pushcclosure(L, io_readline, 2);
}


static int f_lines (inlua_State *L) {
  tofile(L);  /* check that it's a valid file handle */
  aux_lines(L, 1, 0);
  return 1;
}


static int io_lines (inlua_State *L) {
  if (inlua_isnoneornil(L, 1)) {  /* no arguments? */
    /* will iterate over default input */
    inlua_rawgeti(L, INLUA_ENVIRONINDEX, IO_INPUT);
    return f_lines(L);
  }
  else {
    const char *filename = inluaL_checkstring(L, 1);
    FILE **pf = newfile(L);
    *pf = fopen(filename, "r");
    if (*pf == NULL)
      fileerror(L, 1, filename);
    aux_lines(L, inlua_gettop(L), 1);
    return 1;
  }
}


/*
** {======================================================
** READ
** =======================================================
*/


static int read_number (inlua_State *L, FILE *f) {
  inlua_Number d;
  if (fscanf(f, INLUA_NUMBER_SCAN, &d) == 1) {
    inlua_pushnumber(L, d);
    return 1;
  }
  else {
    inlua_pushnil(L);  /* "result" to be removed */
    return 0;  /* read fails */
  }
}


static int test_eof (inlua_State *L, FILE *f) {
  int c = getc(f);
  ungetc(c, f);
  inlua_pushlstring(L, NULL, 0);
  return (c != EOF);
}


static int read_line (inlua_State *L, FILE *f) {
  inluaL_Buffer b;
  inluaL_buffinit(L, &b);
  for (;;) {
    size_t l;
    char *p = inluaL_prepbuffer(&b);
    if (fgets(p, INLUAL_BUFFERSIZE, f) == NULL) {  /* eof? */
      inluaL_pushresult(&b);  /* close buffer */
      return (inlua_objlen(L, -1) > 0);  /* check whether read something */
    }
    l = strlen(p);
    if (l == 0 || p[l-1] != '\n')
      inluaL_addsize(&b, l);
    else {
      inluaL_addsize(&b, l - 1);  /* do not include `eol' */
      inluaL_pushresult(&b);  /* close buffer */
      return 1;  /* read at least an `eol' */
    }
  }
}


static int read_chars (inlua_State *L, FILE *f, size_t n) {
  size_t rlen;  /* how much to read */
  size_t nr;  /* number of chars actually read */
  inluaL_Buffer b;
  inluaL_buffinit(L, &b);
  rlen = INLUAL_BUFFERSIZE;  /* try to read that much each time */
  do {
    char *p = inluaL_prepbuffer(&b);
    if (rlen > n) rlen = n;  /* cannot read more than asked */
    nr = fread(p, sizeof(char), rlen, f);
    inluaL_addsize(&b, nr);
    n -= nr;  /* still have to read `n' chars */
  } while (n > 0 && nr == rlen);  /* until end of count or eof */
  inluaL_pushresult(&b);  /* close buffer */
  return (n == 0 || inlua_objlen(L, -1) > 0);
}


static int g_read (inlua_State *L, FILE *f, int first) {
  int nargs = inlua_gettop(L) - 1;
  int success;
  int n;
  clearerr(f);
  if (nargs == 0) {  /* no arguments? */
    success = read_line(L, f);
    n = first+1;  /* to return 1 result */
  }
  else {  /* ensure stack space for all results and for auxlib's buffer */
    inluaL_checkstack(L, nargs+INLUA_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (inlua_type(L, n) == INLUA_TNUMBER) {
        size_t l = (size_t)inlua_tointeger(L, n);
        success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
      }
      else {
        const char *p = inlua_tostring(L, n);
        inluaL_argcheck(L, p && p[0] == '*', n, "invalid option");
        switch (p[1]) {
          case 'n':  /* number */
            success = read_number(L, f);
            break;
          case 'l':  /* line */
            success = read_line(L, f);
            break;
          case 'a':  /* file */
            read_chars(L, f, ~((size_t)0));  /* read MAX_SIZE_T chars */
            success = 1; /* always success */
            break;
          default:
            return inluaL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (ferror(f))
    return pushresult(L, 0, NULL);
  if (!success) {
    inlua_pop(L, 1);  /* remove last result */
    inlua_pushnil(L);  /* push nil instead */
  }
  return n - first;
}


static int io_read (inlua_State *L) {
  return g_read(L, getiofile(L, IO_INPUT), 1);
}


static int f_read (inlua_State *L) {
  return g_read(L, tofile(L), 2);
}


static int io_readline (inlua_State *L) {
  FILE *f = *(FILE **)inlua_touserdata(L, inlua_upvalueindex(1));
  int sucess;
  if (f == NULL)  /* file is already closed? */
    inluaL_error(L, "file is already closed");
  sucess = read_line(L, f);
  if (ferror(f))
    return inluaL_error(L, "%s", strerror(errno));
  if (sucess) return 1;
  else {  /* EOF */
    if (inlua_toboolean(L, inlua_upvalueindex(2))) {  /* generator created file? */
      inlua_settop(L, 0);
      inlua_pushvalue(L, inlua_upvalueindex(1));
      aux_close(L);  /* close it */
    }
    return 0;
  }
}

/* }====================================================== */


static int g_write (inlua_State *L, FILE *f, int arg) {
  int nargs = inlua_gettop(L) - 1;
  int status = 1;
  for (; nargs--; arg++) {
    if (inlua_type(L, arg) == INLUA_TNUMBER) {
      /* optimization: could be done exactly as for strings */
      status = status &&
          fprintf(f, INLUA_NUMBER_FMT, inlua_tonumber(L, arg)) > 0;
    }
    else {
      size_t l;
      const char *s = inluaL_checklstring(L, arg, &l);
      status = status && (fwrite(s, sizeof(char), l, f) == l);
    }
  }
  return pushresult(L, status, NULL);
}


static int io_write (inlua_State *L) {
  return g_write(L, getiofile(L, IO_OUTPUT), 1);
}


static int f_write (inlua_State *L) {
  return g_write(L, tofile(L), 2);
}


static int f_seek (inlua_State *L) {
  static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
  static const char *const modenames[] = {"set", "cur", "end", NULL};
  FILE *f = tofile(L);
  int op = inluaL_checkoption(L, 2, "cur", modenames);
  long offset = inluaL_optlong(L, 3, 0);
  op = fseek(f, offset, mode[op]);
  if (op)
    return pushresult(L, 0, NULL);  /* error */
  else {
    inlua_pushinteger(L, ftell(f));
    return 1;
  }
}


static int f_setvbuf (inlua_State *L) {
  static const int mode[] = {_IONBF, _IOFBF, _IOLBF};
  static const char *const modenames[] = {"no", "full", "line", NULL};
  FILE *f = tofile(L);
  int op = inluaL_checkoption(L, 2, NULL, modenames);
  inlua_Integer sz = inluaL_optinteger(L, 3, INLUAL_BUFFERSIZE);
  int res = setvbuf(f, NULL, mode[op], sz);
  return pushresult(L, res == 0, NULL);
}



static int io_flush (inlua_State *L) {
  return pushresult(L, fflush(getiofile(L, IO_OUTPUT)) == 0, NULL);
}


static int f_flush (inlua_State *L) {
  return pushresult(L, fflush(tofile(L)) == 0, NULL);
}


static const inluaL_Reg iolib[] = {
  {"close", io_close},
  {"flush", io_flush},
  {"input", io_input},
  {"lines", io_lines},
  {"open", io_open},
  {"output", io_output},
  {"popen", io_popen},
  {"subprocess", io_subprocess},
  {"read", io_read},
  {"tmpfile", io_tmpfile},
  {"type", io_type},
  {"write", io_write},
  {NULL, NULL}
};


static const inluaL_Reg flib[] = {
  {"close", io_close},
  {"flush", f_flush},
  {"lines", f_lines},
  {"read", f_read},
  {"seek", f_seek},
  {"setvbuf", f_setvbuf},
  {"write", f_write},
  {"__gc", io_gc},
  {"__tostring", io_tostring},
  {NULL, NULL}
};


static void createmeta (inlua_State *L) {
  inluaL_newmetatable(L, INLUA_FILEHANDLE);  /* create metatable for file handles */
  inlua_pushvalue(L, -1);  /* push metatable */
  inlua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  inluaL_register(L, NULL, flib);  /* file methods */
}


static void createstdfile (inlua_State *L, FILE *f, int k, const char *fname) {
  *newfile(L) = f;
  if (k > 0) {
    inlua_pushvalue(L, -1);
    inlua_rawseti(L, INLUA_ENVIRONINDEX, k);
  }
  inlua_pushvalue(L, -2);  /* copy environment */
  inlua_setfenv(L, -2);  /* set it */
  inlua_setfield(L, -3, fname);
}


static void newfenv (inlua_State *L, inlua_CFunction cls) {
  inlua_createtable(L, 0, 1);
  inlua_pushcfunction(L, cls);
  inlua_setfield(L, -2, "__close");
}


INLUALIB_API int inluaopen_io (inlua_State *L) {
  createmeta(L);
  /* create (private) environment (with fields IO_INPUT, IO_OUTPUT, __close) */
  newfenv(L, io_fclose);
  inlua_replace(L, INLUA_ENVIRONINDEX);
  /* open library */
  inluaL_register(L, INLUA_IOLIBNAME, iolib);
  /* create (and set) default files */
  newfenv(L, io_noclose);  /* close function for default files */
  createstdfile(L, stdin, IO_INPUT, "stdin");
  createstdfile(L, stdout, IO_OUTPUT, "stdout");
  createstdfile(L, stderr, 0, "stderr");
  inlua_pop(L, 1);  /* pop environment for default files */
  inlua_getfield(L, -1, "popen");
  newfenv(L, io_pclose);  /* create environment for 'popen' */
  inlua_setfenv(L, -2);  /* set fenv for 'popen' */
  inlua_pop(L, 1);  /* pop 'popen' */
  return 1;
}

