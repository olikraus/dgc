/*

  b_dic.h

  dictionary

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

*/

#ifndef _B_DIC_H
#define _B_DIC_H

#include "b_pl.h"

struct _b_dic_struct
{
  b_pl_type pl;
  int (*cmp_fn)(void *el, void *key);
};
typedef struct _b_dic_struct b_dic_struct;
typedef struct _b_dic_struct *b_dic_type;

#define b_dic_GetCnt(dic) b_pl_GetCnt((dic)->pl)
#define b_dic_GetVal(dic, pos) b_pl_GetVal((dic)->pl, (pos))

b_dic_type b_dic_Open();
void b_dic_Close(b_dic_type b_dic);
void b_dic_Clear(b_dic_type b_dic);
void b_dic_SetCmpFn(b_dic_type b_dic, int (*cmp_fn)(void *el, void *key));

/* returns 0 for error, will insert duplicates */
int b_dic_Ins(b_dic_type b_dic, void *ptr, void *key);

/* returns 0 for error */
int b_dic_FindMatchRange(b_dic_type b_dic, void *key, int *pos_p, int *cnt_p);

int b_dic_FindPos(b_dic_type b_dic, void *key);
void *b_dic_Find(b_dic_type b_dic, void *key);
void b_dic_Del(b_dic_type b_dic, void *key);
void b_dic_DelByPos(b_dic_type b_dic, int pos);

size_t b_dic_GetMemUsage(b_dic_type b_dic);


#endif
