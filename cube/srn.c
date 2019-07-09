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

#include "srn.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "mwc.h"



srnode srnOpen(rmatrix rm)
{
  srnode srn;
  srn = (srnode)malloc(sizeof(struct _srnode_struct));
  if ( srn != NULL )
  {
    srn->down = NULL;
    srn->next = NULL;
    srn->ref = -1;
    if ( rInit(rm, &(srn->r)) != 0 )
    {
      return srn;
    }
    free(srn);
  }
  return NULL;
}

void srnClose(srnode srn)
{
  if ( srn == NULL )
    return;
  rDestroy(&(srn->r));
  srnClose(srn->down);
  srnClose(srn->next);
  free(srn);
}

void srnClear(srnode srn)
{
  if ( srn == NULL )
    return;
  srnClose(srn->down);
  srnClose(srn->next);
  srn->down = NULL;
  srn->next = NULL;
}

srnode srnLast(srnode srn)
{
  assert(srn != NULL);
  while ( srn->next != NULL )
    srn = srn->next;
  return srn;
}

void srnAddChild(srnode srn, srnode child)
{
  assert(srn != NULL);
  if ( srn->down == NULL )
    srn->down = child;
  else
    srnLast(srn->down)->next = child;
}


srnode srnAddChildRect(rmatrix rm, srnode srn, rect *r)
{
  /* int pos; */
  srnode child;

  /*
  if ( rCheck(rm, r) == 0 )
    puts("HILFE");
  assert( rCheck(rm, r) != 0 );
  */

  assert( rIsEmpty(rm, r) == 0 );

  child = srnOpen(rm);
  if ( child == NULL )
    return NULL;
  rCopy(rm, &(child->r), r);
  /*
  pos = srlAddRect(rm, srl, r);
  if ( pos < 0 )
  {
    srnClose(child);
    return NULL;
  }
  child->ref = pos;
  */
  srnAddChild(srn, child);
  return child;
}

void _srnShow(rmatrix rm, srnode srn, int depth)
{
  int i;
  if ( srn == NULL )
    return;
  for(i = 0; i < depth*2; i++ )
    printf(" ");
  printf("+%3d ", srn->ref);
  for( i = depth*2+5; i < 20; i++ )
    printf(" ");
  rShowLine(rm, &(srn->r));
  
  _srnShow(rm, srn->down, depth+1);
  _srnShow(rm, srn->next, depth);
}

void srnShow(rmatrix rm, srnode srn)
{
  _srnShow(rm, srn, 0);
}

void _srnGetDepth(srnode srn, int depth, int *max)
{
  if ( srn == NULL )
    return;
  if ( *max < depth )
    *max = depth;
  _srnGetDepth(srn->down, depth+1, max);
  _srnGetDepth(srn->next, depth, max);
}

int srnGetDepth(srnode srn)
{
  int max = 0;
  _srnGetDepth(srn, 1, &max);
  return max;
}

