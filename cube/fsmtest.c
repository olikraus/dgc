
/*

  fsmtest.h
  
  test functions for fsm

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

#include "fsmtest.h"
#include <stdlib.h>
#include "dcube.h"
#include "mwc.h"

fsmtt_type fsmtt_Open(pinfo *pi)
{
  fsmtt_type tt;
  tt = (fsmtt_type)malloc(sizeof(struct _fsmtt_struct));
  if ( tt != NULL )
  {
    if ( dcInit(pi, &(tt->c)) != 0 )
    {
      return tt;
    }
    free(tt);
  }
  return NULL;
}

void fsmtt_Close(fsmtt_type tt)
{
  dcDestroy(&(tt->c));
  free(tt);
}


fsmtl_type fsmtl_Open(fsm_type fsm)
{
  fsmtl_type tl;
  tl = malloc(sizeof(struct _fsmtl_struct));
  if ( tl != NULL )
  {
    tl->l = b_pl_Open();
    if ( tl->l != NULL )
    {
      tl->fsm = fsm;
      if ( dcInit(fsm_GetConditionPINFO(fsm), &(tl->curr_input)) != 0 )
      {
        return tl;
      }
      b_pl_Close(tl->l);
    }
    free(tl);
  }
  return NULL;
}

void fsmtl_Clear(fsmtl_type tl)
{
  int i, cnt = b_pl_GetCnt(tl->l);
  for( i = 0; i < cnt; i++ )
    fsmtt_Close((fsmtt_type)b_pl_GetVal(tl->l, i));
  b_pl_Clear(tl->l);
}

void fsmtl_Close(fsmtl_type tl)
{
  fsmtl_Clear(tl);
  dcDestroy(&(tl->curr_input));
  b_pl_Close(tl->l);
  free(tl);
}


void fsmtl_SetDevice(fsmtl_type tl, int (*outfn)(void *data, fsm_type fsm, int msg, int arg, dcube *c), void *data)
{
  tl->outfn = outfn;
  tl->data = data;
}

int fsmtl_AddTT(fsmtl_type tl, fsmtt_type tt)
{
  if ( b_pl_Add(tl->l, tt) < 0 )
    return 0;
  return 1;
}

int fsmtl_Add(fsmtl_type tl, int edge_id, dcube *c)
{
  pinfo *pi = fsm_GetConditionPINFO(tl->fsm);
  fsmtt_type tt;
  tt = fsmtt_Open(pi);
  if ( tt != NULL )
  {
    tt->edge_id = edge_id;
    dcCopy(pi, &(tt->c), c);
    if ( fsmtl_AddTT(tl, tt) != 0 )
    {
      return 1;
    }
    fsmtt_Close(tt);
  }
  return 0;
}

int fsmtl_GetEdgeCondition(fsmtl_type tl, int edge_id, int cs, dcube *dest)
{
  pinfo *pi = fsm_GetConditionPINFO(tl->fsm);
  dclist cl = fsm_GetEdgeCondition(tl->fsm, edge_id);
  int i, ii, m;
  
  switch(cs)
  {
    case FSM_TEST_CS_RAND:
      if ( dclCnt(cl) == 0 )
        return 0;
      dcCopy( pi, dest, dclGet(cl, 0));
      for( i = 0; i < pi->in_cnt; i++ )
        if ( dcGetIn(dest, i) == 3 )
          dcSetIn(dest, i, 1);
      break;
    case FSM_TEST_CS_MIN_DELTA:
      ii = -1;
      m = pi->in_cnt;
      for( i = 0; i < dclCnt(cl); i++ )
        if ( m > dcDeltaIn(pi, dclGet(cl,i), &(tl->curr_input)) )
        {
          m = dcDeltaIn(pi, dclGet(cl,i), &(tl->curr_input));
          ii = i;
        }
      if ( ii < 0 )
        return 0;
      for( i = 0; i < pi->in_cnt; i++ )
        if ( dcGetIn(dclGet(cl, ii), ii) == 3 )
          dcSetIn(dest, i, dcGetIn(&(tl->curr_input), i));
        else
          dcSetIn(dest, i, dcGetIn(dclGet(cl, ii), i));
      break;
    case FSM_TEST_CS_MAX_DELTA:
      ii = -1;
      m = -1;
      for( i = 0; i < dclCnt(cl); i++ )
        if ( m < dcDeltaIn(pi, dclGet(cl,i), &(tl->curr_input)) )
        {
          m = dcDeltaIn(pi, dclGet(cl,i), &(tl->curr_input));
          ii = i;
        }
      if ( ii < 0 )
        return 0;
      for( i = 0; i < pi->in_cnt; i++ )
        if ( dcGetIn(dclGet(cl, ii), ii) == 3 )
          dcSetIn(dest, i, dcGetIn(&(tl->curr_input), i));
        else
          dcSetIn(dest, i, dcGetIn(dclGet(cl, ii), i));
      break;
  }
  return 1;
}

int fsmtl_AddEdgeCondition(fsmtl_type tl, int edge_id, int cs)
{
  pinfo *pi = fsm_GetConditionPINFO(tl->fsm);
  fsmtt_type tt;
  tt = fsmtt_Open(pi);
  if ( tt != NULL )
  {
    tt->edge_id = edge_id;
    if ( fsmtl_GetEdgeCondition(tl, edge_id, cs, &(tt->c)) != 0 )
    {
      if ( fsmtl_AddTT(tl, tt) != 0 )
      {
        return 1;
      }
    }
    fsmtt_Close(tt);
  }
  return 0;
}

int fsmtl_GoEdge(fsmtl_type tl, int edge_id)
{
  pinfo *pi = fsm_GetConditionPINFO(tl->fsm);
  dcube *c = &(pi->tmp[13]);
  
  if ( fsmtl_GetEdgeCondition(tl, edge_id, FSM_TEST_CS_MAX_DELTA, c) == 0 )
    return fsm_Log(tl->fsm, "FSM TL: Memory (?) error with edge condition calculation (abort)."), 0;
  dcCopy(pi, &(tl->curr_input), c);
  if ( tl->outfn(tl->data, tl->fsm, FSM_TEST_MSG_GO_EDGE, edge_id, c) == 0 )
    return fsm_Log(tl->fsm, "FSM TL: Error with MSG_GO_EDGE (abort)."), 0;
  return 1;
}

int fsmtl_GoNode_cb(void *data, fsm_type fsm, int edge_id, int i, int cnt)
{
  fsmtl_type tl = (fsmtl_type)data;
  return fsmtl_GoEdge(tl, edge_id);
}

int fsmtl_GoNode(fsmtl_type tl, int src_id, int dest_id)
{
  int edge_id;
  if ( src_id == dest_id )
    return 1;
  edge_id = fsm_FindEdge(tl->fsm, src_id, dest_id);
  if ( edge_id > 0 )
    return fsmtl_GoEdge(tl, edge_id);
  if ( fsm_DoShortestPath(tl->fsm, src_id, dest_id, fsmtl_GoNode_cb, tl) == 0 )
  {
    return fsm_Log(tl->fsm, "FSM TL: Memory error within shortest path calculation (abort)."), 0;
  }
  return 1;
}

int fsmtl_Do(fsmtl_type tl)
{
  int i, cnt = b_pl_GetCnt(tl->l);
  int src_id, dest_id;
  int edge_id;
  fsmtt_type tt;

  src_id = tl->fsm->reset_node_id;
  
  if ( src_id < 0 )
    return fsm_Log(tl->fsm, "FSM TL: Transistion walk reqires a reset node (abort)"), 0;

  edge_id = fsm_FindEdge(tl->fsm, src_id, src_id);
  if ( edge_id < 0 )
    return fsm_Log(tl->fsm, "FSM TL: Reset node is not stable (warning)"), 0;
    
  fsm_Log(tl->fsm, "FSM TL: Transition walk started (no of tests: %d).", cnt);


  if ( tl->outfn(tl->data, tl->fsm, FSM_TEST_MSG_START, cnt, NULL) == 0 )
    return fsm_Log(tl->fsm, "FSM TL: Error with MSG_START (abort)."), 0;

  if ( edge_id >= 0 )
  {
    if ( fsmtl_GetEdgeCondition(tl, edge_id, FSM_TEST_CS_RAND, &(tl->curr_input)) == 0 )
      return fsm_Log(tl->fsm, "FSM TL: Memory error (abort)."), 0;

    if ( tl->outfn(tl->data, tl->fsm, FSM_TEST_MSG_RESET, src_id, &(tl->curr_input)) == 0 )
      return fsm_Log(tl->fsm, "FSM TL: Error with MSG_RESET (abort)."), 0;
  }
  else
  {
    if ( tl->outfn(tl->data, tl->fsm, FSM_TEST_MSG_RESET, src_id, NULL) == 0 )
      return fsm_Log(tl->fsm, "FSM TL: Error with MSG_RESET (abort)."), 0;
  }
  
  for( i = 0; i < cnt; i++ )
  {
    tt = (fsmtt_type)b_pl_GetVal(tl->l, i);
    edge_id = tt->edge_id;
    dest_id = fsm_GetEdgeSrcNode(tl->fsm, edge_id);
    if ( fsmtl_GoNode(tl, src_id, dest_id) == 0 )
      return 0;
    dcCopy(fsm_GetConditionPINFO(tl->fsm), &(tl->curr_input), &(tt->c));
    if ( tl->outfn(tl->data, tl->fsm, FSM_TEST_MSG_DO_EDGE, edge_id, &(tt->c)) == 0 )
      return fsm_Log(tl->fsm, "FSM TL: Error with MSG_DO_EDGE (abort)."), 0;
    src_id = fsm_GetEdgeDestNode(tl->fsm, edge_id);
  }

  if ( tl->outfn(tl->data, tl->fsm, FSM_TEST_MSG_END, cnt, NULL) == 0 )
    return 0;

  fsm_Log(tl->fsm, "FSM TL: Transition walk finished.");
  
  return 1;
}

/*---------------------------------------------------------------------------*/

int fsmtl_stdout(void *data, fsm_type fsm, int msg, int arg, dcube *c)
{
  int src_id;
  int dest_id;
  
  switch(msg)
  {
    case FSM_TEST_MSG_RESET:
      printf("reset %s\n", fsm_GetNodeName(fsm, arg));
      break;
    case FSM_TEST_MSG_GO_EDGE:
      src_id = fsm_GetEdgeSrcNode(fsm, arg);
      dest_id = fsm_GetEdgeDestNode(fsm, arg);
      printf("go %s: %s -> %s\n", dcToStr(fsm_GetConditionPINFO(fsm), c, "",""), fsm_GetNodeName(fsm, src_id), fsm_GetNodeName(fsm, dest_id));
      break;
    case FSM_TEST_MSG_DO_EDGE:
      src_id = fsm_GetEdgeSrcNode(fsm, arg);
      dest_id = fsm_GetEdgeDestNode(fsm, arg);
      printf("do %s: %s -> %s\n", dcToStr(fsm_GetConditionPINFO(fsm), c, "",""), fsm_GetNodeName(fsm, src_id), fsm_GetNodeName(fsm, dest_id));
      break;
  }
  return 1;
}

/*---------------------------------------------------------------------------*/

int fsmtl_Show(fsmtl_type tl)
{
  fsmtl_SetDevice(tl, fsmtl_stdout, NULL);
  return fsmtl_Do(tl);
}

/*---------------------------------------------------------------------------*/

static int fsm_find_next_node_out_edge(fsm_type fsm, int node_id)
{
  int loop = -1;
  int edge_id = -1;
  while( fsm_LoopNodeOutEdges(fsm, node_id, &loop, &edge_id) != 0 )
    if ( fsm_GetEdge(fsm, edge_id)->is_visited == 0 )
      return edge_id;
  return -1;
}

static int fsmtl_AddEdgeSequence(fsmtl_type tl, int edge_id)
{
  int node_id;
  
  for(;;)
  {
    fsm_GetEdge(tl->fsm, edge_id)->is_visited = 1;
    if ( fsmtl_AddEdgeCondition(tl, edge_id, FSM_TEST_CS_RAND) == 0 )
      return 0;
    node_id = fsm_GetEdgeDestNode(tl->fsm, edge_id);
    edge_id = fsm_find_next_node_out_edge(tl->fsm, node_id);
    if ( edge_id < 0 )
      break;
  }
  return 1;
}

int fsmtl_AddAllEdgeSequence(fsmtl_type tl)
{
  int edge_id;
  
  edge_id = -1;
  while( fsm_LoopEdges(tl->fsm, &edge_id) != 0 )
    fsm_GetEdge(tl->fsm, edge_id)->is_visited = 0;

  edge_id = -1;
  while( fsm_LoopEdges(tl->fsm, &edge_id) != 0 )
    if ( fsm_GetEdge(tl->fsm, edge_id)->is_visited == 0 )
    {
      if ( fsmtl_AddEdgeSequence(tl, edge_id) == 0 )
        return 0;
      edge_id = -1;
    }

  return 1;
}

