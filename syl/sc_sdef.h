/*

   SC_SDEF.H

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

   Simple Attributes (Definition Object)

   29.06.96  Oliver Kraus

*/

#ifndef _SC_SDEF_H
#define _SC_SDEF_H

#define SC_SDEF_TYP_STRING  1
#define SC_SDEF_TYP_INTEGER 2
#define SC_SDEF_TYP_FLOAT   3
#define SC_SDEF_TYP_BOOLEAN 4
#define SC_SDEF_TYP_IDENTIFIER 5
#define SC_SDEF_TYP_MSTRING 6

struct _sc_sdef_entry
{
   char *name;
   int typ;
};
typedef struct _sc_sdef_entry sc_sdef_entry;

#define SC_SDEF_EXPAND 32

struct _sc_sdef_struct
{
   char *grp;
   sc_sdef_entry *list;
   int cnt;
   int max;
};
typedef struct _sc_sdef_struct sc_sdef_struct;
typedef struct _sc_sdef_struct *sc_sdef_type;

#define sc_sdef_GetGroupName(sd) ((sd)->grp)

struct _sc_sdef_init_struct
{
   char *name;
   int typ;
};
typedef struct _sc_sdef_init_struct sc_sdef_init_struct;

int sc_sdef_InitEntry(sc_sdef_type sp, int n, char *name, int typ);
void sc_sdef_DestroyEntry(sc_sdef_type sp, int n);
sc_sdef_type sc_sdef_Open(char *grp);
void sc_sdef_Close(sc_sdef_type sp);
int sc_sdef_AddDef(sc_sdef_type sp, char *name, int typ);
int sc_sdef_GetIndex(sc_sdef_type sp, char *name);
/* char *sc_sdef_GetAttrNameByIndex(sc_sdef_type sp, int idx); */
#define sc_sdef_GetAttrNameByIndex(sp,idx) ((sp)->list[(idx)].name)
/* int sc_sdef_GetAttrTypByIndex(sc_sdef_type sp, int idx); */
#define sc_sdef_GetAttrTypByIndex(sp,idx) ((sp)->list[(idx)].typ)
int sc_sdef_AddListDef(sc_sdef_type sp, sc_sdef_init_struct *list);

#endif
