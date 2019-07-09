/*

  fsmustt.c
  
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


  
  This module implements the function
    int fsm_EncodePartition(fsm_type fsm, int option)
  with option is one of
    FSM_ASYNC_ENCODE_MAX_ZERO
    FSM_ASYNC_ENCODE_RESET_ZERO
  additionally one can use
    FSM_ASYNC_ENCODE_WITH_OUTPUT
    
  This function does a 
    Unicode Single Transition Time (USTT)
  state encoding of the FSM.
  This encoding is required for asynchronous (race free)
  state machines.
  
  Reference:
    James H. Tracey
    "Internal State Assignment for Asynchronous Sequential Machines"
    IEEE Transactions on electronic computers, 1966
    
    Luciano Lavagno, Alberto Sangiovanni-Vincentelli
    Algorithms for synthesis and testing of asynchronous curcuits, 2nd printing
    Kluwer Academic Publishers, 1996
    page 56
  
*/

#include <assert.h>
#include <stdlib.h>
#include "fsm.h"
#include "mwc.h"

/* #define FSMUSTT_VERBOSE */

void fsm_DestroyAsync(fsm_type fsm)
{
  if ( fsm->pi_async != NULL )
    pinfoClose(fsm->pi_async);
  fsm->pi_async = NULL;
  if ( fsm->cl_async != NULL )
    dclDestroy(fsm->cl_async);
  fsm->cl_async = NULL;
}

int fsm_InitAsync(fsm_type fsm)
{
  fsm_DestroyAsync(fsm);
  fsm->pi_async = pinfoOpen();
  if ( fsm->pi_async == NULL )
    return 0;
  /* pinfoInitProgress(fsm->pi_async); */
  if ( pinfoSetInCnt(fsm->pi_async, b_set_Cnt(fsm->nodes)) == 0 )
    return 0;
  if ( dclInit(&(fsm->cl_async)) == 0 )
    return 0;
  return 1;
}

/*---------------------------------------------------------------------------*/

static void fsm_set_all_group_value(fsm_type fsm, dcube *c, int g, int v)
{
  int n;
  if ( g < 0 )
    return;
  n = -1;
  while( fsm_LoopNodes(fsm, &n) != 0 )
  {
    if ( g == fsm_GetNode(fsm, n)->group_index )
    {
      dcSetIn(c, fsm_GetNode(fsm, n)->user_val, v);
    }
  }
}

static int fsm_insert_partition(fsm_type fsm, int n1, int nn1, int n2, int nn2)
{
  dcube *c;
  int g1, gg1, g2, gg2;
  c = &(fsm->pi_async->tmp[11]);
  dcInSetAll(fsm->pi_async, c, CUBE_IN_MASK_DC);

  /*
  printf("%s %s - %s %s\n",
    fsm_GetNodeName(fsm, n1),
    fsm_GetNodeName(fsm, nn1),
    fsm_GetNodeName(fsm, n2),
    fsm_GetNodeName(fsm, nn2)
    );
  */
  
  n1 = fsm_GetNode(fsm, n1)->user_val;
  nn1 = fsm_GetNode(fsm, nn1)->user_val;
  g1 = fsm_GetNode(fsm, n1)->group_index;
  gg1 = fsm_GetNode(fsm, nn1)->group_index;
  n2 = fsm_GetNode(fsm, n2)->user_val;
  nn2 = fsm_GetNode(fsm, nn2)->user_val;
  g2 = fsm_GetNode(fsm, n2)->group_index;
  gg2 = fsm_GetNode(fsm, nn2)->group_index;
  
  dcSetIn(c, n1, 1);
  dcSetIn(c, nn1, 1);
  dcSetIn(c, n2, 2);
  dcSetIn(c, nn2, 2);
  
  fsm_set_all_group_value(fsm, c, g1, 1);
  fsm_set_all_group_value(fsm, c, gg1, 1);
  fsm_set_all_group_value(fsm, c, g2, 2);
  fsm_set_all_group_value(fsm, c, gg2, 2);
  
  /*puts(dcToStr(fsm->pi_async, c, " ", ""));*/
  /* if ( dcIsTautology(fsm->pi_async, c ) == 0 ) */
  if ( dcInOneCnt(fsm->pi_async, c) != 0 && dcInZeroCnt(fsm->pi_async, c) != 0 )
  {
    if ( dclAdd(fsm->pi_async, fsm->cl_async, c) < 0 )
    {
      fsm_Log(fsm, "fsm_insert_partition: Out of Memory (dclAdd).");
      return 0;
    }
  }
  return 1;
}

/*---------------------------------------------------------------------------*/

static int fsm_partition_stable_states(fsm_type fsm, dcube *input, int n1, int n2)
{
  int e1, l1;
  int e2, l2;
  l1 = -1;
  while( fsm_LoopNodeInEdges(fsm, n1, &l1, &e1) != 0 )
  {
    if ( dclIsSubSet(fsm->pi_cond, fsm_GetEdgeCondition(fsm, e1), input) != 0 )
    {
      l2 = -1;
      while( fsm_LoopNodeInEdges(fsm, n2, &l2, &e2) != 0 )
      {
        if ( dclIsSubSet(fsm->pi_cond, fsm_GetEdgeCondition(fsm, e2), input) != 0 )
        {
          if ( fsm_insert_partition(fsm, n1, fsm_GetEdgeSrcNode(fsm, e1), n2, fsm_GetEdgeSrcNode(fsm, e2)) == 0 )
          {
            return 0;
          }
          {
            char *n1 = fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, e1));
            char *n2 = fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, e1));
            char *n3 = fsm_GetNodeName(fsm, fsm_GetEdgeSrcNode(fsm, e2));
            char *n4 = fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, e2));
            
            if ( n1 == NULL ) n1 = "<unknown>";
            if ( n2 == NULL ) n2 = "<unknown>";
            if ( n3 == NULL ) n3 = "<unknown>";
            if ( n4 == NULL ) n4 = "<unknown>";
#ifdef FSMUSTT_VERBOSE
            fsm_Log(fsm, "FSM partition: Edge %s -> %s and edge %s -> %s",
              n1, n2, n3, n4);
#endif /* FSMUSTT_VERBOSE */
          }
        }
      }
    }
  }
  
  return 1;
}

static int fsm_is_stable_node(fsm_type fsm, dcube *input, int node_id)
{
  int edge_id;
  edge_id = fsm_FindEdge(fsm, node_id, node_id);
  if ( edge_id < 0 )
    return 0;
  return dclIsSubSet(fsm->pi_cond, fsm_GetEdgeCondition(fsm, edge_id), input);
}

static int fsm_partition_input(fsm_type fsm, dcube *input)
{
  int node_id_1;
  int node_id_2;

  node_id_1 = -1;
  node_id_2 = 0;
  while( fsm_LoopNodes(fsm, &node_id_1) != 0 )
  {
    fsm_GetNode(fsm, node_id_1)->user_val = node_id_2;
    node_id_2++;
  }
  
  node_id_1 = -1;
  while( fsm_LoopNodes(fsm, &node_id_1) != 0 )
  {
    if ( fsm_is_stable_node(fsm, input, node_id_1) != 0 )
    {
      node_id_2 = node_id_1;
      while( fsm_LoopNodes(fsm, &node_id_2) != 0 )
      {
        if ( fsm_is_stable_node(fsm, input, node_id_2) != 0 )
        {
          fsm_partition_stable_states(fsm, input, node_id_1, node_id_2);
        }
      }
    }
  }
  return 1;
}

int fsm_BuildPartitionTable(fsm_type fsm)
{
  dcube *input;
  
  if ( fsm_InitAsync(fsm) == 0 )
    return 0;
    
  input = &(fsm->pi_cond->tmp[10]);
  
  dcInSetAll(fsm->pi_cond, input, CUBE_IN_MASK_ZERO);
  
  do
  {
    if ( fsm_partition_input(fsm, input) == 0 )
      return 0;
  } while( dcInc(fsm->pi_cond, input) != 0 );
  
  dclSCCInv(fsm->pi_async, fsm->cl_async);
  /* dclShow(fsm->pi_async, fsm->cl_async); */
  return 1;
}

/* this function does not separate states of the same group */
int fsm_SeparateStates(fsm_type fsm)
{
  int node_id_1;
  int node_id_2;
  dcube *e1;
  dcube *e2;
  int i, cnt = dclCnt(fsm->cl_async);
  e1 = &(fsm->pi_async->tmp[3]);
  e2 = &(fsm->pi_async->tmp[4]);

  node_id_1 = -1;
  node_id_2 = 0;
  while( fsm_LoopNodes(fsm, &node_id_1) != 0 )
  {
    fsm_GetNode(fsm, node_id_1)->user_val = node_id_2;
    node_id_2++;
  }

  node_id_1 = -1;
  while( fsm_LoopNodes(fsm, &node_id_1) != 0 )
  {
    node_id_2 = node_id_1;
    while( fsm_LoopNodes(fsm, &node_id_2) != 0 )
    {
      if ( fsm_GetNodeGroupIndex(fsm,node_id_1) < 0 ||
          fsm_GetNodeGroupIndex(fsm,node_id_2) < 0 ||
          fsm_GetNodeGroupIndex(fsm,node_id_1) != fsm_GetNodeGroupIndex(fsm,node_id_2) )
      {
        dcInSetAll(fsm->pi_async, e1, CUBE_IN_MASK_DC);
        dcOutSetAll(fsm->pi_async, e1, CUBE_OUT_MASK);
        dcInSetAll(fsm->pi_async, e2, CUBE_IN_MASK_DC);
        dcOutSetAll(fsm->pi_async, e2, CUBE_OUT_MASK);
        dcSetIn(e1, fsm_GetNode(fsm, node_id_1)->user_val, 1);
        dcSetIn(e1, fsm_GetNode(fsm, node_id_2)->user_val, 2);
        dcSetIn(e2, fsm_GetNode(fsm, node_id_1)->user_val, 2);
        dcSetIn(e2, fsm_GetNode(fsm, node_id_2)->user_val, 1);
        {
#ifdef FSMUSTT_VERBOSE
          char *n1 = fsm_GetNodeName(fsm, node_id_1);
          char *n2 = fsm_GetNodeName(fsm, node_id_2);
          fsm_Log(fsm, "FSM partition: Node %s and node %s",
            n1, n2);
#endif /* FSMUSTT_VERBOSE */
        }
        for( i = 0; i < cnt; i++ )
        {
          if ( dcIsSubSet(fsm->pi_async, e1, dclGet(fsm->cl_async, i)) != 0 )
            break;
          if ( dcIsSubSet(fsm->pi_async, e2, dclGet(fsm->cl_async, i)) != 0 )
            break;
        }
        if ( i == cnt )
          if ( fsm_insert_partition(fsm, node_id_1, node_id_1, node_id_2, node_id_2) == 0 )
            return 0;
      }
    }
  }
  /*
  printf("Separate States %d\n", dclCnt(fsm->cl_async)-cnt);
  puts("-- pre ---");
  dclShow(fsm->pi_async, fsm->cl_async);
  dclSCCInv(fsm->pi_async, fsm->cl_async);
  puts("-- post ---");
  dclShow(fsm->pi_async, fsm->cl_async);
  */
  return 1;
}


static int fsm_build_output_separation_cube(fsm_type fsm, int o, dcube *c)
{
  int node_id;
  int i;
  int self_edge_id;
  int is_out;
  dclist cl;

  dcInSetAll(fsm->pi_async, c, CUBE_IN_MASK_DC);

  node_id = -1;
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
  {
    self_edge_id = fsm_FindEdge(fsm, node_id, node_id);
    if ( self_edge_id < 0 )
    {
      fsm_Log(fsm, "FSM: Missing stable condition (output separation).");
      return 0;
    }
    is_out = 0;
    cl = fsm_GetEdgeOutput(fsm, self_edge_id);
    for( i = 0; i < dclCnt(cl); i++ )
    {
      if ( dcGetOut(dclGet(cl, i), o) != 0 )
      {
        is_out = 1;
        break;
      }
    }
    dcSetIn(c, fsm_GetNode(fsm, node_id)->user_val, is_out+1);
  }
  return 1;
}


/*---------------------------------------------------------------------------*/

static void fsm_do_msg(void *data, char *fmt, va_list va)
{
  fsm_type fsm = (fsm_type)data;
  fsm_LogVA(fsm, fmt, va);
}

int fsm_MinimizePartitionTable(fsm_type fsm)
{
  return dclMinimizeUSTT(fsm->pi_async, fsm->cl_async, fsm_do_msg, fsm, "FSM: ", NULL);
}

#ifdef OLD_VERSION
int fsm_MinimizePartitionTable(fsm_type fsm)
{
  dclist cl_p;
  int i, j;
  dcube *cp = &(fsm->pi_async->tmp[1]);
  
  if ( dclInitVA(1, &cl_p) == 0 )
    return 0;

  if ( dclCopy(fsm->pi_async, cl_p, fsm->cl_async) == 0 )
    return dclDestroyVA(1, cl_p), 0;
    
  if ( dclMinimizeUSTT(fsm->pi_async, cl_p, fsm_do_msg, fsm, "FSM: ") == 0 )
    return dclDestroyVA(1, cl_p), 0;
    
  /* verify the result: each condition must be satisfied */
  
  for( i = 0; i < dclCnt(fsm->cl_async); i++ )
  {
    dcCopy(fsm->pi_async, cp, dclGet(fsm->cl_async, i));
    dcInvIn(fsm->pi_async, cp);
    for( j = 0; j < dclCnt(cl_p); j++ )
    {
      if ( dcIsSubSet(fsm->pi_async, dclGet(fsm->cl_async, i), dclGet(cl_p, j)) != 0 )
      {
        fsm_LogLev(fsm, 0, "FSM: Partition %3d (%s) satisfied by %d (%s).", 
          i, 
          dcToStr(fsm->pi_async, dclGet(fsm->cl_async, i), "", ""),
          j,
          dcToStr2(fsm->pi_async, dclGet(cl_p, j), "", "")
          );
        break;
      }
      if ( dcIsSubSet(fsm->pi_async, cp, dclGet(cl_p, j)) != 0 )
      {
        fsm_LogLev(fsm, 0, "FSM: Partition %3d (%s) satisfied by %d (%s).", 
          i, 
          dcToStr(fsm->pi_async, cp, "", ""),
          j,
          dcToStr2(fsm->pi_async, dclGet(cl_p, j), "", "")
          );
        break;
      }
    }
    if ( j >= dclCnt(cl_p) )
    {
      fsm_LogDCL(fsm, "FSM: ", 1, "Minimized partition table", fsm->pi_async, cl_p);
      fsm_Log(fsm, "FSM: Invald encoding (internal error, %d:%s not satisfied).", 
        i,
        dcToStr(fsm->pi_async, dclGet(fsm->cl_async, i), " ", ""));
      return dclDestroyVA(1, cl_p), 0;
    }
  }

  /*
  fsm_Log(fsm, "FSM: Selection of prime partitions is valid (old: %d, new: %d).", dclCnt(fsm->cl_async), dclCnt(cl_p));
  */

  if ( dclCopy(fsm->pi_async, fsm->cl_async, cl_p) == 0 )
    return dclDestroyVA(1, cl_p), 0;

  return dclDestroyVA(1, cl_p), 1;
}
#endif

void fsm_MaxZeroPartitionTable(fsm_type fsm)
{
  dcube *c;
  int i, cnt = dclCnt(fsm->cl_async);
  for( i = 0; i < cnt; i++ )
  {
    c = dclGet(fsm->cl_async, i);
    if ( dcInZeroCnt(fsm->pi_async, c) < fsm->pi_async->in_cnt/2 )
      dcInvIn(fsm->pi_async, c);
  }
}


void fsm_ResetZeroPartitionTable(fsm_type fsm)
{
  dcube *c;
  int i, cnt = dclCnt(fsm->cl_async);
  if ( fsm->reset_node_id < 0 )
  {
    fsm_MaxZeroPartitionTable(fsm);
    return;
  }
  for( i = 0; i < cnt; i++ )
  {
    c = dclGet(fsm->cl_async, i);
    if ( dcGetIn(c, fsm->reset_node_id) == 2 )
      dcInvIn(fsm->pi_async, c);
  }
}

int fms_UseOutputFeedbackRemovePartitions(fsm_type fsm)
{
  int o;
  dcube *c = &(fsm->pi_async->tmp[11]);
  int i, cnt;

  if ( dclClearFlags(fsm->cl_async) == 0 )
  {
    fsm_Log(fsm, "fms_UseOutputFeedbackRemovePartitions: Out of Memory (dclClearFlags).");
    return 0;
  }
    
  /* remove all other partitions, that are coverd by the output functions */
  
  cnt = dclCnt(fsm->cl_async);
  for( o = 0 ; o < fsm_GetOutCnt(fsm); o++ )
  {
    if ( fsm_build_output_separation_cube(fsm, o, c) == 0 )
      return 0;
    for( i = 0; i < cnt; i++ )
    {
      if ( dclIsFlag(fsm->cl_async, i) == 0 )
      {
        if ( dcIsInSubSet(fsm->pi_async, dclGet(fsm->cl_async, i), c) != 0 )
          dclSetFlag(fsm->cl_async, i);
        dcInvIn(fsm->pi_async, c);
        if ( dcIsInSubSet(fsm->pi_async, dclGet(fsm->cl_async, i), c) != 0 )
          dclSetFlag(fsm->cl_async, i);
      }
    }
  }
  dclDeleteCubesWithFlag(fsm->pi_async, fsm->cl_async);

  return 1;
}

int fsm_WithOutputPartitionTable(fsm_type fsm)
{

  int o;
  dcube *c = &(fsm->pi_async->tmp[11]);

  fsm_MaxZeroPartitionTable(fsm);
  
  if ( fms_UseOutputFeedbackRemovePartitions(fsm) == 0 )
    return 0;

  /* add output functions */
  
  for( o = 0 ; o < fsm_GetOutCnt(fsm); o++ )
  {
    if ( fsm_build_output_separation_cube(fsm, o, c) == 0 )
      return 0;
    
    /* if ( dcIsTautology(fsm->pi_async, c ) == 0 ) */
    if ( dcInOneCnt(fsm->pi_async, c) != 0 && dcInZeroCnt(fsm->pi_async, c) != 0 )
      if ( dclAdd(fsm->pi_async, fsm->cl_async, c) < 0 )
      {
        fsm_Log(fsm, "fsm_WithOutputPartitionTable: Out of Memory (dclAdd).");
        return 0;
      }
  }
  
  return 1;
}


int fsm_EncodeWithPartitionTable(fsm_type fsm, int code_option)
{
  int node_id;
  int i, j;
  int s;
  dcube *c;
  
  if ( fsm_SetCodeWidth(fsm, dclCnt(fsm->cl_async), code_option) == 0 )
  {
    return 0;
  }


  node_id = -1;
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
  {
    dcInSetAll(fsm_GetCodePINFO(fsm), 
      fsm_GetNodeCode(fsm, node_id), CUBE_IN_MASK_DC );
    dcOutSetAll(fsm_GetCodePINFO(fsm), 
      fsm_GetNodeCode(fsm, node_id), 0 );
  }
  
  node_id = -1;
  i = 0;
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
  {
    c = fsm_GetNodeCode(fsm, node_id);
    for( j = 0; j < dclCnt(fsm->cl_async); j++ )
    {
      s = dcGetIn(dclGet(fsm->cl_async, j), i);
      dcSetIn(c, j, s);
      if ( s == 3 )
        s = 1;
      dcSetOut(c, j, s-1);
    }
    /*
    dcCopyOutToIn(fsm_GetCodePINFO(fsm), fsm_GetNodeCode(fsm, node_id), 0, 
      fsm_GetCodePINFO(fsm), fsm_GetNodeCode(fsm, node_id));
    */
    fsm_Log(fsm, "FSM: Code %s for state '%s'.", dcToStr(fsm_GetCodePINFO(fsm), c, "/", ""), 
      fsm_GetNodeName(fsm, node_id)==NULL?"":fsm_GetNodeName(fsm, node_id));
    i++;
  }
  
  
  
  return 1;
}

int fsm_EncodePartition(fsm_type fsm, int option)
{
  fsm_Log(fsm, "FSM: Building partition table.");

  if ( fsm_BuildPartitionTable(fsm) == 0 )
    return 0;
  if ( fsm_SeparateStates(fsm) == 0 )
    return 0;
  /*
  if ( option == FSM_ASYNC_ENCODE_WITH_OUTPUT )
    if ( fsm_AddOutputSeparation(fsm) == 0 )
      return 0;
  */

  
  if ( option == FSM_ASYNC_ENCODE_WITH_OUTPUT )
  {
    fsm_Log(fsm, "FSM: Removing output partitions.");
    if ( fms_UseOutputFeedbackRemovePartitions(fsm) == 0 )
      return 0;
    fsm_LogDCL(fsm, "FSM: ", 1, "Partition table", fsm->pi_async, fsm->cl_async);
    
  }

  fsm_Log(fsm, "FSM: Minimizing partition table (size %d).", 
    dclCnt(fsm->cl_async));
  if ( fsm_MinimizePartitionTable(fsm) == 0 )
    return 0;
  fsm_LogDCLLev(fsm, 1, "FSM: ", 1, "Partition table", fsm->pi_async, fsm->cl_async);
    
  switch(option)
  {
    case  FSM_ASYNC_ENCODE_ZERO_MAX:
      fsm_MaxZeroPartitionTable(fsm);
      break;
    case  FSM_ASYNC_ENCODE_ZERO_RESET:
      fsm_ResetZeroPartitionTable(fsm);
      break;
    case FSM_ASYNC_ENCODE_WITH_OUTPUT:
      fsm_WithOutputPartitionTable(fsm);
      break;
  }

  fsm_Log(fsm, "FSM: State encoding with partition table (size %d).", 
    dclCnt(fsm->cl_async));

  switch(option)
  {
    case  FSM_ASYNC_ENCODE_ZERO_MAX:
    case  FSM_ASYNC_ENCODE_ZERO_RESET:
      if ( fsm_EncodeWithPartitionTable(fsm, FSM_CODE_DFF_EXTRA_OUTPUT) == 0 )
        return 0;
      break;
    case FSM_ASYNC_ENCODE_WITH_OUTPUT:
      if ( fsm_EncodeWithPartitionTable(fsm, FSM_CODE_DFF_INCLUDED_OUTPUT) == 0 )
        return 0;
      break;
  }

  /* seems to produce hazard violations */
  /*
  if ( fsm_ExpandCodes(fsm) == 0 )
    return 0;
  */

  return 1;
}

