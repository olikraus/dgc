/*

   SC_CDEF.H

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

   Complex Attributes (Definition Object)

   01.07.96  Oliver Kraus

*/

#ifndef _SC_CDEF_H
#define _SC_CDEF_H

#define SC_CDEF_TYP_NONE       0
#define SC_CDEF_TYP_END        0
#define SC_CDEF_TYP_STRING     1
#define SC_CDEF_TYP_INTEGER    2
#define SC_CDEF_TYP_FLOAT      3
#define SC_CDEF_TYP_IDENTIFIER 4
#define SC_CDEF_TYP_MSTRING    5

#define SC_CDEF_ARGS 6

struct _sc_cdef_entry
{
   char *name;
   int typ[SC_CDEF_ARGS];
};
typedef struct _sc_cdef_entry sc_cdef_entry;

#define SC_CDEF_EXPAND 16

struct _sc_cdef_struct
{
   char *grp;
   sc_cdef_entry *list;
   int cnt;
   int max;
};
typedef struct _sc_cdef_struct sc_cdef_struct;
typedef struct _sc_cdef_struct *sc_cdef_type;

struct _sc_cdef_init_struct
{
   char *name;
   int typ[SC_CDEF_ARGS];
};
typedef struct _sc_cdef_init_struct sc_cdef_init_struct;

sc_cdef_type sc_cdef_Open(char *grp);
void sc_cdef_Close(sc_cdef_type cd);
int sc_cdef_AddDef(sc_cdef_type cd, char *name, int *typ);
int sc_cdef_GetIndex(sc_cdef_type cd, char *name);
#define sc_cdef_GetEntryByIndex(cd,idx) ((cd)->list+(idx))
#define sc_cdef_GetAttrTypByIndex(cd,idx) (((cd)->list+(idx))->typ)
#define sc_cdef_GetAttrNameByIndex(cd,idx) (((cd)->list+(idx))->name)
#define sc_cdef_GetEntryByName(cd,name) \
   sc_cdef_GetEntryByIndex((cd), sc_cdef_GetIndex((cd), (name)))

#endif
