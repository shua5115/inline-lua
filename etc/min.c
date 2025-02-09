/*
* min.c -- a minimal Lua interpreter
* loads stdin only with minimal error handling.
* no interaction, and no standard library, only a "print" function.
*/

#include <stdio.h>

#include "inlua.h"
#include "inlauxlib.h"

static int print(inlua_State *L)
{
 int n=inlua_gettop(L);
 int i;
 for (i=1; i<=n; i++)
 {
  if (i>1) printf("\t");
  if (inlua_isstring(L,i))
   printf("%s",inlua_tostring(L,i));
  else if (inlua_isnil(L,i))
   printf("%s","nil");
  else if (inlua_isboolean(L,i))
   printf("%s",inlua_toboolean(L,i) ? "true" : "false");
  else
   printf("%s:%p",inluaL_typename(L,i),inlua_topointer(L,i));
 }
 printf("\n");
 return 0;
}

int main(void)
{
 inlua_State *L=inlua_open();
 inlua_register(L,"print",print);
 if (inluaL_dofile(L,NULL)!=0) fprintf(stderr,"%s\n",inlua_tostring(L,-1));
 inlua_close(L);
 return 0;
}
