/*
** $Id: lstrlib.c,v 1.132.1.5 2010/05/14 15:34:19 roberto Exp $
** Standard library for string operations and pattern-matching
** See Copyright Notice in inlua.h
*/


#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define lstrlib_c
#define INLUA_LIB

#include "inlua.h"

#include "inlauxlib.h"
#include "inlualib.h"


/* macro to `unsign' a character */
#define uchar(c)        ((unsigned char)(c))



static int str_len (inlua_State *L) {
  size_t l;
  inluaL_checklstring(L, 1, &l);
  inlua_pushinteger(L, l);
  return 1;
}


static ptrdiff_t posrelat (ptrdiff_t pos, size_t len) {
  /* relative string position: negative means back from end */
  if (pos < 0) pos += (ptrdiff_t)len + 1;
  return (pos >= 0) ? pos : 0;
}


static int str_sub (inlua_State *L) {
  size_t l;
  const char *s = inluaL_checklstring(L, 1, &l);
  ptrdiff_t start = posrelat(inluaL_checkinteger(L, 2), l);
  ptrdiff_t end = posrelat(inluaL_optinteger(L, 3, -1), l);
  if (start < 1) start = 1;
  if (end > (ptrdiff_t)l) end = (ptrdiff_t)l;
  if (start <= end)
    inlua_pushlstring(L, s+start-1, end-start+1);
  else inlua_pushliteral(L, "");
  return 1;
}

static int str_index (inlua_State *L) {
  size_t l;
  const char *s = inluaL_checklstring(L, 1, &l);
  ptrdiff_t idx = inluaL_checkinteger(L, 2);
  if (idx < 1 || idx > (ptrdiff_t)l) {
    inlua_pushnil(L);
    return 1;
  }
  inlua_pushlstring(L, s+(idx-1), 1);
  return 1;
}

static int str_reverse (inlua_State *L) {
  size_t l;
  inluaL_Buffer b;
  const char *s = inluaL_checklstring(L, 1, &l);
  inluaL_buffinit(L, &b);
  while (l--) inluaL_addchar(&b, s[l]);
  inluaL_pushresult(&b);
  return 1;
}


static int str_lower (inlua_State *L) {
  size_t l;
  size_t i;
  inluaL_Buffer b;
  const char *s = inluaL_checklstring(L, 1, &l);
  inluaL_buffinit(L, &b);
  for (i=0; i<l; i++)
    inluaL_addchar(&b, tolower(uchar(s[i])));
  inluaL_pushresult(&b);
  return 1;
}


static int str_upper (inlua_State *L) {
  size_t l;
  size_t i;
  inluaL_Buffer b;
  const char *s = inluaL_checklstring(L, 1, &l);
  inluaL_buffinit(L, &b);
  for (i=0; i<l; i++)
    inluaL_addchar(&b, toupper(uchar(s[i])));
  inluaL_pushresult(&b);
  return 1;
}

static int str_rep (inlua_State *L) {
  size_t l;
  inluaL_Buffer b;
  const char *s = inluaL_checklstring(L, 1, &l);
  int n = inluaL_checkint(L, 2);
  inluaL_buffinit(L, &b);
  while (n-- > 0)
    inluaL_addlstring(&b, s, l);
  inluaL_pushresult(&b);
  return 1;
}


static int str_byte (inlua_State *L) {
  size_t l;
  const char *s = inluaL_checklstring(L, 1, &l);
  ptrdiff_t posi = posrelat(inluaL_optinteger(L, 2, 1), l);
  ptrdiff_t pose = posrelat(inluaL_optinteger(L, 3, posi), l);
  int n, i;
  if (posi <= 0) posi = 1;
  if ((size_t)pose > l) pose = l;
  if (posi > pose) return 0;  /* empty interval; return no values */
  n = (int)(pose -  posi + 1);
  if (posi + n <= pose)  /* overflow? */
    inluaL_error(L, "string slice too long");
  inluaL_checkstack(L, n, "string slice too long");
  for (i=0; i<n; i++)
    inlua_pushinteger(L, uchar(s[posi+i-1]));
  return n;
}


static int str_char (inlua_State *L) {
  int n = inlua_gettop(L);  /* number of arguments */
  int i;
  inluaL_Buffer b;
  inluaL_buffinit(L, &b);
  for (i=1; i<=n; i++) {
    int c = inluaL_checkint(L, i);
    inluaL_argcheck(L, uchar(c) == c, i, "invalid value");
    inluaL_addchar(&b, uchar(c));
  }
  inluaL_pushresult(&b);
  return 1;
}


static int writer (inlua_State *L, const void* b, size_t size, void* B) {
  (void)L;
  inluaL_addlstring((inluaL_Buffer*) B, (const char *)b, size);
  return 0;
}


static int str_dump (inlua_State *L) {
  inluaL_Buffer b;
  inluaL_checktype(L, 1, INLUA_TFUNCTION);
  inlua_settop(L, 1);
  inluaL_buffinit(L,&b);
  if (inlua_dump(L, writer, &b) != 0)
    inluaL_error(L, "unable to dump given function");
  inluaL_pushresult(&b);
  return 1;
}



/*
** {======================================================
** PATTERN MATCHING
** =======================================================
*/


#define CAP_UNFINISHED	(-1)
#define CAP_POSITION	(-2)

typedef struct MatchState {
  const char *src_init;  /* init of source string */
  const char *src_end;  /* end (`\0') of source string */
  inlua_State *L;
  int level;  /* total number of captures (finished or unfinished) */
  struct {
    const char *init;
    ptrdiff_t len;
  } capture[INLUA_MAXCAPTURES];
} MatchState;


#define L_ESC		'%'
#define SPECIALS	"^$*+?.([%-"


static int check_capture (MatchState *ms, int l) {
  l -= '1';
  if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
    return inluaL_error(ms->L, "invalid capture index");
  return l;
}


static int capture_to_close (MatchState *ms) {
  int level = ms->level;
  for (level--; level>=0; level--)
    if (ms->capture[level].len == CAP_UNFINISHED) return level;
  return inluaL_error(ms->L, "invalid pattern capture");
}


static const char *classend (MatchState *ms, const char *p) {
  switch (*p++) {
    case L_ESC: {
      if (*p == '\0')
        inluaL_error(ms->L, "malformed pattern (ends with " INLUA_QL("%%") ")");
      return p+1;
    }
    case '[': {
      if (*p == '^') p++;
      do {  /* look for a `]' */
        if (*p == '\0')
          inluaL_error(ms->L, "malformed pattern (missing " INLUA_QL("]") ")");
        if (*(p++) == L_ESC && *p != '\0')
          p++;  /* skip escapes (e.g. `%]') */
      } while (*p != ']');
      return p+1;
    }
    default: {
      return p;
    }
  }
}


static int match_class (int c, int cl) {
  int res;
  switch (tolower(cl)) {
    case 'a' : res = isalpha(c); break;
    case 'c' : res = iscntrl(c); break;
    case 'd' : res = isdigit(c); break;
    case 'l' : res = islower(c); break;
    case 'p' : res = ispunct(c); break;
    case 's' : res = isspace(c); break;
    case 'u' : res = isupper(c); break;
    case 'w' : res = isalnum(c); break;
    case 'x' : res = isxdigit(c); break;
    case 'z' : res = (c == 0); break;
    default: return (cl == c);
  }
  return (islower(cl) ? res : !res);
}


static int matchbracketclass (int c, const char *p, const char *ec) {
  int sig = 1;
  if (*(p+1) == '^') {
    sig = 0;
    p++;  /* skip the `^' */
  }
  while (++p < ec) {
    if (*p == L_ESC) {
      p++;
      if (match_class(c, uchar(*p)))
        return sig;
    }
    else if ((*(p+1) == '-') && (p+2 < ec)) {
      p+=2;
      if (uchar(*(p-2)) <= c && c <= uchar(*p))
        return sig;
    }
    else if (uchar(*p) == c) return sig;
  }
  return !sig;
}


static int singlematch (int c, const char *p, const char *ep) {
  switch (*p) {
    case '.': return 1;  /* matches any char */
    case L_ESC: return match_class(c, uchar(*(p+1)));
    case '[': return matchbracketclass(c, p, ep-1);
    default:  return (uchar(*p) == c);
  }
}


static const char *match (MatchState *ms, const char *s, const char *p);


static const char *matchbalance (MatchState *ms, const char *s,
                                   const char *p) {
  if (*p == 0 || *(p+1) == 0)
    inluaL_error(ms->L, "unbalanced pattern");
  if (*s != *p) return NULL;
  else {
    int b = *p;
    int e = *(p+1);
    int cont = 1;
    while (++s < ms->src_end) {
      if (*s == e) {
        if (--cont == 0) return s+1;
      }
      else if (*s == b) cont++;
    }
  }
  return NULL;  /* string ends out of balance */
}


static const char *max_expand (MatchState *ms, const char *s,
                                 const char *p, const char *ep) {
  ptrdiff_t i = 0;  /* counts maximum expand for item */
  while ((s+i)<ms->src_end && singlematch(uchar(*(s+i)), p, ep))
    i++;
  /* keeps trying to match with the maximum repetitions */
  while (i>=0) {
    const char *res = match(ms, (s+i), ep+1);
    if (res) return res;
    i--;  /* else didn't match; reduce 1 repetition to try again */
  }
  return NULL;
}


static const char *min_expand (MatchState *ms, const char *s,
                                 const char *p, const char *ep) {
  for (;;) {
    const char *res = match(ms, s, ep+1);
    if (res != NULL)
      return res;
    else if (s<ms->src_end && singlematch(uchar(*s), p, ep))
      s++;  /* try with one more repetition */
    else return NULL;
  }
}


static const char *start_capture (MatchState *ms, const char *s,
                                    const char *p, int what) {
  const char *res;
  int level = ms->level;
  if (level >= INLUA_MAXCAPTURES) inluaL_error(ms->L, "too many captures");
  ms->capture[level].init = s;
  ms->capture[level].len = what;
  ms->level = level+1;
  if ((res=match(ms, s, p)) == NULL)  /* match failed? */
    ms->level--;  /* undo capture */
  return res;
}


static const char *end_capture (MatchState *ms, const char *s,
                                  const char *p) {
  int l = capture_to_close(ms);
  const char *res;
  ms->capture[l].len = s - ms->capture[l].init;  /* close capture */
  if ((res = match(ms, s, p)) == NULL)  /* match failed? */
    ms->capture[l].len = CAP_UNFINISHED;  /* undo capture */
  return res;
}


static const char *match_capture (MatchState *ms, const char *s, int l) {
  size_t len;
  l = check_capture(ms, l);
  len = ms->capture[l].len;
  if ((size_t)(ms->src_end-s) >= len &&
      memcmp(ms->capture[l].init, s, len) == 0)
    return s+len;
  else return NULL;
}


static const char *match (MatchState *ms, const char *s, const char *p) {
  init: /* using goto's to optimize tail recursion */
  switch (*p) {
    case '(': {  /* start capture */
      if (*(p+1) == ')')  /* position capture? */
        return start_capture(ms, s, p+2, CAP_POSITION);
      else
        return start_capture(ms, s, p+1, CAP_UNFINISHED);
    }
    case ')': {  /* end capture */
      return end_capture(ms, s, p+1);
    }
    case L_ESC: {
      switch (*(p+1)) {
        case 'b': {  /* balanced string? */
          s = matchbalance(ms, s, p+2);
          if (s == NULL) return NULL;
          p+=4; goto init;  /* else return match(ms, s, p+4); */
        }
        case 'f': {  /* frontier? */
          const char *ep; char previous;
          p += 2;
          if (*p != '[')
            inluaL_error(ms->L, "missing " INLUA_QL("[") " after "
                               INLUA_QL("%%f") " in pattern");
          ep = classend(ms, p);  /* points to what is next */
          previous = (s == ms->src_init) ? '\0' : *(s-1);
          if (matchbracketclass(uchar(previous), p, ep-1) ||
             !matchbracketclass(uchar(*s), p, ep-1)) return NULL;
          p=ep; goto init;  /* else return match(ms, s, ep); */
        }
        default: {
          if (isdigit(uchar(*(p+1)))) {  /* capture results (%0-%9)? */
            s = match_capture(ms, s, uchar(*(p+1)));
            if (s == NULL) return NULL;
            p+=2; goto init;  /* else return match(ms, s, p+2) */
          }
          goto dflt;  /* case default */
        }
      }
    }
    case '\0': {  /* end of pattern */
      return s;  /* match succeeded */
    }
    case '$': {
      if (*(p+1) == '\0')  /* is the `$' the last char in pattern? */
        return (s == ms->src_end) ? s : NULL;  /* check end of string */
      else goto dflt;
    }
    default: dflt: {  /* it is a pattern item */
      const char *ep = classend(ms, p);  /* points to what is next */
      int m = s<ms->src_end && singlematch(uchar(*s), p, ep);
      switch (*ep) {
        case '?': {  /* optional */
          const char *res;
          if (m && ((res=match(ms, s+1, ep+1)) != NULL))
            return res;
          p=ep+1; goto init;  /* else return match(ms, s, ep+1); */
        }
        case '*': {  /* 0 or more repetitions */
          return max_expand(ms, s, p, ep);
        }
        case '+': {  /* 1 or more repetitions */
          return (m ? max_expand(ms, s+1, p, ep) : NULL);
        }
        case '-': {  /* 0 or more repetitions (minimum) */
          return min_expand(ms, s, p, ep);
        }
        default: {
          if (!m) return NULL;
          s++; p=ep; goto init;  /* else return match(ms, s+1, ep); */
        }
      }
    }
  }
}



static const char *lmemfind (const char *s1, size_t l1,
                               const char *s2, size_t l2) {
  if (l2 == 0) return s1;  /* empty strings are everywhere */
  else if (l2 > l1) return NULL;  /* avoids a negative `l1' */
  else {
    const char *init;  /* to search for a `*s2' inside `s1' */
    l2--;  /* 1st char will be checked by `memchr' */
    l1 = l1-l2;  /* `s2' cannot be found after that */
    while (l1 > 0 && (init = (const char *)memchr(s1, *s2, l1)) != NULL) {
      init++;   /* 1st char is already checked */
      if (memcmp(init, s2+1, l2) == 0)
        return init-1;
      else {  /* correct `l1' and `s1' to try again */
        l1 -= init-s1;
        s1 = init;
      }
    }
    return NULL;  /* not found */
  }
}


static void push_onecapture (MatchState *ms, int i, const char *s,
                                                    const char *e) {
  if (i >= ms->level) {
    if (i == 0)  /* ms->level == 0, too */
      inlua_pushlstring(ms->L, s, e - s);  /* add whole match */
    else
      inluaL_error(ms->L, "invalid capture index");
  }
  else {
    ptrdiff_t l = ms->capture[i].len;
    if (l == CAP_UNFINISHED) inluaL_error(ms->L, "unfinished capture");
    if (l == CAP_POSITION)
      inlua_pushinteger(ms->L, ms->capture[i].init - ms->src_init + 1);
    else
      inlua_pushlstring(ms->L, ms->capture[i].init, l);
  }
}


static int push_captures (MatchState *ms, const char *s, const char *e) {
  int i;
  int nlevels = (ms->level == 0 && s) ? 1 : ms->level;
  inluaL_checkstack(ms->L, nlevels, "too many captures");
  for (i = 0; i < nlevels; i++)
    push_onecapture(ms, i, s, e);
  return nlevels;  /* number of strings pushed */
}


static int str_find_aux (inlua_State *L, int find) {
  size_t l1, l2;
  const char *s = inluaL_checklstring(L, 1, &l1);
  const char *p = inluaL_checklstring(L, 2, &l2);
  ptrdiff_t init = posrelat(inluaL_optinteger(L, 3, 1), l1) - 1;
  if (init < 0) init = 0;
  else if ((size_t)(init) > l1) init = (ptrdiff_t)l1;
  if (find && (inlua_toboolean(L, 4) ||  /* explicit request? */
      strpbrk(p, SPECIALS) == NULL)) {  /* or no special characters? */
    /* do a plain search */
    const char *s2 = lmemfind(s+init, l1-init, p, l2);
    if (s2) {
      inlua_pushinteger(L, s2-s+1);
      inlua_pushinteger(L, s2-s+l2);
      return 2;
    }
  }
  else {
    MatchState ms;
    int anchor = (*p == '^') ? (p++, 1) : 0;
    const char *s1=s+init;
    ms.L = L;
    ms.src_init = s;
    ms.src_end = s+l1;
    do {
      const char *res;
      ms.level = 0;
      if ((res=match(&ms, s1, p)) != NULL) {
        if (find) {
          inlua_pushinteger(L, s1-s+1);  /* start */
          inlua_pushinteger(L, res-s);   /* end */
          return push_captures(&ms, NULL, 0) + 2;
        }
        else
          return push_captures(&ms, s1, res);
      }
    } while (s1++ < ms.src_end && !anchor);
  }
  inlua_pushnil(L);  /* not found */
  return 1;
}


static int str_find (inlua_State *L) {
  return str_find_aux(L, 1);
}


static int str_match (inlua_State *L) {
  return str_find_aux(L, 0);
}


static int gmatch_aux (inlua_State *L) {
  MatchState ms;
  size_t ls;
  const char *s = inlua_tolstring(L, inlua_upvalueindex(1), &ls);
  const char *p = inlua_tostring(L, inlua_upvalueindex(2));
  const char *src;
  ms.L = L;
  ms.src_init = s;
  ms.src_end = s+ls;
  for (src = s + (size_t)inlua_tointeger(L, inlua_upvalueindex(3));
       src <= ms.src_end;
       src++) {
    const char *e;
    ms.level = 0;
    if ((e = match(&ms, src, p)) != NULL) {
      inlua_Integer newstart = e-s;
      if (e == src) newstart++;  /* empty match? go at least one position */
      inlua_pushinteger(L, newstart);
      inlua_replace(L, inlua_upvalueindex(3));
      return push_captures(&ms, src, e);
    }
  }
  return 0;  /* not found */
}


static int gmatch (inlua_State *L) {
  inluaL_checkstring(L, 1);
  inluaL_checkstring(L, 2);
  inlua_settop(L, 2);
  inlua_pushinteger(L, 0);
  inlua_pushcclosure(L, gmatch_aux, 3);
  return 1;
}


static int gfind_nodef (inlua_State *L) {
  return inluaL_error(L, INLUA_QL("string.gfind") " was renamed to "
                       INLUA_QL("string.gmatch"));
}


static void add_s (MatchState *ms, inluaL_Buffer *b, const char *s,
                                                   const char *e) {
  size_t l, i;
  const char *news = inlua_tolstring(ms->L, 3, &l);
  for (i = 0; i < l; i++) {
    if (news[i] != L_ESC)
      inluaL_addchar(b, news[i]);
    else {
      i++;  /* skip ESC */
      if (!isdigit(uchar(news[i])))
        inluaL_addchar(b, news[i]);
      else if (news[i] == '0')
          inluaL_addlstring(b, s, e - s);
      else {
        push_onecapture(ms, news[i] - '1', s, e);
        inluaL_addvalue(b);  /* add capture to accumulated result */
      }
    }
  }
}


static void add_value (MatchState *ms, inluaL_Buffer *b, const char *s,
                                                       const char *e) {
  inlua_State *L = ms->L;
  switch (inlua_type(L, 3)) {
    case INLUA_TNUMBER:
    case INLUA_TSTRING: {
      add_s(ms, b, s, e);
      return;
    }
    case INLUA_TFUNCTION: {
      int n;
      inlua_pushvalue(L, 3);
      n = push_captures(ms, s, e);
      inlua_call(L, n, 1);
      break;
    }
    case INLUA_TTABLE: {
      push_onecapture(ms, 0, s, e);
      inlua_gettable(L, 3);
      break;
    }
  }
  if (!inlua_toboolean(L, -1)) {  /* nil or false? */
    inlua_pop(L, 1);
    inlua_pushlstring(L, s, e - s);  /* keep original text */
  }
  else if (!inlua_isstring(L, -1))
    inluaL_error(L, "invalid replacement value (a %s)", inluaL_typename(L, -1)); 
  inluaL_addvalue(b);  /* add result to accumulator */
}


static int str_gsub (inlua_State *L) {
  size_t srcl;
  const char *src = inluaL_checklstring(L, 1, &srcl);
  const char *p = inluaL_checkstring(L, 2);
  int  tr = inlua_type(L, 3);
  int max_s = inluaL_optint(L, 4, srcl+1);
  int anchor = (*p == '^') ? (p++, 1) : 0;
  int n = 0;
  MatchState ms;
  inluaL_Buffer b;
  inluaL_argcheck(L, tr == INLUA_TNUMBER || tr == INLUA_TSTRING ||
                   tr == INLUA_TFUNCTION || tr == INLUA_TTABLE, 3,
                      "string/function/table expected");
  inluaL_buffinit(L, &b);
  ms.L = L;
  ms.src_init = src;
  ms.src_end = src+srcl;
  while (n < max_s) {
    const char *e;
    ms.level = 0;
    e = match(&ms, src, p);
    if (e) {
      n++;
      add_value(&ms, &b, src, e);
    }
    if (e && e>src) /* non empty match? */
      src = e;  /* skip it */
    else if (src < ms.src_end)
      inluaL_addchar(&b, *src++);
    else break;
    if (anchor) break;
  }
  inluaL_addlstring(&b, src, ms.src_end-src);
  inluaL_pushresult(&b);
  inlua_pushinteger(L, n);  /* number of substitutions */
  return 2;
}

/* }====================================================== */


/* maximum size of each formatted item (> len(format('%99.99f', -1e308))) */
#define MAX_ITEM	512
/* valid flags in a format specification */
#define FLAGS	"-+ #0"
/*
** maximum size of each format specification (such as '%-099.99d')
** (+10 accounts for %99.99x plus margin of error)
*/
#define MAX_FORMAT	(sizeof(FLAGS) + sizeof(INLUA_INTFRMLEN) + 10)


static void addquoted (inlua_State *L, inluaL_Buffer *b, int arg) {
  size_t l;
  const char *s = inluaL_checklstring(L, arg, &l);
  inluaL_addchar(b, '"');
  while (l--) {
    switch (*s) {
      case '"': case '\\': case '\n': {
        inluaL_addchar(b, '\\');
        inluaL_addchar(b, *s);
        break;
      }
      case '\r': {
        inluaL_addlstring(b, "\\r", 2);
        break;
      }
      case '\0': {
        inluaL_addlstring(b, "\\000", 4);
        break;
      }
      default: {
        inluaL_addchar(b, *s);
        break;
      }
    }
    s++;
  }
  inluaL_addchar(b, '"');
}

static const char *scanformat (inlua_State *L, const char *strfrmt, char *form) {
  const char *p = strfrmt;
  while (*p != '\0' && strchr(FLAGS, *p) != NULL) p++;  /* skip flags */
  if ((size_t)(p - strfrmt) >= sizeof(FLAGS))
    inluaL_error(L, "invalid format (repeated flags)");
  if (isdigit(uchar(*p))) p++;  /* skip width */
  if (isdigit(uchar(*p))) p++;  /* (2 digits at most) */
  if (*p == '.') {
    p++;
    if (isdigit(uchar(*p))) p++;  /* skip precision */
    if (isdigit(uchar(*p))) p++;  /* (2 digits at most) */
  }
  if (isdigit(uchar(*p)))
    inluaL_error(L, "invalid format (width or precision too long)");
  *(form++) = '%';
  strncpy(form, strfrmt, p - strfrmt + 1);
  form += p - strfrmt + 1;
  *form = '\0';
  return p;
}


static void addintlen (char *form) {
  size_t l = strlen(form);
  char spec = form[l - 1];
  strcpy(form + l - 1, INLUA_INTFRMLEN);
  form[l + sizeof(INLUA_INTFRMLEN) - 2] = spec;
  form[l + sizeof(INLUA_INTFRMLEN) - 1] = '\0';
}


static int str_format (inlua_State *L) {
  int top = inlua_gettop(L);
  int arg = 1;
  size_t sfl;
  const char *strfrmt = inluaL_checklstring(L, arg, &sfl);
  const char *strfrmt_end = strfrmt+sfl;
  inluaL_Buffer b;
  inluaL_buffinit(L, &b);
  while (strfrmt < strfrmt_end) {
    if (*strfrmt != L_ESC)
      inluaL_addchar(&b, *strfrmt++);
    else if (*++strfrmt == L_ESC)
      inluaL_addchar(&b, *strfrmt++);  /* %% */
    else { /* format item */
      char form[MAX_FORMAT];  /* to store the format (`%...') */
      char buff[MAX_ITEM];  /* to store the formatted item */
      if (++arg > top)
        inluaL_argerror(L, arg, "no value");
      strfrmt = scanformat(L, strfrmt, form);
      switch (*strfrmt++) {
        case 'c': {
          sprintf(buff, form, (int)inluaL_checknumber(L, arg));
          break;
        }
        case 'd':  case 'i': {
          addintlen(form);
          sprintf(buff, form, (INLUA_INTFRM_T)inluaL_checknumber(L, arg));
          break;
        }
        case 'o':  case 'u':  case 'x':  case 'X': {
          addintlen(form);
          sprintf(buff, form, (unsigned INLUA_INTFRM_T)inluaL_checknumber(L, arg));
          break;
        }
        case 'e':  case 'E': case 'f':
        case 'g': case 'G': {
          sprintf(buff, form, (double)inluaL_checknumber(L, arg));
          break;
        }
        case 'q': {
          addquoted(L, &b, arg);
          continue;  /* skip the 'addsize' at the end */
        }
        case 's': {
          size_t l;
          const char *s = inluaL_checklstring(L, arg, &l);
          if (!strchr(form, '.') && l >= 100) {
            /* no precision and string is too long to be formatted;
               keep original string */
            inlua_pushvalue(L, arg);
            inluaL_addvalue(&b);
            continue;  /* skip the `addsize' at the end */
          }
          else {
            sprintf(buff, form, s);
            break;
          }
        }
        default: {  /* also treat cases `pnLlh' */
          return inluaL_error(L, "invalid option " INLUA_QL("%%%c") " to "
                               INLUA_QL("format"), *(strfrmt - 1));
        }
      }
      inluaL_addlstring(&b, buff, strlen(buff));
    }
  }
  inluaL_pushresult(&b);
  return 1;
}


static int str_meta_index (inlua_State *L) {
  if (inlua_isnumber(L, 2))
    return str_index(L);
  inlua_getmetatable(L, 1); // get string library (a string's metatable)
  inlua_pushvalue(L, 2); // push key
  inlua_rawget(L, -2); // rawget(string library table, key)
  return 1; // return result of rawget
}


static const inluaL_Reg strlib[] = {
  {"byte", str_byte},
  {"char", str_char},
  {"dump", str_dump},
  {"find", str_find},
  {"format", str_format},
  {"gfind", gfind_nodef},
  {"gmatch", gmatch},
  {"gsub", str_gsub},
  {"len", str_len},
  {"lower", str_lower},
  {"match", str_match},
  {"rep", str_rep},
  {"reverse", str_reverse},
  {"sub", str_sub},
  {"upper", str_upper},
  {"at", str_index},
  {NULL, NULL}
};


static void registermetatable (inlua_State *L) {
  // inlua_createtable(L, 0, 1);  /* create metatable for strings */
  // metatable for strings is now the string library
  inlua_pushliteral(L, "");  /* dummy string */
  inlua_pushvalue(L, -2); /* copy of the string library table */
  inlua_setmetatable(L, -2);  /* set string metatable */
  inlua_pop(L, 1);  /* pop dummy string */
  // inlua_pushvalue(L, -2);  /* string library... */
  // inlua_setfield(L, -2, "__lib");  /* ...is stored in metatable for reference */
  inlua_pushcfunction(L, str_meta_index);
  inlua_setfield(L, -2, "__index"); /* set __index metamethod of string library */
  // inlua_pop(L, 1);  /* pop metatable */
}


/*
** Open string library
*/
INLUALIB_API int inluaopen_string (inlua_State *L) {
  inluaL_register(L, INLUA_STRLIBNAME, strlib);
#if defined(INLUA_COMPAT_GFIND)
  inlua_getfield(L, -1, "gmatch");
  inlua_setfield(L, -2, "gfind");
#endif
  registermetatable(L);
  return 1;
}

