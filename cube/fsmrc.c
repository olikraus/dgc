/*

  fsmrc.c
    
  required cubes

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

#include "fsm.h"
#include <assert.h>
#include "mwc.h"


#ifdef USE_OLD_CODE

int fsm_CalcOutEdgeRequiredCubeStartCover(fsm_type fsm, int edge_id, dclist cl_rs)
{
  dclist cl_d1edge, cl_stable;
  if ( dclInitVA(2, &cl_d1edge, &cl_stable) == 0 )
    return 0;
    
  if ( dclDistance1(fsm->pi_cond, cl_d1edge, fsm_GetEdgeCondition(fsm,edge_id)) == 0 )
    return dclDestroyVA(2, cl_d1edge, cl_stable), 0;
  if ( fsm_CalcSICStableCondition(fsm, fsm_GetEdgeSrcNode(fsm, edge_id), cl_stable) == 0 )
    return dclDestroyVA(2, cl_d1edge, cl_stable), 0;
  if ( dclIntersectionList(fsm->pi_cond, cl_rs, cl_d1edge, cl_stable) == 0 )
    return dclDestroyVA(2, cl_d1edge, cl_stable), 0;

  /*
  printf("node %s stable condition\n",
    fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, edge_id))
    );
  dclShow(fsm->pi_cond, cl_stable);

  printf("transition [%s - %s] D1(condition)\n",
    fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, edge_id)),
    fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, edge_id))
    );

  dclShow(fsm->pi_cond, cl_d1edge);

  printf("transition [%s - %s] required cube start cover\n",
    fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, edge_id)),
    fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, edge_id))
    );

  dclShow(fsm->pi_cond, cl_rs);
  */

  return dclDestroyVA(2, cl_d1edge, cl_stable), 1;
}


int fsm_AddOutEdgeRequiredCubes(fsm_type fsm, int edge_id, dclist cl_rc)
{
  dclist cl_rs;
  dclist cl_edge = fsm_GetEdgeCondition(fsm, edge_id);
  int i, cnt;
  int j;
  int ov, nv;
  dcube *c;
  dcube *cc;
  cc = &(fsm->pi_machine->tmp[9]);

  if ( dclInit(&cl_rs) == 0 )
    return 0;
    
  if ( fsm_CalcOutEdgeRequiredCubeStartCover(fsm, edge_id, cl_rs) == 0 )
    return dclDestroy(cl_rs), 0;
    
  if ( dclDontCareExpand(fsm->pi_cond, cl_rs) == 0 )
    return dclDestroy(cl_rs), 0;

  cnt = dclCnt(cl_rs);
  for( i = 0; i < cnt; i++ )
  {
    c = dclGet(cl_rs, i);
    dcInSetAll(fsm->pi_machine, cc, CUBE_IN_MASK_DC);
    dcOutSetAll(fsm->pi_machine, cc, CUBE_OUT_MASK);
    dcCopyInToIn(fsm->pi_machine, cc, 0, fsm->pi_cond, c);
    dcCopyOutToIn( fsm->pi_machine, cc, fsm->pi_cond->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, fsm_GetEdgeSrcNode(fsm, edge_id)));
    
    for( j = 0; j < fsm->pi_cond->in_cnt; j++ )
    {
      ov = dcGetIn(c, j);
      assert( ov == 1 || ov == 2 );
      nv = 3-ov;
      assert( nv == 1 || nv == 2 );
      dcSetIn(c, j, nv);
      dcSetIn(cc, j, nv);
      if ( dclIsSingleSubSet(fsm->pi_cond, cl_edge, c) != 0 )
      {
        dcSetIn(c, j, 3);
        dcSetIn(cc, j, 3);
        if ( dclAdd(fsm->pi_machine, cl_rc, cc) < 0 )
          return 0;
      }
      dcSetIn(c, j, ov);
      dcSetIn(cc, j, ov);
    }
  }

  return dclDestroy(cl_rs), 1;
}

int fsm_AddRequiredCubesNode(fsm_type fsm, int node_id, dclist cl_rc)
{
  int loop, edge_id;
  
  loop = -1;
  edge_id = -1;
  while( fsm_LoopNodeOutEdges(fsm, node_id, &loop, &edge_id) != 0 )
    if ( fsm_GetEdgeSrcNode(fsm, edge_id) != fsm_GetEdgeDestNode(fsm, edge_id) )
      if ( fsm_AddOutEdgeRequiredCubes(fsm, edge_id, cl_rc) == 0 )
        return 0;

  return 1;
}

int fsm_RequiredCubes(fsm_type fsm, dclist cl_rc)
{
  int node_id;
  node_id = -1;
  dclClear(cl_rc);
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
    if ( fsm_AddRequiredCubesNode(fsm, node_id, cl_rc) == 0 )
      return 0;
  return 1;
}

#endif

static int fsm_required_cubes_self_loop(fsm_type fsm, dclist cl_rc, int edge_id, dcube *c )
{
  int src_node = fsm_GetEdgeSrcNode(fsm, edge_id);
  int dest_node = fsm_GetEdgeDestNode(fsm, edge_id);

  pinfo *pi_m = fsm->pi_machine;
  pinfo *pi_o = fsm_GetOutputPINFO(fsm);

  dcube *r = &(fsm->pi_machine->tmp[9]);
  
  assert(src_node == dest_node);

  dcInSetAll(pi_m, r, CUBE_IN_MASK_DC);
  dcOutSetAll(pi_m, r, CUBE_OUT_MASK);
  dcCopyInToIn(pi_m, r, 0, pi_o, c);
  dcCopyOutToIn(pi_m, r, pi_o->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node));
  
  if ( dclAdd(pi_m, cl_rc, r) < 0 )
    return 0;
  return 1;
}

static int fsm_required_cubes_transition(fsm_type fsm, dclist cl_rc, int edge_id, dcube *c)
{
  int src_node = fsm_GetEdgeSrcNode(fsm, edge_id);
  int dest_node = fsm_GetEdgeDestNode(fsm, edge_id);
  
  pinfo *pi_m = fsm->pi_machine;
  pinfo *pi_o = fsm_GetOutputPINFO(fsm);

  dcube *r = &(fsm->pi_machine->tmp[9]);
  dcube *s = &(fsm->pi_machine->tmp[10]);

  assert(src_node != dest_node);

  dcInSetAll(pi_m, r, CUBE_IN_MASK_DC);
  dcOutSetAll(pi_m, r, CUBE_OUT_MASK);
  dcCopyInToIn(pi_m, r, 0, pi_o, c);
  dcCopyOutToIn(pi_m, r, pi_o->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node));

  dcInSetAll(pi_m, s, CUBE_IN_MASK_DC);
  dcOutSetAll(pi_m, s, CUBE_OUT_MASK);
  dcCopyInToIn(pi_m, s, 0, pi_o, c);
  dcCopyOutToIn(pi_m, s, pi_o->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node));

  dcOr(pi_m, r, r, s);
  
  if ( dclAdd(pi_m, cl_rc, r) < 0 )
    return 0;
  return 1;
}

/*
  D1(T(s_i, s_j)) cap T(s_i, s_i)
  
*/
static int fsm_required_cubes_input_stable_change(fsm_type fsm, dclist cl_rc, int edge_id)
{
  int src_node = fsm_GetEdgeSrcNode(fsm, edge_id);
  /* int dest_node = fsm_GetEdgeDestNode(fsm, edge_id); */
  int self_edge_id;
  dclist cl_dt, cl_t, cl_i;
  pinfo *pi = fsm_GetOutputPINFO(fsm);
  pinfo *pi_m = fsm->pi_machine;
  int i, j;
  dcube *r = &(fsm->pi_machine->tmp[9]);
  dcube *s = &(fsm->pi_machine->tmp[10]);

  
  self_edge_id = fsm_FindEdge(fsm, src_node, src_node);
  if ( self_edge_id < 0 )
    return 1; /* nothing to do */
  
  if ( dclInitCachedVA(pi, 3, &cl_dt, &cl_t, &cl_i) == 0 )
    return 0;

  if ( dclCopy(pi, cl_t, fsm_GetEdgeOutput(fsm, edge_id)) == 0 )
    return dclDestroyCachedVA(pi, 3, cl_dt, cl_t, cl_i), 0;
    
  for( i = 0; i < dclCnt(cl_t); i++ )
    dcOutSetAll(pi, dclGet(cl_t, i), CUBE_OUT_MASK);
  dclSCC(pi, cl_t);
  
  if ( dclDistance1(pi, cl_dt, cl_t) == 0 )
    return dclDestroyCachedVA(pi, 3, cl_dt, cl_t, cl_i), 0;
  if ( dclIntersectionList(pi, cl_i, cl_dt, fsm_GetEdgeOutput(fsm, self_edge_id)) == 0 )
    return dclDestroyCachedVA(pi, 3, cl_dt, cl_t, cl_i), 0;
    
  for( i = 0; i < dclCnt(cl_i); i++ )
    dcOutSetAll(pi, dclGet(cl_i, i), CUBE_OUT_MASK);
  dclSCC(pi, cl_i);
  
  for( i = 0; i < dclCnt(cl_i); i++ )
    for( j = 0; j < dclCnt(cl_t); j++ )
    {
      dcInSetAll(pi_m, s, CUBE_IN_MASK_DC);
      dcOutSetAll(pi_m, s, CUBE_OUT_MASK);
      dcCopyInToIn(pi_m, s, 0, pi, dclGet(cl_i, i));
      dcCopyOutToIn(pi_m, s, pi->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node));
      
      dcInSetAll(pi_m, r, CUBE_IN_MASK_DC);
      dcOutSetAll(pi_m, r, CUBE_OUT_MASK);
      dcCopyInToIn(pi_m, r, 0, pi, dclGet(cl_t, j));
      dcCopyOutToIn(pi_m, r, pi->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node));

      dcOr(pi_m, r, r, s);

      if ( dclAdd(pi_m, cl_rc, r) < 0 )
        return dclDestroyCachedVA(pi, 3, cl_dt, cl_t, cl_i), 0;
    }
  
      
  return dclDestroyCachedVA(pi, 3, cl_dt, cl_t, cl_i), 1;
}

int fsm_RequiredCubes(fsm_type fsm, dclist cl_rc)
{
  int edge_id, src_node, dest_node;
  dclist cl_primes;
  pinfo *pi = fsm_GetOutputPINFO(fsm);
  int i, cnt;
  
  if ( dclInit(&cl_primes) == 0 )
    return 0;
  
  edge_id = -1;  
  dclClear(cl_rc);
  while( fsm_LoopEdges(fsm, &edge_id) != 0 )
  {
    if ( dclCopy(pi, cl_primes, fsm_GetEdgeOutput(fsm, edge_id)) == 0 )
      return dclDestroy(cl_primes), 0;
    if ( dclPrimes(pi, cl_primes) == 0 )
      return dclDestroy(cl_primes), 0;
    cnt = dclCnt(cl_primes);
    
    src_node = fsm_GetEdgeSrcNode(fsm, edge_id);
    dest_node = fsm_GetEdgeDestNode(fsm, edge_id);
    if ( src_node == dest_node )
    {
      for( i = 0; i < cnt; i++ )
        if ( fsm_required_cubes_self_loop(fsm, cl_rc, edge_id, dclGet(cl_primes, i)) == 0 )
          return dclDestroy(cl_primes), 0;
    }
    else
    {
      for( i = 0; i < cnt; i++ )
        if ( fsm_required_cubes_transition(fsm, cl_rc, edge_id, dclGet(cl_primes, i)) == 0 )
          return dclDestroy(cl_primes), 0;
      if ( fsm_required_cubes_input_stable_change(fsm, cl_rc, edge_id) == 0 )
        return dclDestroy(cl_primes), 0;
    }
  }
  
  dclSCC(fsm->pi_machine, cl_rc);
  
  return dclDestroy(cl_primes), 1;
}

