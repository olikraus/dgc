/*

  fsmstable.c
  
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


  
  fsm_IsStateStable
    Checks if the states are stable
  
  There are several stable condition:
    FSM_STABLE_MINIMAL      minimum stable condition
    FSM_STABLE_CONTINOUS    continuous stable condition
    FSM_STABLE_IN_OUT       continuous stable from input to output condition
    FSM_STABLE_FULL         state must be fully stable

  fsm_DoMinimalStable
    forces minimum stable condition
    
  
  

*/

#include "fsm.h"
#include "mwc.h"

void fsm_LogStableState(fsm_type fsm, int node_id, int st, int is_log, 
  char *msg)
{
  char *node_name = fsm_GetNodeName(fsm, node_id);
  char *sts = "unknown";
  if ( node_name == NULL )
    node_name = "<unknown>";
  switch(st)
  {
    case FSM_STABLE_MINIMAL:    sts = "minimal"; break;
    case FSM_STABLE_CONTINOUS:  sts = "continous"; break;
    case FSM_STABLE_IN_OUT:     sts = "input/output"; break;
    case FSM_STABLE_FULL:       sts = "full"; break;
  }
  if ( is_log )
    fsm_Log(fsm, "'%s' stable violation with state '%s': %s",
      sts, node_name, msg);
}


int fsm_IsStateStable(fsm_type fsm, int node_id, int st, int is_log)
{
  dclist Tnn;
  int is_stable;
  int loop, edge_id;
  int i, cnt;
  dclist cl_pre, cl_post, cl_tmp;
  pinfo *pi_cond = fsm_GetConditionPINFO(fsm);
  dcube *r = &(pi_cond->tmp[9]);
  dcube *super_post = &(pi_cond->tmp[10]);


  if ( dclInitVA(3, &cl_pre, &cl_post, &cl_tmp) == 0 )
  {
    fsm_LogStableState(fsm, node_id, st, 1, "memory error.");
    return 0;
  }
  if ( fsm_GetNodePreCover(fsm, node_id, cl_pre) == 0 )
  {
    fsm_LogStableState(fsm, node_id, st, 1, "Can not create input cover (memory error?).");
    return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
  }
  if ( fsm_GetNodePostCover(fsm, node_id, cl_post) == 0 )
  {
    fsm_LogStableState(fsm, node_id, st, 1, "Can not create output cover (memory error?).");
    return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
  }
  
  if ( dclIntersectionList(pi_cond, cl_tmp, cl_pre, cl_post) == 0 )
  {
    fsm_LogStableState(fsm, node_id, st, 1, "Can not create in/out intersection (memory error?).");
    return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
  }
  
  if ( dclCnt(cl_tmp) != 0 )
  {
    fsm_LogStableState(fsm, node_id, st, is_log, "Same condition for input and output cover exist.");
    if ( is_log != 0 )
      fsm_LogDCL(fsm, "", 3, "pre", pi_cond, cl_pre, "post", pi_cond, cl_post, "intersect", pi_cond, cl_tmp);
    return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
  }

  Tnn = fsm_GetNodeSelfCondtion( fsm,  node_id);
  if ( Tnn == NULL )
  {
    fsm_LogStableState(fsm, node_id, st, is_log, "Self condition does not exist.");
    return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
  }
  
  
  is_stable = 0;
  switch(st)
  {
    case FSM_STABLE_MINIMAL:
      if ( dclIsSubsetList(pi_cond, Tnn, cl_pre) != 0 )
      {
        is_stable = 1;
      }
      else
      {
        fsm_LogStableState(fsm, node_id, st, is_log, "State not stable.");
      }
      break;
    case FSM_STABLE_CONTINOUS:
      if ( dclCopy(pi_cond, cl_tmp, cl_pre) == 0 )
      {
        fsm_LogStableState(fsm, node_id, st, 1, "memory error?");
        return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
      }
      dclSuper(pi_cond, r, cl_tmp);
      if ( dclCopy(pi_cond, cl_tmp, cl_post) == 0 )
      {
        fsm_LogStableState(fsm, node_id, st, 1, "memory error?");
        return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
      }
      if ( dclAdd(pi_cond, cl_tmp, r) < 0 )
      {
        fsm_LogStableState(fsm, node_id, st, 1, "memory error?");
        return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
      }
      dclSuper(pi_cond, r, cl_tmp);
      dclClear(cl_tmp);
      if ( dclAdd(pi_cond, cl_tmp, r) < 0 )
      {
        fsm_LogStableState(fsm, node_id, st, 1, "memory error?");
        return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
      }
      
      if ( dclSubtract(pi_cond, cl_tmp, cl_post) == 0 )
      {
        fsm_LogStableState(fsm, node_id, st, 1, "memory error?");
        return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
      }
      if ( dclIsSubsetList(pi_cond, Tnn, cl_tmp) != 0 )
      {
        is_stable = 1;
      }
      else
      {
        fsm_LogStableState(fsm, node_id, st, is_log, "State not stable.");
        if ( is_log != 0 )
          fsm_LogDCL(fsm, "", 4, "pre", pi_cond, cl_pre, "post", pi_cond, cl_post, "self", pi_cond, Tnn, "required", pi_cond, cl_tmp);
      }
      break;
    case FSM_STABLE_IN_OUT:
      loop = -1;
      edge_id = -1;
      dclSuper(pi_cond, super_post, cl_post);
      dclClear(cl_tmp);
      while( fsm_LoopNodeInEdges(fsm, node_id, &loop, &edge_id) != 0 )
      {
        dclist cl_cond = fsm_GetEdgeCondition(fsm, edge_id);
        cnt = dclCnt(cl_cond);
        for( i = 0; i < cnt; i++ )
        {
          dcOr(pi_cond, r, dclGet(cl_cond, i), super_post);
          if ( dclAdd(pi_cond, cl_tmp, r) < 0 )
          {
            fsm_LogStableState(fsm, node_id, st, 1, "memory error?");
            return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
          }
        }
      }
      dclSCC(pi_cond, cl_tmp);
      if ( dclSubtract(pi_cond, cl_tmp, cl_post) == 0 )
      {
        fsm_LogStableState(fsm, node_id, st, 1, "memory error?");
        return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
      }
      if ( dclIsSubsetList(pi_cond, Tnn, cl_tmp) != 0 )
      {
        is_stable = 1;
      }
      else
      {
        fsm_LogStableState(fsm, node_id, st, is_log, "State not stable.");
        if ( is_log != 0 )
          fsm_LogDCL(fsm, "", 4, "pre", pi_cond, cl_pre, "post", pi_cond, cl_post, "self", pi_cond, Tnn, "required", pi_cond, cl_tmp);
      }
      break;
    case FSM_STABLE_FULL:
      if ( dclCopy(pi_cond, cl_tmp, cl_post) == 0 )
      {
        fsm_LogStableState(fsm, node_id, st, 1, "memory error?");
        return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
      }
      if ( dclComplement(pi_cond, cl_tmp) == 0 )
      {
        fsm_LogStableState(fsm, node_id, st, 1, "Complement failed (memory error?).");
        return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
      }
      if ( dclIsEquivalent(pi_cond, Tnn, cl_tmp) != 0 )
      {
        is_stable = 1;
      }
      else
      {
        fsm_LogStableState(fsm, node_id, st, is_log, "State not stable.");
        if ( is_log != 0 )
          fsm_LogDCL(fsm, "", 4, "pre", pi_cond, cl_pre, "post", pi_cond, cl_post, "self", pi_cond, Tnn, "required", pi_cond, cl_tmp);
      }
      break;
    default:
      fsm_LogStableState(fsm, node_id, st, 1, "Unknown stable type.");
      return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), 0;
      
  }
  
  
  return dclDestroyVA(3, cl_pre, cl_post, cl_tmp), is_stable;
}


int fsm_IsStable(fsm_type fsm, int st, int is_log)
{
  int node_id = -1;
  int is_stable = 1;
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
  {
    if ( fsm_IsStateStable(fsm, node_id, st, is_log) == 0 )
      is_stable = 0;
  }
  return is_stable;
}

/*-------------------------------------------------------------------------*/

void fsm_LogMinimalStableState(fsm_type fsm, int node_id, int is_log, 
  char *msg)
{
  char *node_name = fsm_GetNodeName(fsm, node_id);
  if ( node_name == NULL )
    node_name = "<unknown>";
  if ( is_log )
    fsm_Log(fsm, "'minimal' stable generator (node '%s'): %s",
      node_name, msg);
}


int fsm_DoMinimalStableState(fsm_type fsm, int node_id, int is_log)
{
  dclist Tnn, o_Tnn;
  int loop, edge_id;
  int i, cnt;
  dclist cl_pre, cl_post, cl_req, cl_tmp, cl_o_req, cl_o_tmp;
  pinfo *pi_cond = fsm_GetConditionPINFO(fsm);
  pinfo *pi_out = fsm_GetOutputPINFO(fsm);
  dcube *c;

  if ( dclInitVA(4, &cl_pre, &cl_post, &cl_req, &cl_tmp) == 0 )
  {
    fsm_LogMinimalStableState(fsm, node_id, 1, "memory error.");
    return 0;
  }
  if ( dclInitVA(2, &cl_o_req, &cl_o_tmp) == 0 )
  {
    fsm_LogMinimalStableState(fsm, node_id, 1, "memory error.");
    return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), 0;
  }
  
  if ( fsm_GetNodePreCover(fsm, node_id, cl_pre) == 0 )
  {
    fsm_LogMinimalStableState(fsm, node_id, 1, "Can not create input cover (memory error?).");
    return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
  }
  if ( fsm_GetNodePostCover(fsm, node_id, cl_post) == 0 )
  {
    fsm_LogMinimalStableState(fsm, node_id, 1, "Can not create output cover (memory error?).");
    return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
  }
  
  if ( dclIntersectionList(pi_cond, cl_tmp, cl_pre, cl_post) == 0 )
  {
    fsm_LogMinimalStableState(fsm, node_id, 1, "Can not create in/out intersection (memory error?).");
    return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
  }
  
  if ( dclCnt(cl_tmp) != 0 )
  {
    fsm_LogMinimalStableState(fsm, node_id, is_log, "Same condition for input and output cover exist.");
    if ( is_log != 0 )
      fsm_LogDCL(fsm, "", 3, "pre", pi_cond, cl_pre, "post", pi_cond, cl_post, "intersect", pi_cond, cl_tmp);
    return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
  }

  Tnn = fsm_GetNodeSelfCondtion(fsm,  node_id);
  if ( Tnn == NULL )
  {
    if ( fsm_Connect(fsm, node_id, node_id) < 0 )
    {
      fsm_LogMinimalStableState(fsm, node_id, is_log, "Can not create self condition (memory error?).");
      return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
    }
    Tnn = fsm_GetNodeSelfCondtion(fsm,  node_id);
    if ( Tnn == NULL )
    {
      fsm_LogMinimalStableState(fsm, node_id, is_log, "Internal error.");
      return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
    }
  }
  
  o_Tnn = fsm_GetNodeSelfOutput(fsm, node_id);
  if ( o_Tnn == NULL )
  {
    fsm_LogMinimalStableState(fsm, node_id, is_log, "Internal error.");
    return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
  }
  
  if ( dclCopy(pi_cond, cl_req, cl_pre) == 0 )
  {
    fsm_LogMinimalStableState(fsm, node_id, 1, "Copy operation failed (memory error?).");
    return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
  }

  if ( dclSubtract(pi_cond, cl_req, Tnn) == 0 )
  {
    fsm_LogMinimalStableState(fsm, node_id, 1, "Cover subtraction failed (memory error?).");
    return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
  }
  
  if ( dclCnt(cl_req) == 0 )
  {
    /* this state is minimal stable */
    return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 1;
  }
  
  cnt = dclCnt(cl_req);
  for( i = 0; i < cnt; i++ )
  {
    c = dclAddEmptyCube(pi_out, cl_o_req);
    if ( c == NULL )
    {
      fsm_LogMinimalStableState(fsm, node_id, 1, "Add operation failed (memory error?).");
      return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
    }
    dcCopyInToIn(pi_out, c, 0, pi_cond, dclGet(cl_req, i));
    dcOutSetAll(pi_out, c, CUBE_OUT_MASK);
  }
  

  loop = -1;
  edge_id = -1;
  while( fsm_LoopNodeInEdges(fsm, node_id, &loop, &edge_id) != 0 && dclCnt(cl_req) != 0 )
  {
    if ( dclIntersectionList(pi_cond, cl_tmp, fsm_GetEdgeCondition(fsm, edge_id), cl_req) == 0 )
    {
      fsm_LogMinimalStableState(fsm, node_id, 1, "Intersection operation failed (memory error?).");
      return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
    }
    if ( dclIntersectionList(pi_out, cl_o_tmp, fsm_GetEdgeOutput(fsm, edge_id), cl_o_req) == 0 )
    {
      fsm_LogMinimalStableState(fsm, node_id, 1, "Intersection operation failed (memory error?).");
      return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
    }
    if ( dclCnt(cl_tmp) != 0 )
    {
      fsm_LogMinimalStableState(fsm, node_id, is_log, "Stable condition expanded");
      if ( is_log != 0 )
        fsm_LogDCL(fsm, "", 3, "in cond", pi_cond, fsm_GetEdgeCondition(fsm, edge_id), " curr self", pi_cond, Tnn, " expand", pi_cond, cl_tmp);
      if ( dclSCCUnion(pi_cond, Tnn, cl_tmp) == 0 )
      {
        fsm_LogMinimalStableState(fsm, node_id, 1, "Union operation failed (memory error?).");
        return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
      }

      if ( dclSCCUnion(pi_out, o_Tnn, cl_o_tmp) == 0 )
      {
        fsm_LogMinimalStableState(fsm, node_id, 1, "Union operation failed (memory error?).");
        return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
      }

      if ( dclSubtract(pi_cond, cl_req, cl_tmp) == 0 )
      {
        fsm_LogMinimalStableState(fsm, node_id, 1, "Cover subtraction failed (memory error?).");
        return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
      }

      if ( dclSubtract(pi_out, cl_o_req, cl_o_tmp) == 0 )
      {
        fsm_LogMinimalStableState(fsm, node_id, 1, "Cover subtraction failed (memory error?).");
        return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
      }
      
    }
  }
  
  if ( dclCnt(cl_req) != 0 )
  {
    fsm_LogMinimalStableState(fsm, node_id, is_log, "Internal error.");
    return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 0;
  }

  fsm_LogMinimalStableState(fsm, node_id, is_log, "Stable condition has been expanded to minimal stable condition");
  if ( is_log != 0 )
    fsm_LogDCL(fsm, "", 3, "pre", pi_cond, cl_pre, "self", pi_cond, Tnn, "output", pi_out, o_Tnn);
  
  return dclDestroyVA(4, cl_pre, cl_post, cl_req, cl_tmp), dclDestroyVA(2, cl_o_req, cl_o_tmp), 1;
}

int fsm_DoMinimalStable(fsm_type fsm, int is_log)
{
  int node_id = -1;
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
  {
    if ( fsm_DoMinimalStableState(fsm, node_id, is_log) == 0 )
      return 0;
  }
  return 1;
}

/*-------------------------------------------------------------------------*/

void fsm_LogInOutStableState(fsm_type fsm, int node_id, int is_log, 
  char *msg)
{
  char *node_name = fsm_GetNodeName(fsm, node_id);
  if ( node_name == NULL )
    node_name = "<unknown>";
  if ( is_log )
    fsm_Log(fsm, "'input-output' stable generator (node '%s'): %s",
      node_name, msg);
}

int fsm_DoInOutStableState(fsm_type fsm, int node_id, int is_log)
{
  dclist cl_post, cl_new, cl_t, Tnn;
  pinfo *pi_out = fsm_GetOutputPINFO(fsm);
  int loop, edge_id, i, cnt;
  dcube *s_post = &(pi_out->tmp[9]);
  dcube *s_all = &(pi_out->tmp[10]);
  
  if ( dclInitVA(2, &cl_post, &cl_new) == 0 )
    return 0;
  
  if ( fsm_GetNodePostOutput(fsm, node_id, cl_post) == 0 )
  {
    fsm_LogInOutStableState(fsm, node_id, 1, "Can not create output cover (memory error?).");
    return dclDestroyVA(2, cl_post, cl_new), 0;
  }
  
  dclSuper(pi_out, s_post, cl_post);
  dclClear(cl_new);
  dclClearFlags(cl_new);

  loop = -1;
  edge_id = -1;
  while( fsm_LoopNodeInEdges(fsm, node_id, &loop, &edge_id) != 0 )
  {
    if ( fsm_GetEdgeSrcNode(fsm,edge_id) != node_id )
    {
      cl_t = fsm_GetEdgeOutput(fsm, edge_id);
      cnt = dclCnt(cl_t);
      for( i = 0; i < cnt; i++ )
      {
        dcOr(pi_out, s_all, dclGet(cl_t, i), s_post);
        dcCopyOut(pi_out, s_all, dclGet(cl_t, i));
        if ( dclSCCAddAndSetFlag(pi_out, cl_new, s_all) == 0 )
        {
          fsm_LogInOutStableState(fsm, node_id, 1, "Add operation failed (memory error?).");
          return dclDestroyVA(2, cl_post, cl_new), 0;
        }
      }
    }
  }
  

  Tnn = fsm_GetNodeSelfOutput(fsm, node_id);
  if ( Tnn == NULL )
  {
    if ( fsm_Connect(fsm, node_id, node_id) < 0 )
    {
      fsm_LogInOutStableState(fsm, node_id, is_log, "Can not create self condition (memory error?).");
      return dclDestroyVA(2, cl_post, cl_new), 0;
    }
    Tnn = fsm_GetNodeSelfOutput(fsm, node_id);
    if ( Tnn == NULL )
    {
      fsm_LogInOutStableState(fsm, node_id, is_log, "Internal error.");
      return dclDestroyVA(2, cl_post, cl_new), 0;
    }
  }
  
  dclSetOutAll(pi_out, cl_post, CUBE_OUT_MASK);
  
  if ( dclSubtract(pi_out, cl_new, cl_post) == 0 )
  {
    fsm_LogInOutStableState(fsm, node_id, is_log, "Subtract operation failed.");
    return dclDestroyVA(2, cl_post, cl_new), 0;
  }

  if ( dclSCCUnion(pi_out, Tnn, cl_new) == 0 )
  {
    fsm_LogInOutStableState(fsm, node_id, is_log, "Union operation failed.");
    return dclDestroyVA(2, cl_post, cl_new), 0;
  }
  
  fsm_LogInOutStableState(fsm, node_id, is_log, "Stable condition has been expanded to input output stability");
  if ( is_log != 0 )
  {
    if ( fsm_GetNodePostOutput(fsm, node_id, cl_post) == 0 )
    {
      fsm_LogInOutStableState(fsm, node_id, 1, "Can not create output cover (memory error?).");
      return dclDestroyVA(2, cl_post, cl_new), 0;
    }
    fsm_LogDCL(fsm, "", 2, "self", pi_out, Tnn, " output", pi_out, cl_post);
  }
    
  return dclDestroyVA(2, cl_post, cl_new), 1;
}

int fsm_DoInOutStable(fsm_type fsm, int is_log)
{
  int node_id = -1;
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
  {
    if ( fsm_DoInOutStableState(fsm, node_id, is_log) == 0 )
      return 0;
  }
  return 1;
}
