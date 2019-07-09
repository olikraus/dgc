/*

   SC_STRL.C

   String List

   Copyright (C) 2001 Oliver Kraus (olikraus@yahoo.com)

   This file is part of DGC.

   DGC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   DGC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with DGC; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  

   16.07.96  Oliver Kraus

*/



#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sc_strl.h"
#include "sc_util.h"
#include "mwc.h"

sc_strl_type sc_strl_Open(void)
{
   sc_strl_type sl;
   sl = (sc_strl_type)malloc(sizeof(sc_strl_struct));
   if ( sl != NULL )
   {
      sl->list = NULL;
      sl->cnt = 0;
      sl->max = 0;
      return sl;
   }
   return NULL;
}

void sc_strl_Clear(sc_strl_type sl)
{
   int i;
   for( i = 0; i < sl->cnt; i++ )
   {
      free(sl->list[i]);      
   }
   sl->cnt = 0;
}

void sc_strl_Close(sc_strl_type sl)
{
   if ( sl != NULL )
   {
      if ( sl->list != NULL )
      {
         sc_strl_Clear(sl);
         free(sl->list);
      }      
      free(sl);
   }
}

int sc_strl_expand(sc_strl_type sl)
{
   assert( sl != NULL );

   if ( sl->list == NULL )
   {
      sl->list =
         (char **)malloc(sizeof(char *)*SC_STRL_EXPAND);
      if ( sl->list == NULL )
         return 0;
      sl->max = SC_STRL_EXPAND;
   }
   else
   {
      char **new_list;
      new_list = (char **)realloc(sl->list,
               sizeof(char *)*(sl->max+SC_STRL_EXPAND));
      if ( new_list == NULL )
         return 0;
      sl->list = new_list;
      sl->max += SC_STRL_EXPAND;
   }
   return 1;
}

int sc_strl_Add(sc_strl_type sl, char *s)
{
   while( sl->cnt >= sl->max )
   {
      if ( sc_strl_expand(sl) == 0 )
         return 0;
   }
   
   sl->list[sl->cnt] = (char *)malloc(strlen(s)+1);
   if ( sl->list[sl->cnt] == NULL )
      return 0;
   strcpy(sl->list[sl->cnt], s);
   sl->cnt++;
   return 1;
}

/* returns -1 if not found */
int sc_strl_Find(sc_strl_type sl, char *s)
{
  int i, cnt = sc_strl_GetCnt(sl);
  for( i = 0; i < cnt; i++ )
  {
    if ( sc_strcmp(sc_strl_Get(sl, i), s) == 0 )
      return i;
  }
  return -1;
}

int sc_strl_AddUnique(sc_strl_type sl, const char *s)
{
  int pos;
  pos = sc_strl_Find(sl, s);
  if ( pos >= 0 )
    return 1;
  return sc_strl_Add(sl, s);
}

