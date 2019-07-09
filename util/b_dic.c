/*
  
  b_dic.c
  
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

#include <stdlib.h>

#include "b_dic.h"
#include "mwc.h"

int b_dic_dummy_fn(void *el, void *key)
{
  return 0;
}

b_dic_type b_dic_Open()
{
  b_dic_type b_dic;
  b_dic = malloc(sizeof(b_dic_struct));
  if ( b_dic != NULL )
  {
    b_dic->cmp_fn = b_dic_dummy_fn;
    b_dic->pl = b_pl_Open();
    if ( b_dic->pl != NULL )
    {
      return b_dic;
    }
    free(b_dic);
  }
  return NULL;
}

void b_dic_Close(b_dic_type b_dic)
{
  b_pl_Close(b_dic->pl);
  b_dic->pl = NULL;
  free(b_dic);
}

void b_dic_Clear(b_dic_type b_dic)
{
  b_pl_Clear(b_dic->pl);
}

void b_dic_SetCmpFn(b_dic_type b_dic, int (*cmp_fn)(void *el, void *key))
{
  b_dic->cmp_fn = cmp_fn;
}

int b_dic_FindInsPos(b_dic_type b_dic, void *key)
{
  int l,u,m;
  int result;

  if ( b_pl_GetCnt(b_dic->pl) == 0 )
    return 0;

  l = 0;
  u = b_pl_GetCnt(b_dic->pl);
  while(l < u)
  {
    m = (size_t)((l+u)/2);
    result = b_dic->cmp_fn(b_pl_GetVal(b_dic->pl, m), key);
    if ( result < 0 )
       l = m+1;
    else if ( result > 0 )
       u = m;
    else
       return m;
  }
  return u;
}

int b_dic_FindPos(b_dic_type b_dic, void *key)
{
  int pos;
  
  pos = b_dic_FindInsPos(b_dic, key);
  if ( pos >= b_pl_GetCnt(b_dic->pl) )
    return -1;
  if ( b_dic->cmp_fn(b_pl_GetVal(b_dic->pl, pos), key) != 0 )
    return -1;
  for(;;)
  {
    if ( pos == 0 )
      break;
    if ( b_dic->cmp_fn(b_pl_GetVal(b_dic->pl, pos-1), key) != 0 )
      break;
    pos--;
  }
  return pos;
}

/* 0: error */
int b_dic_FindMatchRange(b_dic_type b_dic, void *key, int *pos_p, int *cnt_p)
{
  int pos;
  int cnt;
  pos = b_dic_FindPos(b_dic, key);
  if ( pos < 0 )
    return 0;

  cnt = 1;
  for(;;)
  {
    if ( pos+cnt >= b_dic_GetCnt(b_dic) )
      break;
    if ( b_dic->cmp_fn(b_pl_GetVal(b_dic->pl, pos+cnt), key) != 0 )
      break;
    cnt++;
  }
  
  if ( pos_p != NULL )
    *pos_p = pos;

  if ( cnt_p != NULL )
    *cnt_p = cnt;
  
  return 1;
}


/* 0: error */
int b_dic_Ins(b_dic_type b_dic, void *ptr, void *key)
{
  int ins_pos;
  ins_pos = b_dic_FindInsPos(b_dic, key);
  return b_pl_InsByPos(b_dic->pl, ptr, ins_pos);
}

void *b_dic_Find(b_dic_type b_dic, void *key)
{
  int pos;
  pos = b_dic_FindPos(b_dic, key);
  if ( pos < 0 )
    return NULL;
  return b_pl_GetVal(b_dic->pl, pos);
}

void b_dic_DelByPos(b_dic_type b_dic, int pos)
{
  if ( pos < 0 )
    return;
  b_pl_DelByPos(b_dic->pl, pos);
}

void b_dic_Del(b_dic_type b_dic, void *key)
{
  b_dic_DelByPos(b_dic, b_dic_FindPos(b_dic, key));
}

size_t b_dic_GetMemUsage(b_dic_type b_dic)
{
  size_t m = sizeof(b_dic_struct);
  m += b_pl_GetMemUsage(b_dic->pl);
  return m;
}

