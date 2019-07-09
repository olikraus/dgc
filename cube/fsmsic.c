/*

  fsmsic.c
  
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



  check/force single input change
  
  
  
  Let
    S be the self (stable) condition 
    I be the input conditions, excluding the self (stable) condition
    O be the output condition, excluding the self (stable) condition
    super(A,B) be the pairwise supercube of the elements of A and B
    
  Valid graph description
    RULE 1 (Valid Graph): 
      Output condition must not intersect with each other
    
  The stable condition is:
    RULE 2 (Stable States): 
      'I' must be equal or a subset of 'S'
      
  To have a unique description any outputs must not intersect.
  This is a result of RULE 1 and must been checked if 'S' has
  been changed.
    RULE 3 (Valid Graph): 
      'O' must not intersect with 'S'

  The SIC (single input change) requires, that the change of only
  one input signal causes a transition. This requires, that the
  self condition S also covers D1(super(I,O))-O.
    RULE 4: (Minimum SIC Cover)
      'D1(super(I,O))-O' must be equal or a subset of 'S'

  There can be output transition that can never be activated.
  If the D1 set of a output transition is covered by O, than
  the transition can never be used.
    WARN 1: (Illegal SIC Transitions)
      If D1(of a transition) - O is empty, then the transition
      is never used and can be removed.
    
  There can be output transitions that can only be activated
  if a signal changes from one value to another and back again.
  One should better create a different specification for this
  case.
    WARN 2: (Indirect SIC Transitions)
      Assume a transition T, which is a subset of O.
      If 'intersection(super(I,O)-O,D1(T))' is empty, then an
      input signal must change twice.

  Example for WARN 1:
    I(s1) = {{00}}
    O(s1) = {{01},{10},{11}}
    --> D1({01}) - {01,10,11} = {00,11} - {01,10,11} = {00}
        --> valid
    --> D1({11}) - {01,10,11} = {01,10} - {01,10,11} = {}
        --> invalid
      
  Example for WARN 2:
    I(s1) = {{000}}
    O(s1) = {{001},{010},{011}}
    Compare with WARN 1:
      D1({011}) - {001,010,011} = {111,001,010}-{001,010,011} = {111}
    Now WARN 2:
      super(I,O) = {0--} and 
      super(I,O)-O = {000}
      and intersection({000}, {111,001,010}) is empty, so output
      condition {011} requires an additional signal change.
      
  If we can assume, that only direct transitions are possible,
  RULE 4 is too strong. Note that no existing WARN 2 is neccessary
  but not sufficient for this assumption.
  We assume that any input change is within super(I,O) which can not
  be checked and must be enforced by the environment.
    RULE 5: (Minimum Direct SIC Cover)
      1. 'super(I,O)-O' must be equal or a subset of 'S'
      2. Under this condition, Indirect SIC Transitions (WARN 2)
         are also illegal transitions.
   
  Example for RULE 5:
    I(s1) = {{000}}
    O(s1) = {{011}}
    The stable condition 'super(I,O)-O' is {000,001,010}.
    If the input changes from 000 to 100 and back to 000,
    the behavior is unknown.
  
  
  
*/

#include "fsm.h"
#include <assert.h>
#include "mwc.h"


int fsm_CalcSICStableCondition(fsm_type fsm, int node_id, dclist cl)
{
  int loop, edge_id;
  dclClear(cl);

  loop = -1;
  edge_id = -1;
  while( fsm_LoopNodeOutEdges(fsm, node_id, &loop, &edge_id) != 0 )
    if ( fsm_GetEdgeSrcNode(fsm, edge_id) != fsm_GetEdgeDestNode(fsm, edge_id) )
      if ( dclAddDistance1(fsm->pi_cond, cl, fsm_GetEdgeCondition(fsm, edge_id)) == 0 )
        return 0;

  /*
  printf("D1(%s outgoing)\n", fsm_GetNodeName(fsm, node_id));
  dclShow(fsm->pi_cond, cl);
  */

  loop = -1;
  edge_id = -1;
  while( fsm_LoopNodeOutEdges(fsm, node_id, &loop, &edge_id) != 0 )
    if ( fsm_GetEdgeSrcNode(fsm, edge_id) != fsm_GetEdgeDestNode(fsm, edge_id) )
      if ( dclSubtract(fsm->pi_cond, cl, fsm_GetEdgeCondition(fsm, edge_id)) == 0 )
        return 0;

  /*
  printf("D1(%s outgoing) - outgoing\n", fsm_GetNodeName(fsm, node_id));
  dclShow(fsm->pi_cond, cl);
  */

  loop = -1;
  edge_id = -1;
  while( fsm_LoopNodeInEdges(fsm, node_id, &loop, &edge_id) != 0 )
    if ( fsm_GetEdgeSrcNode(fsm, edge_id) != fsm_GetEdgeDestNode(fsm, edge_id) )
      if ( dclSCCUnion(fsm->pi_cond, cl, fsm_GetEdgeCondition(fsm, edge_id)) == 0 )
        return 0;

  /*
  printf("D1(%s outgoing) - outgoing + incoming\n", fsm_GetNodeName(fsm, node_id));
  dclShow(fsm->pi_cond, cl);
  */

  return 1;
}

static void fsm_WarningSICNoStableState(fsm_type fsm, int edge_id)
{
  fsm_Log(fsm, "FSM SIC: Stable condition [%s %s] has been assigned.\n",
    fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, edge_id)),
    fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, edge_id))
    );
}

static void fsm_WarningSICStableStateReplaced(fsm_type fsm, int edge_id)
{
  fsm_Log(fsm, "FSM SIC: Stable condition [%s %s] has been replaced.\n",
    fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, edge_id)),
    fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, edge_id))
    );
}

static void fsm_WarningSICIllegalOutputRemoved(fsm_type fsm, int edge_id)
{
  fsm_Log(fsm, "FSM SIC: Illegal SIC transition [%s %s] removed.\n",
    fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, edge_id)),
    fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, edge_id))
    );
}

static void fsm_WarningSICOutputRestricted(fsm_type fsm, int edge_id)
{
  fsm_Log(fsm, "FSM SIC: SIC transition [%s %s] restricted.\n",
    fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, edge_id)),
    fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, edge_id))
    );
}

static int fsm_RestrictSICOutEdge(fsm_type fsm, dclist d1_stable, int edge_id, int do_change)
{
  dclist cl;
  if ( dclInit(&cl) == 0 )
    return 0;

  if ( dclIntersectionList(fsm->pi_cond, cl, d1_stable, fsm_GetEdgeCondition(fsm, edge_id)) == 0 )
    return dclDestroy(cl), 0;
  
  if ( dclCnt(cl) == 0 )
  {
    if ( do_change != 0 )
    {
      fsm_WarningSICIllegalOutputRemoved(fsm, edge_id);
      fsm_DeleteEdge(fsm, edge_id);
    }
    else
    {
      fsm_Log(fsm, "FSM SIC: Transition [%s %s] invalid (Add more stable conditions or remove this transition).",
        fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, edge_id)),
        fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, edge_id))
      );
    }
  }
  else
  {
    if ( dclIsEquivalent(fsm->pi_cond, cl, fsm_GetEdgeCondition(fsm, edge_id)) == 0 )
    {
      if ( do_change != 0 )
      {
        fsm_WarningSICOutputRestricted(fsm, edge_id);
        if ( dclCopy(fsm->pi_cond, fsm_GetEdgeCondition(fsm, edge_id), cl) == 0 )
          return dclDestroy(cl), 0;
      }
      else
      {
        int i;
        fsm_Log(fsm, "FSM SIC: Transition [%s %s] partially invalid (Add more stable conditions or restrict this transition).",
          fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, edge_id)),
          fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, edge_id))
        );
        for( i = 0; i < dclCnt(cl); i++ )
          fsm_Log(fsm, "FSM SIC:       Restricted condition: %s", 
            dcToStr(fsm_GetConditionPINFO(fsm), dclGet(cl, i), "", ""));
      }
    }
  }
  
  return dclDestroy(cl), 1;
}

static int fsm_RestrictSICNode(fsm_type fsm, int node_id, int do_change)
{
  dclist cl_d1s, cl_s;
  int loop, edge_id;
  if ( dclInitVA(2, &cl_d1s, &cl_s) == 0 )
    return 0;

  if ( fsm_GetNodeInCover(fsm, node_id, cl_s) == 0 )
    return dclDestroyVA(2, cl_d1s, cl_s), 0;

  if ( dclCnt(cl_s) == 0 )
  {
    fsm_Log(fsm, "FSM SIC: No input transitions for node %s.",
      fsm_GetNodeName(fsm, node_id)==NULL?"<unnamed>":fsm_GetNodeName(fsm, node_id)
    );
  }
    
  edge_id = fsm_FindEdge(fsm, node_id, node_id);
  if ( edge_id < 0 )
  {
    if ( do_change != 0 )
    {
      edge_id = fsm_Connect(fsm, node_id, node_id);
      if ( edge_id < 0 )
        return dclDestroyVA(2, cl_d1s, cl_s), 0;
      if ( dclCopy( fsm_GetConditionPINFO(fsm), 
        fsm_GetEdgeCondition(fsm, edge_id), cl_s ) == 0 )
        return dclDestroyVA(2, cl_d1s, cl_s), 0;
      fsm_WarningSICNoStableState(fsm, edge_id);
    }
    else
    {
      int i;
      fsm_Log(fsm, "FSM SIC: Stable condition [%s %s] does not exist.",
        fsm_GetNodeName(fsm, node_id),
        fsm_GetNodeName(fsm, node_id)
      );
      for( i = 0; i < dclCnt(cl_s); i++ )
        fsm_Log(fsm, "FSM SIC:   Minimum stable condition: %s", 
          dcToStr(fsm_GetConditionPINFO(fsm), dclGet(cl_s, i), "", ""));
      if ( fsm_CalcSICStableCondition(fsm, node_id, cl_s) == 0 )
        return dclDestroyVA(2, cl_d1s, cl_s), 0;
      for( i = 0; i < dclCnt(cl_s); i++ )
        fsm_Log(fsm, "FSM SIC:      Full stable condition: %s", 
          dcToStr(fsm_GetConditionPINFO(fsm), dclGet(cl_s, i), "", ""));
    }
  }

  if ( edge_id >= 0 )
  {
    if ( dclSubtract(fsm_GetConditionPINFO(fsm), cl_s, fsm_GetEdgeCondition(fsm, edge_id)) == 0 )
      return 0;
    if ( dclCnt(cl_s) > 0 )
    {
      if ( fsm_GetNodeInCover(fsm, node_id, cl_s) == 0 )
        return dclDestroyVA(2, cl_d1s, cl_s), 0;
      if ( do_change != 0 )
      {
        fsm_WarningSICStableStateReplaced(fsm, edge_id);
        if ( dclCopy( fsm_GetConditionPINFO(fsm), 
          fsm_GetEdgeCondition(fsm, edge_id), cl_s ) == 0 )
          return dclDestroyVA(2, cl_d1s, cl_s), 0;
      }
      else
      {
        int i;
        fsm_Log(fsm, "FSM SIC: Stable condition [%s %s] is not valid.",
          fsm_GetNodeName(fsm, node_id),
          fsm_GetNodeName(fsm, node_id)
        );
        for( i = 0; i < dclCnt(cl_s); i++ )
          fsm_Log(fsm, "FSM SIC:   Minimum stable condition: %s", 
            dcToStr(fsm_GetConditionPINFO(fsm), dclGet(cl_s, i), "", ""));
        if ( fsm_CalcSICStableCondition(fsm, node_id, cl_s) == 0 )
          return dclDestroyVA(2, cl_d1s, cl_s), 0;
        for( i = 0; i < dclCnt(cl_s); i++ )
          fsm_Log(fsm, "FSM SIC:      Full stable condition: %s", 
            dcToStr(fsm_GetConditionPINFO(fsm), dclGet(cl_s, i), "", ""));
      }
    }
  }

  edge_id = fsm_FindEdge(fsm, node_id, node_id);
  if ( edge_id < 0 )
  {
    if ( fsm_CalcSICStableCondition(fsm, node_id, cl_s) == 0 )
      return dclDestroyVA(2, cl_d1s, cl_s), 0;
  }
  else
  {
    if ( dclCopy( fsm_GetConditionPINFO(fsm), cl_s, 
      fsm_GetEdgeCondition(fsm, edge_id) ) == 0 )
      return dclDestroyVA(2, cl_d1s, cl_s), 0;
  } 
  if ( dclDistance1(fsm->pi_cond, cl_d1s, cl_s) == 0 )
    return dclDestroyVA(2, cl_d1s, cl_s), 0;
    
  loop = -1;
  edge_id = -1;
  while( fsm_LoopNodeOutEdges(fsm, node_id, &loop, &edge_id) != 0 )
    if ( fsm_GetEdgeSrcNode(fsm, edge_id) != fsm_GetEdgeDestNode(fsm, edge_id) )
      if ( fsm_RestrictSICOutEdge(fsm, cl_d1s, edge_id, do_change) == 0 )
        return dclDestroyVA(2, cl_d1s, cl_s), 0;

  return dclDestroyVA(2, cl_d1s, cl_s), 1;
}

int fsm_RestrictSIC(fsm_type fsm)
{
  int node_id;

  fsm_Log(fsm, "FSM: Force single input change conditions (SIC).");

  node_id = -1;
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
    if ( fsm_RestrictSICNode(fsm, node_id, 1) == 0 )
      return 0;
  return 1;
}

int fsm_CheckSIC(fsm_type fsm)
{
  int node_id;

  fsm_Log(fsm, "FSM: Check single input change conditions (SIC).");

  node_id = -1;
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
    if ( fsm_RestrictSICNode(fsm, node_id, 0) == 0 )
      return 0;
  return 1;
}

