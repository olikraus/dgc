/*

   SC_SDEF.C

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
   Simple Attributes (Definition Objekt)

   29.06.96  Oliver Kraus

*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sc_sdef.h"
#include "sc_util.h"
#include "mwc.h"

int sc_sdef_InitEntry(sc_sdef_type sp, int n, char *name, int typ)
{
   sc_sdef_entry *e = sp->list+n;
   e->name = (char *)malloc(strlen(name)+1);
   if ( e->name == NULL )
   {
      return 0;
   }
   strcpy(e->name, name);
   e->typ = typ;
   return 1;
}

void sc_sdef_DestroyEntry(sc_sdef_type sp, int n)
{
   sc_sdef_entry *e = sp->list+n;
   free(e->name);
}

sc_sdef_type sc_sdef_Open(char *grp)
{
   sc_sdef_type sp;
   sp = (sc_sdef_type)malloc(sizeof(sc_sdef_struct));
   if ( sp != NULL )
   {
      sp->grp = (char *)malloc(strlen(grp)+1);
      if ( sp->grp != NULL )
      {
         strcpy(sp->grp, grp);
         sp->cnt = 0;
         sp->max = 0;
         sp->list = NULL;
         return sp;
      }
      free(sp);
   }
   return NULL;
}

void sc_sdef_Close(sc_sdef_type sp)
{
   if ( sp != NULL )
   {
      if ( sp->list != NULL )
      {
         int i;
         for( i = 0; i < sp->cnt; i++ )
            sc_sdef_DestroyEntry(sp, i);
         free(sp->list);
      }
      free(sp->grp);
      free(sp);
   }
}

static int sc_sdef_expand(sc_sdef_type sp)
{
   sc_sdef_entry *newlist;
   int newmax;
   if ( sp->list == NULL )
   {
      newlist = (sc_sdef_entry *)malloc(sizeof(sc_sdef_entry)*SC_SDEF_EXPAND);
      newmax = SC_SDEF_EXPAND;
   }
   else
   {
      newlist = (sc_sdef_entry *)realloc(sp->list, sizeof(sc_sdef_entry)*(sp->max+SC_SDEF_EXPAND));
      newmax = sp->max+SC_SDEF_EXPAND;
   }
   if ( newlist == NULL )
      return 0;
   sp->list = newlist;
   sp->max = newmax;
   return 1;
}


int sc_sdef_AddDef(sc_sdef_type sp, char *name, int typ)
{
   while ( sp->cnt >= sp->max )
      if ( sc_sdef_expand(sp) == 0 )
         return 0;
   if ( sc_sdef_InitEntry(sp, sp->cnt, name, typ) == 0 )
      return 0;
   sp->cnt++;
   return 1;
}

static int sc_sdef_find_name(sc_sdef_type sp, char *name)
{
   int i;
   for( i = 0; i < sp->cnt; i++ )
   {
      if ( sc_strcmp( sp->list[i].name, name ) == 0 )
      {
         return i;
      }
   }
   return -1;
}

int sc_sdef_GetIndex(sc_sdef_type sp, char *name)
{
   return sc_sdef_find_name(sp, name);
}

/*
char *sc_sdef_GetAttrNameByIndex(sc_sdef_type sp, int idx)
{
   return sp->list[i].name;
}
*/

/*
int sc_sdef_GetAttrTypByIndex(sc_sdef_type sp, int idx)
{
   return sp->list[i].typ;
}
*/

int sc_sdef_AddListDef(sc_sdef_type sp, sc_sdef_init_struct *list)
{
   while(list != NULL && list->name != NULL)
   {
      if ( sc_sdef_AddDef(sp, list->name, list->typ) == 0 )
         return 0;
      list++;
   }
   return 1;
}
