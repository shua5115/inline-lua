/*
** $Id: lundump.h,v 1.37.1.1 2007/12/27 13:02:25 roberto Exp $
** load precompiled Lua chunks
** See Copyright Notice in inlua.h
*/

#ifndef lundump_h
#define lundump_h

#include "lobject.h"
#include "lzio.h"

/* load one chunk; from lundump.c */
INLUAI_FUNC Proto* luaU_undump (inlua_State* L, ZIO* Z, Mbuffer* buff, const char* name);

/* make header; from lundump.c */
INLUAI_FUNC void luaU_header (char* h);

/* dump one chunk; from ldump.c */
INLUAI_FUNC int luaU_dump (inlua_State* L, const Proto* f, inlua_Writer w, void* data, int strip);

#ifdef luac_c
/* print one chunk; from print.c */
INLUAI_FUNC void luaU_print (const Proto* f, int full);
#endif

/* for header of binary files -- this is Lua 5.1 */
#define LUAC_VERSION		0x51

/* for header of binary files -- this is the official format */
#define LUAC_FORMAT		0

/* size of header of binary files */
#define LUAC_HEADERSIZE		12

#endif
