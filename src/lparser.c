/*
** $Id: lparser.c,v 2.42.1.4 2011/10/21 19:31:42 roberto Exp $
** Lua Parser
** See Copyright Notice in inlua.h
*/

#include <stdio.h> // TODO REMOVE!!!
#include <string.h>

#define lparser_c
#define INLUA_CORE

#include "inlua.h"

#include "lcode.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "llex.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"



#define hasmultret(k)		((k) == VCALL || (k) == VVARARG)

#define getlocvar(fs, i)	((fs)->f->locvars[(fs)->actvar[i]])

#define luaY_checklimit(fs,v,l,m)	if ((v)>(l)) errorlimit(fs,l,m)


/*
** nodes for block list (list of active blocks)
*/
typedef struct BlockCnt {
  struct BlockCnt *previous;  /* chain */
  int breaklist;  /* list of jumps out of this loop */
  lu_byte nactvar;  /* # active locals outside the breakable structure */
  lu_byte freereg; /* previous free register, added because blocks can be expressions */
  lu_byte upval;  /* true if some variable in the block is an upvalue */
  lu_byte isbreakable;  /* true if `block' is a loop */
} BlockCnt;



/*
** prototypes for recursive non-terminal functions
*/
static void chunk (LexState *ls);
static void expr (LexState *ls, expdesc *v);

/* prototypes for usage before definition */
static void retstat (LexState *ls);
static void breakstat (LexState *ls);
static void whileexpr (LexState *ls, int line, expdesc *e);
static void forexpr (LexState *ls, int line, expdesc *e);
static void block (LexState *ls, expdesc *e);

static void anchor_token (LexState *ls) {
  if (ls->t.token == TK_NAME || ls->t.token == TK_STRING) {
    TString *ts = ls->t.seminfo.ts;
    luaX_newstring(ls, getstr(ts), ts->tsv.len);
  }
}


static void error_expected (LexState *ls, int token) {
  luaX_syntaxerror(ls,
      luaO_pushfstring(ls->L, INLUA_QS " expected", luaX_token2str(ls, token)));
}


static void errorlimit (FuncState *fs, int limit, const char *what) {
  const char *msg = (fs->f->linedefined == 0) ?
    luaO_pushfstring(fs->L, "main function has more than %d %s", limit, what) :
    luaO_pushfstring(fs->L, "function at line %d has more than %d %s",
                            fs->f->linedefined, limit, what);
  luaX_lexerror(fs->ls, msg, 0);
}


static int testnext (LexState *ls, int c) {
  if (ls->t.token == c) {
    luaX_next(ls);
    return 1;
  }
  else return 0;
}


static void check (LexState *ls, int c) {
  if (ls->t.token != c)
    error_expected(ls, c);
}

static void checknext (LexState *ls, int c) {
  check(ls, c);
  luaX_next(ls);
}


#define check_condition(ls,c,msg)	{ if (!(c)) luaX_syntaxerror(ls, msg); }



static void check_match (LexState *ls, int what, int who, int where) {
  if (!testnext(ls, what)) {
    if (where == ls->linenumber)
      error_expected(ls, what);
    else {
      luaX_syntaxerror(ls, luaO_pushfstring(ls->L,
             INLUA_QS " expected (to close " INLUA_QS " at line %d)",
              luaX_token2str(ls, what), luaX_token2str(ls, who), where));
    }
  }
}


static TString *str_checkname (LexState *ls) {
  TString *ts;
  check(ls, TK_NAME);
  ts = ls->t.seminfo.ts;
  luaX_next(ls);
  return ts;
}


static void init_exp (expdesc *e, expkind k, int i) {
  e->f = e->t = NO_JUMP;
  e->k = k;
  e->u.s.info = i;
}


static void codestring (LexState *ls, expdesc *e, TString *s) {
  init_exp(e, VK, luaK_stringK(ls->fs, s));
}


static void checkname(LexState *ls, expdesc *e) {
  codestring(ls, e, str_checkname(ls));
}


static int registerlocalvar (LexState *ls, TString *varname) {
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  int oldsize = f->sizelocvars;
  luaM_growvector(ls->L, f->locvars, fs->nlocvars, f->sizelocvars,
                  LocVar, SHRT_MAX, "too many local variables");
  while (oldsize < f->sizelocvars) f->locvars[oldsize++].varname = NULL;
  f->locvars[fs->nlocvars].varname = varname;
  luaC_objbarrier(ls->L, f, varname);
  return fs->nlocvars++;
}


#define new_localvarliteral(ls,v,n) \
  new_localvar(ls, luaX_newstring(ls, "" v, (sizeof(v)/sizeof(char))-1), n)


static void new_localvar (LexState *ls, TString *name, int n) {
  FuncState *fs = ls->fs;
  luaY_checklimit(fs, fs->nactvar+n+1, INLUAI_MAXVARS, "local variables");
  fs->actvar[fs->nactvar+n] = cast(unsigned short, registerlocalvar(ls, name));
}


static void adjustlocalvars (LexState *ls, int nvars) {
  FuncState *fs = ls->fs;
  fs->nactvar = cast_byte(fs->nactvar + nvars);
  for (; nvars; nvars--) {
    getlocvar(fs, fs->nactvar - nvars).startpc = fs->pc;
  }
}


static void removevars (LexState *ls, int tolevel) {
  FuncState *fs = ls->fs;
  while (fs->nactvar > tolevel)
    getlocvar(fs, --fs->nactvar).endpc = fs->pc;
}


static int indexupvalue (FuncState *fs, TString *name, expdesc *v) {
  int i;
  Proto *f = fs->f;
  int oldsize = f->sizeupvalues;
  for (i=0; i<f->nups; i++) {
    if (fs->upvalues[i].k == v->k && fs->upvalues[i].info == v->u.s.info) {
      inlua_assert(f->upvalues[i] == name);
      return i;
    }
  }
  /* new one */
  luaY_checklimit(fs, f->nups + 1, INLUAI_MAXUPVALUES, "upvalues");
  luaM_growvector(fs->L, f->upvalues, f->nups, f->sizeupvalues,
                  TString *, MAX_INT, "");
  while (oldsize < f->sizeupvalues) f->upvalues[oldsize++] = NULL;
  f->upvalues[f->nups] = name;
  luaC_objbarrier(fs->L, f, name);
  inlua_assert(v->k == VLOCAL || v->k == VUPVAL);
  fs->upvalues[f->nups].k = cast_byte(v->k);
  fs->upvalues[f->nups].info = cast_byte(v->u.s.info);
  return f->nups++;
}


static int searchvar (FuncState *fs, TString *n) {
  int i;
  for (i=fs->nactvar-1; i >= 0; i--) {
    if (fs->actvar[i] < fs->f->sizelocvars && n == getlocvar(fs, i).varname)
      return i;
  }
  return -1;  /* not found */
}


static void markupval (FuncState *fs, int level) {
  BlockCnt *bl = fs->bl;
  while (bl && bl->nactvar > level) bl = bl->previous;
  if (bl) bl->upval = 1;
}


static int singlevaraux (FuncState *fs, TString *n, expdesc *var, int base) {
  if (fs == NULL) {  /* no more levels? */
    init_exp(var, VGLOBAL, NO_REG);  /* default is global variable */
    return VGLOBAL;
  }
  else {
    int v = searchvar(fs, n);  /* look up at current level */
    if (v >= 0) {
      init_exp(var, VLOCAL, v);
      if (!base)
        markupval(fs, v);  /* local will be used as an upval */
      return VLOCAL;
    }
    else {  /* not found at current level; try upper one */
      if (singlevaraux(fs->prev, n, var, 0) == VGLOBAL)
        return VGLOBAL;
      var->u.s.info = indexupvalue(fs, n, var);  /* else was LOCAL or UPVAL */
      var->k = VUPVAL;  /* upvalue in this level */
      return VUPVAL;
    }
  }
}


static void singlevar (LexState *ls, expdesc *var) {
  TString *varname = str_checkname(ls);
  FuncState *fs = ls->fs;
  if (singlevaraux(fs, varname, var, 1) == VGLOBAL)
    var->u.s.info = luaK_stringK(fs, varname);  /* info points to global name */
}


static void adjust_assign (LexState *ls, int nvars, int nexps, expdesc *e) {
  FuncState *fs = ls->fs;
  int extra = nvars - nexps;
  if (hasmultret(e->k)) {
    extra++;  /* includes call itself */
    if (extra < 0) extra = 0;
    luaK_setreturns(fs, e, extra);  /* last exp. provides the difference */
    if (extra > 1) luaK_reserveregs(fs, extra-1);
  }
  else {
    /* close last expression */
    if (e->k != VVOID && e->k != VBLOCK) {
      // either no result, or all results are already present
      luaK_exp2nextreg(fs, e);
    }
    if (extra > 0) {
      int reg = fs->freereg;
      luaK_reserveregs(fs, extra);
      luaK_nil(fs, reg, extra);
    }
  }
}


static void enterlevel (LexState *ls) {
  if (++ls->L->nCcalls > INLUAI_MAXCCALLS)
	luaX_lexerror(ls, "chunk has too many syntax levels", 0);
}


#define leavelevel(ls)	((ls)->L->nCcalls--)


static void enterblock (FuncState *fs, BlockCnt *bl, lu_byte isbreakable) {
  int i;
  bl->breaklist = NO_JUMP;
  bl->isbreakable = isbreakable;
  bl->nactvar = fs->nactvar;
  bl->freereg = fs->freereg;
  bl->upval = 0;
  bl->previous = fs->bl;
  fs->bl = bl;
  // WARNING: local variables in between previous nactvar and freereg are uninitialized.
  // This behavior is prevents clobbering the stack when a block is used in an expression with nonrelocables.
  for(i = fs->nactvar; i < fs->freereg; i++) {
    fs->actvar[i] = (unsigned short) -1; // make detectably invalid
  }
  fs->nactvar = fs->freereg;
  // inlua_assert(fs->freereg == fs->nactvar);
}

// If e is not NULL, sets e to be an VBLOCK expression,
static void leaveblock (FuncState *fs, expdesc *e) {
  BlockCnt *bl = fs->bl;
  int ret_reg = fs->nactvar;
  fs->bl = bl->previous;
  
  removevars(fs->ls, bl->freereg);
  inlua_assert(fs->nactvar >= bl->nactvar); // should not remove more variables than it started with
  fs->nactvar = bl->nactvar; // this needs to be explicit, since blocks start by setting fs->nactvar to freereg.

  if (bl->upval)
    luaK_codeABC(fs, OP_CLOSE, bl->freereg, 0, 0);
  /* a block either controls scope or breaks (never both) */
  inlua_assert(!bl->isbreakable || !bl->upval);
  inlua_assert(bl->nactvar == fs->nactvar);
  if (e != NULL) {
    // set block
    init_exp(e, VBLOCK, ret_reg);
    // if block is breakable, can only return value from break expression, not the last remaining registers.
    // Expecting result in register bl->nactvar.
    if (bl->isbreakable) {
      e->u.s.info = bl->freereg; // breakable block's first available register
      e->u.s.aux = e->u.s.info + 1; // returns one value always
    } else {
      e->u.s.aux = fs->freereg;
    }
  }
  // fs->freereg = fs->nactvar;  /* free registers */
  fs->freereg = bl->freereg;
  luaK_patchtohere(fs, bl->breaklist);
}


static void pushclosure (LexState *ls, FuncState *func, expdesc *v) {
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  int oldsize = f->sizep;
  int i;
  luaM_growvector(ls->L, f->p, fs->np, f->sizep, Proto *,
                  MAXARG_Bx, "constant table overflow");
  while (oldsize < f->sizep) f->p[oldsize++] = NULL;
  f->p[fs->np++] = func->f;
  luaC_objbarrier(ls->L, f, func->f);
  init_exp(v, VRELOCABLE, luaK_codeABx(fs, OP_CLOSURE, 0, fs->np-1));
  for (i=0; i<func->f->nups; i++) {
    OpCode o = (func->upvalues[i].k == VLOCAL) ? OP_MOVE : OP_GETUPVAL;
    luaK_codeABC(fs, o, 0, func->upvalues[i].info, 0);
  }
}


static void open_func (LexState *ls, FuncState *fs) {
  inlua_State *L = ls->L;
  Proto *f = luaF_newproto(L);
  fs->f = f;
  fs->prev = ls->fs;  /* linked list of funcstates */
  fs->ls = ls;
  fs->L = L;
  ls->fs = fs;
  fs->pc = 0;
  fs->lasttarget = -1;
  fs->jpc = NO_JUMP;
  fs->freereg = 0;
  fs->nk = 0;
  fs->np = 0;
  fs->nlocvars = 0;
  fs->nactvar = 0;
  fs->bl = NULL;
  f->source = ls->source;
  f->maxstacksize = 2;  /* registers 0/1 are always valid */
  fs->h = luaH_new(L, 0, 0);
  /* anchor table of constants and prototype (to avoid being collected) */
  sethvalue2s(L, L->top, fs->h);
  incr_top(L);
  setptvalue2s(L, L->top, f);
  incr_top(L);
}


static void close_func (LexState *ls) {
  inlua_State *L = ls->L;
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  removevars(ls, 0);
  luaK_ret(fs, 0, 0);  /* final return */
  luaM_reallocvector(L, f->code, f->sizecode, fs->pc, Instruction);
  f->sizecode = fs->pc;
  luaM_reallocvector(L, f->lineinfo, f->sizelineinfo, fs->pc, int);
  f->sizelineinfo = fs->pc;
  luaM_reallocvector(L, f->k, f->sizek, fs->nk, TValue);
  f->sizek = fs->nk;
  luaM_reallocvector(L, f->p, f->sizep, fs->np, Proto *);
  f->sizep = fs->np;
  luaM_reallocvector(L, f->locvars, f->sizelocvars, fs->nlocvars, LocVar);
  f->sizelocvars = fs->nlocvars;
  luaM_reallocvector(L, f->upvalues, f->sizeupvalues, f->nups, TString *);
  f->sizeupvalues = f->nups;
  inlua_assert(luaG_checkcode(f));
  inlua_assert(fs->bl == NULL);
  ls->fs = fs->prev;
  /* last token read was anchored in defunct function; must reanchor it */
  if (fs) anchor_token(ls);
  L->top -= 2;  /* remove table and prototype from the stack */
}


Proto *luaY_parser (inlua_State *L, ZIO *z, Mbuffer *buff, const char *name) {
  struct LexState lexstate;
  struct FuncState funcstate;
  lexstate.buff = buff;
  luaX_setinput(L, &lexstate, z, luaS_new(L, name));
  open_func(&lexstate, &funcstate);
  funcstate.f->is_vararg = VARARG_ISVARARG;  /* main func. is always vararg */
  luaX_next(&lexstate);  /* read first token */
  chunk(&lexstate);
  check(&lexstate, TK_EOS);
  close_func(&lexstate);
  inlua_assert(funcstate.prev == NULL);
  inlua_assert(funcstate.f->nups == 0);
  inlua_assert(lexstate.fs == NULL);
  return funcstate.f;
}



/*============================================================*/
/* GRAMMAR RULES */
/*============================================================*/


static void field (LexState *ls, expdesc *v) {
  /* field -> ['.' | ':'] NAME */
  FuncState *fs = ls->fs;
  expdesc key;
  luaK_exp2anyreg(fs, v);
  luaX_next(ls);  /* skip the dot or colon */
  checkname(ls, &key);
  luaK_indexed(fs, v, &key);
}


// static void yindex (LexState *ls, expdesc *v) {
//   /* index -> '[' expr ']' */
//   luaX_next(ls);  /* skip the '[' */
//   expr(ls, v);
//   luaK_exp2val(ls->fs, v);
//   checknext(ls, ']');
// }


static void eindex(LexState *ls, expdesc *v) {
  /* index -> '(' expr ')' */
  luaX_next(ls);  /* skip the '(' */
  expr(ls, v);
  luaK_exp2val(ls->fs, v);
  checknext(ls, ')');
}


/*
** {======================================================================
** Rules for Constructors
** =======================================================================
*/


struct ConsControl {
  expdesc v;  /* last list item read */
  expdesc *t;  /* table descriptor */
  int nh;  /* total number of `record' elements */
  int na;  /* total number of array elements */
  int tostore;  /* number of array elements pending to be stored */
};


static void recfield (LexState *ls, struct ConsControl *cc) {
  /* recfield -> (.NAME | `.('expr`)') = expr */
  FuncState *fs = ls->fs;
  int reg = ls->fs->freereg;
  expdesc key, val;
  int rkkey;
  luaX_next(ls); // skip '.'
  if (ls->t.token == TK_NAME) {
    luaY_checklimit(fs, cc->nh, MAX_INT, "items in a constructor");
    checkname(ls, &key);
  }
  else {
    check(ls, '(');
    eindex(ls, &key);
  }
  cc->nh++;
  checknext(ls, '=');
  rkkey = luaK_exp2RK(fs, &key);
  expr(ls, &val);
  luaK_codeABC(fs, OP_SETTABLE, cc->t->u.s.info, rkkey, luaK_exp2RK(fs, &val));
  fs->freereg = reg;  /* free registers */
}


static void closelistfield (FuncState *fs, struct ConsControl *cc) {
  if (cc->v.k == VVOID) return;  /* there is no list item */
  luaK_exp2nextreg(fs, &cc->v);
  cc->v.k = VVOID;
  if (cc->tostore == LFIELDS_PER_FLUSH) {
    luaK_setlist(fs, cc->t->u.s.info, cc->na, cc->tostore);  /* flush */
    cc->tostore = 0;  /* no more items pending */
  }
}


static void lastlistfield (FuncState *fs, struct ConsControl *cc) {
  if (cc->tostore == 0) return;
  if (hasmultret(cc->v.k)) {
    luaK_setmultret(fs, &cc->v);
    luaK_setlist(fs, cc->t->u.s.info, cc->na, INLUA_MULTRET);
    cc->na--;  /* do not count last expression (unknown number of elements) */
  }
  else {
    if (cc->v.k == VBLOCK) {
      int adj = luaK_blockresults2regs(fs, &cc->v)-1;
      cc->na += adj;
      cc->tostore += adj;
    } else if (cc->v.k != VVOID)
      luaK_exp2nextreg(fs, &cc->v);
    luaK_setlist(fs, cc->t->u.s.info, cc->na, cc->tostore);
  }
}


static void listfield (LexState *ls, struct ConsControl *cc) {
  expr(ls, &cc->v);
  luaY_checklimit(ls->fs, cc->na, MAX_INT, "items in a constructor");
  cc->na++;
  cc->tostore++;
}


static void constructor (LexState *ls, expdesc *t) {
  /* constructor -> ?? */
  FuncState *fs = ls->fs;
  int line = ls->linenumber;
  int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
  struct ConsControl cc;
  cc.na = cc.nh = cc.tostore = 0;
  cc.t = t;
  init_exp(t, VRELOCABLE, pc);
  init_exp(&cc.v, VVOID, 0);  /* no value (yet) */
  luaK_exp2nextreg(ls->fs, t);  /* fix it at stack top (for gc) */
  checknext(ls, '{');
  do {
    inlua_assert(cc.v.k == VVOID || cc.tostore > 0);
    if (ls->t.token == '}') break;
    closelistfield(fs, &cc);
    switch(ls->t.token) {
      // case TK_NAME: {  /* may be listfields or recfields */
      //   luaX_lookahead(ls);
      //   if (ls->lookahead.token != '=')  /* expression? */
      //     listfield(ls, &cc);
      //   else
      //     recfield(ls, &cc);
      //   break;
      // }
      case '.': {  /* constructor_item -> recfield */
        recfield(ls, &cc);
        break;
      }
      default: {  /* constructor_part -> listfield */
        listfield(ls, &cc);
        break;
      }
    }
  } while (testnext(ls, ',') || testnext(ls, ';'));
  check_match(ls, '}', '{', line);
  lastlistfield(fs, &cc);
  SETARG_B(fs->f->code[pc], luaO_int2fb(cc.na)); /* set initial array size */
  SETARG_C(fs->f->code[pc], luaO_int2fb(cc.nh));  /* set initial table size */
}

/* }====================================================================== */



static void parlist (LexState *ls) {
  /* parlist -> [ param { `,' param } ] */
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  int nparams = 0;
  f->is_vararg = 0;
  if (ls->t.token != ']') {  /* is `parlist' not empty? */
    do {
      switch (ls->t.token) {
        case TK_NAME: {  /* param -> NAME */
          new_localvar(ls, str_checkname(ls), nparams++);
          break;
        }
        case TK_DOTS: {  /* param -> `...' */
          luaX_next(ls);
#if defined(INLUA_COMPAT_VARARG)
          /* use `arg' as default name */
          new_localvarliteral(ls, "arg", nparams++);
          f->is_vararg = VARARG_HASARG | VARARG_NEEDSARG;
#endif
          f->is_vararg |= VARARG_ISVARARG;
          break;
        }
        default: luaX_syntaxerror(ls, "<name> or " INLUA_QL("...") " expected");
      }
    } while (!f->is_vararg && testnext(ls, ','));
  }
  adjustlocalvars(ls, nparams);
  f->numparams = cast_byte(fs->nactvar - (f->is_vararg & VARARG_HASARG));
  luaK_reserveregs(fs, fs->nactvar);  /* reserve register for parameters */
}

// Note: body expects current token to be '['
static void body (LexState *ls, expdesc *e, int needself, int line) {
  /* body ->  `[' parlist `]' (chunk) */
  FuncState new_fs;
  open_func(ls, &new_fs);
  new_fs.f->linedefined = line;
  checknext(ls, '[');
  if (needself) {
    new_localvarliteral(ls, "self", 0);
    adjustlocalvars(ls, 1);
  }
  parlist(ls);
  check_match(ls, ']', '[', new_fs.f->linedefined);
  checknext(ls, '(');
  chunk(ls);
  new_fs.f->lastlinedefined = ls->linenumber;
  check_match(ls, ')', '(', line);
  close_func(ls);
  pushclosure(ls, &new_fs, e);
}


static int explist1 (LexState *ls, expdesc *v) {
  /* explist1 -> expr { `,' expr } */
  FuncState *fs = ls->fs;
  int n = 1;  /* at least one expression */  
  expr(ls, v);
  while (testnext(ls, ',')) {
    luaK_exp2nextreg(fs, v);
    expr(ls, v);
    n++;
  }
  if (v->k == VBLOCK) { // expand block values here
    n += luaK_blockresults2regs(fs, v)-1; // push all results to stack
  }
  return n;
}


static void funcargs (LexState *ls, expdesc *f) {
  FuncState *fs = ls->fs;
  expdesc args;
  int base, nparams;
  int line = ls->linenumber;
  switch (ls->t.token) {
    case '(': {  /* funcargs -> `(' [ explist1 ] `)' */
      if (line != ls->lastline)
        luaX_syntaxerror(ls,"ambiguous syntax (function call x new statement)");
      luaX_next(ls);
      if (ls->t.token == ')')  /* arg list is empty? */
        args.k = VVOID;
      else {
        explist1(ls, &args);
        luaK_setmultret(fs, &args);
      }
      check_match(ls, ')', '(', line);
      break;
    }
    // case '{': {  /* funcargs -> constructor */
    //   constructor(ls, &args);
    //   break;
    // }
    // case TK_STRING: {  /* funcargs -> STRING */
    //   codestring(ls, &args, ls->t.seminfo.ts);
    //   luaX_next(ls);  /* must use `seminfo' before `next' */
    //   break;
    // }
    default: {
      luaX_syntaxerror(ls, "function arguments expected");
      return;
    }
  }
  inlua_assert(f->k == VNONRELOC);
  base = f->u.s.info;  /* base register for call */
  if (hasmultret(args.k))
    nparams = INLUA_MULTRET;  /* open call */
  else {
    if (args.k != VVOID && args.k != VBLOCK)
      luaK_exp2nextreg(fs, &args);  /* close last argument */
    nparams = fs->freereg - (base+1);
  }
  init_exp(f, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams+1, 2));
  luaK_fixline(fs, line);
  fs->freereg = base+1;  /* call remove function and arguments and leaves
                            (unless changed) one result */
}




/*
** {======================================================================
** Expression parsing
** =======================================================================
*/


static void prefixexp (LexState *ls, expdesc *v) {
  /* prefixexp -> NAME | '(' chunk ')' | whileexpr */
  switch (ls->t.token) {
    case '(': { // treat block as expression
      int line = ls->linenumber;
      luaX_next(ls); // skip '('
      block(ls, v);
      check_match(ls, ')', '(', line);
      return;
    }
    case '?': {
      whileexpr(ls, ls->linenumber, v);
      return;
    }
    case TK_FOR: {
      forexpr(ls, ls->linenumber, v);
      return;
    }
    case TK_NAME: {
      singlevar(ls, v);
      return;
    }
    default: {
      luaX_syntaxerror(ls, "unexpected symbol");
      return;
    }
  }
}


static void primaryexp (LexState *ls, expdesc *v) {
  /* primaryexp ->
        BREAK exp |
        RETURN {explist} | 
        prefixexp { `.' NAME | `.(' exp `)' | `:' NAME funcargs | (funcargs) } */
  FuncState *fs = ls->fs;
  if (ls->t.token == TK_RETURN) {
    luaX_next(ls);
    retstat(ls);
    testnext(ls, ';');
    init_exp(v, VNIL, 0);
    return;
  }
  if (ls->t.token == TK_BREAK) {
    luaX_next(ls);
    breakstat(ls);
    testnext(ls, ';');
    init_exp(v, VNIL, 0);
    return;
  }
  prefixexp(ls, v);
  for (;;) {
    switch (ls->t.token) {
      case '.': {  /* field or index */
        luaX_lookahead(ls);
        if (ls->lookahead.token == '(') {
          expdesc key;
          luaX_next(ls); // skip '.'
          luaK_exp2anyreg(fs, v);
          eindex(ls, &key);
          luaK_indexed(fs, v, &key);
        } else {
          field(ls, v);
        }
        break;
      }
      case ':': {  /* `:' NAME funcargs */
        expdesc key;
        luaX_next(ls);
        checkname(ls, &key);
        luaK_self(fs, v, &key);
        funcargs(ls, v);
        break;
      }
      case '(': {  /* funcargs */
        luaK_exp2nextreg(fs, v);
        funcargs(ls, v);
        break;
      }
      default: return;
    }
  }
}


static void simpleexp (LexState *ls, expdesc *v) {
  /* simpleexp -> NUMBER | STRING | NIL | true | false | ... |
                  constructor | FUNCTION body | primaryexp */
  switch (ls->t.token) {
    case TK_NUMBER: {
      init_exp(v, VKNUM, 0);
      v->u.nval = ls->t.seminfo.r;
      break;
    }
    case TK_STRING: {
      codestring(ls, v, ls->t.seminfo.ts);
      break;
    }
    case '~': {
      init_exp(v, VNIL, 0);
      break;
    }
    case TK_DOTS: {  /* vararg */
      FuncState *fs = ls->fs;
      check_condition(ls, fs->f->is_vararg,
                      "cannot use " INLUA_QL("...") " outside a vararg function");
      fs->f->is_vararg &= ~VARARG_NEEDSARG;  /* don't need 'arg' */
      init_exp(v, VVARARG, luaK_codeABC(fs, OP_VARARG, 0, 1, 0));
      break;
    }
    case '{': {  /* constructor */
      constructor(ls, v);
      return;
    }
    case '[': { // function literal [args](body)
      // luaX_next(ls); // body expects '[' as first token
      body(ls, v, 0, ls->linenumber);
      return;
    }
    default: {
      primaryexp(ls, v);
      return;
    }
  }
  luaX_next(ls);
}


static UnOpr getunopr (int op) {
  switch (op) {
    case '!': return OPR_NOT;
    case '-': return OPR_MINUS;
    case '#': return OPR_LEN;
    default: return OPR_NOUNOPR;
  }
}


static BinOpr getbinopr (int op) {
  switch (op) {
    case '+': return OPR_ADD;
    case '-': return OPR_SUB;
    case '*': return OPR_MUL;
    case '/': return OPR_DIV;
    case '%': return OPR_MOD;
    case '^': return OPR_POW;
    case TK_CONCAT: return OPR_CONCAT;
    case TK_NE: return OPR_NE;
    case TK_EQ: return OPR_EQ;
    case '<': return OPR_LT;
    case TK_LE: return OPR_LE;
    case '>': return OPR_GT;
    case TK_GE: return OPR_GE;
    case '&': return OPR_AND;
    case '|': return OPR_OR;
    default: return OPR_NOBINOPR;
  }
}


static const struct {
  lu_byte left;  /* left priority for each binary operator */
  lu_byte right; /* right priority */
} priority[] = {  /* ORDER OPR */
   {6, 6}, {6, 6}, {7, 7}, {7, 7}, {7, 7},  /* `+' `-' `/' `%' */
   {10, 9}, {5, 4},                 /* power and concat (right associative) */
   {3, 3}, {3, 3},                  /* equality and inequality */
   {3, 3}, {3, 3}, {3, 3}, {3, 3},  /* order */
   {2, 2}, {1, 1}                   /* logical (and/or) */
};

#define UNARY_PRIORITY	8  /* priority for unary operators */


/*
** subexpr -> (simpleexp | unop subexpr) { binop subexpr }
** where `binop' is any binary operator with a priority higher than `limit'
*/
static BinOpr subexpr (LexState *ls, expdesc *v, unsigned int limit) {
  BinOpr op;
  UnOpr uop;
  enterlevel(ls);
  uop = getunopr(ls->t.token);
  if (uop != OPR_NOUNOPR) {
    luaX_next(ls);
    subexpr(ls, v, UNARY_PRIORITY);
    luaK_prefix(ls->fs, uop, v);
  }
  else simpleexp(ls, v);
  /* expand while operators have priorities higher than `limit' */
  op = getbinopr(ls->t.token);
  while (op != OPR_NOBINOPR && priority[op].left > limit) {
    expdesc v2;
    BinOpr nextop;
    luaX_next(ls);
    luaK_infix(ls->fs, op, v);
    /* read sub-expression with higher priority */
    nextop = subexpr(ls, &v2, priority[op].right);
    luaK_posfix(ls->fs, op, v, &v2);
    op = nextop;
  }
  leavelevel(ls);
  return op;  /* return first untreated operator */
}


static void expr (LexState *ls, expdesc *v) {
  subexpr(ls, v, 0);
}

/* }==================================================================== */



/*
** {======================================================================
** Rules for Statements
** =======================================================================
*/


static int block_follow (int token) {
  switch (token) {
    // case TK_ELSE: case TK_ELSEIF: case TK_END:
    // case TK_UNTIL: case TK_EOS:
    case ')': case TK_EOS:
      return 1;
    default: return 0;
  }
}

// If the next token cannot be the start of a break/return expression
static int return_break_follow (int token) {
  switch(token) {
    case ')': case TK_EOS: // end of block
    case TK_BREAK: case TK_RETURN: // prevent chaining returns/breaks unecessarily
      return 1;
    default:
      return 0;
  }
}


static void block (LexState *ls, expdesc *e) {
  /* block -> chunk */
  FuncState *fs = ls->fs;
  BlockCnt bl;
  enterblock(fs, &bl, 0);
  chunk(ls);
  inlua_assert(bl.breaklist == NO_JUMP);
  leaveblock(fs, e);
}


/*
** structure to chain all variables in the left-hand side of an
** assignment
*/
struct LHS_assign {
  struct LHS_assign *prev;
  expdesc v;  /* variable (global, local, upvalue, or indexed) */
};


/*
** check whether, in an assignment to a local variable, the local variable
** is needed in a previous assignment (to a table). If so, save original
** local value in a safe place and use this safe copy in the previous
** assignment.
*/
static void check_conflict (LexState *ls, struct LHS_assign *lh, expdesc *v) {
  FuncState *fs = ls->fs;
  int extra = fs->freereg;  /* eventual position to save local variable */
  int conflict = 0;
  for (; lh; lh = lh->prev) {
    if (lh->v.k == VINDEXED) {
      if (lh->v.u.s.info == v->u.s.info) {  /* conflict? */
        conflict = 1;
        lh->v.u.s.info = extra;  /* previous assignment will use safe copy */
      }
      if (lh->v.u.s.aux == v->u.s.info) {  /* conflict? */
        conflict = 1;
        lh->v.u.s.aux = extra;  /* previous assignment will use safe copy */
      }
    }
  }
  if (conflict) {
    luaK_codeABC(fs, OP_MOVE, fs->freereg, v->u.s.info, 0);  /* make copy */
    luaK_reserveregs(fs, 1);
  }
}

static void abort_assignment_aux(FuncState *fs, struct LHS_assign *lh) {
  int expectreg, actualreg;
  fs->freereg -= 1;
  if (lh == NULL) {
    return;
  }
  abort_assignment_aux(fs, lh->prev);

  // printf("abort expkind %d at freereg %d\n", lh->v.k, fs->freereg);
  expectreg = fs->freereg;
  actualreg = luaK_exp2anyreg(fs, &lh->v);
  if (expectreg != actualreg) {
    // printf("mismatch! expected %d, got %d\n", expectreg, actualreg);
    luaK_codeABC(fs, OP_MOVE, expectreg, actualreg, 0);
  }
  fs->freereg = expectreg;
  luaK_reserveregs(fs, 1);
  // printf("after abort %d freereg=%d\n", lh->v.k, fs->freereg);
}

// Returns if the assignment is aborted (values left on stack as temporaries)
static int assignment (LexState *ls, struct LHS_assign *lh, int nvars) {
  expdesc e;
  FuncState *fs = ls->fs;
  // check_condition(ls, VLOCAL <= lh->v.k && lh->v.k <= VINDEXED,
  //                     "syntax error");
  
  luaK_reserveregs(fs, 1);
  
  if (testnext(ls, ',')) {  /* assignment -> `,' primaryexp assignment */
    // int prev_freereg, prev_nactvar;
    struct LHS_assign nv;
    nv.prev = lh;
    // printf("freereg before inner exp: %d\n", fs->freereg);
    expr(ls, &nv.v);
    if (nv.v.k == VCALL) {
      // calls increase freereg by 1 extra (for some reason)
      fs->freereg--;
    }
    // printf("freereg after inner exp: %d\n", fs->freereg);
    if (nv.v.k == VLOCAL)
      check_conflict(ls, lh, &nv.v);
    luaY_checklimit(fs, nvars, INLUAI_MAXCCALLS - ls->L->nCcalls,
                    "variables in assignment");
    if (assignment(ls, &nv, nvars+1)) return 1;
  }
  else if (ls->t.token == '=') {  /* assignment -> `=' explist1 */
    int nexps;

    fs->freereg -= nvars; // allow overwriting temporary expressions on stack
    // printf("starting assignment: freereg=%d\n", fs->freereg);
    luaX_next(ls);
    nexps = explist1(ls, &e);
    // printf("after explist: freereg=%d\n", fs->freereg);
    if (nexps != nvars || e.k == VBLOCK) {
      adjust_assign(ls, nvars, nexps, &e);
      if (nexps > nvars)
        fs->freereg -= nexps - nvars;  /* remove extra values */
    } else {
      luaK_setoneret(fs, &e);  /* close last expression */
      switch(lh->v.k) {
        case VLOCAL: case VUPVAL: case VGLOBAL: case VINDEXED:
          luaK_storevar(fs, &lh->v, &e);
          break;
        default:
          luaX_syntaxerror(ls, "invalid assignment");
          break;
      }
      return 0;  /* avoid default */
    }
  } else {
    // is not actually assigning!
    // discharge all variables in order of appearance.
    abort_assignment_aux(fs, lh->prev);
    // printf("final abort expkind %d at freereg %d\n", lh->v.k, fs->freereg);
    if (lh->v.k == VBLOCK) {
      luaK_blockresults2regs(fs, &lh->v);
    } else {
      int expectreg = fs->freereg;
      int actualreg = luaK_exp2anyreg(fs, &lh->v);
      if (expectreg != actualreg) {
        // printf("mismatch! expected %d, got %d\n", expectreg, actualreg);
        luaK_codeABC(fs, OP_MOVE, expectreg, actualreg, 0);
      }
      fs->freereg = expectreg;
      luaK_reserveregs(fs, 1);
      luaK_exp2nextreg(fs, &lh->v);
    }
    return 1;
  }
  // printf("next assignment: freereg=%d\n", ls->fs->freereg);
  // ls->fs->freereg--; // free register reserved at start
  
  init_exp(&e, VNONRELOC, ls->fs->freereg-1);  /* default assignment */
  switch(lh->v.k) {
    case VLOCAL: case VUPVAL: case VGLOBAL: case VINDEXED:
      luaK_storevar(ls->fs, &lh->v, &e);
      break;
    default:
      luaX_syntaxerror(ls, "invalid assignment");
      break;
  }
  return 0;
}


static int cond (LexState *ls) {
  /* cond -> exp */
  expdesc v;
  expr(ls, &v);  /* read condition */
  if (v.k == VNIL) v.k = VFALSE;  /* `falses' are all equal here */
  luaK_goiftrue(ls->fs, &v);
  return v.f;
}


static void breakstat (LexState *ls) {
  /* stat -> BREAK exp */
  FuncState *fs = ls->fs;
  BlockCnt *bl = fs->bl;
  int upval = 0;
  
  // load optional returned value into breakable block's first temporary register
  if (return_break_follow(ls->t.token) || ls->t.token == ';') {
    while (bl && !bl->isbreakable) {
      upval |= bl->upval;
      bl = bl->previous;
    }
    if (!bl)
      luaX_syntaxerror(ls, "no loop to break");
    // close now so the nil value does not overwrite an upvalue-captured local variable
    if (upval)
      luaK_codeABC(fs, OP_CLOSE, bl->freereg, 0, 0);
    luaK_nil(fs, bl->freereg, 1);
  } else {
    expdesc r; // returned expression
    expr(ls, &r);
    luaK_exp2anyreg(fs, &r); // r becomes a NONRELOC, info is its register
    // In the case where the expression is a function literal, it may capture upvalues itself, so only check for upvalues after.
    while (bl && !bl->isbreakable) {
      upval |= bl->upval;
      bl = bl->previous;
    }
    if (!bl)
    luaX_syntaxerror(ls, "no loop to break");
    // close after return expression is parsed, but before returned expression is moved,
    // so it does not overwrite an upvalue-captured local variable.
    if (upval)
      luaK_codeABC(fs, OP_CLOSE, bl->freereg, 0, 0);
    if (r.u.s.info != bl->freereg) { // move result to return register if not already there
      luaK_codeABC(fs, OP_MOVE, bl->freereg, r.u.s.info, 0);
    }
  }

  luaK_concat(fs, &bl->breaklist, luaK_jump(fs));
}


static void whileexpr (LexState *ls, int line, expdesc *e) {
  /* whileexpr -> ? cond -> (block) */
  FuncState *fs = ls->fs;
  int whileinit;
  int condexit;
  BlockCnt bl;
  luaX_next(ls);  /* skip '?' */
  whileinit = luaK_getlabel(fs);
  condexit = cond(ls);
  enterblock(fs, &bl, 1);
  checknext(ls, TK_ARROW);
  checknext(ls, '(');
  block(ls, NULL);
  luaK_patchlist(fs, luaK_jump(fs), whileinit);
  // check_match(ls, TK_END, TK_WHILE, line);
  check_match(ls, ')', '(', line);
  luaK_patchtohere(fs, condexit);  /* false conditions finish the loop */
  if (e != NULL) luaK_nil(fs, bl.freereg, 1); // load default nil return value
  leaveblock(fs, e); // all breaks will patch to here, after the loadnil
}


// static void repeatstat (LexState *ls, int line) {
//   /* repeatstat -> REPEAT block UNTIL cond */
//   int condexit;
//   FuncState *fs = ls->fs;
//   int repeat_init = luaK_getlabel(fs);
//   BlockCnt bl1, bl2;
//   enterblock(fs, &bl1, 1);  /* loop block */
//   enterblock(fs, &bl2, 0);  /* scope block */
//   luaX_next(ls);  /* skip REPEAT */
//   chunk(ls);
//   check_match(ls, TK_UNTIL, TK_REPEAT, line);
//   condexit = cond(ls);  /* read condition (inside scope block) */
//   if (!bl2.upval) {  /* no upvalues? */
//     leaveblock(fs, 0);  /* finish scope */
//     luaK_patchlist(ls->fs, condexit, repeat_init);  /* close the loop */
//   }
//   else {  /* complete semantics when there are upvalues */
//     breakstat(ls);  /* if condition then break */
//     luaK_patchtohere(ls->fs, condexit);  /* else... */
//     leaveblock(fs, 0);  /* finish scope... */
//     luaK_patchlist(ls->fs, luaK_jump(fs), repeat_init);  /* and repeat */
//   }
//   leaveblock(fs, 0);  /* finish loop */
// }


static int exp1 (LexState *ls) {
  expdesc e;
  int k;
  expr(ls, &e);
  k = e.k;
  luaK_exp2nextreg(ls->fs, &e);
  return k;
}


static void forbody (LexState *ls, int base, int line, int nvars, int isnum) {
  /* forbody: -> (block) */
  BlockCnt bl;
  FuncState *fs = ls->fs;
  int prep, endfor;
  adjustlocalvars(ls, 3);  /* control variables */
  checknext(ls, TK_ARROW);
  prep = isnum ? luaK_codeAsBx(fs, OP_FORPREP, base, NO_JUMP) : luaK_jump(fs);
  enterblock(fs, &bl, 0);  /* scope for declared variables */
  adjustlocalvars(ls, nvars);
  luaK_reserveregs(fs, nvars);
  checknext(ls, '(');
  block(ls, NULL);
  leaveblock(fs, NULL);  /* end of scope for declared variables */
  check_match(ls, ')', '(', line);
  luaK_patchtohere(fs, prep);
  endfor = (isnum) ? luaK_codeAsBx(fs, OP_FORLOOP, base, NO_JUMP) :
                     luaK_codeABC(fs, OP_TFORLOOP, base, 0, nvars);
  luaK_fixline(fs, line);  /* pretend that `OP_FOR' starts the loop */
  luaK_patchlist(fs, (isnum ? endfor : luaK_jump(fs)), prep + 1);
}


static void fornum (LexState *ls, int line) {
  /* fornum -> NAME = exp1,exp1[,exp1] forbody */
  FuncState *fs = ls->fs;
  int base = fs->freereg;
  new_localvarliteral(ls, "(for index)", 0);
  new_localvarliteral(ls, "(for limit)", 1);
  new_localvarliteral(ls, "(for step)", 2);
  new_localvar(ls, str_checkname(ls), 3);
  checknext(ls, '=');
  exp1(ls);  /* initial value */
  checknext(ls, ',');
  exp1(ls);  /* limit */
  if (testnext(ls, ','))
    exp1(ls);  /* optional step */
  else {  /* default step = 1 */
    luaK_codeABx(fs, OP_LOADK, fs->freereg, luaK_numberK(fs, 1));
    luaK_reserveregs(fs, 1);
  }
  forbody(ls, base, line, 1, 1);
}


static void forlist (LexState *ls) {
  /* forlist: [varlist] explist1 forbody */
  FuncState *fs = ls->fs;
  expdesc e;
  int nvars = 0;
  int line;
  int base = fs->freereg;
  /* create control variables */
  new_localvarliteral(ls, "(for generator)", nvars++);
  new_localvarliteral(ls, "(for state)", nvars++);
  new_localvarliteral(ls, "(for control)", nvars++);
  /* create declared variables */
  line = ls->linenumber;
  checknext(ls, '[');
  do {
    new_localvar(ls, str_checkname(ls), nvars++);
  } while (testnext(ls, ','));
  check_match(ls, ']', '[', line);
  line = ls->linenumber;
  adjust_assign(ls, 3, explist1(ls, &e), &e);
  luaK_checkstack(fs, 3);  /* extra space to call generator */
  forbody(ls, base, line, nvars - 3, 0);
}


static void forexpr(LexState *ls, int line, expdesc *e) {
  /* forstat: ?? (fornum | forlist) */
  /* fornum: var=start, stop, step -> (block) */
  /* forlist: [varlist] explist -> (block) */
  FuncState *fs = ls->fs;
  BlockCnt bl;
  enterblock(fs, &bl, 1);  /* scope for loop and control variables */
  luaX_next(ls);  /* skip '??' */
  switch (ls->t.token) {
    case TK_NAME: fornum(ls, line); break;
    case '[': forlist(ls); break;
    default: luaX_syntaxerror(ls, INLUA_QL("[varlist]") " or " INLUA_QL("name=start,stop") " expected");
  }
  // check_match(ls, TK_END, TK_FOR, line);
  if (e != NULL) luaK_nil(fs, bl.freereg, 1); // load default nil return value
  leaveblock(fs, e);  /* loop scope (`break' jumps to this point) */
}


// static int test_then_block (LexState *ls) {
//   /* test_then_block -> [IF | ELSEIF] cond THEN block */
//   int condexit;
//   luaX_next(ls);  /* skip IF or ELSEIF */
//   condexit = cond(ls);
//   checknext(ls, TK_THEN);
//   block(ls);  /* `then' part */
//   return condexit;
// }


// static void ifstat (LexState *ls, int line) {
//   /* ifstat -> IF cond THEN block {ELSEIF cond THEN block} [ELSE block] END */
//   FuncState *fs = ls->fs;
//   int flist;
//   int escapelist = NO_JUMP;
//   flist = test_then_block(ls);  /* IF cond THEN block */
//   while (ls->t.token == TK_ELSEIF) {
//     luaK_concat(fs, &escapelist, luaK_jump(fs));
//     luaK_patchtohere(fs, flist);
//     flist = test_then_block(ls);  /* ELSEIF cond THEN block */
//   }
//   if (ls->t.token == TK_ELSE) {
//     luaK_concat(fs, &escapelist, luaK_jump(fs));
//     luaK_patchtohere(fs, flist);
//     luaX_next(ls);  /* skip ELSE (after patch, for correct line info) */
//     block(ls);  /* `else' part */
//   }
//   else
//     luaK_concat(fs, &escapelist, flist);
//   luaK_patchtohere(fs, escapelist);
//   check_match(ls, TK_END, TK_IF, line);
// }


// static void localfunc (LexState *ls) {
//   expdesc v, b;
//   FuncState *fs = ls->fs;
//   new_localvar(ls, str_checkname(ls), 0);
//   init_exp(&v, VLOCAL, fs->freereg);
//   luaK_reserveregs(fs, 1);
//   adjustlocalvars(ls, 1);
//   body(ls, &b, 0, ls->linenumber);
//   luaK_storevar(fs, &v, &b);
//   /* debug information will only see the variable after this point! */
//   getlocvar(fs, fs->nactvar - 1).startpc = fs->pc;
// }

static void localstat (LexState *ls) {
  /* stat -> LOCAL NAME {`,' NAME} [`=' explist1] */
  int nvars = 0;
  int nexps;
  expdesc e;
  do {
    new_localvar(ls, str_checkname(ls), nvars++);
  } while (testnext(ls, ','));
  if (testnext(ls, '='))
    nexps = explist1(ls, &e);
  else {
    e.k = VVOID;
    nexps = 0;
  }
  adjust_assign(ls, nvars, nexps, &e);
  adjustlocalvars(ls, nvars);
  // clear unused temporaries
  ls->fs->freereg = ls->fs->nactvar;
}


// static int funcname (LexState *ls, expdesc *v) {
//   /* funcname -> NAME {field} [`:' NAME] */
//   int needself = 0;
//   singlevar(ls, v);
//   while (ls->t.token == '.')
//     field(ls, v);
//   if (ls->t.token == ':') {
//     needself = 1;
//     field(ls, v);
//   }
//   return needself;
// }


// static void funcstat (LexState *ls, int line) {
//   /* funcstat -> FUNCTION funcname body */
//   int needself;
//   expdesc v, b;
//   luaX_next(ls);  /* skip FUNCTION */
//   needself = funcname(ls, &v);
//   body(ls, &b, needself, line);
//   luaK_storevar(ls->fs, &v, &b);
//   luaK_fixline(ls->fs, line);  /* definition `happens' in the first line */
// }


static void exprstat (LexState *ls) {
  /* stat -> func | assignment */
  
  struct LHS_assign v;
  
  expr(ls, &v.v);
  if (v.v.k == VCALL) {
    // calls increase freereg by 1 extra
    ls->fs->freereg--;
  }
  
  v.prev = NULL;
  if(assignment(ls, &v, 1) == 0) {
    // is an assignment, so clear unused temporaries
    ls->fs->freereg = ls->fs->nactvar;
  }
}


static void retstat (LexState *ls) {
  /* stat -> RETURN explist */
  FuncState *fs = ls->fs;
  expdesc e;
  int first, nret;  /* registers with returned values */
  // luaX_next(ls);  /* RETURN skipped by caller */
  if (return_break_follow(ls->t.token) || ls->t.token == ';')
    first = nret = 0;  /* return no values */
  else {
    nret = explist1(ls, &e);  /* optional return values */
    if (hasmultret(e.k)) {
      luaK_setmultret(fs, &e);
      if (e.k == VCALL && nret == 1) {  /* tail call? */
        SET_OPCODE(getcode(fs,&e), OP_TAILCALL);
        inlua_assert(GETARG_A(getcode(fs,&e)) == fs->nactvar);
      }
      first = fs->nactvar;
      nret = INLUA_MULTRET;  /* return all values */
    }
    else {
      if (e.k == VBLOCK) {
        // do nothing, values already in place
        first = fs->nactvar;
      } else if (nret == 1)  /* only one single value? */
        first = luaK_exp2anyreg(fs, &e);
      else {
        luaK_exp2nextreg(fs, &e);  /* values must go to the `stack' */
        first = fs->nactvar;  /* return all `active' values */
        inlua_assert(nret == fs->freereg - first);
      }
    }
  }
  luaK_ret(fs, first, nret);
}


static int statement (LexState *ls) {
  // int line = ls->linenumber;  /* may be needed for error messages */
  switch (ls->t.token) {
    // case TK_IF: {  /* stat -> ifstat */
    //   ifstat(ls, line);
    //   return 0;
    // }
    case '?': {  /* stat -> whileexpr */
      whileexpr(ls, ls->linenumber, NULL);
      return 0;
    }
    // case TK_DO: {  /* stat -> DO block END */
    // NOTE: block is now an expression
    // case '(': { /* stat -> (block) */
    //   luaX_next(ls);  /* skip DO */
    //   block(ls);
    //   check_match(ls, ')', '(', line);
    //   return 0;
    // }
    case TK_FOR: {  /* stat -> forstat */
      forexpr(ls, ls->linenumber, NULL);
      return 0;
    }
    // case TK_REPEAT: {  /* stat -> repeatstat */
    //   repeatstat(ls, line);
    //   return 0;
    // }
    // NOTE: functions are now always anonymous
    // case TK_FUNCTION: {
    //   funcstat(ls, line);  /* stat -> funcstat */
    //   return 0;
    // }
    // case TK_LOCAL: {
    case '@': {  /* stat -> localstat */
      luaX_next(ls);  /* skip LOCAL */
      // if (testnext(ls, TK_FUNCTION))  /* local function? */`
      //   localfunc(ls);
      // else
      localstat(ls);
      return 0;
    }
    case TK_RETURN: {  /* stat -> retstat */
      luaX_next(ls);  /* skip RETURN */
      retstat(ls);
      return 1;
    }
    case TK_BREAK: {  /* stat -> breakstat */
      luaX_next(ls);  /* skip BREAK */
      breakstat(ls);
      return 1; 
    }
    default: {
      exprstat(ls);
      return 0;  /* to avoid warnings */
    }
  }
}


static void chunk (LexState *ls) {
  /* chunk -> { stat [`;'] } */
  int islast = 0;
  int colon = 1;
  enterlevel(ls);
  while (!islast && !block_follow(ls->t.token)) {
    islast = statement(ls);
    colon = testnext(ls, ';');
    inlua_assert(ls->fs->f->maxstacksize >= ls->fs->freereg &&
               ls->fs->freereg >= ls->fs->nactvar);
    if (islast || block_follow(ls->t.token)) {
      // for the last statement, only clear registers if followed by a semicolon.
      // islast is only true for brea
      if (colon || islast) ls->fs->freereg = ls->fs->nactvar;  /* free registers */
      break;
    }
    ls->fs->freereg = ls->fs->nactvar;  /* free registers */
  }
  leavelevel(ls);
}

/* }====================================================================== */
