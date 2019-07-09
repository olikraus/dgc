/*

  fsmbmm.c
  
  create a burst mode (extended fundamental) asynchronous state machine

  fsmbmm.c builds smaller state machines than fsmbm.c, but this method
  might not always work.
  
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
#include "dcubehf.h"
#include <assert.h>

/*
  precondition: fsm_GetEdgeOutput(fsm, edge) is prime
  output part is empty --> sc is illegal!
*/
static int fsm_bmm_get_stable_start_condition(fsm_type fsm, dcube *sc, int node_id)
{
  pinfo *pio = fsm_GetOutputPINFO(fsm);
  pinfo *pic = fsm_GetCodePINFO(fsm);
  pinfo *pim = fsm->pi_machine;
  int self_edge = fsm_FindEdge(fsm, node_id, node_id);
  dcube *c = &(pio->tmp[14]);
  dclist cl = fsm_GetEdgeOutput(fsm, self_edge);

  dcInSetAll(pim, sc, CUBE_IN_MASK_DC);
  dcOutSetAll(pim, sc, 0);
  dclAndElements(pio, c, cl);

  if ( dcIsIllegal(pio, c) != 0 )
  {
    fsm_Log(fsm, "FSM: The condition for a stable state must be unate.");
    return 0;
  }

  dcCopyInToIn(pim, sc, 0, pio, c);
  dcCopyOutToIn(pim, sc, pio->in_cnt, pic, fsm_GetNodeCode(fsm, node_id));
  return 1;
}

int fsm_bmm_get_enable_transition_condition(fsm_type fsm, dcube *tc, int snode_id, int dnode_id)
{
  pinfo *pio = fsm_GetOutputPINFO(fsm);
  pinfo *pic = fsm_GetCodePINFO(fsm);
  pinfo *pim = fsm->pi_machine;
  int edge = fsm_FindEdge(fsm, snode_id, dnode_id);
  /* dcube *c = &(pio->tmp[14]); */
  dclist cl = fsm_GetEdgeOutput(fsm, edge);
  
  if ( dclCnt(cl) != 1 )
  {
    fsm_Log(fsm, "FSM: The condition for a transition must be one product term ('%s' -> '%s').", 
      fsm_GetNodeNameStr(fsm, snode_id), fsm_GetNodeNameStr(fsm, dnode_id));
    fsm_LogDCL(fsm, "FSM: ", 1, "Condition", pio, cl);
    return 0;
  }

  dcInSetAll(pim, tc, CUBE_IN_MASK_DC);
  dcOutSetAll(pim, tc, 0);
  dcCopyInToIn(pim, tc, 0, pio, dclGet(cl, 0));
  dcCopyOutToIn(pim, tc, pio->in_cnt, pic, fsm_GetNodeCode(fsm, snode_id));
  
  return 1;
}

int fsm_bmm_get_output(fsm_type fsm, int node_id, int pos)
{
  int value = 0;
  
  if ( pos < fsm_GetCodeWidth(fsm) )
  {
    value = dcGetOut(fsm_GetNodeCode(fsm, node_id), pos);
  }
  else
  {
    int self_edge = fsm_FindEdge(fsm, node_id, node_id);
    dclist cl = fsm_GetEdgeOutput(fsm, self_edge);
    int i, cnt = dclCnt(cl);
    pos = pos - fsm_GetCodeWidth(fsm);
    for( i = 0; i < cnt; i++ )
      if ( dcGetOut(dclGet(cl, i), pos) != 0 )
      {
        value = 1;
        break;
      }
  }
  return value;
}

int fsm_bmm_create_enable_transition(fsm_type fsm, dcube *sc, dcube *dc, int snode_id, int dnode_id)
{
  if ( fsm_bmm_get_stable_start_condition(fsm, sc, snode_id) == 0 )
    return 0;
  if ( fsm_bmm_get_enable_transition_condition(fsm, dc, snode_id, dnode_id) == 0 )
    return 0;
  return 1;
}

int fsm_bmm_create_state_change_transition(fsm_type fsm, dcube *sc, dcube *dc, int snode_id, int dnode_id)
{
  if ( fsm_bmm_get_enable_transition_condition(fsm, sc, snode_id, dnode_id) == 0 )
    return 0;
  if ( fsm_bmm_get_stable_start_condition(fsm, dc, dnode_id) == 0 )
    return 0;
  /* assumes, that also a XBM machine reaches a minterm first */
  /* XBM allows to check the level of a certain signal. The following */
  /* and operation assumes, that the level is still stable during the */
  /* state transition. */
  /*
  {
    int i, cnt = fsm_GetInCnt(fsm);
    for( i = 0; i < cnt; i++ )
      dcSetIn(dc, i, dcGetIn(sc, i));
  }
  */
  return 1;
}

int fsm_bmm_add_req_on_cube(fsm_type fsm, hfp_type hfp, dcube *c)
{
  if (hfp_AddRequiredOnCube(hfp, c) == 0 )
  {
    fsm_Log(fsm, "FSM: Required on-cube violation     (%s).", 
      dcToStr(hfp->pi, c, " ", ""));
    {
      int i, cnt = dclCnt(hfp->cl_req_off);
      for( i = 0; i < cnt; i++ )
        if ( dcIsDeltaNoneZero(hfp->pi, dclGet(hfp->cl_req_off, i), c) == 0 )
          fsm_Log(fsm, "FSM: Intersection with off-set cube (%s).", 
            dcToStr(hfp->pi, dclGet(hfp->cl_req_off, i), " ", ""));
    }

    return 0;
  }
  fsm_LogLev(fsm, 0, "FSM: Req. on-cube added  %s.", 
     dcToStr(hfp->pi, c, " ", ""));
  return 1;
}

int fsm_bmm_add_req_on_clist(fsm_type fsm, hfp_type hfp, dclist cl)
{
  int i, cnt = dclCnt(cl);
  for( i = 0; i < cnt; i++ )
    if ( fsm_bmm_add_req_on_cube(fsm, hfp, dclGet(cl, i)) == 0 )
      return 0;
  return 1;
}

int fsm_bmm_add_req_off_cube(fsm_type fsm, hfp_type hfp, dcube *c)
{
  if ( hfp_AddRequiredOffCube(hfp, c) == 0 )
  {
    fsm_Log(fsm, "FSM: Required off-cube violation (%s).", 
      dcToStr(hfp->pi, c, " ", ""));
    {
      int i, cnt = dclCnt(hfp->cl_req_on);
      for( i = 0; i < cnt; i++ )
        if ( dcIsDeltaNoneZero(hfp->pi, dclGet(hfp->cl_req_on, i), c) == 0 )
          fsm_Log(fsm, "FSM: Intersection with on-set cube %s.", 
            dcToStr(hfp->pi, dclGet(hfp->cl_req_on, i), " ", ""));
    }
    return 0;
  }
  fsm_LogLev(fsm, 0, "FSM: Req. off-cube added %s.", 
     dcToStr(hfp->pi, c, " ", ""));
  return 1;
}

int fsm_bmm_add_req_off_clist(fsm_type fsm, hfp_type hfp, dclist cl)
{
  int i, cnt = dclCnt(cl);
  for( i = 0; i < cnt; i++ )
    if ( fsm_bmm_add_req_off_cube(fsm, hfp, dclGet(cl, i)) == 0 )
      return 0;
  return 1;
}

int fsm_bmm_add_priv_cube(fsm_type fsm, hfp_type hfp, dcube *s, dcube *t)
{
  if ( hfp_AddStartTransitionCube(hfp, s, t) == 0 )
    return 0;
  return 1;
}

int fsm_bmm_apply_transition(fsm_type fsm, hfp_type hfp, dcube *sc, dcube *dc, int sov, int dov, int out_pos)
{
  pinfo *pim = fsm->pi_machine;
  dcube *c = &(pim->tmp[13]);
  dcOr(pim, c, sc, dc);
  dcSetOut(c, out_pos, 1);
  if ( sov == dov )
  {
    if ( sov != 0 )
    {
      if ( fsm_bmm_add_req_on_cube(fsm, hfp, c) == 0 )
        return 0;
    }
    else
    {
      if ( fsm_bmm_add_req_off_cube(fsm, hfp, c) == 0 )
        return 0;
    }
  }
  else 
  {
    dclist cl;
    if ( dclInit(&cl) == 0 )
      return 0;
    if ( dclAdd(pim, cl, c) < 0 )
      return dclDestroy(cl), 0;
    if ( sov != 0 )
    {
      /* 1 -> 0 transition */
      dcSetOut(sc, out_pos, 1);
      dcSetOut(dc, out_pos, 1);
      if ( dclSubtractCube(pim, cl, dc) == 0 )
        return dclDestroy(cl), 0;
      if ( fsm_bmm_add_priv_cube(fsm, hfp, sc, c) == 0 )
        return dclDestroy(cl), 0;
      if ( fsm_bmm_add_req_off_cube(fsm, hfp, dc) == 0 )
        return dclDestroy(cl), 0;
      if ( fsm_bmm_add_req_on_clist(fsm, hfp, cl) == 0 )
        return dclDestroy(cl), 0;
      dcSetOut(sc, out_pos, 0);
      dcSetOut(dc, out_pos, 0);
    }
    else
    {
      /* 0 -> 1 transition */
      dcSetOut(sc, out_pos, 1);
      dcSetOut(dc, out_pos, 1);
      if ( dclSubtractCube(pim, cl, dc) == 0 )
        return dclDestroy(cl), 0;
      if ( fsm_bmm_add_priv_cube(fsm, hfp, dc, c) == 0 )
        return dclDestroy(cl), 0;
      if ( fsm_bmm_add_req_on_cube(fsm, hfp, dc) == 0 )
        return dclDestroy(cl), 0;
      if ( fsm_bmm_add_req_off_clist(fsm, hfp, cl) == 0 )
        return dclDestroy(cl), 0;
      dcSetOut(sc, out_pos, 0);
      dcSetOut(dc, out_pos, 0);
    }
    dclDestroy(cl); 
  }

  /*
  assert( dclIsIntersection(hfp->pi, hfp->cl_req_on, hfp->cl_req_off) == 0 );
  assert( dclIsIntersection(hfp->pi, hfp->cl_dc, hfp->cl_req_off) == 0 );
  */

  return 1;
}

static int fsm_bmm_add_edge_output_transitions(fsm_type fsm, hfp_type hfp, int edge_id, int pos)
{
  pinfo *pim = fsm->pi_machine;
  int snode_id = fsm_GetEdgeSrcNode(fsm, edge_id);
  int dnode_id = fsm_GetEdgeDestNode(fsm, edge_id);
  int sov = fsm_bmm_get_output(fsm, snode_id, pos);
  int dov = fsm_bmm_get_output(fsm, dnode_id, pos);
  dcube *sc = &(pim->tmp[3]);
  dcube *dc = &(pim->tmp[4]);
  
  if ( snode_id == dnode_id )
  {
    /* perhaps: add all ON-conditions */
    return 1;
  }


  if ( fsm_bmm_create_enable_transition(fsm, sc, dc, snode_id, dnode_id) == 0 )
    return 0;

  fsm_LogLev(fsm, 0, "FSM: Enable transition   %s -> %s, states '%s' -> '%s', out %d (%d -> %d).",
    dcToStr(pim, sc, " ", ""), dcToStr2(pim, dc, " ", ""),
    fsm_GetNodeNameStr(fsm, snode_id), fsm_GetNodeNameStr(fsm, dnode_id),
    pos, sov, dov
    );

  if ( fsm_bmm_apply_transition(fsm, hfp, sc, dc, sov, dov, pos) == 0 )
  {
    fsm_Log(fsm, "FSM: Transition '%s' -> '%s' failed.",
      fsm_GetNodeNameStr(fsm, snode_id), fsm_GetNodeNameStr(fsm, dnode_id));
    return 0;
  }
    
  if ( fsm_bmm_create_state_change_transition(fsm, sc, dc, snode_id, dnode_id) == 0 )
    return 0;

  fsm_LogLev(fsm, 0, "FSM: State transition    %s -> %s, states '%s' -> '%s', out %d (%d -> %d).",
    dcToStr(pim, sc, " ", ""), dcToStr2(pim, dc, " ", ""),
    fsm_GetNodeNameStr(fsm, snode_id), fsm_GetNodeNameStr(fsm, dnode_id),
    pos, sov, dov
    );

  if ( fsm_bmm_apply_transition(fsm, hfp, sc, dc, dov, dov, pos) == 0 )
  {
    fsm_Log(fsm, "FSM: Transition '%s' -> '%s' failed.",
      fsm_GetNodeNameStr(fsm, snode_id), fsm_GetNodeNameStr(fsm, dnode_id));
    return 0;
  }
  
  return 1;
}

static int fsm_bmm_add_edge(fsm_type fsm, hfp_type hfp, int edge_id)
{
  pinfo *pim = fsm->pi_machine;
  int i;
  for( i = 0; i < pim->out_cnt; i++ )
    if ( fsm_bmm_add_edge_output_transitions(fsm, hfp, edge_id, i) == 0 )
      return 0;
  return 1;
}

static int fsm_bmm_add_edges(fsm_type fsm, hfp_type hfp)
{
  int edge_id = -1;
  while( fsm_LoopEdges(fsm, &edge_id) != 0 )
    if ( fsm_bmm_add_edge(fsm, hfp, edge_id) == 0 )
      return 0;
  return 1;
}

int fsm_BuildSmallBurstModeTransferFunction(fsm_type fsm, int enc_option)
{
  pinfo *pim = fsm->pi_machine;
  hfp_type hfp;

  fsm_Log(fsm, "FSM: Building 'small' hazardfree control function.");
  

  hfp = hfp_Open(pim->in_cnt, pim->out_cnt);
  if ( hfp == NULL )
  {
    fsm_Log(fsm, "FSM: Out of memory (hfp_Open).");
    return 0;
  }

  if ( fsm_bmm_add_edges(fsm, hfp) == 0 )
  {
    fsm_Log(fsm, "FSM: Error (Building hazardfree conditions).");
    return hfp_Close(hfp), 0;
  }

  /*  
  puts("-- req on set --");
  dclShow(pim, hfp->cl_req_on);
  puts("-- req off set --");
  dclShow(pim, hfp->cl_req_off);
  puts("-- dc set --");
  dclShow(pim, hfp->cl_dc);
  */

  fsm_Log(fsm, "FSM: Minimizing 'small' hazardfree control function.");

  
  if ( hfp_Minimize(hfp, 0, 1) == 0 )
  {
    fsm_Log(fsm, "FSM: Error (Minimizing hazardfree transfer function).");
    return hfp_Close(hfp), 0;
  }

  dclCopy(pim, fsm->cl_machine, hfp->cl_on);
  dclClear(fsm->cl_machine_dc);


  fsm_Log(fsm, "FSM: Control function finished, %d implicants.", dclCnt(fsm->cl_machine));
  fsm_LogDCL(fsm, "FSM: ", 1, "Minimized control function", fsm->pi_machine, fsm->cl_machine);
  
  hfp_Close(hfp);
  return 1;
}


