/*

  gnetsim.c
  
  gate level simulator
  
  purpose: exact calculation of the delay for an asynchronous state machine
  
  
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
  
  
  general mechanism for essential hazard delay calculation
  1. a) apply stable state + input for stable state
     b) do simulation
  2. a) apply new input (transition to next state)
     b) do simulation while events exist (--> t_max) + record all port changes
     c) apply delay t_max - t_port_change
     
  
  calls to gnetlv.c:
    gnc_GetPortLogicVal
    gnc_CalculateCellNodeLogicVal()
    gnc_GetCellNetLogicVal()
    glvSetVal()
    gnc_ApplyCellFSMInputForSimulation();
    gnc_ApplyCellFSMStateForSimulation();
    void gspq_CalculateUnknownValues(gspq_type pq);
    int gspq_ApplyCellFSMSimulationTransitionState(gspq_type pq, pinfo *pi_in, dcube *c_in, pinfo *pi_z, dcube *c_z);
    int gspq_ApplyCellFSMSimulationTransition(gspq_type pq, pinfo *pi_in, dcube *c_in);
  
*/

#include <stdlib.h>
#include <assert.h>
#include "gnetsim.h"




/* simulation event */
typedef struct _gse_struct gse_struct;
struct _gse_struct
{
  double time;
  double tr_dly;
  unsigned int logic_value;
  int net_ref;
  int node_ref;   /* driver of net_ref */
  int time_index;
};


/*----------------- priority queue -------------------*/


int gspq_comp_lower(void *data, const void *a, const void *b)
{
  if ( ((gse_struct *)a)->time > ((gse_struct *)b)->time )
    return 1;
  return 0;
}

void gspq_ClearPool(gspq_type pq, int pool)
{
  int i, cnt = gnc_GetCellPortMax(pq->nc, pq->cell_ref);
  assert( pool >= 0 && pool < GSPQ_T_POOL_CNT );
  for( i = 0; i < cnt; i++ )
    pq->t_max[i + cnt*pool] = 0.0;
}

void gspq_ClearTime(gspq_type pq)
{
  int i;
  pq->time = 0.0;
  for( i = 0; i < GSPQ_INDEXED_TIME_CNT; i++ )
    pq->indexed_time[i] = 0;
}

gspq_type gspq_Open(gnc nc, int cell_ref)
{
  int i;
  gspq_type pq;
  pq = (gspq_type)malloc(sizeof(struct _gspq_struct));
  if ( pq != NULL )
  {
    pq->h = b_pq_Open(gspq_comp_lower, NULL);
    if ( pq->h != NULL )
    {
      pq->nc = nc;
      pq->cell_ref = cell_ref;
      pq->is_stop_at_fsm_input = 0;
      pq->is_calc_max_in_out_dly = 0;
      pq->is_cap_analysis = 0;
      pq->change_cnt = 0;
      pq->capacitance_sum = 0.0;
      pq->ff_capacitance_sum = 0.0;
      pq->cap_analysis_transition_cnt = 0;

      pq->m_max = 
        (double *)malloc(sizeof(double)*gnc_GetCellPortMax(nc, cell_ref)*
                gnc_GetCellPortMax(nc, cell_ref));
      if ( pq->m_max != NULL )
      {
        for( i = 0; i < gnc_GetCellPortMax(nc, cell_ref)*gnc_GetCellPortMax(nc, cell_ref); i++ )
          pq->m_max[i] = 0.0;
      
        pq->t_max = 
          (double *)malloc(sizeof(double)*GSPQ_T_POOL_CNT*
                  gnc_GetCellPortMax(nc, cell_ref));
        if ( pq->t_max != NULL )
        {
          for( i = 0; i < GSPQ_T_POOL_CNT; i++ )
            gspq_ClearPool(pq, i);
          gspq_ClearTime(pq);
          return pq;
        }
        free(pq->m_max);
      }
      b_pq_Close(pq->h);
    }
    free(pq);
  }
  return NULL;
}

void gspq_DelFirst(gspq_type pq)
{
  gse_struct *tmp = b_pq_Max(pq->h);
  if ( tmp == NULL )
    return;
  b_pq_DelMax(pq->h);
  free(tmp);
}

void gspq_Close(gspq_type pq)
{
  for(;;)
  {
    if ( b_pq_Max(pq->h) == NULL )
      break;
    gspq_DelFirst(pq);
  }
  free(pq->t_max);
  b_pq_Close(pq->h);
  free(pq);
}

void gspq_ApplyMaxTime(gspq_type pq, int pool, int pos, double time)
{
  int cnt = gnc_GetCellPortMax(pq->nc, pq->cell_ref);
  assert( pool >= 0 && pool < GSPQ_T_POOL_CNT );
  assert(pos >= 0 && pos < cnt);

  if ( pq->t_max[pos + cnt*pool] < time )
    pq->t_max[pos + cnt*pool] = time;
}

void gspq_ApplyMaxMTime(gspq_type pq, int x, int y, double time)
{
  int cnt = gnc_GetCellPortMax(pq->nc, pq->cell_ref);
  assert(x >= 0 && x < cnt);
  assert(y >= 0 && y < cnt);

  if ( pq->m_max[x + cnt*y] < time )
    pq->m_max[x + cnt*y] = time;
}

double gspq_GetTime(gspq_type pq, int pool, int pos)
{
  int cnt = gnc_GetCellPortMax(pq->nc, pq->cell_ref);
  assert( pool >= 0 && pool < GSPQ_T_POOL_CNT );
  assert(pos >= 0 && pos < cnt);
  return pq->t_max[pos + cnt*pool];
}

double gspq_GetMTime(gspq_type pq, int x, int y)
{
  int cnt = gnc_GetCellPortMax(pq->nc, pq->cell_ref);
  assert(x >= 0 && x < cnt);
  assert(y >= 0 && y < cnt);
  return pq->m_max[x + cnt*y];
}


double gspq_GetMaxTime(gspq_type pq, int pool)
{
  double max = 0.0;
  int i, cnt = gnc_GetCellPortMax(pq->nc, pq->cell_ref);
  assert( pool >= 0 && pool < GSPQ_T_POOL_CNT );
  for( i = 0; i < cnt; i++ )
    if ( max < gspq_GetTime(pq, pool, i) )
      max = gspq_GetTime(pq, pool, i);
  return max;
}


/* insert an event into the priority queue */
int gspq_AddEvent(gspq_type pq, double time, unsigned int logic_value, double tr_dly, int net_ref, int node_ref, int time_index)
{
  gse_struct *tmp = (gse_struct *)malloc(sizeof(gse_struct));
  gse_struct **cur;
  
  if ( tmp == NULL )
    return 0;
  
  tmp->time = time;
  tmp->logic_value = logic_value;
  tmp->tr_dly = tr_dly;
  tmp->net_ref = net_ref;
  tmp->node_ref = node_ref;   /* driver of net_ref */
  tmp->time_index = time_index;

  /*
  gnc_Log(pq->nc, 0, "Simulation: Net %d will get value '%c' at time %lg.",
          net_ref, "?01"[logic_value], time);
  */

  gnc_Log(pq->nc, 0, "Simulation: Next %lg, index %d, value '%c' for net %d.",
    time, time_index, "?01"[logic_value], net_ref); 

  if ( time < pq->time )
  {
    gnc_Log(pq->nc, 0, "Simulation: History event %lg < %lg, assigning current time %lg.", time, pq->time, time);
    tmp->time = time; 
  }
  
  if ( b_pq_Add(pq->h, tmp) < 0 )
    return 0;

  /* assert(time >= pq->time); */
  
  return 1;
}

/* return next event: logic value + net-reference */
/* deletes the event */
/* procedure returns 0 if no event exists */
int gspq_GetEvent(gspq_type pq, unsigned int *logic_value, int *net_ref)
{
  gse_struct *tmp = (gse_struct *)b_pq_Max(pq->h);
  
  if ( tmp == NULL )
    return 0;
  *logic_value = tmp->logic_value;
  *net_ref = tmp->net_ref;
  gspq_DelFirst(pq);
  return 1;
}

/*
 * int main()
 * {
 *   unsigned int logic_value;
 *   int net_ref;
 *   gspq_type pq = gspq_Open();
 *   if ( pq != NULL )
 *   {
 *     gspq_AddEvent(pq, 22.22, 2, 2);
 *     gspq_AddEvent(pq, 11.11, 1, 1);
 *     gspq_AddEvent(pq, 33.33, 3, 3);
 *     
 *     while(gspq_GetEvent(pq, &logic_value, &net_ref) != 0)
 *       printf("%d\n", logic_value);
 *     
 *   }
 *   return NULL;
 * }
 */

/*----------------- capacitance analysis ------------------------*/

void gspq_EnableCapacitanceAnalysis(gspq_type pq)
{
  pq->is_cap_analysis = 1;
  pq->change_cnt = 0;
  pq->capacitance_sum = 0.0;
  pq->ff_capacitance_sum = 0.0;
  pq->cap_analysis_transition_cnt = 0;
  gnc_ClearLogicSimulationChangeCnt(pq->nc, pq->cell_ref);  /* required? */
}

int gspq_GetNetChangeCnt(gspq_type pq)
{
  return pq->change_cnt;
}

double gspq_GetNetChangeCapacitance(gspq_type pq)
{
  return pq->capacitance_sum;
}

double gspq_GetAveragetNetChangeCapacitance(gspq_type pq)
{
  if ( pq->cap_analysis_transition_cnt == 0 )
    return 0.0;
  return gspq_GetNetChangeCapacitance(pq)/(double)pq->cap_analysis_transition_cnt;
}


/*----------------- simulator ------------------------*/

/*
  pq          simulation environment
  node_ref    recalculate the output of this node
  port_ref    reason for recalculating was a change at this port
  in_tr_dly   the transition delay at the input port 'port_ref'
*/
int gspq_CreateNodeEvent(gspq_type pq, int node_ref, int i_port_ref, double in_tr_dly, int time_index)
{
  int val;
  double propagation_delay;
  double transition_delay;
  int o_port_ref;
  int node_cell_ref;
  int calc_cmd;
  

  /* with all output port */
  /* currently, there can be only ONE output */
   
  node_cell_ref = gnc_GetCellNodeCell(pq->nc, pq->cell_ref, node_ref);
  o_port_ref = -1;
  for(;;)
  {
    if ( gnc_LoopCellPort(pq->nc, node_cell_ref, &o_port_ref) == 0 )
      break;
    if ( gnc_GetCellPortType(pq->nc, node_cell_ref, o_port_ref) == GPORT_TYPE_OUT )
    {
      /* this function should depend on o_port_ref */
      /*
        perhaps this function should look into the heap to 
        see whether there are some more suitable values...
      */
      val = gnc_CalculateCellNodeLogicVal(pq->nc, pq->cell_ref, node_ref);

      calc_cmd = GNC_CALC_CMD_NONE;
      switch(val)
      {
        case GLV_ZERO:
          calc_cmd = GNC_CALC_CMD_FALL;
          break;
        case GLV_ONE:
          calc_cmd = GNC_CALC_CMD_RISE;
          break;
      }
      
      if ( calc_cmd != GNC_CALC_CMD_NONE )
      {
        int net_ref = gnc_FindCellNetByPort(pq->nc, pq->cell_ref, node_ref, o_port_ref);
        

        propagation_delay = gnc_GetCellNodePropagationDelay(
          pq->nc, pq->cell_ref, node_ref,
          calc_cmd, in_tr_dly, 
          i_port_ref, o_port_ref);

        transition_delay = gnc_GetCellNodeTransitionDelay(
          pq->nc, pq->cell_ref, node_ref,
          calc_cmd, in_tr_dly, 
          i_port_ref, o_port_ref);
          
        if ( propagation_delay < 0.0 )
          gnc_Log(pq->nc, 0, "Simulation: Negative propagation delay %lg.", propagation_delay);

        if ( gspq_AddEvent(pq, pq->time+propagation_delay, val, transition_delay, net_ref, node_ref, time_index) == 0 )
          return 0;

      }
      else
      {
        /* do something here? */
      }
      
      /* exit the loop after the first output */
      return 1;
    }
  }
  
  return 1;

  
}


int gspq_ProcessEvent(gspq_type pq)
{
  glv_struct *lv;
  int is_changed;
  gse_struct ev;
  int join_ref;
  int node_ref;
  int port_ref;
  int time_index;
  int forced_val;
  
  int register_width = gnc_GetGCELL(pq->nc, pq->cell_ref)->register_width;
  int inputs = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt;

  
  /* get next event */
  
  if ( b_pq_Max(pq->h) == NULL )
    return 1;
    
  ev = *(gse_struct *)b_pq_Max(pq->h);

  /* delete the event from the queue */
  /* so that we can safely add new events */

  gspq_DelFirst(pq);

    
  /* update indexed time */
  time_index = ev.time_index;
  pq->indexed_time[time_index] = ev.time;


  /* update global time */
  pq->time = ev.time;
  
  /* assign the value of the event, check if something changes */

  /* this is the old value */
  lv = gnc_GetCellNetLogicVal(pq->nc, pq->cell_ref, ev.net_ref);
  
  /* actually no one is really in ev->logic_value..., but it is a good default */
  forced_val = ev.logic_value;
  
  /* this is the driven value */

  if ( ev.node_ref >= 0 )
    forced_val = gnc_CalculateCellNodeLogicVal(pq->nc, pq->cell_ref, ev.node_ref);
  
  gnc_Log(pq->nc, 0, "Simulation: Time %lg, index %d, val '%c' for net %d, previous val '%c', forced val '%c'.",
    ev.time, ev.time_index, "?01"[ev.logic_value], ev.net_ref, "?01"[lv->logic_value], "?01"[forced_val] ); 

  is_changed = glvSetVal(lv, forced_val);
  
  if ( is_changed == 0 )
    return 1;
    
  if ( pq->is_cap_analysis != 0 )
  {
    pq->change_cnt++;
    pq->capacitance_sum += gnc_GetCellNetInputLoad(pq->nc, pq->cell_ref, ev.net_ref);
  }

  if ( pq->is_calc_max_in_out_dly != 0 )
  {
    join_ref = -1;
    while( gnc_LoopCellNetJoin(pq->nc, pq->cell_ref, ev.net_ref, &join_ref) != 0 )
    {
      node_ref = gnc_GetCellNetNode(pq->nc, pq->cell_ref, ev.net_ref, join_ref);
      /* look for parent node */
      if ( node_ref < 0 )
      {
        if ( gnc_GetCellNetPortType(pq->nc, pq->cell_ref, ev.net_ref, join_ref) == GPORT_TYPE_OUT )
        {
          port_ref = gnc_GetCellNetPort(pq->nc, pq->cell_ref, ev.net_ref, join_ref);
          gspq_ApplyMaxTime(pq, 0, port_ref, pq->time);
          gspq_ApplyMaxTime(pq, 1, port_ref, pq->time);
          gnc_Log(pq->nc, 0, "Simulation: Recorded time %lg, value '%c' for port '%s' (%d).",
              pq->time, "?01"[ev.logic_value], 
              gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref),
              port_ref);
              
          /* the event has passed an output port, apply different index! */
          time_index = 1;
        }
      }
    }
  }
  
  if ( pq->is_stop_at_fsm_input != 0 )
  {
    /* stop at an output port */
    join_ref = -1;
    while( gnc_LoopCellNetJoin(pq->nc, pq->cell_ref, ev.net_ref, &join_ref) != 0 )
    {
      node_ref = gnc_GetCellNetNode(pq->nc, pq->cell_ref, ev.net_ref, join_ref);
      /* look for parent node */
      if ( node_ref < 0 )
      {
        if ( gnc_GetCellNetPortType(pq->nc, pq->cell_ref, ev.net_ref, join_ref) == GPORT_TYPE_OUT )
        {
          port_ref = gnc_GetCellNetPort(pq->nc, pq->cell_ref, ev.net_ref, join_ref);
          if ( port_ref >= inputs-register_width && port_ref < inputs )
          {
            gnc_Log(pq->nc, 0, "Simulation: Finished at time %lg, value '%c' with port '%s' (%d).",
                pq->time, "?01"[ev.logic_value], 
                gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref),
                port_ref);
            return 1;
          }
        }
      }
    }
  }


  /* loop through the input member of the net */
  /* see also gnc_SetCellNetLogicValue */
  
  join_ref = -1;
  while( gnc_LoopCellNetJoin(pq->nc, pq->cell_ref, ev.net_ref, &join_ref) != 0 )
  {
    node_ref = gnc_GetCellNetNode(pq->nc, pq->cell_ref, ev.net_ref, join_ref);
    /* ignore the parent cell */
    if ( node_ref >= 0 )
    {
      /* only with input ports ... */
      if ( gnc_GetCellNetPortType(pq->nc, pq->cell_ref, ev.net_ref, join_ref) == GPORT_TYPE_IN )
      {
        if ( gspq_CreateNodeEvent(pq, node_ref,
          gnc_GetCellNetPort(pq->nc, pq->cell_ref, ev.net_ref, join_ref),
          ev.tr_dly, time_index) == 0 )
          return 0;
      }
    }
  }

  return 1;
}

int gspq_ProcessEvents(gspq_type pq)
{
  int i;
  for( i = 0; i < pq->nc->simulation_loop_abort; i++ )
  {
    /* are there any events? */
    if ( b_pq_Max(pq->h) == NULL )
    {
      /* no */
      gnc_Log(pq->nc, 0, "Simulation: Stable (time %lg).", pq->time);
      return 1; 
    }
      
    /* yes, process and delete the next event */
    if ( gspq_ProcessEvent(pq) == 0 )
      return 0;

  }
  gnc_Error(pq->nc, "gspq_ProcessEvents: Maximum loops (%d) reached.", 
    pq->nc->simulation_loop_abort);
  return 0;
}

int gspq_ProcessEventsUntil(gspq_type pq, double time)
{
  int i;
  gse_struct *ev;
  for( i = 0; i < pq->nc->simulation_loop_abort; i++ )
  {
  
  
    /* are there any events? */
    ev = (gse_struct *)b_pq_Max(pq->h);
    if ( ev == NULL )
    {
      gnc_Log(pq->nc, 0, "Simulation: Stable (time %lg).", pq->time);
      return 1;
    }

    /* check whether we are allowed to process the event */
    if ( ev->time > time )
      return 1;  /* no, stop here */
      
    /* yes, process the event */
    if ( gspq_ProcessEvent(pq) == 0 )
      return 0;

    /* delete the event from the queue */
    
    gspq_DelFirst(pq);

  }
  gnc_Error(pq->nc, "gspq_ProcessEvents: Maximum loops (%d) reached.", 
    pq->nc->simulation_loop_abort);
  return 0;
}


void gspq_CalculateUnknownValues(gspq_type pq)
{
  int net_ref = -1;
  int node_ref;
  glv_struct *lv;
  int is_changed;
  int is_something_changed;
  int val;
  int cnt = 0;

  do
  {
    is_something_changed = 0;
    net_ref = -1;
    while( gnc_LoopCellNet(pq->nc, pq->cell_ref, &net_ref) != 0 )
    {
      lv = gnc_GetCellNetLogicVal(pq->nc, pq->cell_ref, net_ref);
      if ( lv->logic_value == GLV_UNKNOWN )
      {
        node_ref = gnc_FindCellNetNodeByDriver(pq->nc, pq->cell_ref, net_ref);
        if ( node_ref < 0 )
        {
          gnc_Log(pq->nc, 0, "Simulation: Net %d not driven.", net_ref);
        }
        else
        {
          val = gnc_CalculateCellNodeLogicVal(pq->nc, pq->cell_ref, node_ref);
          
          gnc_Log(pq->nc, 0, "Simulation: Assigning value '%c' to net %d.",
          "?01"[val], net_ref);
          
          is_changed = glvSetVal(lv, val);
          if ( is_changed != 0 )
          {
            is_something_changed = 1;
            cnt++;
          }
        }
      }
    }
  } while(is_something_changed != 0);
  gnc_Log(pq->nc, 1, "Simulation: Emergency net assignments: %d.", cnt);
}

/* returns 1, if something has been changed */
int gspq_UseInputPortValues(gspq_type pq)
{
  int net_ref;
  int port_ref;
  int val;
  
  port_ref = -1;
  while( gnc_LoopCellPort(pq->nc, pq->cell_ref, &port_ref) == 0 )
  {
    if ( gnc_GetCellPortType(pq->nc, pq->cell_ref, port_ref) == GPORT_TYPE_IN )
    {
      val = gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, port_ref)->logic_value;
      if ( val != GLV_UNKNOWN )
      {
        net_ref = gnc_FindCellNetByPort(pq->nc, pq->cell_ref, -1, port_ref);
        if ( net_ref >= 0 )
        {
          if ( gspq_AddEvent(pq, pq->time, val, 0.0, net_ref, -1, 0) == 0 )
            return 0;
          if( gspq_ProcessEventsUntil(pq, pq->time) == 0 )
            return 0;
        }
      }
      
      gnc_Log(pq->nc, 0, "Simulation: Port '%s' has value '%c'.",
        gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref)==NULL?"":
          gnc_GetCellPortName(pq->nc,pq-> cell_ref, port_ref),
        "?01"[val] );
    }
  }
  return 1;
}

int gspq_DoLogicSimulation(gspq_type pq, int cell_ref)
{
  gspq_ClearTime(pq);
  
  /* use the logic values of the input ports */
  if ( gspq_UseInputPortValues(pq) == 0 )
    return 0;   /* error occured */

  /* loop until all recalculate flags are cleared */
  if ( gspq_ProcessEvents(pq) == 0 )
    return 0;   /* error occured */
  
  /* fill output values with the new values of the net */
  gnc_FillOutputPortValues(pq->nc, pq->cell_ref);
  
  return 1;
}

int gspq_UseFSMPortValues(gspq_type pq, int use_state_variables)
{
  int net_ref;
  int port_ref;
  int val;
  int is_changed = 0;
  
  int register_width = gnc_GetGCELL(pq->nc, pq->cell_ref)->register_width;
  int inputs = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt;

  port_ref = -1;
  while( gnc_LoopCellPort(pq->nc, pq->cell_ref, &port_ref) != 0 )
  {
    /* here is a difference to gnc_UseFSMPortValues */
    /* the application of this function requires state values to be set */
    if ( gnc_GetCellPortType(pq->nc, pq->cell_ref, port_ref) == GPORT_TYPE_IN || 
         (use_state_variables != 0 && port_ref < inputs+register_width) )
    {
      val = gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, port_ref)->logic_value;
      if ( val != GLV_UNKNOWN )
      {
        net_ref = gnc_FindCellNetByPort(pq->nc, pq->cell_ref, -1, port_ref);
        if ( net_ref >= 0 )
        {
          if ( gspq_AddEvent(pq, pq->time, val, 0.0, net_ref, -1, 0) == 0 )
            return 0;
            
          gnc_Log(pq->nc, 0, "Simulation: Port '%s' has value '%c' (net %d).",
            gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref)==NULL?"":
              gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref),
            "?01"[val], net_ref );
           
          /*
          if( gspq_ProcessEventsUntil(pq, pq->time) == 0 )
            return 0;          
          */
        }
        else
        {
          gnc_Log(pq->nc, 0, "Simulation: Port '%s' has value '%c' (not connected).",
            gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref)==NULL?"":
              gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref),
            "?01"[val] );
        }
      }
      else
      {
        gnc_Log(pq->nc, 0, "Simulation: Port '%s' ignored (unknown value).",
          gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref)==NULL?"":
            gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref));
      }
    }
    else
    {
      gnc_Log(pq->nc, 0, "Simulation: Port '%s' ignored.",
        gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref)==NULL?"":
          gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref));
    }
  }
  return 1;
}

void gspq_FillOutputPortValues(gspq_type pq)
{
  int net_ref;
  int port_ref;
  glv_struct *src_lv;
  glv_struct *dest_lv;

  port_ref = -1;
  while( gnc_LoopCellPort(pq->nc, pq->cell_ref, &port_ref) != 0 )
  {
    /* if ( gnc_GetCellPortType(pq->nc, pq->cell_ref, port_ref) == GPORT_TYPE_OUT ) */
    {
      net_ref = gnc_FindCellNetByPort(pq->nc, pq->cell_ref, -1, port_ref);
      if ( net_ref >= 0 )
      {
        src_lv = gnc_GetCellNetLogicVal(pq->nc, pq->cell_ref, net_ref);
        dest_lv = gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, port_ref);
        *dest_lv = *src_lv;
      }
    }
  }
}

int gspq_DoFSMLogicSimulation(gspq_type pq, int apply_state_variables)
{
  pq->time = 0.0;
  gspq_ClearPool(pq, 1);

  /* use the logic values of the input ports */
  if ( gspq_UseFSMPortValues(pq, apply_state_variables) == 0 )
    return 0;   /* error occured */

  /* loop until all recalculate flags are cleared */
  if ( gspq_ProcessEvents(pq) == 0 )
    return 0;   /* error occured */

  /* this is a really bad workaround */
  /* there should be a better way to avoid unknown values */
  /* indeed, I think simulation itself is somehow wrong... */
  gspq_CalculateUnknownValues(pq);
  
  /* fill output values with the new values of the net */
  gspq_FillOutputPortValues(pq);
  
  return 1;
}

static int gspq_fsm_simulation(gspq_type pq, int apply_state_variables, int is_reset)
{
  int clr_port = -1;
  int clk_port = -1;
  int clr_inverted = 0;
  int clk_inverted = 0;
  int clr_val = GLV_ONE;
  int clr_norm_val = GLV_ZERO;
  
  double curr_cap = gspq_GetNetChangeCapacitance(pq);
  int    curr_cnt = gspq_GetNetChangeCnt(pq);
  double input_cap;
  int    input_cnt;
  double clk_cap;
  int    clk_cnt;

  /* clr means reset input */
  clr_port = gnc_FindCellPortByFn(pq->nc, pq->cell_ref, GPORT_FN_CLR);
  clk_port = gnc_FindCellPortByFn(pq->nc, pq->cell_ref, GPORT_FN_CLK);
  if ( clr_port >= 0 )
    clr_inverted = gnc_IsCellPortInverted(pq->nc, pq->cell_ref, clr_port);
  if ( clk_port >= 0 )
    clk_inverted = gnc_IsCellPortInverted(pq->nc, pq->cell_ref, clk_port);
  if ( clr_port >= 0 )
    if ( clr_inverted != 0 )
    {
      clr_val = GLV_ZERO;
      clr_norm_val = GLV_ONE; 
    }
  if ( clr_port >= 0 )
    glvSetVal(gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, clr_port), clr_norm_val);
  if ( clk_port >= 0 )
    glvSetVal(gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, clk_port), GLV_ZERO);

  if ( clr_port >= 0 && is_reset != 0 )
  {
    glvSetVal(gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, clr_port), clr_val);
    if ( gspq_DoFSMLogicSimulation(pq,apply_state_variables) == 0 )
      return 0;
    glvSetVal(gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, clr_port), clr_norm_val);
    if ( gspq_DoFSMLogicSimulation(pq,apply_state_variables) == 0 )
      return 0;
  }
  else
  {
    if ( gspq_DoFSMLogicSimulation(pq,apply_state_variables) == 0 )
      return 0;
  }

  input_cap = gspq_GetNetChangeCapacitance(pq);
  input_cnt = gspq_GetNetChangeCnt(pq);

  if ( clk_port >= 0 )                                                                   
  {                                                                                      
    glvSetVal(gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, clk_port), GLV_ONE);         
    if ( gspq_DoFSMLogicSimulation(pq,apply_state_variables) == 0 )                      
      return 0;                                                                          
    glvSetVal(gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, clk_port), GLV_ZERO);        
    if ( gspq_DoFSMLogicSimulation(pq,apply_state_variables) == 0 )                      
      return 0;                                                                          
  }                                                                                      

  clk_cap = gspq_GetNetChangeCapacitance(pq);
  clk_cnt = gspq_GetNetChangeCnt(pq);
  
  if ( pq->is_cap_analysis != 0 )  
  {
    gnc_Log(pq->nc, 3, 
      "Simulation: Net changes: %d, charge/dischage capacitance: %lg (= %lg + %lg).",
      clk_cnt-curr_cnt, 
      clk_cap-curr_cap, input_cap-curr_cap, clk_cap-input_cap);

    pq->ff_capacitance_sum += clk_cap-input_cap;
    pq->cap_analysis_transition_cnt++;
  }
                                                                                         
  return 1;                                                                              
}

int gspq_ApplyCellFSMSimulationTransitionState(gspq_type pq, pinfo *pi_in, dcube *c_in, pinfo *pi_z, dcube *c_z)
{
  gnc_ApplyCellFSMInputForSimulation(pq->nc, pq->cell_ref, pi_in, c_in);
  gnc_ApplyCellFSMStateForSimulation(pq->nc, pq->cell_ref, pi_z, c_z);
  return gspq_fsm_simulation(pq, 1, 0);
}

/* the name XBM is probably wrong, yet it is only used by the XBM procedures */
int gspq_ApplyCellXBMSimulationTransitionState(gspq_type pq, dcube *c)
{
  gnc_ApplyCellInputForSimulation(pq->nc, pq->cell_ref, c);
  gnc_ApplyCellStateForSimulation(pq->nc, pq->cell_ref, c);
  return gspq_fsm_simulation(pq, 1, 0);
}

/* the name XBM is probably wrong, yet it is only used by the XBM procedures */
int gspq_ApplyCellXBMSimulationTransitionResetState(gspq_type pq, dcube *c)
{
  gnc_ApplyCellInputForSimulation(pq->nc, pq->cell_ref, c);
  gnc_ApplyCellStateForSimulation(pq->nc, pq->cell_ref, c);
  return gspq_fsm_simulation(pq, 1, 1);
}


int gspq_ApplyCellFSMSimulationTransition(gspq_type pq, pinfo *pi_in, dcube *c_in)
{
  gnc_ApplyCellFSMInputForSimulation(pq->nc, pq->cell_ref, pi_in, c_in);
  return gspq_fsm_simulation(pq, 0, 0);
}

/* the name XBM is probably wrong, yet it is only used by the XBM procedures */
int gspq_ApplyCellXBMSimulationTransition(gspq_type pq, dcube *c)
{
  gnc_ApplyCellInputForSimulation(pq->nc, pq->cell_ref, c);
  return gspq_fsm_simulation(pq, 0, 0);
}


/* only compares the state variables BEFORE the delay chain */
/* expects a code pinfo and a code cube... */
int gspq_CompareCellFSMStateForSimulation(gspq_type pq, pinfo *pi, dcube *c)
{
  int inputs = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt;
  int register_width = gnc_GetGCELL(pq->nc, pq->cell_ref)->register_width;
  int port_ref;
  int val;
  int i;

  if ( 0 >= pq->nc->log_level )
  {
    static char s[100];
    int total = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt
                 + gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->out_cnt;
    for( i = 0; i < total && i < 100-1; i++ )
    {
      switch( gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, i)->logic_value )
      {
        case GLV_ZERO: s[i] = '0'; break;
        case GLV_ONE: s[i] = '1'; break;
        default: s[i] = '?'; break;
      }
    }
    s[i] = '\0';
    gnc_Log(pq->nc, 1, "Simulation: Current port values are '%s'.", s);
  }

  for( port_ref = inputs, i = 0; port_ref < inputs+register_width ; port_ref++, i++ )
  {
    switch(dcGetOut(c, i))
    {
      case 0:
        val = GLV_ZERO;
        /*
        if ( gnc_IsCellPortInverted(pq->nc, pq->cell_ref, port_ref) != 0 )
          val = GLV_ONE;
        */
        break;
      case 1:
        val = GLV_ONE;
        /*
        if ( gnc_IsCellPortInverted(pq->nc, pq->cell_ref, port_ref) != 0 )
          val = GLV_ZERO;
        */
        break;
      default:
        val = GLV_UNKNOWN;
        break;
    }
    if ( gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, port_ref-register_width)->logic_value != val )
    {
      gnc_Error(pq->nc, "gspq_CompareCellFSMStateForSimulation: "
        "Simulation result wrong "
        "(port '%s'/'%s', current '%c'/'%c', expected '%c', inverted %d/%d).",
        gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref)==NULL?"":gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref),
        gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref)==NULL?"":gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref-register_width),
        "?01"[gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, port_ref)->logic_value],
        "?01"[gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, port_ref-register_width)->logic_value],
        "?01"[val],
        gnc_IsCellPortInverted(pq->nc, pq->cell_ref, port_ref),
        gnc_IsCellPortInverted(pq->nc, pq->cell_ref, port_ref-register_width)
        );
      return 0;
    }
  }
  gnc_Log(pq->nc, 1, "Simulation: Simulated state valid (%s).", dcOutToStr(pi, c, ""));
  return 1;
}

/* only compares the state variables BEFORE the delay chain */
/* expects a pi_machine cube... */
/* compares against the output part of c */
int gspq_CompareCellXBMStateForSimulation(gspq_type pq, dcube *c)
{
  int inputs = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt;
  int fb_cnt = gnc_GetGCELL(pq->nc, pq->cell_ref)->register_width;
  int port_ref;
  int val;
  int i;

  if ( 0 >= pq->nc->log_level )
  {
    static char s[100];
    int total = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt
                 + gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->out_cnt;
    for( i = 0; i < total && i < 100-1; i++ )
    {
      switch( gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, i)->logic_value )
      {
        case GLV_ZERO: s[i] = '0'; break;
        case GLV_ONE: s[i] = '1'; break;
        default: s[i] = '?'; break;
      }
    }
    s[i] = '\0';
    gnc_Log(pq->nc, 1, "Simulation: Current port values are '%s'.", s);
  }

  for( port_ref = inputs, i = 0; port_ref < inputs+fb_cnt ; port_ref++, i++ )
  {
    switch(dcGetOut(c, i))
    {
      case 0:
        val = GLV_ZERO;
        /*
        if ( gnc_IsCellPortInverted(pq->nc, pq->cell_ref, port_ref) != 0 )
          val = GLV_ONE;
        */
        break;
      case 1:
        val = GLV_ONE;
        /*
        if ( gnc_IsCellPortInverted(pq->nc, pq->cell_ref, port_ref) != 0 )
          val = GLV_ZERO;
        */
        break;
      default:
        val = GLV_UNKNOWN;
        break;
    }
    if ( gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, port_ref-fb_cnt)->logic_value != val )
    {
      gnc_Error(pq->nc, "gspq_CompareCellFSMStateForSimulation: "
        "Simulation result wrong "
        "(port '%s'/'%s', current '%c'/'%c', expected '%c', inverted %d/%d).",
        gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref)==NULL?"":gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref),
        gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref)==NULL?"":gnc_GetCellPortName(pq->nc, pq->cell_ref, port_ref-fb_cnt),
        "?01"[gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, port_ref)->logic_value],
        "?01"[gnc_GetCellPortLogicVal(pq->nc, pq->cell_ref, port_ref-fb_cnt)->logic_value],
        "?01"[val],
        gnc_IsCellPortInverted(pq->nc, pq->cell_ref, port_ref),
        gnc_IsCellPortInverted(pq->nc, pq->cell_ref, port_ref-fb_cnt)
        );
      return 0;
    }
  }
  gnc_Log(pq->nc, 1, "Simulation: Simulated state valid (%s).", 
    dcOutToStr(gnc_GetGCELL(pq->nc, pq->cell_ref)->pi, c, ""));
  return 1;
}

