/*

  xbmess.c

  essential hazards for eXtended burst mode machines

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

#include <assert.h>

#include "xbm.h"


/* m is of type x->pi_machine */
static int xbm_find_state_pos_by_machine(xbm_type x, dcube *m)
{
  int st_pos = -1;
  xbm_state_type s;
  while( xbm_LoopSt(x, &st_pos) )
  {
    s = xbm_GetSt(x, st_pos);
    if ( x->is_fbo == 0 )
    {
      if ( dcIsEqualOutRange(&(s->code), 0, m, 0, xbm_GetPiCode(x)->out_cnt) != 0 )
        return st_pos;
    }
    else
    {
      if ( dcIsEqualOutRange(&(s->code), 0, m, 0, xbm_GetPiCode(x)->out_cnt) != 0 
        && dcIsEqualOutRange(&(s->out), 0, m, xbm_GetPiCode(x)->out_cnt, xbm_GetPiOut(x)->out_cnt) != 0 )
        return st_pos;
    }
  }
  return -1;
}

/*
  pi_machine:  
    i: input    c: code   o: output
  
  is_fbo == 0:
    iiiiiiiicccccc ccccccooo
    
  is_fbo == 1:
    iiiiiiiiccccccooo ccccccooo
    

*/

static int xbm_ess_check_ess_hazard(xbm_type x, int tr_pos, dcube *m)
{
  int i;
  dcube *n = &(xbm_GetPiIn(x)->tmp[15]);
  xbm_transition_type tr = xbm_GetTr(x, tr_pos);
  dcube *n1 = &(x->pi_machine->tmp[15]);
  dcube *n2 = &(x->pi_machine->tmp[16]);
  dcube *n3 = &(x->pi_machine->tmp[17]);
  int st1_pos = xbm_GetTrSrcStPos(x, tr_pos);
  int st2_pos = xbm_GetTrDestStPos(x, tr_pos);
  int st3_pos = -1;  /* not yet known */
  
  int cmp_cnt;
  char *n_str;
  char *m_str;
  
  for( i = 0; i < x->inputs; i++ )
  {
    /* create a copy */
    dcCopy(xbm_GetPiIn(x), n, m);
    
    /* invert variable i */
    dcSetIn(n, i, dcGetIn(n, i)^3);
    
    if ( dcIsInSubSet(xbm_GetPiIn(x), &(tr->in_end_cond), n) ) /* Is n subset of tr->...? */
    {
      x->total_cnt++;
      dcCopyInToIn(x->pi_machine, n1, 0, xbm_GetPiIn(x), m);
      dcCopyOutToIn(x->pi_machine, n1, x->inputs, 
        xbm_GetPiCode(x), &(xbm_GetSt(x, st1_pos)->code));
      if ( x->is_fbo != 0 )
        dcCopyOutToIn(x->pi_machine, n1, x->inputs+xbm_GetPiCode(x)->out_cnt,
          xbm_GetPiOut(x), &(xbm_GetSt(x, st1_pos)->out));
      dclResult(x->pi_machine, n1, x->cl_machine);
      assert( dcIsEqualOutRange(n1, 0, 
        &(xbm_GetSt(x, st1_pos)->code), 0, 
        xbm_GetPiCode(x)->out_cnt) != 0 );
      if ( x->is_fbo != 0 )
      {
        assert( dcIsEqualOutRange(n1, xbm_GetPiCode(x)->out_cnt, 
          &(xbm_GetSt(x, st1_pos)->out), 0, 
          xbm_GetPiOut(x)->out_cnt) != 0 );
      }
      
      
      dcCopyInToIn(x->pi_machine, n2, 0, xbm_GetPiIn(x), n);
      dcCopyOutToInRange(x->pi_machine, n2, x->inputs, 
        x->pi_machine, n1, 0, xbm_GetPiCode(x)->out_cnt);
      if ( x->is_fbo != 0 )
        dcCopyOutToInRange(x->pi_machine, n2, x->inputs+xbm_GetPiCode(x)->out_cnt, 
          x->pi_machine, n1, xbm_GetPiCode(x)->out_cnt, xbm_GetPiOut(x)->out_cnt);
      dclResult(x->pi_machine, n2, x->cl_machine);
      assert( dcIsEqualOutRange(n2, 0, 
        &(xbm_GetSt(x, st2_pos)->code), 0, 
        xbm_GetPiCode(x)->out_cnt) != 0 );
      if ( x->is_fbo != 0 )
      {
        assert( dcIsEqualOutRange(n2, xbm_GetPiCode(x)->out_cnt, 
          &(xbm_GetSt(x, st2_pos)->out), 0, 
          xbm_GetPiOut(x)->out_cnt) != 0 );
      }
      
      dcCopyInToIn(x->pi_machine, n3, 0, xbm_GetPiIn(x), m);
      dcCopyOutToInRange(x->pi_machine, n3, x->inputs, 
        x->pi_machine, n2, 0, xbm_GetPiCode(x)->out_cnt);
      if ( x->is_fbo != 0 )
        dcCopyOutToInRange(x->pi_machine, n3, x->inputs+xbm_GetPiCode(x)->out_cnt, 
          x->pi_machine, n2, xbm_GetPiCode(x)->out_cnt, xbm_GetPiOut(x)->out_cnt);
      dclResult(x->pi_machine, n3, x->cl_machine);
      
      cmp_cnt = xbm_GetPiCode(x)->out_cnt;
      if ( x->is_fbo != 0 )
        cmp_cnt += xbm_GetPiOut(x)->out_cnt;
        
      if ( dcIsEqualOutCnt(n1, n3, 0, cmp_cnt) == 0 &&
           dcIsEqualOutCnt(n2, n3, 0, cmp_cnt) == 0 )
      {
        n_str = dcToStr(xbm_GetPiIn(x), n, "", "");
        m_str = dcToStr2(xbm_GetPiIn(x), m, "", "");
        st3_pos = xbm_find_state_pos_by_machine(x, n3);
      
        x->ess_cnt++;
        xbm_Log(x, 3, "XBM: Essential hazard, "
           "var %d/%-12s, input/state %s/'%s' --> %s/'%s' --> %s'%s'.",
           i,
           xbm_GetVarNameStrByIndex(x, XBM_DIRECTION_IN, i),
           m_str,
           xbm_GetStNameStr(x, st1_pos),
           n_str,
           xbm_GetStNameStr(x, st2_pos),
           m_str,
           xbm_GetStNameStr(x, st3_pos)
        );

        xbm_Log(x, 3, "XBM: Essential hazard, "
           "var %d/%-12s, state (cube) "
           "'%s' (%s) --> '%s' (%s) --> '%s' (%s).",
           i,
           xbm_GetVarNameStrByIndex(x, XBM_DIRECTION_IN, i),
           xbm_GetStNameStr(x, st1_pos),
           dcToStr( x->pi_machine, n1, " ", ""),
           xbm_GetStNameStr(x, st2_pos),
           dcToStr2(x->pi_machine, n2, " ", ""),
           xbm_GetStNameStr(x, st3_pos),
           dcToStr3(x->pi_machine, n3, " ", "")
        );
        
        if ( x->ess_cb(x, x->ess_data, n1, n2, n3) == 0 )
          return 0;
        
      }
    }
  }
  return 1;
}

static int xbm_ess_loop_minterms(xbm_type x, int tr_pos)
{
  int i;
  pinfo *pi = xbm_GetPiIn(x);
  dclist d1, m;
  int src_st_pos = xbm_GetTrSrcStPos(x, tr_pos);
  xbm_transition_type tr = xbm_GetTr(x, tr_pos);
  
  if ( dclInitCachedVA(pi, 2, &d1, &m) == 0 )
    return 0;
  
  /* calculate M = EXPAND_ONES(INTERSECTION(T(s,s),D1(T(s,t)))) */
  
  
  if ( dclDistance1Cube(pi, d1, &(tr->in_end_cond)) == 0 )
    return dclDestroyCachedVA(pi, 2, d1, m), 0;
    
  if ( dclIntersectionList(pi, m, xbm_GetSt(x, src_st_pos)->in_self_cl, d1) == 0 )
    return dclDestroyCachedVA(pi, 2, d1, m), 0;
    
  if ( dclDontCareExpand(pi, m) == 0 )
    return dclDestroyCachedVA(pi, 2, d1, m), 0;

  for( i = 0; i < dclCnt(m); i++ )
    if ( xbm_ess_check_ess_hazard(x, tr_pos, dclGet(m, i)) == 0 )
      return dclDestroyCachedVA(pi, 2, d1, m), 0;
  
  return dclDestroyCachedVA(pi, 2, d1, m), 1;
}

static int xbm_ess_loop_transitions(xbm_type x)
{
  int tr_pos = -1;
  while( xbm_LoopTr(x, &tr_pos) != 0 )
    if ( xbm_ess_loop_minterms(x, tr_pos) == 0 )
      return 0;
  return 1;
}

int xbm_CalcEssentialHazards(xbm_type x, int (*ess_cb)(xbm_type x, void *data, dcube *n1, dcube *n2, dcube *n3), void *data)
{
  x->ess_cb = ess_cb;
  x->ess_data = data;
  x->total_cnt = 0;
  x->ess_cnt = 0;
  return xbm_ess_loop_transitions(x);
}

/*---------------------------------------------------------------------------*/


static int xbm_do_minterm_transition(xbm_type x, int tr_pos, dcube *m)
{
  int i;
  dcube *n = &(xbm_GetPiIn(x)->tmp[15]);
  xbm_transition_type tr = xbm_GetTr(x, tr_pos);
  dcube *n1 = &(x->pi_machine->tmp[15]);
  dcube *n2 = &(x->pi_machine->tmp[16]);
  int st1_pos = xbm_GetTrSrcStPos(x, tr_pos);
  int st2_pos = xbm_GetTrDestStPos(x, tr_pos);
  int st3_pos = -1;  /* not yet known */
  
  int cmp_cnt;
  char *n_str;
  char *m_str;
  
  for( i = 0; i < x->inputs; i++ )
  {
    /* create a copy */
    dcCopy(xbm_GetPiIn(x), n, m);
    
    /* invert variable i */
    dcSetIn(n, i, dcGetIn(n, i)^3);
    
    if ( dcIsInSubSet(xbm_GetPiIn(x), &(tr->in_end_cond), n) ) /* Is n subset of tr->...? */
    {
      x->total_cnt++;
      dcCopyInToIn(x->pi_machine, n1, 0, xbm_GetPiIn(x), m);
      dcCopyOutToIn(x->pi_machine, n1, x->inputs, 
        xbm_GetPiCode(x), &(xbm_GetSt(x, st1_pos)->code));
      if ( x->is_fbo != 0 )
        dcCopyOutToIn(x->pi_machine, n1, x->inputs+xbm_GetPiCode(x)->out_cnt,
          xbm_GetPiOut(x), &(xbm_GetSt(x, st1_pos)->out));
      dclResult(x->pi_machine, n1, x->cl_machine);
      assert( dcIsEqualOutRange(n1, 0, 
        &(xbm_GetSt(x, st1_pos)->code), 0, 
        xbm_GetPiCode(x)->out_cnt) != 0 );
      if ( x->is_fbo != 0 )
      {
        assert( dcIsEqualOutRange(n1, xbm_GetPiCode(x)->out_cnt, 
          &(xbm_GetSt(x, st1_pos)->out), 0, 
          xbm_GetPiOut(x)->out_cnt) != 0 );
      }
      
      
      dcCopyInToIn(x->pi_machine, n2, 0, xbm_GetPiIn(x), n);
      dcCopyOutToInRange(x->pi_machine, n2, x->inputs, 
        x->pi_machine, n1, 0, xbm_GetPiCode(x)->out_cnt);
      if ( x->is_fbo != 0 )
        dcCopyOutToInRange(x->pi_machine, n2, x->inputs+xbm_GetPiCode(x)->out_cnt, 
          x->pi_machine, n1, xbm_GetPiCode(x)->out_cnt, xbm_GetPiOut(x)->out_cnt);
      dclResult(x->pi_machine, n2, x->cl_machine);
      assert( dcIsEqualOutRange(n2, 0, 
        &(xbm_GetSt(x, st2_pos)->code), 0, 
        xbm_GetPiCode(x)->out_cnt) != 0 );
      if ( x->is_fbo != 0 )
      {
        assert( dcIsEqualOutRange(n2, xbm_GetPiCode(x)->out_cnt, 
          &(xbm_GetSt(x, st2_pos)->out), 0, 
          xbm_GetPiOut(x)->out_cnt) != 0 );
      }
      
      {
        
        if ( x->tr_cb(x, x->tr_data, tr_pos, i, n1, n2) == 0 )
          return 0;
        
      }
    }
  }
  return 1;
}

static int xbm_loop_minterms(xbm_type x, int tr_pos)
{
  int i;
  pinfo *pi = xbm_GetPiIn(x);
  dclist d1, m;
  int src_st_pos = xbm_GetTrSrcStPos(x, tr_pos);
  xbm_transition_type tr = xbm_GetTr(x, tr_pos);
  
  if ( dclInitCachedVA(pi, 2, &d1, &m) == 0 )
    return 0;
  
  /* calculate M = EXPAND_ONES(INTERSECTION(T(s,s),D1(T(s,t)))) */
  
  
  if ( dclDistance1Cube(pi, d1, &(tr->in_end_cond)) == 0 )
    return dclDestroyCachedVA(pi, 2, d1, m), 0;
    
  if ( dclIntersectionList(pi, m, xbm_GetSt(x, src_st_pos)->in_self_cl, d1) == 0 )
    return dclDestroyCachedVA(pi, 2, d1, m), 0;
    
  if ( dclDontCareExpand(pi, m) == 0 )
    return dclDestroyCachedVA(pi, 2, d1, m), 0;

  for( i = 0; i < dclCnt(m); i++ )
    if ( xbm_do_minterm_transition(x, tr_pos, dclGet(m, i)) == 0 )
      return dclDestroyCachedVA(pi, 2, d1, m), 0;
  
  return dclDestroyCachedVA(pi, 2, d1, m), 1;
}

static int xbm_loop_transitions(xbm_type x)
{
  int tr_pos = -1;
  while( xbm_LoopTr(x, &tr_pos) != 0 )
    if ( xbm_loop_minterms(x, tr_pos) == 0 )
      return 0;
  return 1;
}


int xbm_DoTransitions(xbm_type x, int (*tr_cb)(xbm_type x, void *data, int tr_pos, int var_in_idx, dcube *n1, dcube *n2), void *data)
{
  x->tr_cb = tr_cb;
  x->tr_data = data;
  return xbm_loop_transitions(x);
}

