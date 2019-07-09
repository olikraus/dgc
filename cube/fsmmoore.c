/*

  fsmmoore.c
  
  conversion to moore state machine
  (output only depends on the state)

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

static int dclSeparateByOutput(pinfo *pi, dclist cl, dclist cl_eq, dclist cl_r)
{
  int i, cnt = dclCnt(cl);
  dclClear(cl_eq);
  dclClear(cl_r);
  
  if ( cnt == 0 )
    return 1;
  
  if ( dclAdd(pi, cl_eq, dclGet(cl, 0)) < 0 )
    return 0;
  
  for( i = 1; i < cnt; i++ )
  {
    if ( dcIsEqualOut(pi, dclGet(cl, 0), dclGet(cl, i)) != 0 )
    {
      if ( dclAdd(pi, cl_eq, dclGet(cl, i)) < 0 )
        return 0;
    }
    else
    {
      if ( dclAdd(pi, cl_r, dclGet(cl, i)) < 0 )
        return 0;
    }
  }
  
  return 1;
}


int fsm_DupMoore(fsm_type fsm, int node_id)
{
  dclist cl_eq, cl_r, cl_i, cl;
  pinfo *pi = fsm_GetOutputPINFO(fsm);
  int loop, edge_id;
  int new_node_id;
  
  edge_id = fsm_FindEdge(fsm, node_id, node_id);
  if ( edge_id < 0 )
    return 1;
    
  cl = fsm_GetEdgeOutput(fsm, edge_id);
  if ( cl == NULL )
    return 1;
    
  if ( dclCnt(cl) <= 1 )
  {
    fsm_GetNode(fsm, node_id)->user_val = 1;
    return 1;
  }
    
  if ( dclInitVA(3, &cl_eq, &cl_r, &cl_i) == 0 )
    return 0;
    
  if ( dclSeparateByOutput(pi, cl, cl_eq, cl_r) == 0 )
    return dclDestroyVA(3, cl_eq, cl_r, cl_i), 0;

  if ( dclCnt(cl_r) == 0 )
  {
    fsm_GetNode(fsm, node_id)->user_val = 1;
    return dclDestroyVA(3, cl_eq, cl_r, cl_i), 1;
  }
    
  if ( dclIntersectionList(pi, cl_i, cl_eq, cl_r) == 0 )
    return dclDestroyVA(3, cl_eq, cl_r, cl_i), 0;
    
  if ( dclSubtract(pi, cl_r, cl_i) == 0 )
    return dclDestroyVA(3, cl_eq, cl_r, cl_i), 0;

  if ( dclCnt(cl_r) == 0 )
    return dclDestroyVA(3, cl_eq, cl_r, cl_i), 1;
  
  new_node_id = fsm_DupNode(fsm, node_id);
  if ( new_node_id < 0 )
    return dclDestroyVA(3, cl_eq, cl_r, cl_i), 0;
  fsm_GetNode(fsm, new_node_id)->user_val = 1;
  
  /* apply self (stable) condition */
  
  edge_id = fsm_FindEdge(fsm, node_id, node_id);
  assert(edge_id >= 0);
  if ( dclCopy(pi, fsm_GetEdgeOutput(fsm, edge_id), cl_eq ) == 0 )
    return dclDestroyVA(3, cl_eq, cl_r, cl_i), 0;

  edge_id = fsm_FindEdge(fsm, new_node_id, new_node_id);
  assert(edge_id >= 0);
  if ( dclCopy(pi, fsm_GetEdgeOutput(fsm, edge_id), cl_r) == 0 )
    return dclDestroyVA(3, cl_eq, cl_r, cl_i), 0;

  /* restrict input conditions */
  
  /*
  cnt = dclCnt(cl_eq);
  for( i = 0; i < cnt; i++ )
    dcOutSetAll(pi, dclGet(cl_eq, i), CUBE_OUT_MASK);
  
  cnt = dclCnt(cl_r);
  for( i = 0; i < cnt; i++ )
    dcOutSetAll(pi, dclGet(cl_r, i), CUBE_OUT_MASK);
  */
  
  loop = -1;
  edge_id = -1;
  while( fsm_LoopNodeInEdges(fsm, node_id, &loop, &edge_id) != 0 )
    if ( fsm_GetEdgeSrcNode(fsm,edge_id) != fsm_GetEdgeDestNode(fsm,edge_id) )
      if ( dclSubtract(pi, fsm_GetEdgeOutput(fsm, edge_id), cl_r ) == 0 )
        return dclDestroyVA(3, cl_eq, cl_r, cl_i), 0;

  loop = -1;
  edge_id = -1;
  while( fsm_LoopNodeInEdges(fsm, new_node_id, &loop, &edge_id) != 0 )
    if ( fsm_GetEdgeSrcNode(fsm,edge_id) != fsm_GetEdgeDestNode(fsm,edge_id) )
      if ( dclSubtract(pi, fsm_GetEdgeOutput(fsm, edge_id), cl_eq ) == 0 )
        return dclDestroyVA(3, cl_eq, cl_r, cl_i), 0;

  fsm_GetNode(fsm, node_id)->user_val = 1;
  return dclDestroyVA(3, cl_eq, cl_r, cl_i), 1;
}


int fsm_ConvertToMoore(fsm_type fsm)
{
  int node_id;
  int is_dup_called;

  node_id = -1;
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
    fsm_GetNode(fsm, node_id)->user_val = 0;
  
  do
  {
    node_id = -1;
    is_dup_called = 0;
    while( fsm_LoopNodes(fsm, &node_id) != 0 )
      if ( fsm_GetNode(fsm, node_id)->user_val == 0 )
      {
        if ( fsm_DupMoore(fsm, node_id) == 0 )
          return 0;
        is_dup_called = 1;
      }
  } while( is_dup_called != 0 );
  
  return 1;
}


