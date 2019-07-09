/*

  recu.c
  
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

#include "recu.h"
#include <stdlib.h>
#include <assert.h>
#include "mwc.h"

recu rcOpen()
{
  recu rc;
  rc = (recu)malloc(sizeof(struct _recu_struct));
  if ( rc != NULL )
  {
    rc->rm = NULL;
    rc->input_mask = NULL;
    rc->full_input_cover = NULL;
    rc->area_mask = NULL;
    rc->top = NULL;
    rc->pi = pinfoOpen();
    if ( rc->pi != NULL )
    {
      if ( dclInit(&(rc->cl)) != 0 )
      {
        rc->lrl = lrlOpen();
        if ( rc->lrl != NULL )
        {
          rc->lrl_tmp = lrlOpen();
          if ( rc->lrl_tmp != NULL )
          {
            rc->lrl_primes = lrlOpen();
            if ( rc->lrl_primes != NULL )
            {
              rc->lrl_output = lrlOpen();
              if ( rc->lrl_output != NULL )
              {
                return rc;
              }
              lrlClose(rc->lrl_output);
            }
            lrlClose(rc->lrl_tmp);
          }
          lrlClose(rc->lrl);
        }
        dclDestroy(rc->cl);
      }
      pinfoClose(rc->pi);
    }
    free(rc);
  }
  return NULL;
}

void rcClose(recu rc)
{

  if ( rc->input_mask != NULL )
    rClose(rc->input_mask);
  if ( rc->full_input_cover != NULL )
    rClose(rc->full_input_cover);
  if ( rc->area_mask != NULL )
    rClose(rc->area_mask);
  if ( rc->top != NULL )
    srnClose(rc->top);

  if ( rc->rm != NULL )
    rmClose(rc->rm);

  lrlClose(rc->lrl_output);
  lrlClose(rc->lrl_primes);
  lrlClose(rc->lrl_tmp);
  lrlClose(rc->lrl);
  dclDestroy(rc->cl);
  pinfoClose(rc->pi);
  free(rc);
}

static int rc_refill_matrix(recu rc)
{
  int i;
  if ( rc->rm != NULL )
    rmClose(rc->rm);
  rc->rm = rmOpen(rc->pi->in_cnt + rc->pi->out_cnt, dclCnt(rc->cl));
  if ( rc->rm == NULL )
    return 0;
    
  if ( rc->input_mask != NULL )
    rClose(rc->input_mask);
  if ( rc->full_input_cover != NULL )
    rClose(rc->full_input_cover);
  if ( rc->area_mask != NULL )
    rClose(rc->area_mask);
  if ( rc->top != NULL )
    srnClose(rc->top);
  
  rc->input_mask = rOpen(rc->rm);
  rc->full_input_cover = rOpen(rc->rm);
  rc->area_mask = rOpen(rc->rm);
  rc->top = srnOpen(rc->rm);
  
  if ( rc->input_mask == NULL ) return 0;
  if ( rc->full_input_cover == NULL ) return 0;
  if ( rc->area_mask == NULL ) return 0;
  if ( rc->top == NULL ) return 0;
  
  for( i = 0; i < rc->pi->in_cnt; i++ )
    rSet(rc->input_mask, 0, i, 3);

  rmFillByCL(rc->rm, rc->pi, rc->cl);
  lrlClear(rc->lrl);
  return 1;
}

int rcSetDCList(recu rc, pinfo *pi, dclist cl)
{
  if ( pinfoSetInCnt(rc->pi, pi->in_cnt) == 0 )
    return 0;
  if ( pinfoSetOutCnt(rc->pi, pi->out_cnt) == 0 )
    return 0;
  if ( dclCopy(rc->pi, rc->cl, cl) == 0 )
    return 0;
  return rc_refill_matrix(rc);
}

recu rcOpenByDCList(pinfo *pi, dclist cl)
{
  recu rc;
  rc = rcOpen();
  if ( rc != NULL )
  {
    if ( rcSetDCList(rc, pi, cl) != 0 )
    {
      return rc;
    }
    rcClose(rc);
  }
  return NULL;
}

int rcLoadDCList(recu rc, char *filename)
{
  if ( dclReadPLA(rc->pi, rc->cl, NULL,  filename) == 0 )
    return 0;
  return rc_refill_matrix(rc);
}

int rcLoadAndMinimizeDCList(recu rc, char *filename)
{
  if ( dclReadPLA(rc->pi, rc->cl, NULL,  filename) == 0 )
    return 0;
  if ( dclPrimes(rc->pi, rc->cl) == 0 )
    return 0;
  if ( dclIrredundant(rc->pi, rc->cl, NULL) == 0 )
    return 0;
  dclRestrictOutput(rc->pi, rc->cl);
  puts("--- dclist start ---");
  dclShow(rc->pi, rc->cl);
  puts("--- dclist end ---");
  return rc_refill_matrix(rc);
}

int rcAddSpecialInputRectangles(recu rc, lrlist lrl, rect *cover)
{
  rect rr;
  int in_col, in_col_2;
  int symbol;
  
  rInit(rc->rm, &rr);

  for( in_col = 0; in_col < rc->pi->in_cnt*2; in_col++ )
  {
    symbol = (in_col&1)+1;
    rCopy(rc->rm, &rr, cover);
    if ( rRealCntDim(rc->rm, 1, &rr) > 1 )
    {
      if ( rmAddRectColumn(rc->rm, &rr, in_col/2, (in_col&1)+1) != 0 )
      {
        if ( rRealCntDim(rc->rm, 1, &rr) > 1 )
        {
          rmExtendToMaxWidth(rc->rm, &rr);
          assert(rIsEmpty(rc->rm, &rr) == 0);
          if ( lrlIsEqualElement(rc->rm, lrl, &rr) == 0 )
            if ( lrlPut(rc->rm, lrl, &rr) == 0 )
              return rDestroy(&rr), 0;
        }
      }
    }
  }

  for( in_col = 0; in_col < rc->pi->in_cnt*2; in_col++ )
  {
    for( in_col_2 = in_col+1; in_col_2 < rc->pi->in_cnt*2; in_col_2++ )
    {
      rCopy(rc->rm, &rr, cover);
      if ( rRealCntDim(rc->rm, 1, &rr) > 1 )
      {
        if ( rmAddRectColumn(rc->rm, &rr, in_col/2, (in_col&1)+1) != 0 )
        {
          if ( rRealCntDim(rc->rm, 1, &rr) > 1 )
          {
            if ( rmAddRectColumn(rc->rm, &rr, in_col_2/2, (in_col_2&1)+1) != 0 )
            {
              if ( rRealCntDim(rc->rm, 1, &rr) > 1 )
              {
                rmExtendToMaxWidth(rc->rm, &rr);
                if ( lrlIsEqualElement(rc->rm, lrl, &rr) == 0 )
                  if ( lrlPut(rc->rm, lrl, &rr) == 0 )
                    return rDestroy(&rr), 0;
              }
            }
          }
        }
      }
    }
  }
  
  return rDestroy(&rr), 1;
}

/*
  sucht in lrl nach einem rectangle, dass 
  dim[1] von r moeglichst gut ueberdeckt.
*/
int rcSearchBestFit(recu rc, rect *r, int max_height, rect *r_factored, lrlist lrl)
{
  int i, pos, cnt;
  int c1, c2, curr, max;
  rect r_curr;

  rInit(rc->rm, &r_curr);

  max = 1;
  pos = -1;
  
  cnt = lrlCnt(lrl);
  for( i = 0; i < cnt; i++ )
  {
    rNotAndDim(rc->rm, 0, &r_curr, r_factored, lrlGet(lrl, i));
    rAndDim(rc->rm, 1, &r_curr, r, lrlGet(lrl, i));
    c1 = rRealCntDim(rc->rm, 0, &r_curr);
    c2 = rRealCntDim(rc->rm, 1, &r_curr);
    curr =  c1*c2*c2;
    /* if ( rIsSubSetDim(rc->rm, 0, r_factored, lrlGet(lrl, i)) == 0 ) */
    {
      if ( max < curr && c2 >= 2 )
      {
        max = curr;
        pos = i;
      }
    }
  }
  rDestroy(&r_curr);
  return pos;
}

int rcIsFullInputCover(recu rc, rect *r_cover, rect *r_factored)
{
  r_int *row;
  int y;
  int i;
  rCopyDim(rc->rm, 0, rc->full_input_cover, r_factored);
  rAndDim(rc->rm, 0, rc->full_input_cover, rc->full_input_cover, rc->input_mask);
  for( y = 0; y < rc->rm->dim[1];y++ )
  {
    if ( rGet(r_cover, 1, y) != 0 )
    {
      row = rmGetRow(rc->rm, y);
      for( i = 0; i < rc->rm->word_dim[0]; i++ )
      {
        if ( ( rc->input_mask->assignment[0][i] & row[i] & 
          ~(rc->full_input_cover->assignment[0][i])) != 0 )
          return 0;
      }
    }
  }
  return 1;
}

/*
  berechnet eine cover mit rectangles, so dass dim[1] von r 
  komplett ueberdeckt wird. Die rectangles werden aus lrl 
  ausgewaehlt. Eventuell werden neue rectangles erzeugt
  und in cover abgelegt.
  dim[0] von r enthaelt die bereits vorher ausgeklammerten terme.
*/
int rcBuildCover(recu rc, rlist cover, rect *r_cover, rect *r_factored, lrlist lrl)
{
  rect rr;
  int pos, x, y;
  int symbol;
  int height = rRealCntDim(rc->rm, 1, r_cover);

  rlClear(cover);
  
  rInit(rc->rm, &rr);
  rCopy(rc->rm, &rr, r_cover);
  for(;;)  
  {
    if ( rRealCntDim(rc->rm, 1, &rr) == 0 )
      break;
  
    pos = rcSearchBestFit(rc, &rr, height, r_factored, lrl);
    if ( pos < 0 )
      break;
    assert( rIsEmpty(rc->rm, lrlGet(lrl, pos)) == 0 );
    pos = rlAdd(rc->rm, cover, lrlGet(lrl, pos));
    if ( pos < 0 )
      return rDestroy(&rr), 0;
    rAndDim(rc->rm, 1,    rlGet(cover, pos), &rr,               rlGet(cover, pos));
    rNotAndDim(rc->rm, 0, rlGet(cover, pos), r_factored,        rlGet(cover, pos));
    rNotAndDim(rc->rm, 1, &rr,               rlGet(cover, pos), &rr);
  }

  for( y = 0; y < rc->rm->dim[1]; y++ )
  {
    symbol = rGet(&rr, 1, y);
    if ( symbol != 0 )
    {
      pos = rlAddEmpty(rc->rm, cover);
      if ( pos < 0 )
        return rDestroy(&rr), 0;
      rSet(rlGet(cover, pos), 1, y, symbol);
      for( x = 0; x < rc->rm->dim[0]; x++ )
        rSet(rlGet(cover, pos), 0, x, rm_get_xy(rc->rm, x, y));
      assert( rcIsFullInputCover(rc, rlGet(cover, pos), rlGet(cover, pos)) != 0 );
      rNotAndDim(rc->rm, 0, rlGet(cover, pos), r_factored, rlGet(cover, pos));
      /* 14 nov 2001  ok  bugfix: there must be no empty elements */
      if ( rIsEmpty(rc->rm, rlGet(cover, pos)) != 0 ) 
        rlDel(rc->rm, cover, pos);
    }
  }

  return rDestroy(&rr), 1;
}

int _rcBuildOutputCoverTree(recu rc, srnode srn, rect *r_factored, lrlist lrl, int depth)
{
  rlist cover;
  srnode child;
  int i;
  
  cover = rlOpen();
  
  if ( rcBuildCover(rc, cover, &(srn->r), r_factored, lrl) == 0 )
    return rlClose(cover), 0;

  for( i = 0; i < rlCnt(cover); i++ )
  {
    child = srnAddChildRect(rc->rm, srn, rlGet(cover, i));
    if ( child == NULL )
      return rlClose(cover), 0;

    {      
      rOrDim( rc->rm, 0, r_factored, rlGet(cover, i), r_factored);
      if ( rcIsFullInputCover(rc, rlGet(cover, i), r_factored) == 0 )
        if ( _rcBuildOutputCoverTree(rc, child, r_factored, lrl, depth+1) == 0 )
          return rlClose(cover), 0;
      rNotAndDim( rc->rm, 0, r_factored, rlGet(cover, i), r_factored);
    }
  }
  
  return rlClose(cover), 1;
}

/* tries to fill 'srn' with elements from 'lrl' */
/* default: parent = &(srlGet(rc->srl, srn->ref)->r) */
int rcBuildOutputCoverTree(recu rc, srnode srn, lrlist lrl)
{
  rect r;
  if ( rInit(rc->rm, &r) == 0 )
    return 0;
  _rcBuildOutputCoverTree(rc, srn, &r, lrl, 0);
  return rDestroy(&r), 1;
}

srnode rcOpenOutputCoverTree(recu rc, rect *cover, int is_optimize)
{
  lrlist lrl;
  srnode srn;
  
  lrl = lrlOpen();
  if ( lrl == NULL )
    return NULL;
    
  rCopy(rc->rm, rc->area_mask, cover);
  rFillDim(0, rc->area_mask, 0, rc->pi->in_cnt, 1);
  
  rmFindInitialListArea(rc->rm, lrl, rc->area_mask);
  lrlClear(rc->lrl_primes);
  /*
  rmExtendList(rc->rm, rc->lrl_primes, lrl, rc->area_mask);
  lrlDelLowerEqualWidth(rc->rm, rc->lrl_primes, 1);
  lrlDelLowerEqualHeight(rc->rm, rc->lrl_primes, 1);
  */
  
  if ( is_optimize != 0 )
    rcAddSpecialInputRectangles(rc, rc->lrl_primes, cover);
  
  srn = srnOpen(rc->rm);
  rCopy(rc->rm, &(srn->r), cover);
  

  rcBuildOutputCoverTree(rc, srn, rc->lrl_primes);
  lrlClose(lrl);
  
  return srn;
}

void rcOutputToFunction(recu rc, srnode srn)
{
  int i, s;
  int is_first;
  srnode child;
  
  child = srn->down;
  is_first = 1;
  while(child != NULL)
  {
    if ( is_first != 0 )
      is_first = 0;
    else
      printf(" + ");
    for( i = 0; i < rc->pi->in_cnt; i++ )
    {
      s = rGet(&(child->r), 0, i);
      if ( s != 0 )
        printf("%s%d ", s==1?"/":"", i);
    }
    if ( child->down != NULL )
    {
      printf("(");
      rcOutputToFunction(rc, child);
      printf(")");
    }
    child = child->next;
  }
}

int rcBuildTree(recu rc, int is_lev_optimize, int is_out_optimize)
{
  int out_start;
  int out_end;
  int col;
  int i;
  rect *cover;
  cover = rOpen(rc->rm);
  
  out_start = rc->pi->in_cnt;
  out_end = out_start + rc->pi->out_cnt;
  
  rClear(rc->rm, rc->area_mask);
  rFill(rc->area_mask, out_start, out_end, 0, rc->rm->dim[1], 2);
  
  if ( is_out_optimize != 0 )
  {
    rmFindInitialListAreaMaxWidth(rc->rm, rc->lrl, rc->area_mask);
    rmExtendList(rc->rm, rc->lrl_output, rc->lrl, rc->area_mask);
    lrlDelLowerEqualWidth(rc->rm, rc->lrl_output, 1);
    lrlDisjunctY(rc->rm, rc->lrl_output);
  }

  srnClear(rc->top);

  /* build common outputs */

  /* lrlShow(rc->rm, rc->lrl_output); */
  
  for(i = 0; i < lrlCnt(rc->lrl_output); i++ )
  {
    srnAddChild(rc->top, rcOpenOutputCoverTree(rc, lrlGet(rc->lrl_output, i), is_lev_optimize));
  }
 
  
  /* build individual outputs */
  
  for( col = out_start; col < out_end; col++ )
  {
    
    rClear(rc->rm, cover);
    rmFillWithCol(rc->rm, cover, col, 2);
    for(i = 0; i < lrlCnt(rc->lrl_output); i++ )
      if ( rGet(lrlGet(rc->lrl_output, i), 0, col) != 0 )
        rNotAndDim(rc->rm, 1, cover, lrlGet(rc->lrl_output, i), cover);
        
    
    /* 9 nov 2001 added if statement: fix a bug in tech mapping */
    if ( rIsEmpty(rc->rm, cover) == 0 ) 
    {
      srnAddChild(rc->top, rcOpenOutputCoverTree(rc, cover, is_lev_optimize));
    }
  }

  rClose(cover);

  return 1;
}


