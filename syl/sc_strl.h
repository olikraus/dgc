/*

   SC_STRL.H

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

   String List

   16.07.96  Oliver Kraus

*/


#ifndef _SC_STRL_H
#define _SC_STRL_H

#define SC_STRL_EXPAND 64

struct _sc_strl_struct
{
   char **list;
   int cnt;
   int max;
};
typedef struct _sc_strl_struct sc_strl_struct;
typedef struct _sc_strl_struct *sc_strl_type;

#define sc_strl_GetCnt(sl) ((sl)->cnt)
#define sc_strl_Get(sl, no) ((sl)->list[no])

sc_strl_type sc_strl_Open(void);
void sc_strl_Clear(sc_strl_type sl);
void sc_strl_Close(sc_strl_type sl);
int sc_strl_Add(sc_strl_type sl, char *s);
int sc_strl_AddUnique(sc_strl_type sl, const char *s);
int sc_strl_Find(sc_strl_type sl, char *s);

#endif
