/*

   SC_SLD.H

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


#ifndef _SC_SLD_H
#define _SC_SLD_H

#include "sc_sdef.h"
#include "sc_cdef.h"

#define SC_SLD_GRP_BITS 6
#define SC_SLD_GRP_MAX (1<<SC_SLD_GRP_BITS)
#define SC_SLD_GRP_MASK (SC_SLD_GRP_MAX-1)

struct _sc_sld_entry
{
   sc_sdef_type sd;
   sc_cdef_type cd;
};
typedef struct _sc_sld_entry sc_sld_entry;

struct _sc_sld_struct
{
   sc_sld_entry list[SC_SLD_GRP_MAX];
   int cnt;
};
typedef struct _sc_sld_struct sc_sld_struct;
typedef struct _sc_sld_struct *sc_sld_type;

#define sc_sld_GetSDEFCnt(sld) ((sld)->cnt)
#define sc_sld_GetSDEFByIndex(sld,idx) ((sld)->sd[(idx)&SC_SLD_GRP_MASK])

struct _sc_sdl_init_struct
{
   char *grp;
   sc_sdef_init_struct *sdef_init;
   sc_cdef_init_struct *cdef_init;
};
typedef struct _sc_sdl_init_struct sc_sdl_init_struct;

sc_sld_type sc_sld_Open(void);
void sc_sld_Close(sc_sld_type sl);
char *sc_sld_GetGroupNameByIndex(sc_sld_type sl, int idx);
int sc_sld_AddSDef(sc_sld_type sl, char *grp, char *name, int typ);
int sc_sld_AddCDef(sc_sld_type sl, char *grp, char *name, int *typ);
int sc_sld_AddListDef(sc_sld_type sl, sc_sdl_init_struct *sdl_init);
sc_sdef_type sc_sld_GetSDEFByName(sc_sld_type sl, char *grp);
sc_cdef_type sc_sld_GetCDEFByName(sc_sld_type sl, char *grp);

#endif
