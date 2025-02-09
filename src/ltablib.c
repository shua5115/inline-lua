/*
** $Id: ltablib.c,v 1.38.1.3 2008/02/14 16:46:58 roberto Exp $
** Library for Table Manipulation
** See Copyright Notice in inlua.h
*/


#include <stddef.h>

#define ltablib_c
#define INLUA_LIB

#include "inlua.h"

#include "inlauxlib.h"
#include "inlualib.h"


#define aux_getn(L,n)	(inluaL_checktype(L, n, INLUA_TTABLE), inluaL_getn(L, n))


static int foreachi (inlua_State *L) {
  int i;
  int n = aux_getn(L, 1);
  inluaL_checktype(L, 2, INLUA_TFUNCTION);
  for (i=1; i <= n; i++) {
    inlua_pushvalue(L, 2);  /* function */
    inlua_pushinteger(L, i);  /* 1st argument */
    inlua_rawgeti(L, 1, i);  /* 2nd argument */
    inlua_call(L, 2, 1);
    if (!inlua_isnil(L, -1))
      return 1;
    inlua_pop(L, 1);  /* remove nil result */
  }
  return 0;
}


static int foreach (inlua_State *L) {
  inluaL_checktype(L, 1, INLUA_TTABLE);
  inluaL_checktype(L, 2, INLUA_TFUNCTION);
  inlua_pushnil(L);  /* first key */
  while (inlua_next(L, 1)) {
    inlua_pushvalue(L, 2);  /* function */
    inlua_pushvalue(L, -3);  /* key */
    inlua_pushvalue(L, -3);  /* value */
    inlua_call(L, 2, 1);
    if (!inlua_isnil(L, -1))
      return 1;
    inlua_pop(L, 2);  /* remove value and result */
  }
  return 0;
}


static int maxn (inlua_State *L) {
  inlua_Number max = 0;
  inluaL_checktype(L, 1, INLUA_TTABLE);
  inlua_pushnil(L);  /* first key */
  while (inlua_next(L, 1)) {
    inlua_pop(L, 1);  /* remove value */
    if (inlua_type(L, -1) == INLUA_TNUMBER) {
      inlua_Number v = inlua_tonumber(L, -1);
      if (v > max) max = v;
    }
  }
  inlua_pushnumber(L, max);
  return 1;
}


static int getn (inlua_State *L) {
  inlua_pushinteger(L, aux_getn(L, 1));
  return 1;
}


static int setn (inlua_State *L) {
  inluaL_checktype(L, 1, INLUA_TTABLE);
#ifndef inluaL_setn
  inluaL_setn(L, 1, inluaL_checkint(L, 2));
#else
  inluaL_error(L, INLUA_QL("setn") " is obsolete");
#endif
  inlua_pushvalue(L, 1);
  return 1;
}


static int tinsert (inlua_State *L) {
  int e = aux_getn(L, 1) + 1;  /* first empty element */
  int pos;  /* where to insert new element */
  switch (inlua_gettop(L)) {
    case 2: {  /* called with only 2 arguments */
      pos = e;  /* insert new element at the end */
      break;
    }
    case 3: {
      int i;
      pos = inluaL_checkint(L, 2);  /* 2nd argument is the position */
      if (pos > e) e = pos;  /* `grow' array if necessary */
      for (i = e; i > pos; i--) {  /* move up elements */
        inlua_rawgeti(L, 1, i-1);
        inlua_rawseti(L, 1, i);  /* t[i] = t[i-1] */
      }
      break;
    }
    default: {
      return inluaL_error(L, "wrong number of arguments to " INLUA_QL("insert"));
    }
  }
  inluaL_setn(L, 1, e);  /* new size */
  inlua_rawseti(L, 1, pos);  /* t[pos] = v */
  return 0;
}


static int tremove (inlua_State *L) {
  int e = aux_getn(L, 1);
  int pos = inluaL_optint(L, 2, e);
  if (!(1 <= pos && pos <= e))  /* position is outside bounds? */
   return 0;  /* nothing to remove */
  inluaL_setn(L, 1, e - 1);  /* t.n = n-1 */
  inlua_rawgeti(L, 1, pos);  /* result = t[pos] */
  for ( ;pos<e; pos++) {
    inlua_rawgeti(L, 1, pos+1);
    inlua_rawseti(L, 1, pos);  /* t[pos] = t[pos+1] */
  }
  inlua_pushnil(L);
  inlua_rawseti(L, 1, e);  /* t[e] = nil */
  return 1;
}


static void addfield (inlua_State *L, inluaL_Buffer *b, int i) {
  inlua_rawgeti(L, 1, i);
  if (!inlua_isstring(L, -1))
    inluaL_error(L, "invalid value (%s) at index %d in table for "
                  INLUA_QL("concat"), inluaL_typename(L, -1), i);
    inluaL_addvalue(b);
}


static int tconcat (inlua_State *L) {
  inluaL_Buffer b;
  size_t lsep;
  int i, last;
  const char *sep = inluaL_optlstring(L, 2, "", &lsep);
  inluaL_checktype(L, 1, INLUA_TTABLE);
  i = inluaL_optint(L, 3, 1);
  last = inluaL_opt(L, inluaL_checkint, 4, inluaL_getn(L, 1));
  inluaL_buffinit(L, &b);
  for (; i < last; i++) {
    addfield(L, &b, i);
    inluaL_addlstring(&b, sep, lsep);
  }
  if (i == last)  /* add last value (if interval was not empty) */
    addfield(L, &b, i);
  inluaL_pushresult(&b);
  return 1;
}



/*
** {======================================================
** Quicksort
** (based on `Algorithms in MODULA-3', Robert Sedgewick;
**  Addison-Wesley, 1993.)
*/


static void set2 (inlua_State *L, int i, int j) {
  inlua_rawseti(L, 1, i);
  inlua_rawseti(L, 1, j);
}

static int sort_comp (inlua_State *L, int a, int b) {
  if (!inlua_isnil(L, 2)) {  /* function? */
    int res;
    inlua_pushvalue(L, 2);
    inlua_pushvalue(L, a-1);  /* -1 to compensate function */
    inlua_pushvalue(L, b-2);  /* -2 to compensate function and `a' */
    inlua_call(L, 2, 1);
    res = inlua_toboolean(L, -1);
    inlua_pop(L, 1);
    return res;
  }
  else  /* a < b? */
    return inlua_lessthan(L, a, b);
}

static void auxsort (inlua_State *L, int l, int u) {
  while (l < u) {  /* for tail recursion */
    int i, j;
    /* sort elements a[l], a[(l+u)/2] and a[u] */
    inlua_rawgeti(L, 1, l);
    inlua_rawgeti(L, 1, u);
    if (sort_comp(L, -1, -2))  /* a[u] < a[l]? */
      set2(L, l, u);  /* swap a[l] - a[u] */
    else
      inlua_pop(L, 2);
    if (u-l == 1) break;  /* only 2 elements */
    i = (l+u)/2;
    inlua_rawgeti(L, 1, i);
    inlua_rawgeti(L, 1, l);
    if (sort_comp(L, -2, -1))  /* a[i]<a[l]? */
      set2(L, i, l);
    else {
      inlua_pop(L, 1);  /* remove a[l] */
      inlua_rawgeti(L, 1, u);
      if (sort_comp(L, -1, -2))  /* a[u]<a[i]? */
        set2(L, i, u);
      else
        inlua_pop(L, 2);
    }
    if (u-l == 2) break;  /* only 3 elements */
    inlua_rawgeti(L, 1, i);  /* Pivot */
    inlua_pushvalue(L, -1);
    inlua_rawgeti(L, 1, u-1);
    set2(L, i, u-1);
    /* a[l] <= P == a[u-1] <= a[u], only need to sort from l+1 to u-2 */
    i = l; j = u-1;
    for (;;) {  /* invariant: a[l..i] <= P <= a[j..u] */
      /* repeat ++i until a[i] >= P */
      while (inlua_rawgeti(L, 1, ++i), sort_comp(L, -1, -2)) {
        if (i>u) inluaL_error(L, "invalid order function for sorting");
        inlua_pop(L, 1);  /* remove a[i] */
      }
      /* repeat --j until a[j] <= P */
      while (inlua_rawgeti(L, 1, --j), sort_comp(L, -3, -1)) {
        if (j<l) inluaL_error(L, "invalid order function for sorting");
        inlua_pop(L, 1);  /* remove a[j] */
      }
      if (j<i) {
        inlua_pop(L, 3);  /* pop pivot, a[i], a[j] */
        break;
      }
      set2(L, i, j);
    }
    inlua_rawgeti(L, 1, u-1);
    inlua_rawgeti(L, 1, i);
    set2(L, u-1, i);  /* swap pivot (a[u-1]) with a[i] */
    /* a[l..i-1] <= a[i] == P <= a[i+1..u] */
    /* adjust so that smaller half is in [j..i] and larger one in [l..u] */
    if (i-l < u-i) {
      j=l; i=i-1; l=i+2;
    }
    else {
      j=i+1; i=u; u=j-2;
    }
    auxsort(L, j, i);  /* call recursively the smaller one */
  }  /* repeat the routine for the larger one */
}

static int sort (inlua_State *L) {
  int n = aux_getn(L, 1);
  inluaL_checkstack(L, 40, "");  /* assume array is smaller than 2^40 */
  if (!inlua_isnoneornil(L, 2))  /* is there a 2nd argument? */
    inluaL_checktype(L, 2, INLUA_TFUNCTION);
  inlua_settop(L, 2);  /* make sure there is two arguments */
  auxsort(L, 1, n);
  return 0;
}

/* }====================================================== */


static const inluaL_Reg tab_funcs[] = {
  {"concat", tconcat},
  {"foreach", foreach},
  {"foreachi", foreachi},
  {"getn", getn},
  {"maxn", maxn},
  {"insert", tinsert},
  {"remove", tremove},
  {"setn", setn},
  {"sort", sort},
  {NULL, NULL}
};


INLUALIB_API int inluaopen_table (inlua_State *L) {
  inluaL_register(L, INLUA_TABLIBNAME, tab_funcs);
  return 1;
}

