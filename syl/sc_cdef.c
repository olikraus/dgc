/*

   SC_CDEF.C

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

   Complrex Attributes (Definition Object)

   01.07.96  Oliver Kraus

*/

#include <stdlib.h>
#include <string.h>
#include "sc_cdef.h"
#include "sc_util.h"
#include "mwc.h"

int sc_cdef_InitEntry(sc_cdef_type cd, int n, char *name, int *typ)
{
   int i;
   sc_cdef_entry *e = cd->list+n;
   e->name = (char *)malloc(strlen(name)+1);
   if ( e->name == NULL )
   {
      return 0;
   }
   strcpy(e->name, name);
   for( i = 0; i < SC_CDEF_ARGS; i++ )
   {
      e->typ[i] = SC_CDEF_TYP_NONE;
   }
   for( i = 0; i < SC_CDEF_ARGS; i++ )
   {
      if ( typ == SC_CDEF_TYP_NONE )
         break;
      e->typ[i] = *typ++;
   }
   return 1;
}

void sc_cdef_DestroyEntry(sc_cdef_type cd, int n)
{
   sc_cdef_entry *e = cd->list+n;
   free(e->name);
}

sc_cdef_type sc_cdef_Open(char *grp)
{
   sc_cdef_type cd;
   cd = (sc_cdef_type)malloc(sizeof(sc_cdef_struct));
   if ( cd != NULL )
   {
      cd->grp = (char *)malloc(strlen(grp)+1);
      if ( cd->grp != NULL )
      {
         strcpy(cd->grp, grp);
         cd->cnt = 0;
         cd->max = 0;
         cd->list = NULL;
         return cd;
      }
      free(cd);
   }
   return NULL;
}

void sc_cdef_Close(sc_cdef_type cd)
{
   if ( cd != NULL )
   {
      if ( cd->list != NULL )
      {
         int i;
         for( i = 0; i < cd->cnt; i++ )
            sc_cdef_DestroyEntry(cd, i);
         free(cd->list);
      }
      free(cd->grp);
      free(cd);
   }
}

static int sc_cdef_expand(sc_cdef_type cd)
{
   sc_cdef_entry *newlist;
   int newmax;
   if ( cd->list == NULL )
   {
      newlist = (sc_cdef_entry *)malloc(sizeof(sc_cdef_entry)*SC_CDEF_EXPAND);
      newmax = SC_CDEF_EXPAND;
   }
   else
   {
      newlist = (sc_cdef_entry *)realloc(cd->list, sizeof(sc_cdef_entry)*(cd->max+SC_CDEF_EXPAND));
      newmax = cd->max+SC_CDEF_EXPAND;
   }
   if ( newlist == NULL )
      return 0;
   cd->list = newlist;
   cd->max = newmax;
   return 1;
}

int sc_cdef_AddDef(sc_cdef_type cd, char *name, int *typ)
{
   while ( cd->cnt >= cd->max )
      if ( sc_cdef_expand(cd) == 0 )
         return 0;
   if ( sc_cdef_InitEntry(cd, cd->cnt, name, typ) == 0 )
      return 0;
   cd->cnt++;
   return 1;
}

static int sc_cdef_find_name(sc_cdef_type cd, char *name)
{
   int i;
   for( i = 0; i < cd->cnt; i++ )
   {
      if ( sc_strcmp( cd->list[i].name, name ) == 0 )
      {
         return i;
      }
   }
   return -1;
}

int sc_cdef_GetIndex(sc_cdef_type cd, char *name)
{
   return sc_cdef_find_name(cd, name);
}

/*
sc_cdef_init_struct x[] = 
{
   { "name", { 1, 2, 3 } },
   { "name", { 1, 2, 3 } }
};

*/
