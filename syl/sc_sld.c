/*

   SC_SLD.C

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
   Simple Attributes List (Definition Object)

   29.06.96  Oliver Kraus

*/


#include <stdlib.h>
#include "sc_util.h"
#include "sc_sld.h"
#include "sc_data.h"
#include "mwc.h"

int sc_sld_InitEntry(sc_sld_type sl, int i, char *grp_name)
{
   sc_sld_entry *e;
   
   if ( i >= SC_SLD_GRP_MAX )
      return 0;
   
   e = sl->list+i;
   e->sd = sc_sdef_Open(grp_name);
   if ( e->sd != NULL )
   {
      e->cd = sc_cdef_Open(grp_name);
      if ( e->cd != NULL )
      {
         return 1;
      }
      sc_sdef_Close(e->sd);
   }
   return 0;
}

void sc_sld_DestroyEntry(sc_sld_type sl, int i)
{
   sc_sld_entry *e = sl->list+i;
   sc_sdef_Close(e->sd);
   sc_cdef_Close(e->cd);
}

sc_sld_type sc_sld_Open(void)
{
   sc_sld_type sl;
   sl = (sc_sld_type)malloc(sizeof(sc_sld_struct));
   if ( sl != NULL )
   {
      sl->cnt = 0;
      return sl;
   }
   return NULL;
}

void sc_sld_Close(sc_sld_type sl)
{
   if ( sl != NULL )
   {
      int i = 0;
      while( i < sl->cnt )
         sc_sld_DestroyEntry(sl, i++);
      free(sl);
   }
}

char *sc_sld_GetGroupNameByIndex(sc_sld_type sl, int idx)
{
   return sc_sdef_GetGroupName(sl->list[idx&SC_SLD_GRP_MASK].sd);
}

static int sc_sld_find_group(sc_sld_type sl, char *grp)
{
   int i = 0;
   while( i < sl->cnt )
   {
      if ( sc_strcmp( sc_sdef_GetGroupName(sl->list[i].sd), grp ) == 0 )
         return i;
      i++;
   }
   return -1;
}

static int sc_sld_add_sdef(sc_sld_type sl, char *grp, char *name, int typ)
{
   int pos;
   pos = sc_sld_find_group(sl, grp);
   if ( pos < 0 )
   {
      if ( sc_sld_InitEntry(sl, sl->cnt, grp) == 0 )
         return 0;
      pos = sl->cnt;
      sl->cnt++;
   }
   return sc_sdef_AddDef(sl->list[pos].sd, name, typ);
}

static int sc_sld_add_cdef(sc_sld_type sl, char *grp, char *name, int *typ)
{
   int pos;
   pos = sc_sld_find_group(sl, grp);
   if ( pos < 0 )
   {
      if ( sc_sld_InitEntry(sl, sl->cnt, grp) == 0 )
         return 0;
      pos = sl->cnt;
      sl->cnt++;
   }
   return sc_cdef_AddDef(sl->list[pos].cd, name, typ);
}

int sc_sld_AddSDef(sc_sld_type sl, char *grp, char *name, int typ)
{
   int pos;
   pos = sc_sld_find_group(sl, grp);
   if ( pos < 0 )
      return 0;
   return sc_sdef_AddDef(sl->list[pos].sd, name, typ);
}

int sc_sld_AddCDef(sc_sld_type sl, char *grp, char *name, int *typ)
{
   int pos;
   pos = sc_sld_find_group(sl, grp);
   if ( pos < 0 )
      return 0;
   return sc_cdef_AddDef(sl->list[pos].cd, name, typ);
}

int sc_sld_AddListDef(sc_sld_type sl, sc_sdl_init_struct *sdl_init)
{
   sc_sdef_init_struct *slist;
   sc_cdef_init_struct *clist;
   while( sdl_init != NULL && sdl_init->grp != NULL )
   {
      slist = sdl_init->sdef_init;
      while(slist != NULL && slist->name != NULL)
      {
         if ( sc_sld_add_sdef(sl, sdl_init->grp, slist->name, slist->typ) == 0 )
            return 0;
         slist++;
      }
      clist = sdl_init->cdef_init;
      while(clist != NULL && clist->name != NULL)
      {
         if ( sc_sld_add_cdef(sl, sdl_init->grp, clist->name, clist->typ) == 0 )
            return 0;
         clist++;
      }
      sdl_init++;
   }
   return 1;
}

sc_sdef_type sc_sld_GetSDEFByName(sc_sld_type sl, char *grp)
{
   int pos;
   pos = sc_sld_find_group(sl, grp);
   if ( pos < 0 )
      return NULL;
   return sl->list[pos].sd;
}

sc_cdef_type sc_sld_GetCDEFByName(sc_sld_type sl, char *grp)
{
   int pos;
   pos = sc_sld_find_group(sl, grp);
   if ( pos < 0 )
      return NULL;
   return sl->list[pos].cd;
}

/*
int sc_sld_GetIndex(sc_sld_type sl, char *grp, char *name)
{
   int pos, attr;
   pos = sc_sld_find_group(sl, grp);
   if ( pos < 0 )
      return -1;
   attr = sc_sdef_GetIndex(sl->sd[pos], name);
   if ( attr < 0 )
      return -1;
   return (attr<<SC_SLD_GRP_BITS)+pos;
}
*/

/*
char *sc_sld_GetAttrNameByIndex(sc_sld_type sl, int idx)
{
   return sc_sdef_GetAttrNameByIndex(
      sl->sd[idx&SC_SLD_GRP_MASK],
      idx>>SC_SLD_GRP_BITS);
}
*/

/*
int sc_sld_GetAttrTypByIndex(sc_sld_type sl, int idx)
{
   return sc_sdef_GetAttrTypByIndex(
      sl->sd[idx&SC_SLD_GRP_MASK],
      idx>>SC_SLD_GRP_BITS);
}
*/


int sc_sld_main()
{
   sc_sld_type sl;
   sl = sc_sld_Open();
   if ( sl == NULL )
      return 0;
   sc_sld_AddListDef(sl, sc_sdl_init_data);
   sc_sld_Close(sl);
   return 1;
}
