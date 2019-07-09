/*

  srect.c
  
  rectangles for synthesis

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

#include "srect.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "mwc.h"


int srInit(rmatrix rm, srect *r)
{
  r->p_net_ref = -1;
  r->n_net_ref = -1;
  return rInit(rm, &(r->r));
}

void srDestroy(srect *r)
{
  rDestroy(&(r->r));
  r->p_net_ref = -1;
  r->n_net_ref = -1;
}

void srCopy(rmatrix rm, srect *dest, srect *src)
{
  dest->p_net_ref = src->p_net_ref;
  dest->n_net_ref = src->n_net_ref;
  rCopy(rm, &(dest->r), &(src->r));
}

/*---------------------------------------------------------------------------*/

srlist srlOpen(void)
{
  srlist srl;
  srl = (srlist)malloc(sizeof(struct _srlist_struct));
  if ( srl != NULL )
  {
    srl->list = NULL;
    srl->max = 0;
    srl->cnt = 0;
    return srl;
  }
  return NULL;
}

void srlClear(srlist srl)
{
  srl->cnt = 0;
}

srect *srlGet(srlist srl, int pos)
{
  assert(pos < srl->cnt);
  assert(srl->cnt <= srl->max);
  return ((srl)->list+(pos));
}

void srlClose(srlist srl)
{
  int i;
  srlClear(srl);
  for( i = 0; i < srl->max; i++ )
    srDestroy(srl->list+i);
  if ( srl->list != NULL )
    free(srl->list);
  free(srl);
}

int srlExpandTo(rmatrix rm, srlist srl, int max)
{
  void *ptr;
  max = (max+31)&~31;
  if ( max <= srl->max )
    return 1;
  assert(max > srl->max);
  if ( srl->list == NULL )
    ptr = malloc(max*sizeof(srect));
  else                      
    ptr = realloc(srl->list, max*sizeof(srect));
    
  if ( ptr == NULL )
    return 0;
    
  srl->list = (srect *)ptr;
  while( srl->max < max )
  {
    if ( srInit(rm, srl->list+srl->max) == 0 )
      return 0;
    srl->max++;
  }
  assert(max == srl->max);
  return 1;
}

int srlAddEmpty(rmatrix rm, srlist srl)
{
  if ( srlExpandTo(rm, srl, srl->cnt+1) == 0 )
    return -1;
  assert(srl->max > srl->cnt);
  srl->cnt++;
  return srl->cnt-1;
}

int srlFindRect(rmatrix rm, srlist srl, rect *r)
{
  int i, cnt = srlCnt(srl);
  for( i = 0; i < cnt; i++ )
  {
    if ( rIsEqual(rm, &(srlGet(srl, i)->r), r) != 0 )
      return i;
  }
  return -1;
}

int srlAddRect(rmatrix rm, srlist srl, rect *r)
{
  int pos;
  pos = srlFindRect(rm, srl, r);
  if ( pos >= 0 )
    return pos;
  pos = srlAddEmpty(rm, srl);
  if ( pos < 0 )
    return -1;
  rCopy(rm, &(srlGet(srl, pos)->r), r);
  return pos;
}

