/*
** $Id: llex.h,v 1.58.1.1 2007/12/27 13:02:25 roberto Exp $
** Lexical Analyzer
** See Copyright Notice in inlua.h
*/

#ifndef llex_h
#define llex_h

#include "lobject.h"
#include "lzio.h"


#define FIRST_RESERVED	257

/* maximum length of a reserved word */
#define TOKEN_LEN	(sizeof("function")/sizeof(char))


/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER RESERVED"
*/
enum RESERVED {
  /* terminal symbols denoted by reserved words */
  // TK_AND = FIRST_RESERVED, 
  TK_BREAK = FIRST_RESERVED,
  // TK_DO, 
  // TK_ELSE, 
  // TK_ELSEIF, 
  // TK_END, 
  // TK_FALSE, 
  TK_FOR, 
  // TK_FUNCTION,
  // TK_IF, 
  // TK_IN, 
  // TK_LOCAL, 
  // TK_NIL, 
  // TK_NOT, 
  // TK_OR, 
  // TK_REPEAT,
  TK_RETURN, 
  // TK_THEN, 
  // TK_TRUE, 
  // TK_UNTIL, 
  // TK_WHILE,
  /* other terminal symbols */
  TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE, TK_ARROW,
  TK_NUMBER, TK_NAME, TK_STRING, TK_EOS
};

/* number of reserved words */
// inline has no reserved words, all special syntax is symbolic
#define NUM_RESERVED (0) // (cast(int, TK_WHILE-FIRST_RESERVED+1))


/* array with token `names' */
INLUAI_DATA const char *const luaX_tokens [];


typedef union {
  inlua_Number r;
  TString *ts;
} SemInfo;  /* semantics information */


typedef struct Token {
  int token;
  SemInfo seminfo;
} Token;


typedef struct LexState {
  int current;  /* current character (charint) */
  int linenumber;  /* input line counter */
  int lastline;  /* line of last token `consumed' */
  Token t;  /* current token */
  Token lookahead;  /* look ahead token */
  struct FuncState *fs;  /* `FuncState' is private to the parser */
  struct inlua_State *L;
  ZIO *z;  /* input stream */
  Mbuffer *buff;  /* buffer for tokens */
  TString *source;  /* current source name */
  char decpoint;  /* locale decimal point */
} LexState;


INLUAI_FUNC void luaX_init (inlua_State *L);
INLUAI_FUNC void luaX_setinput (inlua_State *L, LexState *ls, ZIO *z,
                              TString *source);
INLUAI_FUNC TString *luaX_newstring (LexState *ls, const char *str, size_t l);
INLUAI_FUNC void luaX_next (LexState *ls);
INLUAI_FUNC void luaX_lookahead (LexState *ls);
INLUAI_FUNC void luaX_lexerror (LexState *ls, const char *msg, int token);
INLUAI_FUNC void luaX_syntaxerror (LexState *ls, const char *s);
INLUAI_FUNC const char *luaX_token2str (LexState *ls, int token);


#endif
