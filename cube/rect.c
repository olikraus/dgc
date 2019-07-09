/*

  rect.c
  
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

  
  rect->assignment[0]  contains the two bits: 00, 10 or 01
  rect->assignment[1]  contains only the combinations 00 and 10
  
*/

#include "rect.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mwc.h"

/*---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*/

int r_bitcount(r_int x)
{
  register int cnt1 = 0;
  register int cnt2 = 0;
  register int i = sizeof(r_int)*8/4;
  register unsigned a = x;
  register unsigned b = x>>(sizeof(r_int)*8/2);
  register unsigned mask;
  
  mask = 1 + (1<<(sizeof(r_int)*8/4));
  
  while( i > 0 )
  {  
    cnt1 += a&mask;
    cnt2 += b&mask;
    a>>=1;
    b>>=1;
    i--;
  }
  return (cnt1&255)+(cnt2&255)+(cnt1>>(sizeof(r_int)*8/4))+(cnt2>>(sizeof(r_int)*8/4));
}

/*---------------------------------------------------------------------------*/

int rInit(rmatrix rm, rect *r)
{
  int i;

  for( i = 0; i < R_DIMENSIONS; i++ )
    r->assignment[i] = NULL;
    
  for( i = 0; i < R_DIMENSIONS; i++ )
  {
    r->assignment[i] = malloc(rm->word_dim[i]*sizeof(r_int));
    if ( r->assignment[i] == NULL )
      break;
  }
  if ( i == R_DIMENSIONS )
  {
    for( i = 0; i < R_DIMENSIONS; i++ )
    {
      memset(r->assignment[i], 0, rm->word_dim[i]*sizeof(r_int));
      r->cnt[i] = 0;
    }
    return 1;
  }

  for( i = 0; i < R_DIMENSIONS; i++ )
    if ( r->assignment[i] != NULL )
      free(r->assignment[i]);
  
  return 0;
}

rect *rOpen(rmatrix rm)
{
  rect *r;
  r = malloc(sizeof(rect));
  if ( r != NULL )
  {
    if ( rInit(rm, r) != 0 )
    {
      return r;
    }
    free(r);
  }
  return NULL;
}

void rClearDim(rmatrix rm, int d, rect *r)
{
  memset(r->assignment[d], 0, rm->word_dim[d]*sizeof(r_int));
  r->cnt[d] = 0;
}

void rClear(rmatrix rm, rect *r)
{
  int i;
  for( i = 0; i < R_DIMENSIONS; i++ )
    rClearDim(rm, i, r);
}

void rDestroy(rect *r)
{
  int i;
  for( i = 0; i < R_DIMENSIONS; i++ )
  {
    free(r->assignment[i]);
    r->assignment[i] = NULL;
    r->cnt[i] = -1;
  }
}

void rClose(rect *r)
{
  rDestroy(r);
  free(r);
}

/* d = not a and b */
void rNotAndDim(rmatrix rm, int d, rect *r, rect *a, rect *b)
{
  int j;
  for( j = 0; j < rm->word_dim[d]; j++ )
    r->assignment[d][j] = (~a->assignment[d][j] & b->assignment[d][j]);
}

void rNotAnd(rmatrix rm, rect *r, rect *a, rect *b)
{
  int i;
  for( i = 0; i < R_DIMENSIONS; i++ )
    rNotAndDim(rm, i, r, a, b);
}

/* d = a or b */
void rOrDim(rmatrix rm, int d, rect *r, rect *a, rect *b)
{
  int j;
  for( j = 0; j < rm->word_dim[d]; j++ )
    r->assignment[d][j] = (a->assignment[d][j] | b->assignment[d][j]);
}

void rOr(rmatrix rm, rect *r, rect *a, rect *b)
{
  int i;
  for( i = 0; i < R_DIMENSIONS; i++ )
    rOrDim(rm, i, r, a, b);
}

/* d = a and b */
void rAndDim(rmatrix rm, int d, rect *r, rect *a, rect *b)
{
  int j;
  for( j = 0; j < rm->word_dim[d]; j++ )
    r->assignment[d][j] = (a->assignment[d][j] & b->assignment[d][j]);
}

void rAnd(rmatrix rm, rect *r, rect *a, rect *b)
{
  int i;
  for( i = 0; i < R_DIMENSIONS; i++ )
    rAndDim(rm, i, r, a, b);
}

int rIsIntersectionDim(rmatrix rm, int d, rect *a, rect *b)
{
  int j;
  for( j = 0; j < rm->word_dim[d]; j++ )
    if ( (a->assignment[d][j] & b->assignment[d][j]) != 0 )
      return 1;
  return 0;
}

int rRealCntDim(rmatrix rm, int d, rect *r)
{
  int j;
  int b = 0;
  for( j = 0; j < rm->word_dim[d]; j++ )
    b += r_bitcount(r->assignment[d][j]);
  return b;
}

/* Ist eine richtung von b Teilmenge der richtung von a? */
int rIsSubSetDim(rmatrix rm, int d, rect *a, rect *b)
{
  int j;
  for( j = 0; j < rm->word_dim[d]; j++ )
    if ( (a->assignment[d][j] & b->assignment[d][j]) != b->assignment[d][j] )
      return 0;
  return 1;
}

/* Ist b Teilmenge von a?                     */
/* Ja: Rueckgabe ist 1, nein: Rueckgabe ist 0 */
int rIsSubSet(rmatrix rm, rect *a, rect *b)
{ 
  int i, j;
  for( i = 0; i < R_DIMENSIONS; i++ )
    for( j = 0; j < rm->word_dim[i]; j++ )
      if ( (a->assignment[i][j] & b->assignment[i][j]) != b->assignment[i][j] )
        return 0;
  return 1;
}

int rIsEqual(rmatrix rm, rect *a, rect *b)
{
  int j;

  for( j = 0; j < rm->word_dim[0]; j++ )
    if ( a->assignment[0][j] != b->assignment[0][j] )
      return 0;
  for( j = 0; j < rm->word_dim[1]; j++ )
    if ( a->assignment[1][j] != b->assignment[1][j] )
      return 0;
  return 1;
}

void rCopyDim(rmatrix rm, int d, rect *dest, rect *src)
{
  memcpy(dest->assignment[d], src->assignment[d], rm->word_dim[d]*sizeof(r_int));
  dest->cnt[d] = src->cnt[d];
}

void rCopy(rmatrix rm, rect *dest, rect *src)
{
  int i;
  for( i = 0; i < R_DIMENSIONS; i++ )
  {
    memcpy(dest->assignment[i], src->assignment[i], rm->word_dim[i]*sizeof(r_int));
    dest->cnt[i] = src->cnt[i];
  }
}

int rGet(rect *r, int dim, int val)
{
  return (r->assignment[dim][val/R_SYM_PER_WORD]>>( (val % R_SYM_PER_WORD)*2 )) & 3;
}

void rSet(rect *r, int dim, int val, int symbol)
{
  int old_symbol = rGet(r, dim, val);
  r->assignment[dim][val/R_SYM_PER_WORD] &= ~(3 << ( (val % R_SYM_PER_WORD)*2 ));
  r->assignment[dim][val/R_SYM_PER_WORD] |= symbol << ( (val % R_SYM_PER_WORD)*2 );
  if ( old_symbol != 0 )
    r->cnt[dim]--;

  if ( symbol != 0 )
    r->cnt[dim]++;
}

void rFillDim(int dim, rect *r, int from, int to, int symbol)
{
  for(; from < to; from++ )
    rSet(r, dim, from, symbol);
}

void rFill(rect *r, int x1, int x2, int y1, int y2, int symbol)
{
  rFillDim(0, r, x1, x2, symbol);
  rFillDim(1, r, y1, y2, symbol);
}

int rCheck(rmatrix rm, rect *r)
{
  int i, j;
  int cnt;
  for( i = 0; i < R_DIMENSIONS; i++ )
  {
    cnt = 0;
    for( j = 0; j < rm->dim[i]; j++ )
    {
      if ( rGet(r, i, j) != 0 )
        cnt++;
    }
    if ( r->cnt[i] != cnt )
    {
      printf("rCheck failed: ");
      rShowLine(rm, r);
      return 0;
    }
  }
  return 1;
}

int rIsEmpty(rmatrix rm, rect *r)
{
  int i, j;
  int cnt;
  for( i = 0; i < R_DIMENSIONS; i++ )
  {
    cnt = 0;
    for( j = 0; j < rm->dim[i]; j++ )
    {
      if ( rGet(r, i, j) != 0 )
        cnt++;
    }
    if ( cnt == 0 )
      return 1;
  }
  return 0;
  
}

void rShowDimLine(rmatrix rm, rect *r, int dim)
{
  int i;
  for( i = 0; i < rm->dim[dim]; i++ )
    printf("%c", ".01*"[rGet(r, dim, i)]);
}

void rShowLine(rmatrix rm, rect *r)
{
  int i;
  for( i = 0; i < R_DIMENSIONS; i++ )
  {
    rShowDimLine(rm, r, i);
    if ( i < R_DIMENSIONS-1 )
      printf(" | ");
  }
  puts(" rect");
}

/*---------------------------------------------------------------------------*/

rlist rlOpen(void)
{
  rlist rl;
  rl = (rlist)malloc(sizeof(struct _rlist_struct));
  if ( rl != NULL )
  {
    rl->list = NULL;
    rl->max = 0;
    rl->cnt = 0;
    return rl;
  }
  return NULL;
}

void rlClear(rlist rl)
{
  /*
  int i, cnt = rlCnt(rl);
  for( i = 0; i < cnt; i++ )
    rClear(rm, rlGet(rl,i));
  */
  rl->cnt = 0;
}

void rlClose(rlist rl)
{
  int i;
  rlClear(rl);
  for( i = 0; i < rl->max; i++ )
    rDestroy(rl->list+i);
  if ( rl->list != NULL )
    free(rl->list);
  free(rl);
}

int rlExpandTo(rmatrix rm, rlist rl, int max)
{
  void *ptr;
  max = (max+31)&~31;
  if ( max <= rl->max )
    return 1;
  if ( rl->list == NULL )   
    ptr = malloc(max*sizeof(rect));
  else                      
    ptr = realloc(rl->list, max*sizeof(rect));
    
  if ( ptr == NULL )
    return 0;
    
  rl->list = (rect *)ptr;
  while( rl->max < max )
  {
    if ( rInit(rm, rl->list+rl->max) == 0 )
      return 0;
    assert((rl->list+rl->max)->assignment[0][0] == 0);
    assert((rl->list+rl->max)->assignment[1][0] == 0);
    rl->max++;
  }
  rl->max = max;
  return 1;
}

int rlAddEmpty(rmatrix rm, rlist rl)
{
  if ( rlExpandTo(rm, rl, rl->cnt+1) == 0 )
    return -1;
  assert(rl->max > rl->cnt);
  rl->cnt++;
  rClear(rm, rl->list+rl->cnt-1);
  return rl->cnt-1;
}

int rlAdd(rmatrix rm, rlist rl, rect *r)
{
  if ( rlExpandTo(rm, rl, rl->cnt+1) == 0 )
    return -1;
  assert(rl->max > rl->cnt);
  rl->cnt++;
  rCopy(rm, rl->list+rl->cnt-1, r);
  return rl->cnt-1;
}

void rlDel(rmatrix rm, rlist rl, int pos)
{
  if ( rl->cnt == 0 || pos >= rl->cnt || pos < 0 )
    return;
  while( pos < rl->cnt-1 )
  {
    rCopy(rm, rl->list+pos, rl->list+pos+1);
    pos++;
  }
  rl->cnt--;
}

int rlSRCAdd(rmatrix rm, rlist rl, rect *r)
{
  int i, cnt = rlCnt(rl);
  for( i = 0; i < cnt; i++ )
    if ( rIsSubSet(rm, rlGet(rl, i), r) != 0 )
      return 1;
  if ( rlAdd(rm, rl, r) < 0 )
    return 0;
  return 1;
}

void rlSet(rmatrix rm, rlist rl, int pos, rect *r)
{
  assert(pos < rl->cnt);
  rCopy(rm, rl->list+pos, r);
}

int rlCopy(rmatrix rm, rlist dest, rlist src)
{
  int i, cnt = rlCnt(src);
  rlClear(dest);
  for( i = 0; i < cnt; i++ )
    if ( rlAdd(rm, dest, rlGet(src, i)) < 0 )
      return 0;
  return 1;
}

int rlIsDisjunctY(rmatrix rm, rlist rl, int pos)
{
  int i, cnt = rlCnt(rl);
  for( i = 0; i < cnt; i++ )
  {
    if ( i != pos )
      if ( rIsIntersectionDim(rm, 1, rlGet(rl, i), rlGet(rl, pos)) != 0 )
        return 0;
  }
  return 1;
}

int rlFindMaxRectangle(rmatrix rm, rlist rl)
{
  int i, pos;
  int curr, max;
  int c1, c2;
  max = -1;
  pos = -1;
  for( i = 0; i < rlCnt(rl); i++ )
  {
    c1 = rRealCntDim(rm, 0, rlGet(rl, i));
    c2 = rRealCntDim(rm, 1, rlGet(rl, i));
    curr = c1*c2;
    if ( max < curr )
    {
      max = curr;
      pos = i;
    }
  }
  return pos;
}

int rlDisjunctY(rmatrix rm, rlist rl)
{
  rlist es, ne;
  int i;
  int pos;
  
  es = rlOpen();
  if ( es == NULL )
    return 0;
  ne = rlOpen();
  if ( ne == NULL )
    return rlClose(es), 0;
   
  for( i = 0; i < rlCnt(rl); i++ )
    if ( rlIsDisjunctY(rm, rl, i) != 0 )
    {
      if ( rlAdd(rm, es, rlGet(rl, i)) < 0 )
        return rlClose(es), rlClose(ne), 0;
    }
    else
    {
      if ( rlAdd(rm, ne, rlGet(rl, i)) < 0 )
        return rlClose(es), rlClose(ne), 0;
    }
  
  while( rlCnt(ne) != 0 )
  {
    pos = rlFindMaxRectangle(rm, ne);
    if ( rRealCntDim(rm, 1, rlGet(ne, pos)) >= 2 )
    {
      for( i = 0; i < rlCnt(ne); i++ )
      {
        if ( i != pos )
        {
          rNotAndDim(rm, 1, rlGet(ne, i), rlGet(ne, pos), rlGet(ne, i));
        }
      }
      if ( rlAdd(rm, es, rlGet(ne, pos)) < 0 )
        return rlClose(es), rlClose(ne), 0;
    }
    
    rlDel(rm, ne, pos);
  }
  
  if ( rlCopy(rm, rl, es) == 0 )
    return rlClose(es), rlClose(ne), 0;
  
  return rlClose(es), rlClose(ne), 1;
}

/*---------------------------------------------------------------------------*/

void lrlClear(lrlist lrl)
{
  rlClear(lrl->rl);
  lrl->pos_min = -1;
  lrl->pos_max = -1;
}

/* die liste wird nur geloescht, wenn das neue limit kleiner ist, */
/* als das alte */
void lrlSetLimit(lrlist lrl, int limit)
{
  assert(limit > 0);
  if ( lrl->limit > limit)
    lrlClear(lrl);
  lrl->limit = limit;
}

lrlist lrlOpen()
{
  lrlist lrl;
  
  lrl = (lrlist)malloc(sizeof(struct _lrlist_struct));
  if ( lrl != NULL )
  {
    lrl->limit = 200;
    lrl->pos_min = -1;
    lrl->pos_max = -1;
    lrl->rl = rlOpen();
    if ( lrl->rl != NULL )
    {
      return lrl;
    }
    free(lrl);
  }
  return NULL;
}

void lrlClose(lrlist lrl)
{
  lrlClear(lrl);
  rlClose(lrl->rl);
  lrl->rl = NULL;
  free(lrl);
}

int lrl_cost(lrlist lrl, int c1, int c2)
{
  return c1*c2;
}

int lrlFindMinimumPos(rmatrix rm, lrlist lrl)
{
  int min = lrl_cost(lrl, rm->dim[0], rm->dim[1])+1;
  int pos = -1;
  int cost;
  int i, cnt = rlCnt(lrl->rl);
  rect *r;
  for( i = 0; i < cnt; i++ )
  {
    r = rlGet(lrl->rl, i);
    cost = lrl_cost(lrl, r->cnt[0], r->cnt[1]);
    if ( min > cost )
    {
      min = cost;
      pos = i;
    }
  }
  return pos;
}

int lrlFindMaximumPos(rmatrix rm, lrlist lrl)
{
  int max = -1;
  int pos = -1;
  int i, cnt = rlCnt(lrl->rl);
  rect *r;
  for( i = 0; i < cnt; i++ )
  {
    r = rlGet(lrl->rl, i);
    if ( max < lrl_cost(lrl, r->cnt[0], r->cnt[1]) )
    {
      max = lrl_cost(lrl, r->cnt[0], r->cnt[1]);
      pos = i;
    }
  }
  return pos;
}


int lrlIsSubSetElement(rmatrix rm, lrlist lrl, rect *r)
{
  int i, cnt = rlCnt(lrl->rl);
  for( i = 0; i < cnt; i++ )
    if ( rIsSubSet(rm, rlGet(lrl->rl, i), r) != 0 )
      return 1;
  return 0;
}

int lrlIsEqualElement(rmatrix rm, lrlist lrl, rect *r)
{
  int i, cnt = rlCnt(lrl->rl);
  for( i = 0; i < cnt; i++ )
    if ( rIsEqual(rm, rlGet(lrl->rl, i), r) != 0 )
      return 1;
  return 0;
}

void lrlDel(rmatrix rm, lrlist lrl, int pos)
{
  rlDel(rm, lrl->rl, pos);
  if ( lrl->pos_min == pos )
    lrl->pos_min = lrlFindMinimumPos(rm, lrl);
  else if (lrl->pos_min > pos)
    lrl->pos_min--;
  if ( lrl->pos_max == pos )
    lrl->pos_max = lrlFindMaximumPos(rm, lrl);
  else if (lrl->pos_max > pos)
    lrl->pos_max--;
}

/* returns 0 for error */
int lrlPut(rmatrix rm, lrlist lrl, rect *r)
{
  int pos;
  rect *r_min;
    
  assert(lrl->pos_min < rlCnt(lrl->rl));
  assert(lrl->pos_max < rlCnt(lrl->rl));

  /* assert( rCheck(rm, r) != 0 ); */
    
  if ( rlCnt(lrl->rl) < lrl->limit )
  {
    /* add the element if there es more room available */
  
    pos = rlAdd(rm, lrl->rl, r);
    if ( pos < 0 )
      return 0;
    assert(pos < rlCnt(lrl->rl));
  }
  else
  {

    /* do not add something, if the size is lower than the current minimum */
    if ( lrl->pos_min >= 0 )
    {
      r_min = rlGet(lrl->rl, lrl->pos_min);
      if ( lrl_cost(lrl, r_min->cnt[0], r_min->cnt[1]) >= lrl_cost(lrl, r->cnt[0], r->cnt[1]) )
        return 1;
    }

    /* else replace the lowest element */
    
    assert(lrl->pos_min >= 0);
    rlSet(rm, lrl->rl, lrl->pos_min, r);
    pos = lrl->pos_min;
    
    assert(pos < rlCnt(lrl->rl));
  }
  
  /* find the new minimum, update the index */
  
  lrl->pos_min = lrlFindMinimumPos(rm, lrl);


  /* update the maximum */

  assert(pos < rlCnt(lrl->rl));
  if ( lrl->pos_max < 0 )
  {
    lrl->pos_max = pos;
  }
  else
  {
    rect *r_max;
    r_max = rlGet(lrl->rl, lrl->pos_max);
    if ( lrl_cost(lrl, r_max->cnt[0], r_max->cnt[1]) < lrl_cost(lrl, r->cnt[0], r->cnt[1]) )
      lrl->pos_max = pos;
  }

  assert(lrl->pos_min < rlCnt(lrl->rl));
  assert(lrl->pos_max < rlCnt(lrl->rl));
  
  return 1;
}


int lrlCopy(rmatrix rm, lrlist dest, lrlist src)
{
  int i, cnt = lrlCnt(src);
  lrlClear(dest);
  for( i = 0; i < cnt; i++ )
    if ( lrlPut(rm, dest, lrlGet(src, i)) == 0 )
      return 0;
  return 1;
}


int lrlShow(rmatrix rm, lrlist lrl)
{
  int i, cnt = lrlCnt(lrl);
  rect *r;
  
  for( i = 0; i < cnt; i++ )
  {
    r = lrlGet(lrl, i);
    printf("%05d ", i);
    if ( i == lrl->pos_min )
      printf("<");
    else if ( i == lrl->pos_max )
      printf(">");
    else
      printf(" ");
    printf("%03d ", lrl_cost(lrl, r->cnt[0], r->cnt[1]));
    rShowLine(rm, r);
  }
  return 1;
}



void lrlDelLowerEqualWidth(rmatrix rm, lrlist lrl, int width)
{
  int src, dest;
  src = 0;
  dest = 0;
  while( src < rlCnt(lrl->rl) )
  {
    if ( rRealCntDim(rm, 0, rlGet(lrl->rl, src)) <= width )
    {
      src++;
    }
    else
    {
      if ( src != dest )
        rCopy(rm, rlGet(lrl->rl, dest), rlGet(lrl->rl, src));
      src++;
      dest++;
    }
  }
  lrl->rl->cnt = dest;  /* ohh dear... a hack */

  lrl->pos_min = lrlFindMinimumPos(rm, lrl);
  lrl->pos_max = lrlFindMaximumPos(rm, lrl);
}

void lrlDelLowerEqualHeight(rmatrix rm, lrlist lrl, int height)
{
  int src, dest;
  src = 0;
  dest = 0;
  while( src < rlCnt(lrl->rl) )
  {
    if ( rRealCntDim(rm, 1, rlGet(lrl->rl, src)) <= height )
    {
      src++;
    }
    else
    {
      if ( src != dest )
        rCopy(rm, rlGet(lrl->rl, dest), rlGet(lrl->rl, src));
      src++;
      dest++;
    }
  }
  lrl->rl->cnt = dest;  /* ohh dear... a hack */

  lrl->pos_min = lrlFindMinimumPos(rm, lrl);
  lrl->pos_max = lrlFindMaximumPos(rm, lrl);
}


int lrlDisjunctY(rmatrix rm, lrlist lrl)
{
  if ( rlDisjunctY(rm, lrl->rl) == 0 )
    return 0;
  lrl->pos_min = lrlFindMinimumPos(rm, lrl);
  lrl->pos_max = lrlFindMaximumPos(rm, lrl);
  return 1;
}

/*---------------------------------------------------------------------------*/

void rmClear(rmatrix rm)
{
  int i;
  for( i = 0; i < rm->word_dim[0]*rm->dim[1]; i++ )
    rm->xy[0][i] = 0;
  for( i = 0; i < rm->word_dim[1]*rm->dim[0]; i++ )
    rm->xy[1][i] = 0;
}


rmatrix rmOpen(int width, int height)
{
  rmatrix rm;
  rm = (rmatrix)malloc(sizeof(struct _rmatrix_struct));
  if ( rm != NULL )
  {
    rm->dim[0] = width;
    rm->dim[1] = height;
    rm->word_dim[0] = (rm->dim[0]+R_SYM_PER_WORD-1)/R_SYM_PER_WORD;
    rm->word_dim[1] = (rm->dim[1]+R_SYM_PER_WORD-1)/R_SYM_PER_WORD;
    rm->xy[0] = (r_int *)malloc(rm->word_dim[0]*rm->dim[1]*sizeof(r_int));
    if ( rm->xy[0] != NULL )
    {
      rm->xy[1] = (r_int *)malloc(rm->word_dim[1]*rm->dim[0]*sizeof(r_int));
      if ( rm->xy[1] != NULL )
      {
        if ( rInit(rm, &(rm->buffer)) != 0 )
        {
          rmClear(rm);
          return rm;
        }
        free(rm->xy[1]);
      }
      free(rm->xy[0]);
    }
    free(rm);
  }
  return NULL;
}

void rmClose(rmatrix rm)
{
  rDestroy(&(rm->buffer));
  free(rm->xy[0]);
  free(rm->xy[1]);
  free(rm);
}

void rm_set_xy(rmatrix rm, int x, int y, int value)
{
  r_int *ptr;
  assert(x>=0&&x<rm->dim[0]);
  assert(y>=0&&y<rm->dim[1]);
  ptr = rm->xy[0] + y*rm->word_dim[0] + x/R_SYM_PER_WORD;
  *ptr &= ~(((r_int)3) << ((x % R_SYM_PER_WORD)*2));
  *ptr |= ((r_int)value) << ((x % R_SYM_PER_WORD)*2);
  assert(rm_get_xy(rm, x, y) == value);
}

void rm_set_yx(rmatrix rm, int x, int y, int value)
{
  r_int *ptr;
  assert(x>=0&&x<rm->dim[0]);
  assert(y>=0&&y<rm->dim[1]);
  ptr = rm->xy[1] + x*rm->word_dim[1] + y/R_SYM_PER_WORD;
  *ptr &= ~(((r_int)3) << ((y % R_SYM_PER_WORD)*2));
  *ptr |= ((r_int)value) << ((y % R_SYM_PER_WORD)*2);
  assert(rm_get_yx(rm, x, y) == value);
}

int rm_get_xy(rmatrix rm, int x, int y)
{
  return 
    (rm->xy[0][y*rm->word_dim[0]+x/R_SYM_PER_WORD]>>((x%R_SYM_PER_WORD)*2))&3;
}

int rm_get_yx(rmatrix rm, int x, int y)
{
  return 
    (rm->xy[1][x*rm->word_dim[1]+y/R_SYM_PER_WORD]>>((y%R_SYM_PER_WORD)*2))&3;
}

void rmFillByCL(rmatrix rm, pinfo *pi, dclist cl)
{
  int x, y;
  int symbol;
  
  if ( dclCnt(cl) != rm->dim[1] )
    return;
  if ( pi->in_cnt + pi->out_cnt  != rm->dim[0] )
    return;
  
  rmClear(rm);
  for( y = 0; y < rm->dim[1]; y++ )
  {
    for( x = 0; x < rm->dim[0]; x++ )
    {
      if ( x < pi->in_cnt )
      {
        symbol = dcGetIn(dclGet(cl, y), x);
        if ( symbol == 1 || symbol == 2 )
        {
          rm_set_xy(rm, x, y, symbol);
          rm_set_yx(rm, x, y, symbol);
          assert(rm_get_xy(rm, x, y) == rm_get_yx(rm, x, y));
        }
      }
      else
      {
        if ( dcGetOut(dclGet(cl, y), x - pi->in_cnt) != 0 )
        {
          rm_set_xy(rm, x, y, 2);
          rm_set_yx(rm, x, y, 2);
          assert(rm_get_xy(rm, x, y) == rm_get_yx(rm, x, y));
        }
      }
    }
  }
}

void rmShow(rmatrix rm)
{
  int x, y;
  int symbol;
  
  for( y = 0; y < rm->dim[1]; y++ )
  {
    for( x = 0; x < rm->dim[0]; x++ )
    {
      symbol = rm_get_xy(rm, x, y);
      assert(rm_get_xy(rm, x, y) == rm_get_yx(rm, x, y));
      printf("%c", ".01*"[symbol]);
    }
    printf("\n");
  }  
}

/*
  rueckgabe:
    0: keine spalte gefunden
    1: 01 spalte gefunden
    2: 10 spalte gefunden
    
    0 <= x < rm->dim[0]
*/
int rmIsExtendRectCol(rmatrix rm, rect *r, int x)
{
  r_int c = 0;
  r_int *m_ptr;
  r_int *d_ptr;
  int i;

  assert(x >= 0);
  assert(x < rm->dim[0]);
  
  /* m_ptr = rm->xy[1] + x*rm->word_dim[1]; */
  m_ptr = rmGetCol(rm, x);
  d_ptr = r->assignment[1];
  
  for( i = 0; i < rm->word_dim[1]; i++ )
    if ( (m_ptr[i] & d_ptr[i]) != d_ptr[i] )
      break;
  if ( i == rm->word_dim[1] )
    return 2;  /* '10' spalte gefunden */
  
  for( i = 0; i < rm->word_dim[1]; i++ )
  {
    c = d_ptr[i] >> 1;
    if ( (m_ptr[i] & c) != c )
      break;
  }
  if ( i == rm->word_dim[1] )
    return 1;  /* '01' spalte gefunden */
    
  return 0;
}

/*
  die zeile des rechtecks enthaelt
    01 '01' spalte
    10 '10' spalte
    00 fuer don't care
  return 0: kein match
         2: match erfolgreich

  0 <= y < rm->dim[1]
*/
int rmIsExtendRectRow(rmatrix rm, rect *r, int y)
{
  r_int *m_ptr;
  r_int *d_ptr;
  int i;

  assert(y >= 0);
  assert(y < rm->dim[1]);
  
  /* m_ptr = rm->xy[0] + y*rm->word_dim[0]; */
  m_ptr = rmGetRow(rm, y);
  d_ptr = r->assignment[0];
  
  for( i = 0; i < rm->word_dim[0]; i++ )
    if ( (m_ptr[i] & d_ptr[i]) != d_ptr[i] )
      break;
  if ( i == rm->word_dim[0] )
    return 2;
  return 0;
}

void rmExtendToMaxHeight(rmatrix rm, rect *r)
{
  int i, cnt = rm->dim[1];
  for( i = 0; i < cnt; i++ )
  {
    if ( rGet(r, 1, i) == 0 )
    {
      if ( rmIsExtendRectRow(rm, r, i) != 0 )
        rSet(r, 1, i, 2);
    }
  }
}

void rmExtendToMaxHeightArea(rmatrix rm, rect *r, rect *area)
{
  int i, cnt = rm->dim[1];
  for( i = 0; i < cnt; i++ )
  {
    if ( rGet(r, 1, i) == 0 && rGet(area, 1, i) != 0 )
    {
      if ( rmIsExtendRectRow(rm, r, i) != 0 )
        rSet(r, 1, i, 2);
    }
  }
}

int rmIsMaxHeight(rmatrix rm, rect *r)
{
  int i;
  for( i = 0; i < rm->dim[1]; i++ )
    if ( rGet(r, 1, i) == 0 )
      if ( rmIsExtendRectRow(rm, r, i) != 0 )
        return 0;
  return 1;
}

int rmIsMaxHeightArea(rmatrix rm, rect *r, rect *area)
{
  int i;
  for( i = 0; i < rm->dim[1]; i++ )
    if ( rGet(r, 1, i) == 0 && rGet(area, 1, i) != 0 )
      if ( rmIsExtendRectRow(rm, r, i) != 0 )
        return 0;
  return 1;
}

void rmExtendToMaxWidth(rmatrix rm, rect *r)
{
  int i, cnt = rm->dim[0];
  int symbol;
  for( i = 0; i < cnt; i++ )
  {
    if ( rGet(r, 0, i) == 0 )
    {
      symbol = rmIsExtendRectCol(rm, r, i);
      if ( symbol != 0 )
        rSet(r, 0, i, symbol);
    }
  }
}

void rmExtendToMaxWidthArea(rmatrix rm, rect *r, rect *area)
{
  int i, cnt = rm->dim[0];
  int symbol;
  for( i = 0; i < cnt; i++ )
  {
    if ( rGet(r, 0, i) == 0 && rGet(area, 0, i) != 0 )
    {
      symbol = rmIsExtendRectCol(rm, r, i);
      if ( symbol != 0 )
        rSet(r, 0, i, symbol);
    }
  }
}

void rmExtendToMaxWidthRange(rmatrix rm, rect *r, int low_x, int high_x)
{
  int i;
  int symbol;
  for( i = low_x; i < high_x; i++ )
  {
    if ( rGet(r, 0, i) == 0 )
    {
      symbol = rmIsExtendRectCol(rm, r, i);
      if ( symbol != 0 )
        rSet(r, 0, i, symbol);
    }
  }
}

int rmIsMaxWidth(rmatrix rm, rect *r)
{
  int i;
  for( i = 0; i < rm->dim[0]; i++ )
    if ( rGet(r, 0, i) == 0 )
      if ( rmIsExtendRectCol(rm, r, i) != 0 )
        return 0;
  return 1;
}

int rmIsMaxWidthArea(rmatrix rm, rect *r, rect *area)
{
  int i;
  for( i = 0; i < rm->dim[0]; i++ )
    if ( rGet(r, 0, i) == 0 && rGet(area, 0, i) != 0 )
      if ( rmIsExtendRectCol(rm, r, i) != 0 )
        return 0;
  return 1;
}

int rmIsMaxWidthRange(rmatrix rm, rect *r, int low_x, int high_x)
{
  int i;
  for( i = low_x; i < high_x; i++ )
    if ( rGet(r, 0, i) == 0 )
      if ( rmIsExtendRectCol(rm, r, i) != 0 )
        return 0;
  return 1;
}




/*
  Diese funktion geht davon aus, dass 'r' in x und y
  richtung erweitert werden kann. Es werden alle diejenigen
  rechtecke an 'lrl' angehaengt die in beide Richtungen
  erweitert werden koennen.
  Rechtecke, von denen klar ist, dass sie prime sind, werden
  an 'primes' angehaengt. 'lrl' und 'primes' koennen identisch
  sein.
*/
int rmExtendXY(rmatrix rm, lrlist lrl, lrlist primes, rect *r, rect *area)
{
  int i, j;
  int s_i, s_j;
  rect rr;
  
  rInit(rm, &rr);


  /* Versuche beide richtungen gleichzeitig */

  for( i = 0; i < rm->dim[1]; i++ )
  {
    if ( rGet(r, 1, i) == 0 && rGet(area, 1, i) != 0 )
    {
      s_i = rmIsExtendRectRow(rm, r, i);
      if ( s_i != 0 )
      {
        rSet(r, 1, i, s_i);
        for( j = 0; j < rm->dim[0]; j++ )
        {
          if ( rGet(r, 0, j) == 0 && rGet(area, 0, j) != 0 )
          {
            s_j = rmIsExtendRectCol(rm, r, j);
            if ( s_j != 0 )
            {
              rSet(r, 0, j, s_j);
              if ( lrlIsEqualElement(rm, lrl, r) == 0 )
                if ( lrlPut(rm, lrl, r) == 0 )
                  return rDestroy(&rr), 0;
              rSet(r, 0, j, 0);
            }
          }
        }
        rSet(r, 1, i, 0);
      }
    }
  }

  rCopy(rm, &rr, r);
  rmExtendToMaxHeightArea(rm, &rr, area);
  if ( rr.cnt[1] > 1 )
    if ( rmIsMaxWidthArea(rm, &rr, area) != 0 )
      if ( lrlIsEqualElement(rm, primes, &rr) == 0 )
        if ( lrlPut(rm, primes, &rr) == 0 )
          return rDestroy(&rr), 0;

  if ( r->cnt[1] > 1 )
  {
    rCopy(rm, &rr, r);
    rmExtendToMaxWidthArea(rm, &rr, area);
    if ( rmIsMaxHeightArea(rm, &rr, area) != 0 )
      if ( lrlIsEqualElement(rm, primes, &rr) == 0 )
        if ( lrlPut(rm, primes, &rr) == 0 )
          return rDestroy(&rr), 0;
  }
            
  return rDestroy(&rr), 1;
}

int rmExtendList(rmatrix rm, lrlist dest, lrlist src, rect *area)
{
  int loop_cnt = 0;
  int i, cnt;
  lrlist primes;
  
  primes = lrlOpen();
  if ( primes == NULL )
    return 0;
    
  lrlClear(dest);
  
  while( lrlCnt(src) > 0 )
  {
    /*
    printf("loop %d  primes %d  cnt %d \r", loop_cnt, lrlCnt(dest), lrlCnt(src)); 
    fflush(stdout);
    */
    
    lrlClear(dest);
    cnt = lrlCnt(src);
    for( i = 0; i < cnt; i++ )
    {
      if ( rmExtendXY(rm, dest, primes, lrlGet(src, i), area) == 0 )
        return lrlClose(primes), 0;
    }
    lrlCopy(rm, src, dest);
    loop_cnt++;
  }

  lrlCopy(rm, dest, primes);

  /*
  printf("Loops: %d (Primes: %d)              \n", loop_cnt, lrlCnt(primes));
  */
  return lrlClose(primes), 1;
}

/*
  Versucht einige rechtecke zu finden die die angegebene
  spalte beinhalten. Ergebnisse werden an lrl angehaengt.
  Die Anzahl der ergebnisse wird durch das limit von lrl
  bestimmt und durch die gesamtzahl an cubes pro ausgang.
*/
int rmFindInitialList(rmatrix rm, lrlist lrl, int col, rlist exclude)
{
  int i, j;
  int s;
  rect r;
  
  if ( rInit(rm, &r) == 0 )
    return 0;
  
  /* put all non-null elements into the list */  
  
  assert(col < rm->dim[0]);
  for( i = 0; i < rm->dim[1]; i++ )
  {
    s = rm_get_xy(rm, col, i);
    if ( s != 0 )
    {
      if ( exclude != NULL )
        for( j = 0; j < rlCnt(exclude); j++ )
          if ( rGet(rlGet(exclude, j), 0, col) != 0 )
            if ( rGet(rlGet(exclude, j), 1, i) != 0 )
            {
              puts("xxx");
              continue;
            }
    
      rClear(rm, &r);
      rSet(&r, 0, col, s);
      rSet(&r, 1, i, 2);
      rmExtendToMaxWidth(rm, &r);
      if ( lrlPut(rm, lrl, &r) == 0 )
        return rDestroy(&r), 0;
    }
  }
  
  /* restrict the elements to the requested column */
  
  for( i = 0; i < lrlCnt(lrl); i++ )
  {
    s = rGet(lrlGet(lrl, i), 0, col);
    rClearDim(rm, 0, lrlGet(lrl, i));
    rSet(lrlGet(lrl, i), 0, col, s);
  }
  
  return rDestroy(&r), 1;
}

void rmFillWithCol(rmatrix rm, rect *r, int col, int s)
{
  int i;
  rClear(rm, r);
  
  assert(col < rm->dim[0]);
  for( i = 0; i < rm->dim[1]; i++ )
  {
    if ( rm_get_xy(rm, col, i) == s )
    {
      rSet(r, 0, col, s);
      rSet(r, 1, i,   2);
    }
  }
}


int rmFindInitialListArea(rmatrix rm, lrlist lrl, rect *area)
{
  int i, j;
  int s;
  rect r;
  
  if ( rInit(rm, &r) == 0 )
    return 0;
  
  for( i = 0; i < rm->dim[1]; i++ )
  {
    if ( rGet(area, 1, i) != 0 )
    {
      for( j = 0; j < rm->dim[0]; j++ )
      {
        if ( rGet(area, 0, j) != 0 )
        {
          s = rm_get_xy(rm, j, i);
          if ( s != 0 )
          {
            rClear(rm, &r);
            rSet(&r, 0, j, s);
            rSet(&r, 1, i, 2);
            
            if ( lrlPut(rm, lrl, &r) == 0 )
              return rDestroy(&r), 0;
          }
        }
      }
    }
  }
  
  return rDestroy(&r), 1;
}


int rmFindInitialListAreaMaxWidth(rmatrix rm, lrlist lrl, rect *area)
{
  int i, j;
  int s;
  rect r;
  
  if ( rInit(rm, &r) == 0 )
    return 0;
  
  for( i = 0; i < rm->dim[1]; i++ )
  {
    if ( rGet(area, 1, i) != 0 )
    {
      for( j = 0; j < rm->dim[0]; j++ )
      {
        if ( rGet(area, 0, j) != 0 )
        {
          s = rm_get_xy(rm, j, i);
          if ( s != 0 )
          {
            rClear(rm, &r);
            rSet(&r, 0, j, s);
            rSet(&r, 1, i, 2);
            
            rmExtendToMaxWidthArea(rm, &r, area);
            if ( lrlPut(rm, lrl, &r) == 0 )
              return rDestroy(&r), 0;
            break;
          }
        }
      }
    }
  }
  
  return rDestroy(&r), 1;
}

/* 0: nothing added */
int rmAddRectColumn(rmatrix rm, rect *r, int column, int symbol)
{
  int y;

  /*  
  if ( rGet(area, 0, column) == 0 )
    return 0;
  */
  
  if ( rRealCntDim(rm, 0, r) == 0 )
  {
    for( y = 0; y < rm->dim[1]; y++ )
    {
      /* if ( rGet(area, 1, y) != 0 ) */
      {
        if ( rm_get_xy(rm, column, y) == symbol )
        {
          rSet(r, 0, column, symbol);
          rSet(r, 1, y, 2);
        }
      }
    }
    if ( rRealCntDim(rm, 0, r) == 0 )
    {
      rClear(rm, r);
      return 0;
    }
  }
  else
  {
    for( y = 0; y < rm->dim[1]; y++ )
    {
      /* if ( rGet(area, 1, y) != 0 ) */
      {
        if ( rGet(r, 1, y) != 0 )
        {
          if ( rm_get_xy(rm, column, y) == symbol )
          {
            rSet(r, 0, column, symbol);
          }
          else
          {
            rSet(r, 1, y, 0);
          }
        }
      }
    }
    if ( rRealCntDim(rm, 1, r) == 0 )
    {
      rClear(rm, r);
      return 0;
    }
  }
  return 1;
}

