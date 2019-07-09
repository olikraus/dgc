/*

  fsmbm.c
  
  create a burst mode (extended fundamental) asynchronous state machine
  
  
  fsmbm.c builds more robust state machines than fsmbmm.c, but they might
  be much larger.
  
  
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
#include "mwc.h"
#include <assert.h>

static int dclMergeEqualIn(pinfo *pi, dclist cl)
{
  int i, j, cnt;
  cnt = dclCnt(cl);
  if ( dclClearFlags(cl) == 0 )
    return 0;
  for( i = 0; i < cnt; i++ )
  {
    for( j = i+1; j < cnt; j++ )
    {
      if ( dcIsEqualIn(pi, dclGet(cl, i), dclGet(cl, j)) != 0 )
      {
        dcOrOut(pi, dclGet(cl, i), dclGet(cl, i), dclGet(cl, j));
        dclSetFlag(cl, j);
      }
    }
  }
  dclDeleteCubesWithFlag(pi, cl);
  return 1;
}

static void dclExpandInIntoSet(pinfo *pi, dcube *c, dclist cl)
{
  int i;
  int s;
  for( i = 0; i < pi->in_cnt; i++ )
  {
    s = dcGetIn(c, i);
    if ( s == 1 || s == 2 )
    {
      dcSetIn(c, i, 3-s);
      if ( dclIsSubSet(pi, cl, c) == 0 )
        dcSetIn(c, i, s);
      else
        dcSetIn(c, i, 3);
    }
  }
}

static int dclExpandListIntoSet(pinfo *pi, dclist a, dclist b)
{
  int i, j, cnt = dclCnt(a);
  dclist n;
  if ( dclInit(&n) == 0 )
    return 0;
  if ( dclCopy(pi, n, a) == 0 )
    return dclDestroy(n), 0;
  for( i = 0; i < cnt; i++ )
    dclExpandInIntoSet(pi, dclGet(a, i), b);

  /* assume: each element in a corresponds to an element in n at the same pos */    
  
  if ( dclClearFlags(a) == 0 )
    return dclDestroy(n), 0;
    
  for( i = 0; i < cnt; i++ )
  {
    if ( dclIsFlag(a, i) == 0 )
    {
      for( j = 0; j < cnt; j++ )
      {
        if ( j != i && dclIsFlag(a, j) == 0 )
        {
          if ( dcIsSubSet(pi, dclGet(a, j), dclGet(n, i)) != 0 )
            dclSetFlag(a, i);
        }
      }
    }
  }
  dclDeleteCubesWithFlag(pi, a);

  return dclDestroy(n), 1;
}

/* calculates a hazard free consensus, modifes a and b, creates result r */
static int dcConsensusHF(pinfo *pi, dcube *r, dcube *a, dcube *b)
{
  int d_in;
  int d_out;
  
  d_out = dcDeltaOut(pi, a, b);
  if ( d_out != 0 )
    return 0;
  
  d_in = dcDeltaIn(pi, a, b);
  if ( d_in != 1 )
    return 0;
    
  if ( dcConsensus(pi, r, a, b) == 0 )
    return 0;
    
  if ( dcIsInSubSet(pi, r, a) == 0 )
    return 0;

  if ( dcIsInSubSet(pi, r, b) == 0 )
    return 0;

  /* minimize only of a or b could be deleted */
  if ( dcIsSubSet(pi, r, a) == 0 && dcIsSubSet(pi, r, b) == 0 )
    return 0;

  dcNotAndOut(pi, a, r, a);
  dcNotAndOut(pi, b, r, b);

  return 1;
}

static int dclHeuristicMinimizeHF(pinfo *pi, dclist cl)
{
  dcube *r = &(pi->tmp[11]);
  int i, j, cnt;
  
  if ( dclClearFlags(cl) == 0 )
    return 0;
  
  cnt = dclCnt(cl);
  for( i = 0; i < cnt; i++ )
    if ( dclIsFlag(cl, i) == 0 )
      for( j = i+1; j < cnt; j++ )
        if ( dclIsFlag(cl, j) == 0 )
          if ( dcConsensusHF(pi, r, dclGet(cl, i), dclGet(cl, j)) != 0 )
          {
            if ( dcIsOutIllegal(pi, dclGet(cl, i)) != 0 )
              dclSetFlag(cl, i);
            if ( dcIsOutIllegal(pi, dclGet(cl, j)) != 0 )
              dclSetFlag(cl, j);
            if ( dclAdd(pi, cl, r) < 0 )
              return 0;
          }
          
  dclDeleteCubesWithFlag(pi, cl);
  return 1;
}

static int fsm_bm_to_out_cube(fsm_type fsm, dcube *d, int src_node, int dest_node, dcube *o, int out, int use_in_code)
{
  dcube *c = &(fsm->pi_machine->tmp[9]);
  pinfo *pio = fsm_GetOutputPINFO(fsm);
  pinfo *pim = fsm->pi_machine;

  dcInSetAll(pim, d, CUBE_IN_MASK_DC);
  dcOutSetAll(pim, d, 0);
  
  /* the second part of the input is the super cube of */
  /* the two state codes */
  
  if ( use_in_code == 0 )
  {
    dcCopyOutToIn(pim, d, 
      pio->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node));
    dcCopyOutToIn(pim, c, 
      pio->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node));
  }
  else
  {
    dcCopyInToIn(pim, d, 
      pio->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node));
    dcCopyInToIn(pim, c, 
      pio->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node));
  }
  
  dcOr(pim, d, d, c);
  dcOutSetAll(pim, d, 0);   /* we should use something like dcOrIn here */


  /* the first part of the input is the condition for this transition */

  dcCopyInToIn(pim, d, 0, pio, o);

  /* the first part of the output is always zero */

  /* the second part of the output is the output of the state machine */

  assert(fsm->code_width+out < pim->out_cnt);
  dcSetOut(d, fsm->code_width+out, 1);
  
  if ( dcIsIllegal(pim, d) != 0 )
    return 0;

  return 1;
}

static int fsm_bm_fn_output(fsm_type fsm, int edge, int o, int use_in_code)
{
  int src_node = fsm_GetEdgeSrcNode(fsm, edge);
  int dest_node = fsm_GetEdgeDestNode(fsm, edge);
  int self_src_edge = fsm_FindEdge(fsm, src_node, src_node);
  int self_dest_edge = fsm_FindEdge(fsm, dest_node, dest_node);
  pinfo *pim = fsm->pi_machine;
  dclist st_cl, td_cl, all_cl;
  dclist s_cl, d_cl, t_cl;
  int i;
  dcube *c = &(fsm->pi_machine->tmp[10]);
  
  if ( self_src_edge < 0 || self_dest_edge < 0 )
  {
    fsm_Log(fsm, "FSM: Missing stable condition (calculation of output function).");
    return 0;
  }
  
  t_cl = fsm_GetEdgeOutput(fsm, edge);
  d_cl = fsm_GetEdgeOutput(fsm, self_dest_edge);
  s_cl = fsm_GetEdgeOutput(fsm, self_src_edge);
  
  if ( dclInitCachedVA(pim, 3, &st_cl, &td_cl, &all_cl) == 0 )
    return 0;
    
  for( i = 0; i < dclCnt(s_cl); i++ )
    if ( dcGetOut(dclGet(s_cl, i), o) != 0 )
      if ( fsm_bm_to_out_cube(fsm, c, src_node, src_node, dclGet(s_cl, i), o, use_in_code) != 0 )
        if ( dclAdd(pim, st_cl, c) < 0 )
          return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  for( i = 0; i < dclCnt(s_cl); i++ )
    if ( fsm_bm_to_out_cube(fsm, c, src_node, src_node, dclGet(s_cl, i), o, use_in_code) != 0 )
      if ( dclAdd(pim, all_cl, c) < 0 )
        return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  for( i = 0; i < dclCnt(t_cl); i++ )
    if ( dcGetOut(dclGet(t_cl, i), o) != 0 )
      if ( fsm_bm_to_out_cube(fsm, c, src_node, dest_node, dclGet(t_cl, i), o, use_in_code) != 0 )
      {
        if ( dclAdd(pim, st_cl, c) < 0 )
          return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
        if ( dclAdd(pim, td_cl, c) < 0 )
          return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
      }

  for( i = 0; i < dclCnt(t_cl); i++ )
    if ( fsm_bm_to_out_cube(fsm, c, src_node, dest_node, dclGet(t_cl, i), o, use_in_code) != 0 )
      if ( dclAdd(pim, all_cl, c) < 0 )
        return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  for( i = 0; i < dclCnt(d_cl); i++ )
    if ( dcGetOut(dclGet(d_cl, i), o) != 0 )
      if ( fsm_bm_to_out_cube(fsm, c, dest_node, dest_node, dclGet(d_cl, i), o, use_in_code) != 0 )
        if ( dclAdd(pim, td_cl, c) < 0 )
          return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  for( i = 0; i < dclCnt(d_cl); i++ )
    if ( fsm_bm_to_out_cube(fsm, c, dest_node, dest_node, dclGet(d_cl, i), o, use_in_code) != 0 )
      if ( dclAdd(pim, all_cl, c) < 0 )
        return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  if ( dclPrimes(pim, st_cl) == 0 )
    return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
  if ( dclPrimes(pim, td_cl) == 0 )
    return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;


  /*
  printf("-- %d (%s -> %s) --\n", edge, fsm_GetNodeName(fsm, src_node), fsm_GetNodeName(fsm, dest_node));
  dclShow(pim, cl);
  */

  /*
  if (dclCnt(st_cl) != 0 || dclCnt(td_cl) != 0)
  {
    fsm_Log(fsm, "FSM: Output function %d.", o);
    fsm_LogDCL(fsm, "FSM: ", 2, "Src-T", pim, st_cl, "T-Dest", pim, td_cl);
  }
  */

  for( i = 0; i < dclCnt(st_cl); i++ )
    if ( dclAdd(pim, fsm->cl_machine, dclGet(st_cl, i)) < 0 )
      return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
  for( i = 0; i < dclCnt(td_cl); i++ )
    if ( dclAdd(pim, fsm->cl_machine, dclGet(td_cl, i)) < 0 )
      return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  if ( dclSubtract(pim, fsm->cl_machine_dc, all_cl) == 0 )
    return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
    
  /*
  fsm_LogDCL(fsm, "FSM: ", 2, "transition area", fsm->pi_machine, all_cl, "don't care function", fsm->pi_machine, fsm->cl_machine_dc);
  */
      
  return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 1;
}

static int fsm_bm_to_state_cube(fsm_type fsm, dcube *d, int src_node, int dest_node, dcube *o, int z, int use_in_code)
{
  dcube *c = &(fsm->pi_machine->tmp[9]);
  pinfo *pio = fsm_GetOutputPINFO(fsm);
  pinfo *pim = fsm->pi_machine;

  dcInSetAll(pim, d, CUBE_IN_MASK_DC);
  dcOutSetAll(pim, d, 0);
  
  /* the second part of the input is the super cube of */
  /* the two state codes */
  
  if ( use_in_code == 0 )
  {
    dcCopyOutToIn(pim, d, 
      pio->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node));
    dcCopyOutToIn(pim, c, 
      pio->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node));
  }
  else
  {
    dcCopyInToIn(pim, d, 
      pio->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node));
    dcCopyInToIn(pim, c, 
      pio->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node));
  }
  dcOr(pim, d, d, c);
  dcOutSetAll(pim, d, 0);   /* we should use something like dcOrIn here */


  /* the first part of the input is the condition for this transition */

  dcCopyInToIn(pim, d, 0, pio, o);

  /* the first part of the output contains the state code */

  assert(z < pim->out_cnt);
  dcSetOut(d, z, 1);

  /* the second part of the output is always zero */

  /* dcSetOut(d, fsm->code_width+out, 1); */

  if ( dcIsIllegal(pim, d) != 0 )
    return 0;

  return 1;
}

static int fsm_bm_fn_next_state(fsm_type fsm, int edge, int z, int use_in_code)
{
  int src_node = fsm_GetEdgeSrcNode(fsm, edge);
  int dest_node = fsm_GetEdgeDestNode(fsm, edge);
  int self_src_edge = fsm_FindEdge(fsm, src_node, src_node);
  int self_dest_edge = fsm_FindEdge(fsm, dest_node, dest_node);
  pinfo *pim = fsm->pi_machine;
  dclist st_cl, td_cl, all_cl;
  dclist s_cl, d_cl, t_cl;
  dcube *src_c = fsm_GetNodeCode(fsm, src_node);
  dcube *dest_c = fsm_GetNodeCode(fsm, dest_node);
  int i;
  dcube *c = &(fsm->pi_machine->tmp[10]);

  if ( self_src_edge < 0 || self_dest_edge < 0 )
  {
    fsm_Log(fsm, "FSM: Missing stable condition.");
    return 0;
  }
  
  t_cl = fsm_GetEdgeOutput(fsm, edge);
  d_cl = fsm_GetEdgeOutput(fsm, self_dest_edge);
  s_cl = fsm_GetEdgeOutput(fsm, self_src_edge);
  
  if ( dclInitCachedVA(pim, 3, &st_cl, &td_cl, &all_cl) == 0 )
    return 0;
    
  if ( dcGetOut(src_c, z) != 0 )
    for( i = 0; i < dclCnt(s_cl); i++ )
      if ( fsm_bm_to_state_cube(fsm, c, src_node, src_node, dclGet(s_cl, i), z, use_in_code) != 0 )
        if ( dclAdd(pim, st_cl, c) < 0 )
          return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  for( i = 0; i < dclCnt(s_cl); i++ )
    if ( fsm_bm_to_state_cube(fsm, c, src_node, src_node, dclGet(s_cl, i), z, use_in_code) != 0 )
      if ( dclAdd(pim, all_cl, c) < 0 )
         return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  if ( dcGetOut(dest_c, z) != 0 )
    for( i = 0; i < dclCnt(t_cl); i++ )
      if ( fsm_bm_to_state_cube(fsm, c, src_node, dest_node, dclGet(t_cl, i), z, use_in_code) != 0 )
      {
        if ( dclAdd(pim, st_cl, c) < 0 )
          return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
        if ( dclAdd(pim, td_cl, c) < 0 )
          return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
      }
      
  for( i = 0; i < dclCnt(t_cl); i++ )
    if ( fsm_bm_to_state_cube(fsm, c, src_node, dest_node, dclGet(t_cl, i), z, use_in_code) != 0 )
      if ( dclAdd(pim, all_cl, c) < 0 )
         return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;


  if ( dcGetOut(dest_c, z) != 0 )
    for( i = 0; i < dclCnt(d_cl); i++ )
      if ( fsm_bm_to_state_cube(fsm, c, dest_node, dest_node, dclGet(d_cl, i), z, use_in_code) != 0 )
        if ( dclAdd(pim, td_cl, c) < 0 )
          return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  for( i = 0; i < dclCnt(d_cl); i++ )
    if ( fsm_bm_to_state_cube(fsm, c, dest_node, dest_node, dclGet(d_cl, i), z, use_in_code) != 0 )
      if ( dclAdd(pim, all_cl, c) < 0 )
         return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  if ( dclPrimes(pim, st_cl) == 0 )
    return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
  if ( dclPrimes(pim, td_cl) == 0 )
    return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;

  /*
  printf("-- %d (%s -> %s) --\n", edge, fsm_GetNodeName(fsm, src_node), fsm_GetNodeName(fsm, dest_node));
  dclShow(pim, cl);
  */

  /*
  if (dclCnt(st_cl) != 0 || dclCnt(td_cl) != 0)
  {
    fsm_Log(fsm, "FSM: Next state function %d.", z);
    fsm_LogDCL(fsm, "FSM: ", 2, "Src-T", pim, st_cl, "T-Dest", pim, td_cl);
  }
  */

  for( i = 0; i < dclCnt(st_cl); i++ )
    if ( dclAdd(pim, fsm->cl_machine, dclGet(st_cl, i)) < 0 )
      return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
  for( i = 0; i < dclCnt(td_cl); i++ )
    if ( dclAdd(pim, fsm->cl_machine, dclGet(td_cl, i)) < 0 )
      return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
 
  if ( dclSubtract(pim, fsm->cl_machine_dc, all_cl) == 0 )
    return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 0;
  /*
  fsm_LogDCL(fsm, "FSM: ", 2, "transition area", fsm->pi_machine, all_cl, "don't care function", fsm->pi_machine, fsm->cl_machine_dc);
  */
      
  return dclDestroyCachedVA(pim, 3, st_cl, td_cl, all_cl), 1;
}


static int fsm_bm_self_to_different(fsm_type fsm, int edge, int use_in_code)
{
  int src_node = fsm_GetEdgeSrcNode(fsm, edge);
  int self_edge = fsm_FindEdge(fsm, src_node, src_node);
  dclist s_cl, d_cl;
  int o, z;
  
  if ( self_edge < 0 )
    return 0;

  /*
  {
    int src_node = fsm_GetEdgeSrcNode(fsm, edge);
    int dest_node = fsm_GetEdgeDestNode(fsm, edge);
    char *src_name = fsm_GetNodeName(fsm, src_node);
    char *dest_name = fsm_GetNodeName(fsm, dest_node);
    dcube *src_c = fsm_GetNodeCode(fsm, src_node);
    dcube *dest_c = fsm_GetNodeCode(fsm, dest_node);
    fsm_Log(fsm, "FSM: Transition from '%s' (code '%s', node %d) to '%s' (code '%s', node %d).", 
      src_name==NULL?"<unknown>":src_name,
      dcToStr(fsm_GetCodePINFO(fsm), src_c, "", ""),
      src_node,
      dest_name==NULL?"<unknown>":dest_name,
      dcToStr(fsm_GetCodePINFO(fsm), dest_c, "", ""),
      dest_node
      );
  }
  */

  
  d_cl = fsm_GetEdgeOutput(fsm, edge);
  s_cl = fsm_GetEdgeOutput(fsm, self_edge);

  for( o = 0; o < fsm->pi_machine->out_cnt - fsm_GetCodeWidth(fsm); o++ )
  {
    if ( fsm_bm_fn_output(fsm, edge, o, use_in_code) == 0 )
      return 0;
  }

  for( z = 0; z < fsm_GetCodeWidth(fsm); z++ )
  {
    if ( fsm_bm_fn_next_state(fsm, edge, z, use_in_code) == 0 )
      return 0;
  }
  
  return 1;
}

static int fsm_bm_edge(fsm_type fsm, int edge, int use_in_code)
{
  int src_node = fsm_GetEdgeSrcNode(fsm, edge);
  int dest_node = fsm_GetEdgeDestNode(fsm, edge);
  if ( src_node != dest_node )
    return fsm_bm_self_to_different(fsm, edge, use_in_code);
  return 1;
}

int fsm_BuildRobustBurstModeTransferFunction(fsm_type fsm)
{
  int edge_id;
  int use_in_code = 1;
  dclRealClear(fsm->cl_machine);
  dclRealClear(fsm->cl_machine_dc);
  if ( dclAdd(fsm->pi_machine, fsm->cl_machine_dc, 
    &(fsm->pi_machine->tmp[0])) < 0 )
  {
    fsm_Log(fsm, "FSM: Memory error (dclAdd).");
    return 0;
  }

  fsm_Log(fsm, "FSM: Building 'robust' hazardfree control function.");

  edge_id = -1;
  while( fsm_LoopEdges(fsm, &edge_id) != 0 )
    if ( fsm_bm_edge(fsm, edge_id, use_in_code) == 0 )
      return 0;

  {
    dclist cl;
    fsm_Log(fsm, "FSM: Checking don't care function.");
    dclInit(&cl);
    dclIntersectionList(fsm->pi_machine, cl, fsm->cl_machine, fsm->cl_machine_dc);
    if ( dclCnt(cl) != 0 )
    {
      fsm_Log(fsm, "FSM: Invald don't care function.");
      return 0;
    }
    dclDestroy(cl);
  }

  if ( dclSCC(fsm->pi_machine, fsm->cl_machine) == 0 )
  {
    fsm_Log(fsm, "FSM: Memory error (SCC operation).");
    return 0;
  }

  fsm_LogDCL(fsm, "FSM: ", 1, "Plain control function", fsm->pi_machine, fsm->cl_machine);

  fsm_Log(fsm, "FSM: Expanding 'robust' hazardfree control function (Expand into dc-set).");
  dclExpandListIntoSet(fsm->pi_machine, fsm->cl_machine, fsm->cl_machine_dc);
  
  fsm_LogDCL(fsm, "FSM: ", 1, "Expanded control function", fsm->pi_machine, fsm->cl_machine);

  fsm_Log(fsm, "FSM: Minimizing 'robust' hazardfree control function (Merge equal inputs).");
  if ( dclMergeEqualIn(fsm->pi_machine, fsm->cl_machine) == 0 )
  {
    fsm_Log(fsm, "FSM: Memory error (Output merge operation).");
    return 0;
  }


  fsm_Log(fsm, "FSM: Minimizing 'robust' hazardfree control function (Consensus on inputs).");
  if ( dclHeuristicMinimizeHF(fsm->pi_machine, fsm->cl_machine) == 0 )
  {
    fsm_Log(fsm, "FSM: Memory error (Heuristic minimize operation).");
    return 0;
  }
  
  fsm_Log(fsm, "FSM: Control function finished, %d implicants.", dclCnt(fsm->cl_machine));
  fsm_LogDCL(fsm, "FSM: ", 1, "Minimized control function", fsm->pi_machine, fsm->cl_machine);

  /*
  fsm_LogDCL(fsm, "FSM: ", 1, "don't care function", fsm->pi_machine, fsm->cl_machine_dc);
  */

  return 1;
}


/*
  enc_option is the same as for 'option' in fsm_EncodePartition().
  check_level: 0  no check
  check_level: 1  check transitions without self transitions
  check_level: 2  check all transitions
  
*/
int fsm_BuildBurstModeMachine(fsm_type fsm, int check_level, int enc_option)
{
  int is_self_transition = 0;
  
  fsm_Log(fsm, "FSM: Building control function for burst mode machine.");
  if ( fsm_EncodePartition(fsm, enc_option) == 0 )
    return 0;
    
  if ( fsm_CheckSIC(fsm) == 0 )
    return 0;

  if ( fsm_BuildSmallBurstModeTransferFunction(fsm, enc_option) == 0 )
  {
    fsm_Log(fsm, "FSM: Failed to build 'small' control function for burst mode machines.");
    fsm_Log(fsm, "FSM: Switching to 'robust' burst mode algorithm.");
    if ( fsm_BuildRobustBurstModeTransferFunction(fsm) == 0 )
    {
      fsm_Log(fsm, "FSM: Failed to build 'robust' control function for burst mode machines.");
      return 0;
    }
  }


  if ( check_level >= 1 )
  {
    if ( check_level >= 2 )
      is_self_transition = 1;
    if ( fsm_CheckHazardfreeMachine(fsm, is_self_transition) == 0 )
    {
      fsm_Log(fsm, "FSM: Control function contains critical transitions, construction aborted.");
      return 0;
    }
  }
  fsm_Log(fsm, "FSM: Control function for burst mode machines finished, %d implicants, %d literals.", 
    dclCnt(fsm->cl_machine), dclGetLiteralCnt(fsm->pi_machine, fsm->cl_machine));
  if ( enc_option == FSM_ASYNC_ENCODE_WITH_OUTPUT )
    return fsm_AssignLabelsToMachine(fsm, FSM_CODE_DFF_INCLUDED_OUTPUT);
    
  return fsm_AssignLabelsToMachine(fsm, FSM_CODE_DFF_EXTRA_OUTPUT);
}

