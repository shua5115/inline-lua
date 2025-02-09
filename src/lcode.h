/*
** $Id: lcode.h,v 1.48.1.1 2007/12/27 13:02:25 roberto Exp $
** Code generator for Lua
** See Copyright Notice in inlua.h
*/

#ifndef lcode_h
#define lcode_h

#include "llex.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"


/*
** Marks the end of a patch list. It is an invalid value both as an absolute
** address, and as a list link (would link an element to itself).
*/
#define NO_JUMP (-1)


/*
** grep "ORDER OPR" if you change these enums
*/
typedef enum BinOpr {
  OPR_ADD, OPR_SUB, OPR_MUL, OPR_DIV, OPR_MOD, OPR_POW,
  OPR_CONCAT,
  OPR_NE, OPR_EQ,
  OPR_LT, OPR_LE, OPR_GT, OPR_GE,
  OPR_AND, OPR_OR,
  OPR_NOBINOPR
} BinOpr;


typedef enum UnOpr { OPR_MINUS, OPR_NOT, OPR_LEN, OPR_NOUNOPR } UnOpr;


#define getcode(fs,e)	((fs)->f->code[(e)->u.s.info])

#define luaK_codeAsBx(fs,o,A,sBx)	luaK_codeABx(fs,o,A,(sBx)+MAXARG_sBx)

#define luaK_setmultret(fs,e)	luaK_setreturns(fs, e, INLUA_MULTRET)

INLUAI_FUNC int luaK_codeABx (FuncState *fs, OpCode o, int A, unsigned int Bx);
INLUAI_FUNC int luaK_codeABC (FuncState *fs, OpCode o, int A, int B, int C);
INLUAI_FUNC void luaK_fixline (FuncState *fs, int line);
INLUAI_FUNC void luaK_nil (FuncState *fs, int from, int n);
INLUAI_FUNC void luaK_reserveregs (FuncState *fs, int n);
INLUAI_FUNC void luaK_checkstack (FuncState *fs, int n);
INLUAI_FUNC int luaK_stringK (FuncState *fs, TString *s);
INLUAI_FUNC int luaK_numberK (FuncState *fs, inlua_Number r);
INLUAI_FUNC void luaK_dischargevars (FuncState *fs, expdesc *e);
INLUAI_FUNC int luaK_exp2anyreg (FuncState *fs, expdesc *e);
INLUAI_FUNC void luaK_exp2nextreg (FuncState *fs, expdesc *e);
INLUAI_FUNC void luaK_exp2val (FuncState *fs, expdesc *e);
INLUAI_FUNC int luaK_exp2RK (FuncState *fs, expdesc *e);
INLUAI_FUNC void luaK_self (FuncState *fs, expdesc *e, expdesc *key);
INLUAI_FUNC void luaK_indexed (FuncState *fs, expdesc *t, expdesc *k);
INLUAI_FUNC void luaK_goiftrue (FuncState *fs, expdesc *e);
INLUAI_FUNC void luaK_storevar (FuncState *fs, expdesc *var, expdesc *e);
INLUAI_FUNC void luaK_setreturns (FuncState *fs, expdesc *e, int nresults);
INLUAI_FUNC void luaK_setoneret (FuncState *fs, expdesc *e);
INLUAI_FUNC int luaK_blockresults2regs(FuncState *fs, expdesc *e);
INLUAI_FUNC int luaK_jump (FuncState *fs);
INLUAI_FUNC void luaK_ret (FuncState *fs, int first, int nret);
INLUAI_FUNC void luaK_patchlist (FuncState *fs, int list, int target);
INLUAI_FUNC void luaK_patchtohere (FuncState *fs, int list);
INLUAI_FUNC void luaK_concat (FuncState *fs, int *l1, int l2);
INLUAI_FUNC int luaK_getlabel (FuncState *fs);
INLUAI_FUNC void luaK_prefix (FuncState *fs, UnOpr op, expdesc *v);
INLUAI_FUNC void luaK_infix (FuncState *fs, BinOpr op, expdesc *v);
INLUAI_FUNC void luaK_posfix (FuncState *fs, BinOpr op, expdesc *v1, expdesc *v2);
INLUAI_FUNC void luaK_setlist (FuncState *fs, int base, int nelems, int tostore);


#endif
