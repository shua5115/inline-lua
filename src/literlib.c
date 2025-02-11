#define inliterlib_c
#define INLUA_LIB

#include "inlua.h"

#include "inlauxlib.h"
#include "inlualib.h"

/*
** {======================================================
** Iterator library
** =======================================================
*/

#define ITER_TYPENAME "inlua_iter"

/*
** CONSUMER FUNCTIONS
*/

static int iter_consume(inlua_State *L) {
  inlua_getfield(L, 1, "next");
  for(;;) {
    inlua_pushvalue(L, -1);
    inlua_call(L, 0, 1);
    if(inlua_isnil(L, -1)) return 0;
    inlua_pop(L, 1);
  }
}


static int iter_foreach(inlua_State *L) {
  int fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next"); // fn_next points here
  for(;;) {
    inlua_pushvalue(L, 2); // push function 'f' from args
    inlua_pushvalue(L, fn_next); // push next function
    inlua_call(L, 0, INLUA_MULTRET);
    if(inlua_isnoneornil(L, fn_next+2)) return 0;
    // stack: fn_next, f, arg1, arg2, argn
    //        |        |  |     |     ^top
    //        3        4  5     6     7 -> 7-3=4, need 3 so subtract 1
    inlua_call(L, inlua_gettop(L) - fn_next - 1, 0);
  }
}


static int iter_count(inlua_State *L) {
  inlua_Integer i = 0;
  inlua_getfield(L, 1, "next");
  for(;;) {
    inlua_pushvalue(L, -1);
    inlua_call(L, 0, 1);
    if(inlua_isnil(L, -1)) break;
    inlua_pop(L, 1);
    i++;
  }
  inlua_pushinteger(L, i);
  return 1;
}


static int iter_last(inlua_State *L) {
  int i, newret;
  int fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next");
  for(;;) {
    // stack: fn_next, ret1, ret2, ...retn
    inlua_pushvalue(L, fn_next);
    newret = inlua_gettop(L);
    inlua_call(L, 0, INLUA_MULTRET);
    // stack: fn_next, ret1, ret2, ...retn, newret1, newret2, ...newretn
    if(inlua_isnoneornil(L, newret)) {
      inlua_settop(L, --newret);
      // stack: fn_next, ret1, ret2, ...retn <- newret points here now
      //        1        2     3     4 -> 4-1 = 3, perfect
      return newret-fn_next;
    }
    for(i = 0; i+newret<=inlua_gettop(L); i++) {
      inlua_pushvalue(L, newret+i);
      inlua_replace(L, fn_next+1+i);
      // stack after first: fn_next, newret1, ret2, retn, newret1, newret2, newretn
      //                    ^fn_next |        |     |     ^newret  |        ^top
      //                    1        2        3     4     5        6        7
    }
    inlua_settop(L, fn_next+1+inlua_gettop(L) - newret); // 1+1+7-5 = 4, perfect
    // stack after: fn_next, newret1, newret2, ...newretn
  }
}


static int iter_nth(inlua_State *L) {
  inlua_Integer i = 0;
  inlua_Integer n = inluaL_checkinteger(L, 2);
  int fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next");
  for(;;) {
    // stack: fn_next
    inlua_pushvalue(L, fn_next);
    inlua_call(L, 0, INLUA_MULTRET);
    // stack: fn_next, ret1, ret2, ...retn
    //        ^fn_next |     |     ^top
    //        1        2     3     4 -> 4-1 = 3, perfect
    if(inlua_isnoneornil(L, fn_next+1)) {
      return 0;
    }
    i++;
    if(i == n) {
      return inlua_gettop(L)-fn_next;
    }
    inlua_settop(L, fn_next);
  }
}


static int iter_collect(inlua_State *L) {
  inlua_Integer i = 0; // prev index for array insertion
  int fn_next = inlua_gettop(L)+1;
  if (inlua_isnoneornil(L, 2)) { // getfield happens to take slot 2, need to check before pushing
    inlua_getfield(L, 1, "next");
    inlua_createtable(L, 0, 0);
  } else {
    inlua_getfield(L, 1, "next");
    inlua_pushvalue(L, 2);
  }
  // stack: fn_next, t
  for(;;) {
    inlua_pushvalue(L, fn_next);
    inlua_call(L, 0, 2);
    // stack: fn_next, t, k, v
    if(inlua_isnil(L, -2)) {
      inlua_settop(L, fn_next+1);
      break;
    }
    if(inlua_isnil(L, -1)) {
      inlua_pop(L, 1);
      inlua_pushinteger(L, ++i);  // stack after: fn_next, t, k, i
      inlua_pushvalue(L, -2);     // stack after: fn_next, t, k, i, k
      inlua_settable(L, fn_next+1);
      inlua_settop(L, fn_next+1);
    } else {
      inlua_settable(L, fn_next+1); // stack after: fn_next, t
    }
  }
  
  return 1;
}


static int iter_all(inlua_State *L) {
  int fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next"); // fn_next points here
  for(;;) {
    inlua_pushvalue(L, 2); // push function 'f' from args
    inlua_pushvalue(L, fn_next); // push next function
    inlua_call(L, 0, INLUA_MULTRET);
    if(inlua_isnoneornil(L, fn_next+2)) break;
    // stack: fn_next, f, arg1, arg2, argn
    //        |        |  |     |     ^top
    //        3        4  5     6     7 -> 7-3=4, need 3 so subtract 1
    inlua_call(L, inlua_gettop(L) - fn_next - 1, 1);
    // stack: fn_next, res
    if (!inlua_toboolean(L, -1)) {
      inlua_pushboolean(L, 0);
      return 1;
    }
    inlua_settop(L, fn_next);
  }
  inlua_pushboolean(L, 1);
  return 1;
}


static int iter_any(inlua_State *L) {
  int fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next"); // fn_next points here
  for(;;) {
    inlua_pushvalue(L, 2); // push function 'f' from args
    inlua_pushvalue(L, fn_next); // push next function
    inlua_call(L, 0, INLUA_MULTRET);
    if(inlua_isnoneornil(L, fn_next+2)) break;
    // stack: fn_next, f, arg1, arg2, argn
    //        |        |  |     |     ^top
    //        3        4  5     6     7 -> 7-3=4, need 3 so subtract 1
    inlua_call(L, inlua_gettop(L) - fn_next - 1, 1);
    // stack: fn_next, res
    if (inlua_toboolean(L, -1)) {
      inlua_pushboolean(L, 1);
      return 1;
    }
    inlua_settop(L, fn_next);
  }
  inlua_pushboolean(L, 0);
  return 1;
}


static int iter_find(inlua_State *L) {
  int fn_next, ffirst, i;
  fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next");
  while (1) {
    inlua_pushvalue(L, fn_next);
    inlua_call(L, 0, INLUA_MULTRET);
    if(inlua_isnoneornil(L, fn_next+1)) {
      // no more values
      return 0;
    }
    inlua_pushvalue(L, 2); // push function 'f', ffirst points here
    ffirst = inlua_gettop(L);
    // stack: res1, res2, ...resn, f
    //        ^fn_next+1           ^ffirst
    for(i=1; i+fn_next<ffirst; i++) {
      inlua_pushvalue(L, i+fn_next);
    }
    // stack: res1, res2, ...resn, f, res1, res2, ...resn
    //        ^fn_next+1  |        ^ffirst        ^top
    //        2     3     4        5  6     7     8 -> 5-1=4, need 3 so subtract 1
    inlua_call(L, ffirst-fn_next-1, 1);
    // stack: res1, res2, ...resn, fres
    //        ^start+1             ^ffirst
    if (inlua_toboolean(L, -1)) {
      inlua_pop(L, 1);
      // stack: fn_next, res1, res2, ...resn
      //        1        2     3     4 -> 4-1 = 3, perfect
      return inlua_gettop(L)-fn_next;
    }
    inlua_settop(L, fn_next);
  }
  return 0;
}


static int iter_position(inlua_State *L) {
  inlua_Integer pos = 1;
  int fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next"); // fn_next points here
  for(;;) {
    inlua_pushvalue(L, 2); // push function 'f' from args
    inlua_pushvalue(L, fn_next); // push next function
    inlua_call(L, 0, INLUA_MULTRET);
    if(inlua_isnoneornil(L, fn_next+2)) break;
    // stack: fn_next, f, arg1, arg2, argn
    //        |        |  |     |     ^top
    //        3        4  5     6     7 -> 7-3=4, need 3 so subtract 1
    inlua_call(L, inlua_gettop(L) - fn_next - 1, 1);
    // stack: fn_next, res
    if (inlua_toboolean(L, -1)) {
      inlua_pushinteger(L, pos);
      return 1;
    }
    pos++;
    inlua_settop(L, fn_next);
  }
  inlua_pushboolean(L, 0);
  return 1;
}


static int iter_min(inlua_State *L) {
  int cmp = !inlua_isnoneornil(L, 2);
  int fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next"); // fn_next points here
  inlua_pushnil(L); // optimal value
  for(;;) {
    // stack: fn_next, optimal
    inlua_pushvalue(L, fn_next); // push next function
    inlua_call(L, 0, 1);
    if(inlua_isnoneornil(L, -1)) {
      inlua_settop(L, fn_next+1);
      break;
    }
    // stack: fn_next, optimal, ret
    // attempt to call comparison function
    if (inlua_isnil(L, fn_next+1)) {
      inlua_replace(L, fn_next+1);
    } else if (cmp) {
      inlua_pushvalue(L, 2);
      inlua_pushvalue(L, fn_next+1);
      inlua_pushvalue(L, -3);
      inlua_call(L, 2, 1); // lt(optimal, ret)
      // stack: fn_next, optimal, ret, (optimal<ret)
      if (!inlua_toboolean(L, -1)) { // if (ret <= optimal)
        inlua_pop(L, 1);
        inlua_replace(L, fn_next+1);  
      }
    } else if(!inlua_lessthan(L, fn_next+1, -1)) {
      inlua_replace(L, fn_next+1);
    }
    inlua_settop(L, fn_next+1);
  }
  return 1;
}


static int iter_max(inlua_State *L) {
  int cmp = !inlua_isnoneornil(L, 2);
  int fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next"); // fn_next points here
  inlua_pushnil(L); // optimal value
  for(;;) {
    // stack: fn_next, optimal
    inlua_pushvalue(L, fn_next); // push next function
    inlua_call(L, 0, 1);
    if(inlua_isnoneornil(L, -1)) {
      inlua_settop(L, fn_next+1);
      break;
    }
    // stack: fn_next, optimal, ret
    // attempt to call comparison function
    if (inlua_isnil(L, fn_next+1)) {
      inlua_replace(L, fn_next+1);
    } else if (cmp) {
      inlua_pushvalue(L, 2);
      inlua_pushvalue(L, -2);
      inlua_pushvalue(L, fn_next+1);
      inlua_call(L, 2, 1); // lt(optimal, ret)
      // stack: fn_next, optimal, ret, (optimal<ret)
      if (!inlua_toboolean(L, -1)) { // if (ret <= optimal)
        inlua_pop(L, 1);
        inlua_replace(L, fn_next+1);  
      }
    } else if(!inlua_lessthan(L, -1, fn_next+1)) {
      inlua_replace(L, fn_next+1);
    }
    inlua_settop(L, fn_next+1);
  }
  return 1;
}


static int iter_compare(inlua_State *L) {
  int cmp = !inlua_isnoneornil(L, 3);
  int start = inlua_gettop(L);
  inlua_getfield(L, 1, "next");
  inlua_getfield(L, 2, "next");
  for (;;) {
    // stack: nxt1, nxt2
    inlua_pushvalue(L, start+1);
    inlua_call(L, 0, 1);
    inlua_pushvalue(L, start+2);
    inlua_call(L, 0, 1);
    if (inlua_isnil(L, -1) || inlua_isnil(L, -2)) {
      // then one of the iterators is done.
      // if one is shorter than another, the shorter one is less, otherwise they are equal.
      if (inlua_isnil(L, -2) && !inlua_isnil(L, -1)) {
        // first is shorter -> lt
        inlua_pushnumber(L, -1);
        return 1;
      }
      if (!inlua_isnil(L, -2) && inlua_isnil(L, -1)) {
        // second is shorter -> gt
        inlua_pushnumber(L, 1);
        return 1;
      }
      inlua_pushnumber(L, 0);
      return 1;
    }
    // stack: start, nxt1, nxt2, v1, v2
    if (cmp) {
      inlua_pushvalue(L, 3); // stack: start, nxt1, nxt2, v1, v2, cmp
      inlua_pushvalue(L, -3); 
      inlua_pushvalue(L, -3);
      inlua_call(L, 2, 1);
      if (!inlua_isnumber(L, -1)) inluaL_error(L, "expected number from comparison function, got %s", inluaL_typename(L, -1));
      if (inlua_tonumber(L, -1) != 0) {
        return 1;
      }
    } else {
      if(inlua_lessthan(L, -2, -1)) {
        // first result is less -> lt
        inlua_pushnumber(L, -1);
        return 1;
      }
      if(inlua_lessthan(L, -1, -2)) {
        // second result is less -> gt
        inlua_pushnumber(L, 1);
        return 1;
      }
    }
    inlua_settop(L, start+2);
  }
  return 0;
}


static int iter_fold(inlua_State *L) {
  int fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next"); // fn_next points here
  inlua_pushvalue(L, 2); // accumulator ("acc")
  for(;;) {
    // stack: fn_next, acc
    inlua_pushvalue(L, 3); // push function 'f' from args
    inlua_pushvalue(L, fn_next+1);
    inlua_pushvalue(L, fn_next); // push next function
    inlua_call(L, 0, INLUA_MULTRET);
    // stack: fn_next, acc, f, acc, arg1, arg2, argn
    if(inlua_isnoneornil(L, fn_next+4)) {
      inlua_settop(L, fn_next+1);
      break;
    }
    // stack: fn_next, acc, f, acc, arg1, arg2, argn
    //        |        |    |  |    |     |     ^top
    //        3        4    5  6    7     8     9 -> 9-3=6, need 4 so subtract 2
    inlua_call(L, inlua_gettop(L) - fn_next - 2, 1);
    // stack: fn_next, acc, ret
    inlua_replace(L, fn_next+1);
  }
  return 1;
}


static int iter_reduce(inlua_State *L) {
  /* iter_fold(it, it.next() | ^^, f) */
  inlua_pushcfunction(L, iter_fold);
  inlua_pushvalue(L, 1);
  inlua_getfield(L, 1, "next");
  inlua_call(L, 0, 1);
  if (inlua_isnil(L, -1)) return 0;
  inlua_pushvalue(L, 2);
  inlua_call(L, 3, 1);
  return 1;
}


static int iter_unzip(inlua_State *L) {
  inlua_Integer i = 0;
  int fn_next = inlua_gettop(L)+1;
  inlua_getfield(L, 1, "next"); // fn_next points here
  if (inlua_isnoneornil(L, 2)) {
    inlua_createtable(L, 0, 0);
  } else inlua_pushvalue(L, 2);
  if (inlua_isnoneornil(L, 3)) {
    inlua_createtable(L, 0, 0);
  } else inlua_pushvalue(L, 3);
  // stack: fn_next, t1, t2
  for(;;) {
    inlua_pushvalue(L, fn_next);
    inlua_call(L, 0, 2);
    // stack: fn_next, t1, t2, v1, v2
    if(inlua_isnil(L, -2) || inlua_isnil(L, -1)) {
      inlua_settop(L, fn_next+2);
      break;
    }
    i += 1;
    inlua_pushinteger(L, i);  // stack: fn_next, t1, t2, v1, v2, i
    inlua_pushvalue(L, -3);     // stack after: fn_next, t1, t2, v1, v2, i, v1
    inlua_settable(L, fn_next+1);
    inlua_pushinteger(L, i);  // stack: fn_next, t1, t2, v1, v2, i
    inlua_pushvalue(L, -2);     // stack after: fn_next, t1, t2, v1, v2, i, v2
    inlua_settable(L, fn_next+2);
    inlua_settop(L, fn_next+2);
  }
  
  return 2;
}


/*
** ADAPTER FUNCTIONS
*/


// expects a two upvalues:
// 1. the previous iterator's next function
// 2. the function to apply to the result of next 
static int iter_map_next(inlua_State *L) {
  int start;
  if (inlua_isnil(L, inlua_upvalueindex(1))) return 0;

  start = inlua_gettop(L);
  inlua_pushvalue(L, inlua_upvalueindex(2)); // push function 'f'
  inlua_pushvalue(L, inlua_upvalueindex(1)); // push previous next function
  inlua_call(L, 0, INLUA_MULTRET);
  if(inlua_isnoneornil(L, start+2)) {
    // no more values, clear next function
    inlua_pushnil(L);
    inlua_replace(L, inlua_upvalueindex(1));
    return 0;
  }
  // stack: f, arg1, arg2, argn
  //        ^start+1 |     ^top
  //        1  2     3     4 -> 4-0 = 4, need to subtract 1
  inlua_call(L, inlua_gettop(L)-start-1, INLUA_MULTRET);
  // stack: res1,   res2, resn
  //        ^start+1|     ^top
  //        1       2     3 -> 3-0 = 3, perfect
  // for no results: 0,     1 -> 0-1 -> -1, need 0 so add 1
  //                 ^top   ^f_idx
  return inlua_gettop(L)-start;
}

static int iter_map(inlua_State *L) {
  inlua_getfield(L, 1, "next");
  inlua_pushvalue(L, 2);
  inlua_pushcclosure(L, iter_map_next, 2);
  inlua_setfield(L, 1, "next");
  inlua_pushvalue(L, 1);
  return 1;
}


// expects 2 upvalues:
// 1. original next (possibly nil after it finishes)
// 2. new next
static int iter_chain_next(inlua_State *L) {
  int start = inlua_gettop(L); // will point to one-before the return values
  if (!inlua_isnil(L, inlua_upvalueindex(1))) { // if 1 is not done iterating
    inlua_pushvalue(L, inlua_upvalueindex(1));
    inlua_call(L, 0, INLUA_MULTRET);
    if (inlua_isnoneornil(L, start+1)) {
      // then first iterator is done, clear it and call second iterator instead
      inlua_pushnil(L);
      inlua_replace(L, inlua_upvalueindex(1));
      inlua_settop(L, start);
      inlua_pushvalue(L, inlua_upvalueindex(2));
      inlua_call(L, 0, INLUA_MULTRET);
    }
  } else {
    inlua_pushvalue(L, inlua_upvalueindex(2));
    inlua_call(L, 0, INLUA_MULTRET);
  }
  return inlua_gettop(L)-start;
}

static int iter_chain(inlua_State *L) {
  inlua_getfield(L, 1, "next");
  inlua_getfield(L, 2, "next");
  inlua_pushcclosure(L, iter_chain_next, 2);
  inlua_setfield(L, 1, "next");
  inlua_pushvalue(L, 1);
  return 1;
}


// expects 2 upvalues:
// 1. original next (possibly nil after it finishes)
// 2. new next (possibly nil after it finishes)
static int iter_zip_next(inlua_State *L) {
  if (!inlua_isnil(L, inlua_upvalueindex(1)) && !inlua_isnil(L, inlua_upvalueindex(2))) {
    inlua_pushvalue(L, inlua_upvalueindex(1));
    inlua_call(L, 0, 1);
    inlua_pushvalue(L, inlua_upvalueindex(2));
    inlua_call(L, 0, 1);
    if (inlua_isnil(L, -1) || inlua_isnil(L, -2)) {
      // then one of the iterators is done, clear both, and return nil
      inlua_pushnil(L);
      inlua_replace(L, inlua_upvalueindex(1));
      inlua_pushnil(L);
      inlua_replace(L, inlua_upvalueindex(2));
      return 0;
    }
    return 2;
  }
  return 0;
}

static int iter_zip(inlua_State *L) {
  inlua_getfield(L, 1, "next");
  inlua_getfield(L, 2, "next");
  inlua_pushcclosure(L, iter_zip_next, 2);
  inlua_setfield(L, 1, "next");
  inlua_pushvalue(L, 1);
  return 1;
}


// expects 2 upvalues:
// 1. original next (possibly nil after it finishes)
// 2. predicate function
static int iter_filter_next(inlua_State *L) {
  int start, ffirst, i;
  start = inlua_gettop(L);
  while (!inlua_isnil(L, inlua_upvalueindex(1))) {
    inlua_pushvalue(L, inlua_upvalueindex(1)); // push previous next function
    inlua_call(L, 0, INLUA_MULTRET);
    if(inlua_isnoneornil(L, start+1)) {
      // no more values, clear next function
      inlua_pushnil(L);
      inlua_replace(L, inlua_upvalueindex(1));
      return 0;
    }
    inlua_pushvalue(L, inlua_upvalueindex(2)); // push function 'f', ffirst points here
    ffirst = inlua_gettop(L);
    // stack: res1, res2, ...resn, f
    //        ^start+1             ^ffirst
    for(i=1; i+start<ffirst; i++) {
      inlua_pushvalue(L, start+i);
    }
    // stack: res1, res2, ...resn, f, res1, res2, ...resn
    //        ^start+1             ^ffirst        ^top
    inlua_call(L, ffirst-start-1, 1);
    // stack: res1, res2, ...resn, fres
    //        ^start+1             ^ffirst
    if (inlua_toboolean(L, -1)) {
      inlua_pop(L, 1);
      return inlua_gettop(L)-start;
    }
    inlua_settop(L, start);
  }
  return 0;
}

static int iter_filter(inlua_State *L) {
  inlua_getfield(L, 1, "next");
  inlua_pushvalue(L, 2);
  inlua_pushcclosure(L, iter_filter_next, 2);
  inlua_setfield(L, 1, "next");
  inlua_pushvalue(L, 1);
  return 1;
}


// expects two upvalues:
// 1. original next function
// 2. current index (initialized to 0)
static int iter_enumerate_next(inlua_State *L) {
  int start;
  if(inlua_isnil(L, inlua_upvalueindex(1))) return 0;
  start = inlua_gettop(L);
  inlua_pushinteger(L, inlua_tointeger(L, inlua_upvalueindex(2))+1); // new integer will be in index start+1
  inlua_pushvalue(L, inlua_upvalueindex(1));
  inlua_call(L, 0, INLUA_MULTRET);
  if (inlua_isnoneornil(L, start+2)) {
    // no more values, clear next function
    inlua_pushnil(L);
    inlua_replace(L, inlua_upvalueindex(1));
    return 0;
  }
  // store new index in upvalue
  inlua_pushvalue(L, start+1);
  inlua_replace(L, inlua_upvalueindex(2));
  return inlua_gettop(L)-start;
}

static int iter_enumerate(inlua_State *L) {
  inlua_getfield(L, 1, "next");
  inlua_pushinteger(L, 0);
  inlua_pushcclosure(L, iter_enumerate_next, 2);
  inlua_setfield(L, 1, "next");
  inlua_pushvalue(L, 1);
  return 1;
}


// expects 2 upvalues:
// 1. original next function (possibly nil)
// 2. how many elements to skip (possibly nil)
static int iter_skip_next(inlua_State *L) {
  int start;
  if (inlua_isnil(L, inlua_upvalueindex(1))) return 0;
  start = inlua_gettop(L);
  if (!inlua_isnil(L, inlua_upvalueindex(2))) {
    inlua_Integer i = 0;
    inlua_Integer n = inlua_tointeger(L, inlua_upvalueindex(2));
    for(; i<n; i++) {
      inlua_pushvalue(L, inlua_upvalueindex(1));
      inlua_call(L, 0, INLUA_MULTRET);
      if (inlua_isnoneornil(L, start+1)) {
        inlua_pushnil(L);
        inlua_replace(L, inlua_upvalueindex(1));
        return 0;
      }
      inlua_settop(L, start);
    }
    inlua_pushnil(L);
    inlua_replace(L, inlua_upvalueindex(2));
  }
  inlua_pushvalue(L, inlua_upvalueindex(1));
  inlua_call(L, 0, INLUA_MULTRET);
  if (inlua_isnoneornil(L, start+1)) {
    inlua_pushnil(L);
    inlua_replace(L, inlua_upvalueindex(1));
    return 0;
  }
  return inlua_gettop(L)-start;
}

static int iter_skip(inlua_State *L) {
  inluaL_checkinteger(L, 2);
  inlua_getfield(L, 1, "next");
  inlua_pushvalue(L, 2);
  inlua_pushcclosure(L, iter_skip_next, 2);
  inlua_setfield(L, 1, "next");
  inlua_pushvalue(L, 1);
  return 1;
}


// expects 3 upvalues:
// 1. original next function (possibly nil)
// 2. how many elements to take (possibly nil)
// 3. return count, initialized to 0
static int iter_take_next(inlua_State *L) {
  inlua_Integer i;
  int start;
  if (inlua_isnil(L, inlua_upvalueindex(1)) || inlua_isnil(L, inlua_upvalueindex(2))) return 0;
  start = inlua_gettop(L);
  inlua_pushvalue(L, inlua_upvalueindex(1));
  inlua_call(L, 0, INLUA_MULTRET);
  if (inlua_isnoneornil(L, start+1)) {
    inlua_pushnil(L);
    inlua_replace(L, inlua_upvalueindex(1));
    return 0;
  }
  i = inlua_tointeger(L, inlua_upvalueindex(3))+1;
  if (i >= inlua_tointeger(L, inlua_upvalueindex(2))) {
    inlua_pushnil(L);
    inlua_replace(L, inlua_upvalueindex(2));
  } else {
    inlua_pushinteger(L, i);
    inlua_replace(L, inlua_upvalueindex(3));
  }
  return inlua_gettop(L)-start;
}

static int iter_take(inlua_State *L) {
  inluaL_checkinteger(L, 2);
  inlua_getfield(L, 1, "next");
  inlua_pushvalue(L, 2);
  inlua_pushinteger(L, 0);
  inlua_pushcclosure(L, iter_take_next, 3);
  inlua_setfield(L, 1, "next");
  inlua_pushvalue(L, 1);
  return 1;
}


// expects 2 upvalues:
// 1. original next function (possibly nil)
// 2. inspection function
static int iter_inspect_next(inlua_State *L) {
  int start, ffirst, i;
  if (inlua_isnil(L, inlua_upvalueindex(1))) return 0;
  start = inlua_gettop(L);
  inlua_pushvalue(L, inlua_upvalueindex(1)); // push previous next function
  inlua_call(L, 0, INLUA_MULTRET);
  if(inlua_isnoneornil(L, start+1)) {
    // no more values, clear next function
    inlua_pushnil(L);
    inlua_replace(L, inlua_upvalueindex(1));
    return 0;
  }
  inlua_pushvalue(L, inlua_upvalueindex(2)); // push function 'f', ffirst points here
  ffirst = inlua_gettop(L);
  // stack: res1, res2, ...resn, f
  //        ^start+1             ^ffirst
  for(i=1; i+start<ffirst; i++) {
    inlua_pushvalue(L, start+i);
  }
  // stack: res1, res2, ...resn, f, res1, res2, ...resn
  //        ^start+1             ^ffirst        ^top
  inlua_call(L, ffirst-start-1, 0);
  // stack: res1, res2, ...resn, ~
  //        ^start+1    ^top     ^ffirst
  return inlua_gettop(L)-start;
}

static int iter_inspect(inlua_State *L) {
  inlua_getfield(L, 1, "next");
  inlua_pushvalue(L, 2);
  inlua_pushcclosure(L, iter_inspect_next, 2);
  inlua_setfield(L, 1, "next");
  inlua_pushvalue(L, 1);
  return 1;
}



// All iterator methods
static const inluaL_Reg iterlib[] = {
  // consumers
  {"consume", iter_consume},
  {"foreach", iter_foreach},
  {"count", iter_count},
  {"last", iter_last},
  {"nth", iter_nth},
  {"collect", iter_collect},
  {"all", iter_all},
  {"any", iter_any},
  {"find", iter_find},
  {"position", iter_position},
  {"min", iter_min},
  {"max", iter_max},
  {"compare", iter_compare},
  {"fold", iter_fold},
  {"reduce", iter_reduce},
  {"unzip", iter_unzip},
  // adapters
  {"map", iter_map},
  {"chain", iter_chain},
  {"zip", iter_zip},
  {"filter", iter_filter},
  {"enumerate", iter_enumerate},
  {"skip", iter_skip},
  {"take", iter_take},
  {"inspect", iter_inspect},
  {NULL, NULL}
};


// expects three upvalues in order: func, state, key
static int iter_wrap_stateless(inlua_State *L) {
  int first = inlua_gettop(L)+1;
  inlua_pushvalue(L, inlua_upvalueindex(1));
  inlua_pushvalue(L, inlua_upvalueindex(2));
  inlua_pushvalue(L, inlua_upvalueindex(3));
  inlua_call(L, 2, INLUA_MULTRET); // sets top, first points to first return value
  if (!inlua_isnone(L, first)) inlua_pushvalue(L, first);
  else inlua_pushnil(L);
  inlua_replace(L, inlua_upvalueindex(3)); // key = first
  if (inlua_isnoneornil(L, first)) return 0;
  return 1+inlua_gettop(L)-first;
  // 4,  5,  6 -> 6-4 = 2, need 3 returns so add 1
  // ^first  ^top
  // 1,  -> 1-1=0, need 1 return, so add 1
  // ^first,top
}


// expects 3 upvalues:
// 1. current index
// 2. final value
// 3. step amount
static int range_aux (inlua_State *L) {
  inlua_Number cur = inlua_tonumber(L, inlua_upvalueindex(1));
  inlua_Number stop = inlua_tonumber(L, inlua_upvalueindex(2));
  inlua_Number step = inlua_tonumber(L, inlua_upvalueindex(3));
  inlua_pushnumber(L, cur);
  cur += step;
  if (step > 0 && cur > stop) {
    return 0;
  }
  if (step < 0 && cur < stop) {
    return 0;  
  }
  inlua_pushvalue(L, -1);
  inlua_replace(L, inlua_upvalueindex(1));
  return 1;
}

static int inlua_range (inlua_State *L) {
  inlua_Number start = inluaL_checknumber(L, 1);
  inlua_Number stop = inluaL_optnumber(L, 2, start);
  inlua_Number step = inluaL_optnumber(L, 3, 1);
  if (inlua_isnoneornil(L, 2) && inlua_isnoneornil(L, 3)) {
    start = 1;
  }
  inlua_pushnumber(L, start);
  inlua_pushnumber(L, stop);
  inlua_pushnumber(L, step);
  inlua_pushcclosure(L, range_aux, 3);
  return 1;
}

// expects 2 upvalues
// 1. table
// 2. previous index
static int values_aux(inlua_State *L) {
  inlua_Integer i;
  if (inlua_isnil(L, inlua_upvalueindex(1))) return 0;
  i = inlua_tointeger(L, inlua_upvalueindex(2));
  inlua_pushinteger(L, i+1);
  inlua_pushvalue(L, -1);
  inlua_replace(L, inlua_upvalueindex(2));
  inlua_gettable(L, inlua_upvalueindex(1));
  if(inlua_isnil(L, -1)) {
    inlua_replace(L, inlua_upvalueindex(1));
    return 0;
  }
  return 1;
}

static int inlua_values(inlua_State *L) {
  inluaL_checkany(L, 1);
  inlua_pushvalue(L, 1);
  inlua_pushinteger(L, 0);
  inlua_pushcclosure(L, values_aux, 2);
  return 1;
}


static int inlua_itercreate(inlua_State *L) {
  if(inlua_isfunction(L, 1)) {
    inlua_createtable(L, 0, 1);
    inlua_pushvalue(L, 1);
    (void) inluaL_opt(L, inlua_pushvalue, 2, inlua_pushnil(L));
    (void) inluaL_opt(L, inlua_pushvalue, 3, inlua_pushnil(L));
    inlua_pushcclosure(L, iter_wrap_stateless, 3);
    inlua_setfield(L, -2, "next");
    inluaL_getmetatable(L, ITER_TYPENAME);
    inlua_setmetatable(L, -2);
    return 1;
  }
  inluaL_checkany(L, 1);
  inluaL_getmetatable(L, ITER_TYPENAME);
  inlua_setmetatable(L, 1);
  inlua_getfield(L, 1, "next");
  if (inlua_isnil(L, -1)) inluaL_error(L, "given iterator missing " INLUA_QL("next") " function");
  inlua_pushvalue(L, 1);
  return 1;
}


static int createitermeta(inlua_State *L) {
  inluaL_newmetatable(L, ITER_TYPENAME);  /* create metatable for iterators */
  inlua_pushvalue(L, -1);  /* push metatable */
  inlua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  inluaL_register(L, NULL, iterlib);  /* iterator methods */
  return 1;
}

/* }====================================================== */

INLUALIB_API int inluaopen_iter(inlua_State *L) {
  createitermeta(L);
  inlua_register(L, "range", inlua_range);
  inlua_register(L, "ivals", inlua_values);
  inlua_pushcfunction(L, inlua_itercreate);
  inlua_pushvalue(L, -1);
  inlua_setglobal(L, "iter");
  return 1;
}
