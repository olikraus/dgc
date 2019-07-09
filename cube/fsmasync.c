/*

  fsmasync.c
  
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
#include "dcube.h"
#include "mcov.h"
#include <assert.h>
#include "mwc.h"


/*---------------------------------------------------------------------------*/

static int fsm_is_cl_machine_ok(fsm_type fsm)
{
  int i, j;
  for( i = 0; i < dclCnt(fsm->cl_machine); i++ )
    for( j = i+1; j < dclCnt(fsm->cl_machine); j++ )
      if ( dcIsDeltaNoneZero(fsm->pi_machine, dclGet(fsm->cl_machine, i), dclGet(fsm->cl_machine, j)) == 0 )
        if ( dcIsEqualOut(fsm->pi_machine, dclGet(fsm->cl_machine, i), dclGet(fsm->cl_machine, j)) == 0 )
          return 0;
  return 1;
}

static int fsm_add_hazard_free_edge(fsm_type fsm, int edge_id)
{
  dcube *c = &(fsm->pi_machine->tmp[9]);
  dcube *cc = &(fsm->pi_machine->tmp[10]);
  int src_node = fsm_GetEdgeSrcNode(fsm, edge_id);
  int dest_node = fsm_GetEdgeDestNode(fsm, edge_id);
  dclist cl;
  pinfo *pie = fsm_GetOutputPINFO(fsm);
  pinfo *pim = fsm->pi_machine;
  int i, cnt;
  
  if ( dclInit(&cl) == 0 )
    return 0;
  dclCopy(pie, cl, fsm_GetEdgeOutput(fsm, edge_id));
  dclPrimes(pie, cl);

  cnt = dclCnt(cl);
  for( i = 0; i < cnt; i++ )
  {

    dcInSetAll(pim, c, CUBE_IN_MASK_DC);
    dcOutSetAll(pim, c, 0);

    dcInSetAll(pim, cc, CUBE_IN_MASK_DC);
    dcOutSetAll(pim, cc, 0);


    /* the second part of the input is the super cube of */
    /* the two state codes */
    dcCopyOutToIn(pim, c, 
      pie->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node));
    dcCopyOutToIn(pim, cc, 
      pie->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node));
    dcOr(pim, c, c, cc);
    
    /* the first part of the input is the condition for this transition */
    
    dcCopyInToIn(pim, c, 0, pie, dclGet(cl, i));

    /* the first part of the output is the target state code */

    dcCopyOutToOut(pim, c, 0, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node));

    /* the second part of the output is the output of the state machine */

    dcCopyOutToOut(pim, c, fsm->code_width, pie, dclGet(cl, i));

    if ( dcIsIllegal(pim, c) == 0 )
      if ( dclAdd(pim, fsm->cl_machine, c) < 0 )
        return dclDestroy(cl), 0;
  }
  return dclDestroy(cl), 1;
}

/*
*/
int fsm_BuildHazardfreeMachine(fsm_type fsm)
{
  int edge_id;
  dclRealClear(fsm->cl_machine);

  fsm_Log(fsm, "FSM: Building hazardfree control function.");

  edge_id = -1;
  while( fsm_LoopEdges(fsm, &edge_id) != 0 )
    if ( fsm_add_hazard_free_edge(fsm, edge_id) == 0 )
      return 0;

  /*  
  edge_id = -1;
  while( fsm_LoopEdges(fsm, &edge_id) != 0 )
    if ( fsm_add_asynchronous_output(fsm, edge_id) == 0 )
      return 0;
  */
  
  dclSCC(fsm->pi_machine, fsm->cl_machine);
  /* dclShow(fsm->pi_machine, fsm->cl_machine); */
  if ( fsm_is_cl_machine_ok(fsm) != 0 )
    fsm_Log(fsm, "FSM: Valid control function with %d implicants.", 
      dclCnt(fsm->cl_machine));
  else
    fsm_Log(fsm, "FSM: Control function (%d implicants) has illegal intersections.", 
      dclCnt(fsm->cl_machine));
  return 1;
}

/*---------------------------------------------------------------------------*/

int fsm_MinimizeAsynchonousMachine(fsm_type fsm)
{
  dclist cl_es, cl_fr, cl_pr, cl_rc;
  dclist cl = fsm->cl_machine;
  pinfo *pi = fsm->pi_machine;
  
  dclInitVA(4, &cl_es, &cl_fr, &cl_pr, &cl_rc);
  
  fsm_Log(fsm, "FSM: Optimizing hazardfree control function (HCF) with %d implicants.", dclCnt(cl));

  if ( dclPrimes(pi, cl) == 0 )
    return dclDestroyVA(4, cl_es, cl_fr, cl_pr, cl_rc), 0;


  /*  
  dclShow(pi, cl);
  return dclDestroyVA(4, cl_es, cl_fr, cl_pr, cl_rc), 1;
  */
    
  fsm_Log(fsm, "FSM: HCF contains %d prime implicants.", dclCnt(cl));

  if ( fsm_RequiredCubes(fsm, cl_rc) == 0 )
    return dclDestroyVA(4, cl_es, cl_fr, cl_pr, cl_rc), 0;
  
  fsm_Log(fsm, "FSM: HCF should contain %d required cubes.", dclCnt(cl_rc));

  if ( dclSplitRelativeEssential(pi, cl_es, cl_fr, cl_pr, cl, NULL, cl_rc) == 0 )
    return dclDestroyVA(4, cl_es, cl_fr, cl_pr, cl_rc), 0;

  fsm_Log(fsm, "FSM: HCF has %d essential, %d partial redundant and %d fully redundant cubes.", dclCnt(cl_es), dclCnt(cl_pr), dclCnt(cl_fr));

  /*
  obsolete, done by dclSplitRelativeEssential 
  if ( dclJoin(pi, cl_pr, cl_fr) == 0 )
    return dclDestroyVA(4, cl_es, cl_fr, cl_pr, cl_rc), 0;
  */

  fsm_LogDCL(fsm, "", 3, "essentials", pi, cl_es, "redundant", pi, cl_pr, "required", pi, cl_rc);


  if ( dclIrredundantGreedy(pi, cl_es, cl_pr, NULL, cl_rc) == 0 )
    return dclDestroyVA(4, cl_es, cl_fr, cl_pr, cl_rc), 0;

  if ( dclJoin(pi, cl_pr, cl_es) == 0 )
    return dclDestroyVA(4, cl_es, cl_fr, cl_pr, cl_rc), 0;

  fsm_LogDCL(fsm, "", 2, "result", pi, cl_pr, "required", pi, cl_rc);

  dclRestrictOutput(pi, cl_pr);

  if( dclIsEquivalent(pi, cl, cl_pr) == 0 )
    return dclDestroyVA(4, cl_es, cl_fr, cl_pr, cl_rc), 0;
    
  if ( dclCopy(pi, cl, cl_pr) == 0 )
    return dclDestroyVA(4, cl_es, cl_fr, cl_pr, cl_rc), 0;
    
  fsm_Log(fsm, "FSM: HCF finished with %d implicants.", dclCnt(cl));
  
  return dclDestroyVA(4, cl_es, cl_fr, cl_pr, cl_rc), 1;
}

/*---------------------------------------------------------------------------*/

static int fsm_check_hazard_free_edge(fsm_type fsm, int edge_id, int is_self_transition, int *error_cnt)
{
  dcube *c = &(fsm->pi_machine->tmp[9]);
  dcube *cc = &(fsm->pi_machine->tmp[10]);
  dcube *m = &(fsm->pi_machine->tmp[14]);
  int src_node_id = fsm_GetEdgeSrcNode(fsm, edge_id);
  int dest_node_id = fsm_GetEdgeDestNode(fsm, edge_id);
  dclist edge_cl;
  dclist self_cl;
  int i_edge, cnt_edge;
  int i_self, cnt_self;
  int src_self_edge_id;
  int is_error = 0;

  if ( is_self_transition == 0 && src_node_id == dest_node_id )
    return 1;

  src_self_edge_id = fsm_FindEdge(fsm, src_node_id, src_node_id);
  if ( src_self_edge_id < 0 )
  {
    fsm_Log(fsm, "FSM HF: Node %d (%s) is not stable.", 
      src_node_id, 
      fsm_GetNodeName(fsm, src_node_id)==NULL?"":fsm_GetNodeName(fsm, src_node_id)
      );
    return 0;
  }
  
  if ( dclInit(&self_cl) == 0 )
    return 0;
  
  edge_cl = fsm_GetEdgeCondition(fsm, edge_id);
  if ( dclCopy(fsm->pi_cond, self_cl, fsm_GetEdgeCondition(fsm, src_self_edge_id)) == 0 )
    return dclDestroy(self_cl), 0;
    
  cnt_edge = dclCnt(edge_cl);
  cnt_self = dclCnt(self_cl);
  
  /* STATE TRANSITION */

  for( i_edge = 0; i_edge < cnt_edge; i_edge++ )
  {

    /* start */

    dcInSetAll(fsm->pi_machine, c, CUBE_IN_MASK_DC);
    dcOutSetAll(fsm->pi_machine, c, 0);

    dcCopyInToIn(fsm->pi_machine, c, 
      0, fsm->pi_cond, dclGet(edge_cl, i_edge));
    dcCopyOutToIn(fsm->pi_machine, c, 
      fsm->pi_cond->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node_id));
    dcCopyOutToOut(fsm->pi_machine, c, 
      0, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node_id));

    /* end */

    dcInSetAll(fsm->pi_machine, cc, CUBE_IN_MASK_DC);
    dcOutSetAll(fsm->pi_machine, cc, 0);

    dcCopyInToIn(fsm->pi_machine, cc, 
      0, fsm->pi_cond, dclGet(edge_cl, i_edge));
    dcCopyOutToIn(fsm->pi_machine, cc, 
      fsm->pi_cond->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node_id));
    dcCopyOutToOut(fsm->pi_machine, cc, 
      0, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node_id));

    if ( dclIsHazardfreeTransition(fsm->pi_machine, fsm->cl_machine, c, cc) == 0 )
    {
      fsm_Log(fsm, "FSM HF: State transition from %d (%s) to %d (%s) is invalid.",
        src_node_id, 
        fsm_GetNodeName(fsm, src_node_id)==NULL?"":fsm_GetNodeName(fsm, src_node_id),
        dest_node_id, 
        fsm_GetNodeName(fsm, dest_node_id)==NULL?"":fsm_GetNodeName(fsm, dest_node_id)
      );
      fsm_Log(fsm, "FSM HF:         Source Point: %s.", 
        dcToStr(fsm->pi_machine, c, " ", ""));
      fsm_Log(fsm, "FSM HF:    Destination Point: %s.", 
        dcToStr(fsm->pi_machine, cc, " ", ""));
      is_error = 1;
      (*error_cnt)++;
    }
    else
    {
      /*
      fsm_Log(fsm, "FSM HF: State transition from %d (%s) to %d (%s) is valid.",
        src_node_id, 
        fsm_GetNodeName(fsm, src_node_id)==NULL?"":fsm_GetNodeName(fsm, src_node_id),
        dest_node_id, 
        fsm_GetNodeName(fsm, dest_node_id)==NULL?"":fsm_GetNodeName(fsm, dest_node_id)
      );
      fsm_Log(fsm, "FSM HF:         Source Point: %s.", 
        dcToStr(fsm->pi_machine, c, " ", ""));
      fsm_Log(fsm, "FSM HF:    Destination Point: %s.", 
        dcToStr(fsm->pi_machine, cc, " ", ""));
      */
    }
  }

  /* INPUT TRANSITION */

  edge_cl = fsm_GetEdgeCondition(fsm, edge_id);
    
  cnt_edge = dclCnt(edge_cl);

  for( i_edge = 0; i_edge < cnt_edge; i_edge++ )
  {
    /* arbitrary change conditions */
    /*
    if ( dclCopy(fsm->pi_cond, self_cl, fsm_GetEdgeCondition(fsm, src_self_edge_id)) == 0 )
      return dclDestroy(self_cl), 0;
    if ( dclDontCareExpand(fsm->pi_cond, self_cl) == 0 )
      return dclDestroy(self_cl), 0;
    */

    /* SIC conditions */
    /*
    if ( dclCopy(fsm->pi_cond, self_cl, fsm_GetEdgeCondition(fsm, src_self_edge_id)) == 0 )
      return dclDestroy(self_cl), 0;
    if ( dclRestrictByDistance1(fsm->pi_cond, self_cl, edge_cl) == 0 )
      return dclDestroy(self_cl), 0;
    */

    /* burst mode conditions */
    if ( fsm_GetNodePreCover(fsm, src_node_id, self_cl) == 0 )
      return dclDestroy(self_cl), 0;
    if ( dclPrimes(fsm->pi_cond, self_cl) == 0 )
      return 0;
    dclAndElements(fsm->pi_cond, m, self_cl);
    
    if ( dclDontCareExpand(fsm->pi_cond, self_cl) == 0 )
      return dclDestroy(self_cl), 0;
      
    cnt_self = dclCnt(self_cl);
    
    for( i_self = 0; i_self < cnt_self; i_self++ )
    {
    
      /* start */
    
      dcInSetAll(fsm->pi_machine, c, CUBE_IN_MASK_DC);
      dcOutSetAll(fsm->pi_machine, c, 0);

      dcCopyInToIn(fsm->pi_machine, c, 
        0, fsm->pi_cond, dclGet(self_cl, i_self));
      dcCopyOutToIn(fsm->pi_machine, c, 
        fsm->pi_cond->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node_id));
      dcCopyOutToOut(fsm->pi_machine, c, 
        0, fsm->pi_code, fsm_GetNodeCode(fsm, src_node_id));

      /* end */

      dcInSetAll(fsm->pi_machine, cc, CUBE_IN_MASK_DC);
      dcOutSetAll(fsm->pi_machine, cc, 0);

      dcCopyInToIn(fsm->pi_machine, cc, 
        0, fsm->pi_cond, dclGet(edge_cl, i_edge));
      dcCopyOutToIn(fsm->pi_machine, cc, 
        fsm->pi_cond->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node_id));
      dcCopyOutToOut(fsm->pi_machine, cc, 
        0, fsm->pi_code, fsm_GetNodeCode(fsm, src_node_id));

      if ( dclIsHazardfreeTransition(fsm->pi_machine, fsm->cl_machine, c, cc) == 0 )
      {
        fsm_Log(fsm, "FSM HF: Input only transition from %s to %s is invalid.",
          fsm_GetNodeName(fsm, src_node_id)==NULL?"":fsm_GetNodeName(fsm, src_node_id),
          fsm_GetNodeName(fsm, dest_node_id)==NULL?"":fsm_GetNodeName(fsm, dest_node_id)
        );
        fsm_Log(fsm, "FSM HF:         Source Point: %s.", 
          dcToStr(fsm->pi_machine, c, " ", ""));
        fsm_Log(fsm, "FSM HF:    Destination Point: %s.", 
          dcToStr(fsm->pi_machine, cc, " ", ""));
        is_error = 1;
        (*error_cnt)++;
      }
      else
      {
        fsm_Log(fsm, "FSM HF:         Source Point: %s.", 
          dcToStr(fsm->pi_machine, c, " ", ""));
        fsm_Log(fsm, "FSM HF:    Destination Point: %s.", 
          dcToStr(fsm->pi_machine, cc, " ", ""));
      }
    }
  }

/* #define FULL_CHECK */
#ifdef FULL_CHECK

  edge_cl = fsm_GetEdgeCondition(fsm, edge_id);
  if ( dclCopy(fsm->pi_cond, self_cl, fsm_GetEdgeCondition(fsm, src_self_edge_id)) == 0 )
    return dclDestroy(self_cl), 0;
    
  cnt_edge = dclCnt(edge_cl);
  cnt_self = dclCnt(self_cl);
  
  for( i_self = 0; i_self < cnt_self; i_self++ )
  {
    for( i_edge = 0; i_edge < cnt_edge; i_edge++ )
    {
    
      /* start */
    
      dcInSetAll(fsm->pi_machine, c, CUBE_IN_MASK_DC);
      dcOutSetAll(fsm->pi_machine, c, 0);

      dcCopyInToIn(fsm->pi_machine, c, 
        0, fsm->pi_cond, dclGet(self_cl, i_self));
      dcCopyOutToIn(fsm->pi_machine, c, 
        fsm->pi_cond->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, src_node_id));
      dcCopyOutToOut(fsm->pi_machine, c, 
        0, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node_id));

      /* end */

      dcInSetAll(fsm->pi_machine, cc, CUBE_IN_MASK_DC);
      dcOutSetAll(fsm->pi_machine, cc, 0);

      dcCopyInToIn(fsm->pi_machine, cc, 
        0, fsm->pi_cond, dclGet(edge_cl, i_edge));
      dcCopyOutToIn(fsm->pi_machine, cc, 
        fsm->pi_cond->in_cnt, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node_id));
      dcCopyOutToOut(fsm->pi_machine, cc, 
        0, fsm->pi_code, fsm_GetNodeCode(fsm, dest_node_id));

      if ( dclIsHazardfreeTransition(fsm->pi_machine, fsm->cl_machine, c, cc) == 0 )
      {
        fsm_Log(fsm, "FSM HF: Full transition from %d (%s) to %d (%s) is invalid.",
          src_node_id, 
          fsm_GetNodeName(fsm, src_node_id)==NULL?"":fsm_GetNodeName(fsm, src_node_id),
          dest_node_id, 
          fsm_GetNodeName(fsm, dest_node_id)==NULL?"":fsm_GetNodeName(fsm, dest_node_id)
        );
        fsm_Log(fsm, "FSM HF:         Source Point: %s.", 
          dcToStr(fsm->pi_machine, c, " ", ""));
        fsm_Log(fsm, "FSM HF:    Destination Point: %s.", 
          dcToStr(fsm->pi_machine, cc, " ", ""));
        is_error = 1;
        (*error_cnt)++;
      }
    }
  }

#endif  

  dclDestroy(self_cl);

  if ( is_error != 0 )
    return 0;
  return 1;
  
}

int fsm_CheckHazardfreeMachine(fsm_type fsm, int is_self_transition)
{
  int edge_id;
  int check_ok = 1;
  int error_cnt = 0;

  fsm_Log(fsm, "FSM: Checking hazardfree control function (HCF).");

  edge_id = -1;
  while( fsm_LoopEdges(fsm, &edge_id) != 0 )
    if ( fsm_check_hazard_free_edge(fsm, edge_id, is_self_transition, &error_cnt) == 0 )
      check_ok = 0;

  if ( check_ok == 0 ) 
    fsm_Log(fsm, "FSM: Check finished, %d error%s found.", error_cnt, error_cnt==1?"":"s");
  else
    fsm_Log(fsm, "FSM: Check finished, no errors found. This function has no hazards.");

  return check_ok;
}

/*---------------------------------------------------------------------------*/

/* replaced by fsm_BuildBurstModeMachine */
/* still used by simfsm.c */
int fsm_BuildAsynchronousMachine(fsm_type fsm, int do_check)
{
  fsm_Log(fsm, "FSM: Building control function for asynchronous state machine.");
  if ( fsm_EncodePartition(fsm, FSM_ASYNC_ENCODE_ZERO_RESET) == 0 )
    return 0;
    
  /* fsm_ShowEdges(fsm, stdout); */

  if ( fsm_CheckSIC(fsm) == 0 )
    return 0;
  if ( fsm_BuildHazardfreeMachine(fsm) == 0 )
    return 0;
  fsm_LogDCL(fsm, "FSM: ", 1, "none minimized control function", fsm->pi_machine, fsm->cl_machine);
  if ( fsm_MinimizeAsynchonousMachine(fsm) == 0 )
    return 0;
  if ( do_check != 0 )
  {
    if ( fsm_CheckHazardfreeMachine(fsm, 0) == 0 )
    {
      fsm_Log(fsm, "FSM: Control function contains critical transitions, construction aborted.");
      return 0;
    }
    fsm_Log(fsm, "FSM: Control function finished, %d implicants.", dclCnt(fsm->cl_machine));
    fsm_LogDCL(fsm, "FSM: ", 1, "minimized control function", fsm->pi_machine, fsm->cl_machine);
  }
  return fsm_AssignLabelsToMachine(fsm, FSM_CODE_DFF_EXTRA_OUTPUT);
}
