/*

  sydelay.c
  
  synthesis: delay calculation
  
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

#include <assert.h>
#include "gnet.h"
#include "gnetsim.h"
#include "b_dg.h"
#include <limits.h>
#include <math.h>
#include "mwc.h"

#define GNC_DELAY_SAFETY ((nc)->syparam.delay_safety)



/*
int gcellInitDelay(gcell cell);
int gcellSetDelay(gcell cell, int in_port_ref, int out_port_ref, double delay);
double gcellGetDelayMin(gcell cell, int in_port_ref, int out_port_ref);
double gcellGetDelayMax(gcell cell, int in_port_ref, int out_port_ref);
int gcellIsDelayUsed(gcell cell, int in_port_ref, int out_port_ref);
*/

int gnc_InitCellDelay(gnc nc, int cell_ref)
{
  return gcellInitDelay(gnc_GetGCELL(nc, cell_ref));
}

int gnc_SetCellDelay(gnc nc, int cell_ref, int in_out_port_ref, int out_port_ref, double delay)
{
  if ( gcellSetDelay(gnc_GetGCELL(nc, cell_ref), in_out_port_ref, out_port_ref, delay) == 0 )
  {
    gnc_Error(nc, "DELAY: Out of memory.");
    return 0;
  }
  return 1;
}

double gnc_GetCellMinDelay(gnc nc, int cell_ref, int in_out_port_ref, int out_port_ref)
{
  return gcellGetDelayMin(gnc_GetGCELL(nc, cell_ref), in_out_port_ref, out_port_ref);
}

double gnc_GetCellMaxDelay(gnc nc, int cell_ref, int in_out_port_ref, int out_port_ref)
{
  return gcellGetDelayMax(gnc_GetGCELL(nc, cell_ref), in_out_port_ref, out_port_ref);
}

double gnc_IsCellDelayUsed(gnc nc, int cell_ref, int in_out_port_ref, int out_port_ref)
{
  return gcellIsDelayUsed(gnc_GetGCELL(nc, cell_ref), in_out_port_ref, out_port_ref);
}

double gnc_GetCellTotalMax(gnc nc, int cell_ref)
{
  return gcellGetDelayTotalMax(gnc_GetGCELL(nc, cell_ref));
}

double gnc_GetCellTotalMin(gnc nc, int cell_ref)
{
  return gcellGetDelayTotalMin(gnc_GetGCELL(nc, cell_ref));
}

static int gnc_contains_cell_port_range(gnc nc, int cell_ref, int net_ref, 
  int start_port_ref, int cnt)
{
  int join_ref = -1;
  int port_ref;
  while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
  {
    if ( gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref) < 0 )
    {
      port_ref = gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref);
      if ( port_ref >= start_port_ref && port_ref < start_port_ref+cnt )
        return 1;
    }
  }
  return 0;
}

int gnc_is_fsm_register(gnc nc, int cell_ref, int i_net_ref, int o_net_ref)
{
  pinfo *pi;
  int register_width = gnc_GetGCELL(nc, cell_ref)->register_width;
  if ( register_width <= 0 )
    return 0;
    
  pi = gnc_GetGCELL(nc, cell_ref)->pi;
  if ( pi == NULL )
    return 0;
  if ( gnc_contains_cell_port_range(nc, cell_ref, i_net_ref, pi->in_cnt-register_width, register_width*2) == 0 )
    return 0;
  if ( gnc_contains_cell_port_range(nc, cell_ref, o_net_ref, pi->in_cnt-register_width, register_width*2) == 0 )
    return 0;
  return 1;
}

/* returns node_ref */
static int gnc_get_door_node(gnc nc, int cell_ref, int z_port_ref, 
  int *i_port_ref_p, int *o_port_ref_p)
{
  pinfo *pi;

  int i_port_ref;
  int o_port_ref;
  int i_net_ref;
  int o_net_ref;
  int i_join_ref;
  int o_join_ref;
  
  int port1_ref;
  int port2_ref;
  int node1_ref;
  int node2_ref;
  
  int register_width = gnc_GetGCELL(nc, cell_ref)->register_width;
  if ( register_width <= 0 )
    return 0.0;
    
  pi = gnc_GetGCELL(nc, cell_ref)->pi;
  
  assert( z_port_ref >= pi->in_cnt-register_width );
  assert( z_port_ref < pi->in_cnt );
  
  i_port_ref = z_port_ref + register_width;
  o_port_ref = z_port_ref;

  i_net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, i_port_ref);
  o_net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, o_port_ref);
  if ( i_net_ref != o_net_ref && i_net_ref >= 0 && o_net_ref >= 0 )
  {

    i_join_ref = -1;
    while( gnc_LoopCellNetJoin(nc, cell_ref, i_net_ref, &i_join_ref) != 0 )
    {
      port1_ref = gnc_GetCellNetPort(nc, cell_ref, i_net_ref, i_join_ref);
      node1_ref = gnc_GetCellNetNode(nc, cell_ref, i_net_ref, i_join_ref);

      o_join_ref = -1;
      while( gnc_LoopCellNetJoin(nc, cell_ref, o_net_ref, &o_join_ref) != 0 )
      {
        port2_ref = gnc_GetCellNetPort(nc, cell_ref, o_net_ref, o_join_ref);
        node2_ref = gnc_GetCellNetNode(nc, cell_ref, o_net_ref, o_join_ref);

        if ( node1_ref == node2_ref && node1_ref >= 0 )
        {
          *i_port_ref_p = port1_ref;
          *o_port_ref_p = port2_ref;
          
          assert(gnc_GetCellNetPortType(nc, cell_ref, i_net_ref, i_join_ref) == GPORT_TYPE_IN);
          assert(gnc_GetCellNetPortType(nc, cell_ref, o_net_ref, o_join_ref) == GPORT_TYPE_OUT);
        
          return node1_ref;
        }
      }
    }
  }
  
  return -1;
  
}

static double gnc_get_door_min_delay(gnc nc, int cell_ref, int z_port_ref)
{
  int node_ref;
  int i_port_ref;
  int o_port_ref;

  node_ref = gnc_get_door_node(nc, cell_ref, z_port_ref, &i_port_ref, &o_port_ref);
  if ( node_ref < 0 )
    return 0.0;

  return gnc_GetCellNodeMinDelay(nc, cell_ref, node_ref, i_port_ref, o_port_ref);
}


/* register_width must be 0 for pure logic cells */
/* is_inverted_weight == 1: use graph for longest path calculation */
/* is_inverted_weight == 0: use graph for shortest path calculation */
static int gnc_convert_to_directed_graph(gnc nc, int cell_ref, b_dg_type dg, int is_inverted_weight)
{
  int net_ref;
  int vertex_ref;
  int node_ref;
  int node_cell_ref;
  gpoti poti;
  int i_port_ref;
  int o_port_ref;
  int i_net_ref;
  int o_net_ref;
  int i_vertex_ref;
  int o_vertex_ref;

  double delay;
        

  b_dg_Clear(dg);
  
  /* create a node (vertex in the graph) for each net */
  
  net_ref = -1;
  while( gnc_LoopCellNet(nc, cell_ref, &net_ref) != 0 )
  {
    vertex_ref = b_dg_AddNode(dg, net_ref);
    if ( vertex_ref < 0 )
      return gnc_Error(nc, "DELAY: Out of memory (b_dg_AddNode)."), 0;
    gnc_GetCellGNET(nc, cell_ref, net_ref)->user_val = vertex_ref;
  }

  /* create connections with a negative weight so that we can use */
  /* the shortest path calculation to obtain the longest path.    */
  /* see Corman, Leiserson, Rivest p538 */

  node_ref = -1;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
  {
    node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
    
    o_port_ref = -1;
    while( gnc_LoopCellPort(nc, node_cell_ref, &o_port_ref) != 0 )
    {
      if ( gnc_GetCellPortType(nc, node_cell_ref, o_port_ref) == GPORT_TYPE_OUT )
      {

        i_port_ref = -1;
        while( gnc_LoopCellPort(nc, node_cell_ref, &i_port_ref) != 0 )
        {
          if ( gnc_GetCellPortType(nc, node_cell_ref, i_port_ref) == GPORT_TYPE_IN )
          {

            /* get the timing structure,            */
            /* do not create a new timing structure */

            poti = gnc_GetCellPortGPOTI(nc, node_cell_ref, 
                                    i_port_ref, o_port_ref, 0);

            i_net_ref = gnc_FindCellNetByPort(nc, cell_ref, node_ref, i_port_ref);
            o_net_ref = gnc_FindCellNetByPort(nc, cell_ref, node_ref, o_port_ref);
            
            /* there might be unconnected output, so check this case */
            
            if ( i_net_ref >= 0 && o_net_ref >= 0 )
            {
              /* break cyclic fsms into acyclic graphs */
              if ( gnc_is_fsm_register(nc, cell_ref, i_net_ref, o_net_ref) == 0 )
              {
                if ( poti == NULL )
                {
                  /* no suitable timing structure, assume zero delay */

                  delay = 0.0;
                }
                else
                {
                  if ( is_inverted_weight != 0 )
                    delay = gnc_GetCellNodeMaxDelay(nc, cell_ref, node_ref, i_port_ref, o_port_ref);
                  else
                    delay = gnc_GetCellNodeMinDelay(nc, cell_ref, node_ref, i_port_ref, o_port_ref);
                }

                i_vertex_ref = gnc_GetCellGNET(nc, cell_ref, i_net_ref)->user_val;
                o_vertex_ref = gnc_GetCellGNET(nc, cell_ref, o_net_ref)->user_val;

                if ( is_inverted_weight != 0 )
                  delay = -delay;
                
                if ( b_dg_Connect(dg, i_vertex_ref, o_vertex_ref, delay, node_ref) < 0 )
                  return gnc_Error(nc, "DELAY: Out of memory (b_dg_Connect)."), 0;
                        
              }
            }
            else
            {
              /*
              printf("register found nets %d %d\n", i_net_ref, o_net_ref);
              */
            }
          }
        } /* while: with all input ports */
      }
    } /* while: with all output ports */
  } /* while: with all nodes */
  
  return 1;
}

/*
static void sydelay_dg_show(b_dg_type dg, int node_ref)
{
  int edge_ref;
  printf("%d", node_ref);
  while( b_dg_LoopPath(dg, &node_ref, &edge_ref) != 0 )
    printf(" <-[%.2f]-- %d", b_dg_GetEdgeWeight(dg, edge_ref), node_ref);
  printf("\n");
}
*/

static int gnc_path_calculation(gnc nc, int cell_ref, b_dg_type dg, int is_inverted_weight)
{
  int i_port_ref;
  int o_port_ref;
  
  int fn;

  int i_net_ref;
  int o_net_ref;

  int i_vertex_ref;
  int o_vertex_ref;
  
  double weight;

  i_port_ref = -1;
  while( gnc_LoopCellPort(nc, cell_ref, &i_port_ref) != 0 )
  {
    fn = gnc_GetCellPortFn(nc, cell_ref, i_port_ref);
    if ( fn != GPORT_FN_CLR && fn != GPORT_FN_SET )
    {
      i_net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, i_port_ref);
      if ( i_net_ref >= 0 )
      {
        i_vertex_ref = gnc_GetCellGNET(nc, cell_ref, i_net_ref)->user_val;

        if ( b_dg_CalcAcyclicShortestPaths(dg, i_vertex_ref) != 0 )
        {
          o_port_ref = -1;
          while( gnc_LoopCellPort(nc, cell_ref, &o_port_ref) != 0 )
          {
            if ( gnc_GetCellPortType(nc, cell_ref, o_port_ref) == GPORT_TYPE_OUT )
            {
              o_net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, o_port_ref);
              if ( o_net_ref >= 0 )
              {
                o_vertex_ref = gnc_GetCellGNET(nc, cell_ref, o_net_ref)->user_val;

                if ( b_dg_GetPathWeight(dg, i_vertex_ref, o_vertex_ref, &weight) != 0 )
                {
                  if ( is_inverted_weight != 0 )
                    weight = -weight;
                  if ( gnc_SetCellDelay(nc, cell_ref, i_port_ref, o_port_ref, weight) == 0 )
                    return 0;
                  gnc_Log(nc, 0, "Delay calculation: Delay from '%s' to '%s' is %lg.",
                    gnc_GetCellPortName(nc, cell_ref, i_port_ref)==NULL?"":gnc_GetCellPortName(nc, cell_ref, i_port_ref),
                    gnc_GetCellPortName(nc, cell_ref, o_port_ref)==NULL?"":gnc_GetCellPortName(nc, cell_ref, o_port_ref),
                    weight
                    );
                  /*
                  {
                    int l = o_vertex_ref;
                    int e, n;
                    while( b_dg_LoopPath(dg, &l, &e) != 0 ) 
                    {
                      n = b_dg_GetEdgeUserVal(dg, e);
                      printf("%s ", gnc_GetCellNodeName(nc, cell_ref, n));
                    }
                    printf("\n");
                  }
                  */
                  
                }
                else
                {
                  /*
                  gnc_Log(nc, 0, "Delay calculation: No Path from '%s' to '%s'.",
                    gnc_GetCellPortName(nc, cell_ref, i_port_ref)==NULL?"":gnc_GetCellPortName(nc, cell_ref, i_port_ref),
                    gnc_GetCellPortName(nc, cell_ref, o_port_ref)==NULL?"":gnc_GetCellPortName(nc, cell_ref, o_port_ref)
                    );
                  */
                }
              }
            }
          } /* while */
        }
        else
        {
          if ( dg->is_acyclic == 0 ) 
          {
            gnc_Error(nc, "DELAY: Path calculation failed (cyclic graph).");
          }
          else
          {
            gnc_Error(nc, "DELAY: Path calculation failed (memory error).");
          }
        }
      }
    } 
  } /* while */
  return 1;
}



int gnc_CalcCellDelay(gnc nc, int cell_ref)
{
  b_dg_type dg;
  dg = b_dg_Open();
  if ( dg == NULL )
    return gnc_Error(nc, "DELAY: Out of memory (b_dg_Open)."), 0;

  if ( gnc_InitCellDelay(nc, cell_ref) == 0 )
    return gnc_Error(nc, "DELAY: Out of memory."), 0;
    
  /* longest path calculation */

  gnc_Log(nc, 1, "Delay calculation: Longest path.");
    
  if ( gnc_convert_to_directed_graph(nc, cell_ref, dg, 1) == 0 )
    return b_dg_Close(dg), 0;
    
  if ( gnc_path_calculation(nc, cell_ref, dg, 1) == 0 )
    return b_dg_Close(dg), 0;
    
  if ( dg->is_acyclic == 0 ) 
    return b_dg_Close(dg), 0;

  /* shortest path calculation */

  gnc_Log(nc, 1, "Delay calculation: Shortest path.");
    
  if ( gnc_convert_to_directed_graph(nc, cell_ref, dg, 0) == 0 )
    return b_dg_Close(dg), 0;
    
  if ( gnc_path_calculation(nc, cell_ref, dg, 0) == 0 )
    return b_dg_Close(dg), 0;
    
  if ( dg->is_acyclic == 0 ) 
    return b_dg_Close(dg), 0;

  return b_dg_Close(dg), 1;
}

/*---------------------------------------------------------------------------*/

static int gnc_GetDelayCellRef(gnc nc)
{
  if ( nc->async_delay_cell_ref < 0 ) 
    return gnc_GetCellById(nc, GCELL_ID_INV);
  return nc->async_delay_cell_ref;
}


/*---------------------------------------------------------------------------*/

int gnc_GetInverterPairCnt(gnc nc, int cell_ref, double dly, double *gen_dly)
{
  int delay_cell_ref;
  int id_port_ref;
  int od_port_ref;
  double inv_min, inv_max;
  double cap;
  int pair_cnt;

  delay_cell_ref = gnc_GetDelayCellRef(nc);
  if ( delay_cell_ref < 0 )
    return -1;

  id_port_ref = gnc_FindCellPortByType(nc, delay_cell_ref, GPORT_TYPE_IN);
  if ( id_port_ref < 0 )
    return -1;
  od_port_ref = gnc_FindCellPortByType(nc, delay_cell_ref, GPORT_TYPE_OUT);
  if ( od_port_ref < 0 )
    return -1;
  cap = gnc_GetCellPortInputLoad(nc, delay_cell_ref, id_port_ref);
  inv_max = gnc_GetCellDefMaxDelay(nc, delay_cell_ref, id_port_ref, od_port_ref, cap);
  inv_min = gnc_GetCellDefMinDelay(nc, delay_cell_ref, id_port_ref, od_port_ref, cap);

  pair_cnt = (int)ceil((dly*GNC_DELAY_SAFETY)/(inv_max+inv_min));
  
  gnc_Log(nc, 3, "Delay calculation: Delay per inverter pair %lg, required delay %lg, inverter pairs %d.", 
    inv_max+inv_min, dly, pair_cnt);

  if ( gen_dly != NULL )
    *gen_dly = pair_cnt*(inv_max+inv_min);

  return pair_cnt;
}


/*---------------------------------------------------------------------------*/
/* insert a delay element into the netlist */

/* only ID's with one input and one output are allowed */
/* at the moment this is only GCELL_ID_INV */

static int gnc_insert_cell_after_port(gnc nc, int cell_ref, int z_port_ref, int new_cell_ref)
{
  int old_net_ref;
  int new_net_ref;
  int join_ref;
  int node_ref;
  int port_ref;
  int i_door_port_ref;
  int o_door_port_ref;
  int door_node_ref;

  door_node_ref = gnc_get_door_node(nc, cell_ref, z_port_ref, 
    &i_door_port_ref, &o_door_port_ref);
    
  if ( door_node_ref < 0 )
    return gnc_Error(nc, "FEEDBACK DELAY not found."), 0;

  
  old_net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, z_port_ref);
  if ( old_net_ref < 0 )
    return 0;
      
  join_ref = gnc_FindCellNetJoin(nc, cell_ref, old_net_ref, -1, z_port_ref);
  if ( join_ref < 0 )
    return 0;
  gnc_DelCellNetPort(nc, cell_ref, old_net_ref, join_ref);
  
  join_ref = gnc_FindCellNetJoin(nc, cell_ref, old_net_ref, door_node_ref, o_door_port_ref);
  if ( join_ref < 0 )
    return 0;
  gnc_DelCellNetPort(nc, cell_ref, old_net_ref, join_ref);
  
  new_net_ref = gnc_AddCellNet(nc, cell_ref, NULL);
  if ( new_net_ref < 0 )
    return 0;

  if ( gnc_AddCellNetJoin(nc, cell_ref, new_net_ref, -1, z_port_ref, 0) < 0 )
    return 0;

  if ( gnc_AddCellNetJoin(nc, cell_ref, new_net_ref, door_node_ref, o_door_port_ref, 0) < 0 )
    return 0;
    
  node_ref = gnc_AddCellNode(nc, cell_ref, NULL, new_cell_ref);
  if ( node_ref < 0 )
    return 0;
    
  port_ref = gnc_FindCellPortByType(nc, new_cell_ref, GPORT_TYPE_IN);
  if ( port_ref < 0 )
    return 0;

  if ( gnc_AddCellNetJoin(nc, cell_ref, new_net_ref, node_ref, port_ref, 0) < 0 )
    return 0;
  
  port_ref = gnc_FindCellPortByType(nc, new_cell_ref, GPORT_TYPE_OUT);
  if ( port_ref < 0 )
    return 0;

  if ( gnc_AddCellNetJoin(nc, cell_ref, old_net_ref, node_ref, port_ref, 0) < 0 )
    return 0;
    
  return 1;
}

int gnc_InsertPortFeedbackDelayElement(gnc nc, int cell_ref, int z_port_ref)
{
  gcell cell;
  cell = gnc_GetGCELL(nc, cell_ref);
  if ( cell->register_width <= 0 || cell->pi == NULL  )
    return 0;
  assert(z_port_ref >= cell->pi->in_cnt-cell->register_width);
  assert(z_port_ref < cell->pi->in_cnt);
  
  if ( gnc_insert_cell_after_port(nc, cell_ref, z_port_ref, gnc_GetDelayCellRef(nc)) == 0 )
    return gnc_Error(nc, "FEEDBACK DELAY: Can not insert inverter"), 0;
  if ( gnc_insert_cell_after_port(nc, cell_ref, z_port_ref, gnc_GetDelayCellRef(nc)) == 0 )
    return gnc_Error(nc, "FEEDBACK DELAY: Can not insert inverter"), 0;

  return 1;  
}

int gnc_InsertPortFeedbackDelay(gnc nc, int cell_ref, int z_port_ref, double dly, double *path_delay)
{
  double inner_path_delay;
  int i, pair_cnt;
  
  pair_cnt = gnc_GetInverterPairCnt(nc, cell_ref, dly, &inner_path_delay);
  if ( pair_cnt < 0 )
    return 0;

  for( i = 0; i < pair_cnt; i++ )
    if ( gnc_InsertPortFeedbackDelayElement(nc, cell_ref, z_port_ref) == 0 )
      return 0;

  gnc_Log(nc, 4, "Asynchronous FSM: Added %d inverters (delay %lg >= %lg) on feed-back line '%s'.", 
    pair_cnt*2, 
    inner_path_delay,
    dly,
    gnc_GetCellPortName(nc, cell_ref, z_port_ref)
    );
    
  if ( path_delay != NULL )
    *path_delay = inner_path_delay;
    
  return 1;
}


/*---------------------------------------------------------------------------*/
/* special feed-back delay calculation */
/*

  Inputs: States
          Transitions T(s,t)
          State codes Z(s)
          fully specified boolean function F: i * Z(s) --> Z(t) * o

  with all state lines
    d(state line) = 0.0
  
  with all transitions T(s,t)
    calculate M = EXPAND_DC(INTERSECTION(T(s,s),D1(T(s,t))))
    with all minterms m ELEMENT_OF M
      with all input variables x_i
        n = m with complemented variable x_i
        if n IS A SUBSET OF T(s,t) then
          if m IS NOT A SUBSET OF T(t,t) and NOT A SUBSET OF T(t,s) then
            determine state u from the fully specified machine function
            if ( u != t && u != s )
            
              DELAY SIMULATION: input n, state s, output u
                            --> input m, state t, output v

              max = simulation settle time Dmax (see below)
              with all changing state lines
                curr = change time of the state line
                if ( d(state line) < max-curr )
                  d(state line) =  max-curr
            
  Simulation:
    Simulation must calculate three different times:
    - delay from inputs to zoX
    - delay from inputs to ziX, D(ziX) > D(zoX)
    - delay from inputs to any output D(oX)
    - the time of the last event, that has not passed any zoX port
      (events with index 0, D(0))

    - def: Dmax = MAX(D(ziX), D(oX), D(0))
      Dmax is an upper bound for the time where no other signal
      must be feed back into the circuit.
    - Each feed-back line must be delayed by Dmax - D(zoX)
    
    
  with all transitions T(s,t), t != s
    with all minterms m in INTERSECTION(T(s,s),D1(T(s,t)))
      with all input variables x_i
        n = m with complemented variable x_i
          if DELTA(n, s) == t then
            if DELTA(m, t) != t && DELTA(m, t) != s
        
            
*/

/* IFD = insert feed-back delay */
static void gspq_IFD_time(gspq_type pq, fsm_type fsm, int edge_id)
{
  int src_node_id = fsm_GetEdgeSrcNode(fsm, edge_id);
  int dest_node_id = fsm_GetEdgeDestNode(fsm, edge_id);
  pinfo *pi = fsm_GetCodePINFO(fsm);
  dcube *src_c = fsm_GetNodeCode(fsm, src_node_id);
  dcube *dest_c = fsm_GetNodeCode(fsm, dest_node_id);
  int register_width = gnc_GetGCELL(pq->nc, pq->cell_ref)->register_width;
  int state_out_width = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->out_cnt;
  int inputs = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt;
  int i;
  int is_min = 0, is_max = 0;
  double time, min = 0.0, max = 0.0, diff;
  
  assert(register_width == pi->out_cnt);
  assert( register_width <= state_out_width );
  
  
  /* NOTE: delay of the output lines IS considered in the following loop */
  
  for( i = 0; i < state_out_width; i++ )
  {
    /*
      The following 'if' is executed if 
        1. this is an output value (i >= register_width) or 
        2. the state bit has changed.
      This condition could be stronger: 
        1. the output bit has changed or 
        2. the state bit has changed.
      Well, getting the state's output value is a little bit complicated
      and avoided here.
      Also note that dcGetOut(src_c, i) with i>=register_width would
      access invalid memory.
    */
    if ( i >= register_width || dcGetOut(src_c, i) != dcGetOut(dest_c, i) )
    {
      if ( i >= register_width )
      {
        time = gspq_GetTime(pq, 1, inputs + i);

        gnc_Log(pq->nc, 0, "Delay calculation: Delay on line '%s' for transition '%s' --> '%s' "
           "is %lg.",
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
           fsm_GetNodeNameStr(fsm, src_node_id),
           fsm_GetNodeNameStr(fsm, dest_node_id),
           time
        );
      }
      else
      {
        time = gspq_GetTime(pq, 1, inputs + i);
        
        gnc_Log(pq->nc, 0, "Delay calculation: Delay on line '%s'/'%s' for transition '%s' --> '%s' "
           "is %lg/%lg.",
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs - register_width + i),
           fsm_GetNodeNameStr(fsm, src_node_id),
           fsm_GetNodeNameStr(fsm, dest_node_id),
           gspq_GetTime(pq, 1, inputs + i),
           gspq_GetTime(pq, 1, inputs - register_width  + i)
        );
      }
      

      if ( is_min == 0 )
      {
        min = time;
        is_min = 1;
      }
      if ( min > time )
        min = time;
        
      if ( is_max == 0 )
      {
        max = time;
        is_max = 1;
      }
      if ( max < time )
        max = time;
    }
  }

  gnc_Log(pq->nc, 0, "Delay calculation: Stable delay %lg, index time 0: %lg, 1: %lg.",
     pq->time, pq->indexed_time[0],  pq->indexed_time[1]
  );

  if ( max < pq->indexed_time[0] )
  {
    gnc_Log(pq->nc, 0, "Delay calculation: Index time 0 (%lg) used as maximum (> %lg).",
       pq->indexed_time[0],  max
    );
    max = pq->indexed_time[0];
  }
  
  diff = max - min;
  
  gnc_Log(pq->nc, 2, "Delay calculation: For transition '%s' --> '%s' all lines are valid after %lg.",
     fsm_GetNodeNameStr(fsm, src_node_id),
     fsm_GetNodeNameStr(fsm, dest_node_id),
     max
  );

  /*
    Now we know the maximum delay to either the next state or the output
    values. Any feed-back line that changes its value must not be faster 
    than this maximum delay. So the feed-back lines must be delayed
    by the difference to the maximum delay.
    At the moment I think, that this procedure also avoids essential
    hazards on output lines.
  */

  
  for( i = 0; i < pi->out_cnt; i++ )
  {
    if ( dcGetOut(src_c, i) != dcGetOut(dest_c, i) )
    {
      /* this is the conservative value */
      /* time = gspq_GetTime(pq, 1, inputs + i); */
      /* we do use this value, because the door element serves as a delay */
      time = gspq_GetTime(pq, 1, inputs - register_width + i);
      gspq_ApplyMaxTime(pq, 2, inputs + i, max-time);

      gnc_Log(pq->nc, 1, "Delay calculation: Required delay between '%s' (%lg) and '%s' (%lg) caused by transition '%s' --> '%s' "
         "is %lg - %lg = %lg.",
         gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
         gspq_GetTime(pq, 1, inputs + i),
         gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs - register_width + i),
         gspq_GetTime(pq, 1, inputs - register_width + i),
         fsm_GetNodeNameStr(fsm, src_node_id),
         fsm_GetNodeNameStr(fsm, dest_node_id),
         max, 
         time,
         max-time
      );

      gnc_Log(pq->nc, 1, "Delay calculation: Current additional delay for feed-back line '%s' "
         "is %lg.",
         gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
         gspq_GetTime(pq, 2, inputs + i)
      );

    }
  }
}

/* IFD = insert feed-back delay */
static int gspq_IFD_minterm(gspq_type pq, fsm_type fsm, int edge_id, dcube *m, int *ess_cnt, int *total_cnt)
{
  int i;
  pinfo *pi = fsm_GetConditionPINFO(fsm);
  dcube *n = &(pi->tmp[15]);
  int node_1_id = fsm_GetEdgeSrcNode(fsm, edge_id);
  int node_2_id = fsm_GetEdgeDestNode(fsm, edge_id);
  char *n_str;
  char *m_str;
  dcube *code_node_1 = fsm_GetNodeCode(fsm, node_1_id);
  dcube *code_node_2 = fsm_GetNodeCode(fsm, node_2_id);  
  
  for( i = 0; i < pi->in_cnt; i++ )
  {
    /* create a copy */
    dcCopy(pi, n, m);
    
    /* invert variable i */
    dcSetIn(n, i, dcGetIn(n, i)^3);
    
    
    if ( dclIsSingleSubSet(pi, fsm_GetEdgeCondition(fsm, edge_id), n) != 0 )
    {
      int loop = -1;
      int edge_2_3_id = -1;
      int node_3_id = -1;
      dcube *code_node_3 = &(fsm_GetCodePINFO(fsm)->tmp[10]);

      if ( total_cnt != NULL )
        (*total_cnt)++;
      
      fsm_GetNextNodeCode(fsm, node_2_id, m, &node_3_id, code_node_3);
      
      if ( node_2_id != 
           node_3_id
           &&
           node_1_id !=
           node_3_id
           &&
           dcIsEqualOut(fsm_GetCodePINFO(fsm), code_node_2, code_node_3) == 0
           &&
           dcIsEqualOut(fsm_GetCodePINFO(fsm), code_node_1, code_node_3) == 0
           )
      {


        n_str = dcToStr(pi, n, "", ""), 
        m_str = dcToStr2(pi, m, "", ""),

        gnc_Log(pq->nc, 3, "Delay calculation: Essential hazard, "
           "var %d, input/state %s/'%s' --> %s/'%s' --> %s/'%s'.",
           i,
           m_str,
           fsm_GetNodeNameStr(fsm, node_1_id),
           n_str,
           fsm_GetNodeNameStr(fsm, node_2_id),
           m_str,
           fsm_GetNodeNameStr(fsm, node_3_id)
        );

        gnc_Log(pq->nc, 3, "Delay calculation: Essential hazard, "
           "var %d, state (code) "
           "'%s' (%s) --> '%s' (%s) --> '%s' (%s).",
           i,
           fsm_GetNodeNameStr(fsm, node_1_id),
           dcToStr(fsm_GetCodePINFO(fsm), code_node_1, "/", ""),
           fsm_GetNodeNameStr(fsm, node_2_id),
           dcToStr2(fsm_GetCodePINFO(fsm), code_node_2, "/", ""),
           fsm_GetNodeNameStr(fsm, node_3_id),
           dcToStr3(fsm_GetCodePINFO(fsm), code_node_3, "/", "")
        );

        if ( ess_cnt != NULL )
          (*ess_cnt)++;

        pq->is_calc_max_in_out_dly = 0;
        
        gnc_SetAllCellNetLogicVal(pq->nc, pq->cell_ref, GLV_UNKNOWN);
        gnc_SetAllCellPortLogicVal(pq->nc, pq->cell_ref, GLV_UNKNOWN);

        /* gspq_PrepareFSMSimulation(pq); */

        gspq_ApplyCellFSMSimulationTransitionState(pq,
          pi, 
          m, 
          fsm_GetCodePINFO(fsm), 
          fsm_GetNodeCode(fsm, node_1_id)
        );

        {
          int net_ref = -1;
          glv_struct *lv;

          while( gnc_LoopCellNet(pq->nc, pq->cell_ref, &net_ref) != 0 )
          {
            lv = gnc_GetCellNetLogicVal(pq->nc, pq->cell_ref, net_ref);
            if ( lv->logic_value == GLV_UNKNOWN )
            {
              gnc_Log(pq->nc, 0, "Simulation: UNKNOWN value for net %d.",
                net_ref);
            }
          }
        }


        if ( gspq_CompareCellFSMStateForSimulation(pq, 
          fsm_GetCodePINFO(fsm), 
          fsm_GetNodeCode(fsm, node_1_id)) == 0 )
          return 0;

        pq->is_calc_max_in_out_dly = 1;
        gspq_ApplyCellFSMSimulationTransition(pq,
          pi, 
          n
        );

        {
          int net_ref = -1;
          glv_struct *lv;

          while( gnc_LoopCellNet(pq->nc, pq->cell_ref, &net_ref) != 0 )
          {
            lv = gnc_GetCellNetLogicVal(pq->nc, pq->cell_ref, net_ref);
            if ( lv->logic_value == GLV_UNKNOWN )
            {
              gnc_Log(pq->nc, 0, "Simulation: UNKNOWN value for net %d.",
                net_ref);
            }
          }
        }

        if ( gspq_CompareCellFSMStateForSimulation(pq, 
          fsm_GetCodePINFO(fsm), 
          fsm_GetNodeCode(fsm, node_2_id)) == 0 )
          return 0;
        /*
        for( i = 0; i < gnc_GetCellPortMax(pq->nc, pq->cell_ref); i++ )
          printf("%d  %10s %lg\n", i, 
            gnc_GetCellPortName(pq->nc, pq->cell_ref, i)==NULL?"":gnc_GetCellPortName(pq->nc, pq->cell_ref, i),
            pq->t_max[i]);
        */

        gspq_IFD_time(pq, fsm, edge_id);

      }
    }
  }
  
  return 1;
  
}

static int gspq_IFD_state(gspq_type pq, fsm_type fsm, int edge_id, int *ess_cnt, int *total_cnt)
{
  int i;
  pinfo *pi = fsm_GetConditionPINFO(fsm);
  dclist d1, m;
  int src_node_id = fsm_GetEdgeSrcNode(fsm, edge_id);
  
  if ( dclInitCachedVA(pi, 2, &d1, &m) == 0 )
    return 0;
  
  /* calculate M = EXPAND_ONES(INTERSECTION(T(s,s),D1(T(s,t)))) */
  
  
  if ( dclDistance1(pi, d1, fsm_GetEdgeCondition(fsm, edge_id)) == 0 )
    return dclDestroyCachedVA(pi, 2, d1, m), 0;
    
  if ( dclIntersectionList(pi, m, fsm_GetNodeSelfCondtion(fsm, src_node_id), d1) == 0 )
    return dclDestroyCachedVA(pi, 2, d1, m), 0;
    
  if ( dclDontCareExpand(pi, m) == 0 )
    return dclDestroyCachedVA(pi, 2, d1, m), 0;

  for( i = 0; i < dclCnt(m); i++ )
    if ( gspq_IFD_minterm(pq, fsm, edge_id, dclGet(m, i), ess_cnt, total_cnt) == 0 )
      return dclDestroyCachedVA(pi, 2, d1, m), 0;
  
  return dclDestroyCachedVA(pi, 2, d1, m), 1;
}

int gspq_InsertFeedbackDelay(gspq_type pq, fsm_type fsm)
{
  pinfo *pi = fsm_GetCodePINFO(fsm);
  int inputs = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt;
  int edge_id;
  int i;
  double time;
  double path_delay;
  double logic_delay = 0.0;
  double max_path_delay = 0.0;
  int ess_cnt = 0, total_cnt = 0;

  /* only for calculating the fundamental delay */
  logic_delay = gnc_GetCellTotalMaxDelay(pq->nc, pq->cell_ref);
  
  edge_id = -1;
  while( fsm_LoopEdges(fsm, &edge_id) != 0 )
  {
    gnc_Log(pq->nc, 0, "Delay calculation: Checking transition '%s' --> '%s' for essential hazard.",
      fsm_GetNodeNameStr(fsm, fsm_GetEdgeSrcNode(fsm, edge_id)),
      fsm_GetNodeNameStr(fsm, fsm_GetEdgeDestNode(fsm, edge_id))
      );
    if ( gspq_IFD_state(pq, fsm, edge_id, &ess_cnt, &total_cnt) == 0 )
      return 0;
  }

  if ( total_cnt > 0 )
    gnc_Log(pq->nc, 4, "Asynchronous FSM: %d of %d transitions (%.1f%%) could generate an essential hazard.", 
      ess_cnt, total_cnt, (double)((double)ess_cnt*100.0)/(double)total_cnt);
  
  for( i = 0; i < pi->out_cnt; i++ )
  {
    time = gspq_GetTime(pq, 2, inputs + i);
    gnc_Log(pq->nc, 3, "Delay calculation: Required delay for feed-back line '%s' "
       "is %lg.",
       gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
       time
    );
    if ( gnc_InsertPortFeedbackDelay(pq->nc, pq->cell_ref, inputs - pi->out_cnt + i, time, &path_delay) == 0 )
      return 0;
    if ( max_path_delay < path_delay )
      max_path_delay = path_delay;
  }

  gnc_Log(pq->nc, 4, "Asynchronous FSM: Maximum feed-back delay is %lf.", 
    max_path_delay );

  gnc_Log(pq->nc, 4, "Asynchronous FSM: Logic delay (longest path) is %lf.", 
    logic_delay );

  gnc_Log(pq->nc, 5, "Asynchronous FSM: Estimated (longest path) 'fundamental mode' delay is %lf.", 
    max_path_delay+2*logic_delay );
  
  gnc_Log(pq->nc, 4, "Asynchronous FSM: Total cell area after delay correction is %lf.",
    gnc_CalcCellNetNodeArea(pq->nc, pq->cell_ref));


  
  return 1;
}

int gnc_InsertFeedbackDelay(gnc nc, int cell_ref, fsm_type fsm)
{
  gspq_type pq;
  pq = gspq_Open(nc, cell_ref);
  if ( pq != NULL )
  {
    pq->is_stop_at_fsm_input = 1;
    if ( gspq_InsertFeedbackDelay(pq, fsm) != 0 )
    {
      gspq_Close(pq);
      return 1;
    }
    gspq_Close(pq);
  }
  return 0;
}


/*---------------------------------------------------------------------------*/



double gnc_GetCellTotalMaxDelay(gnc nc, int cell_ref)
{
  if ( gnc_CalcCellDelay(nc, cell_ref) == 0 )
    return -1.0;
  return gnc_GetCellTotalMax(nc, cell_ref);
}

int gnc_InsertFeedbackDelayElements(gnc nc, int cell_ref)
{
  double max;
  double min;
  double path_delay;
  int pair_cnt;
  int z_port_ref;
  int i;
  gcell cell = gnc_GetGCELL(nc, cell_ref);

  /* until we have a better algorithm... */

  if ( gnc_CalcCellDelay(nc, cell_ref) == 0 )
    return 0;

  max = gnc_GetCellTotalMax(nc, cell_ref);
  min = gnc_GetCellTotalMin(nc, cell_ref);

  gnc_Log(nc, 4, "Asynchronous FSM: Min delay %lg, max delay %lg, difference %lg.", 
    min, max, max-min);

  pair_cnt = gnc_GetInverterPairCnt(nc, cell_ref, max-min, &path_delay);
  if ( pair_cnt < 0 )
    return 0;
  
  for( z_port_ref = cell->pi->in_cnt-cell->register_width;
       z_port_ref < cell->pi->in_cnt;
       z_port_ref++ )
    for( i = 0; i < pair_cnt; i++ )
      if ( gnc_InsertPortFeedbackDelayElement(nc, cell_ref, z_port_ref) == 0 )
        return 0;

  gnc_Log(nc, 4, "Asynchronous FSM: Added %d inverters on each feedback line (Total cell area %lf).", 
    pair_cnt*2, gnc_CalcCellNetNodeArea(nc, cell_ref));

  gnc_Log(nc, 4, "Asynchronous FSM: Feed-back delay %lg.", 
    path_delay);

  gnc_Log(nc, 4, "Asynchronous FSM: Estimated 'fundamental mode' delay is %lg.", 
    2*max + path_delay);

  gnc_Log(nc, 4, "Asynchronous FSM: Total cell area after delay correction is %lf.",
    gnc_CalcCellNetNodeArea(nc, cell_ref));


  return 1;
}


/*---------------------------------------------------------------------------*/





int gnc_ShowCellDelay(gnc nc, int cell_ref)
{
  int port_ref, out_port_ref;
  
  if ( gnc_CalcCellDelay(nc, cell_ref) == 0 )
    return 0;
  
  printf("%-12s", "MAX DELAY");
  out_port_ref = -1;
  while( gnc_LoopCellPort(nc, cell_ref, &out_port_ref) != 0 )
    if ( gnc_GetCellPortType(nc, cell_ref, out_port_ref) == GPORT_TYPE_OUT )
      printf("%12s", gnc_GetCellPortName(nc, cell_ref, out_port_ref));
  printf("\n");
  
  port_ref = -1;
  while( gnc_LoopCellPort(nc, cell_ref, &port_ref) != 0 )
  {
    printf("%-12s", gnc_GetCellPortName(nc, cell_ref, port_ref));
    out_port_ref = -1;
    while( gnc_LoopCellPort(nc, cell_ref, &out_port_ref) != 0 )
      if ( gnc_GetCellPortType(nc, cell_ref, out_port_ref) == GPORT_TYPE_OUT )
        printf("%12.3g", gnc_GetCellMaxDelay(nc, cell_ref, port_ref, out_port_ref));
    printf("\n");
  }


  printf("%-12s", "MIN DELAY");
  out_port_ref = -1;
  while( gnc_LoopCellPort(nc, cell_ref, &out_port_ref) != 0 )
    if ( gnc_GetCellPortType(nc, cell_ref, out_port_ref) == GPORT_TYPE_OUT )
      printf("%12s", gnc_GetCellPortName(nc, cell_ref, out_port_ref));
  printf("\n");
  
  port_ref = -1;
  while( gnc_LoopCellPort(nc, cell_ref, &port_ref) != 0 )
  {
    printf("%-12s", gnc_GetCellPortName(nc, cell_ref, port_ref));
    out_port_ref = -1;
    while( gnc_LoopCellPort(nc, cell_ref, &out_port_ref) != 0 )
      if ( gnc_GetCellPortType(nc, cell_ref, out_port_ref) == GPORT_TYPE_OUT )
        printf("%12.3g", gnc_GetCellMinDelay(nc, cell_ref, port_ref, out_port_ref));
    printf("\n");
  }

  return 1;
}

/*---------------------------------------------------------------------------*/

int gnc_BuildDelayPath(gnc nc, int cell_ref, double dly, const char *in_delay_port_name, const char *out_delay_port_name)
{
  const char *in_delay_name = "req";
  const char *out_delay_name = "ack";
  int no_of_inverter_pairs;
  int id_port_ref;
  int od_port_ref;
  int ip_port_ref;
  int op_port_ref;
  int net_ref;
  int node_ref;
  int i;
  int delay_cell_ref;

  delay_cell_ref = gnc_GetDelayCellRef(nc);;
  if ( delay_cell_ref < 0 )
    return 0;

  if ( in_delay_port_name != NULL )
    in_delay_name = in_delay_port_name;

  if ( out_delay_port_name != NULL )
    out_delay_name = out_delay_port_name;

  id_port_ref = gnc_FindCellPortByType(nc, delay_cell_ref, GPORT_TYPE_IN);
  if ( id_port_ref < 0 )
    return 0;
  od_port_ref = gnc_FindCellPortByType(nc, delay_cell_ref, GPORT_TYPE_OUT);
  if ( od_port_ref < 0 )
    return 0;
  
  no_of_inverter_pairs = gnc_GetInverterPairCnt(nc, cell_ref, dly, NULL);

  /* now, create the delay path */

  
  ip_port_ref = gnc_FindCellPort(nc, cell_ref, in_delay_name);
  if ( ip_port_ref < 0 )
  {
    ip_port_ref = gnc_AddCellPort(nc, cell_ref, GPORT_TYPE_IN, in_delay_name);
    if ( ip_port_ref < 0 )
      return 0;
  }
  op_port_ref = gnc_FindCellPort(nc, cell_ref, out_delay_name);
  if ( op_port_ref < 0 )
  {
    op_port_ref = gnc_AddCellPort(nc, cell_ref, GPORT_TYPE_OUT, out_delay_name);
    if ( op_port_ref < 0 )
      return 0;
  }
    
  /* get the net_ref of the net which is connected to the input port */
  net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, ip_port_ref, 0, 0);
  
  if ( no_of_inverter_pairs == 0 )
  {
    int out_net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, op_port_ref, 0, 0);
    if ( out_net_ref < 0 )
      return 0;
    if ( gnc_MergeNets(nc, cell_ref, net_ref, out_net_ref) == 0 )
      return 0;
  }
  else
  {
    i = 0;
    for(;;)
    {
      /* create a new inverter */
      node_ref = gnc_AddCellNode(nc, cell_ref, NULL, delay_cell_ref);
      if ( node_ref < 0 )
        return 0;

      /* connect the input port of the inverter to the current net_ref */
      if ( gnc_AddCellNetJoin(nc, cell_ref, net_ref, node_ref, id_port_ref, 0) < 0 )
        return 0;

      /* at this point the ouput port of the inverter is still unconnected */

      if ( i >= no_of_inverter_pairs*2-1 )
        break;

      /* create a new net, overwrite the current net_ref  */
      net_ref = gnc_AddCellNet(nc, cell_ref, NULL);
      if ( net_ref < 0 )
        return 0;

      /* connect the output port of the inverter to the new net */
      if ( gnc_AddCellNetJoin(nc, cell_ref, net_ref, node_ref, od_port_ref, 0) < 0 )
        return 0;
      i++;
    }

    net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, op_port_ref, 0, 0);
    if ( net_ref < 0 )
      return 0;
    if ( gnc_AddCellNetJoin(nc, cell_ref, net_ref, node_ref, od_port_ref, 0) < 0 )
      return 0;
  }

  return 1;  
}

/* delay path for logic blocks */

int gnc_AddDelayPath(gnc nc, int cell_ref, double min_dly, const char *in_delay_port_name, const char *out_delay_port_name)
{
  double net_max_delay;
  
  if ( gnc_CalcCellDelay(nc, cell_ref) == 0 )
    return 0;

  net_max_delay = gnc_GetCellTotalMax(nc, cell_ref);
  if ( net_max_delay < min_dly )
    net_max_delay = min_dly;

  return gnc_BuildDelayPath(nc, cell_ref, net_max_delay, in_delay_port_name, out_delay_port_name);
}

/*---------------------------------------------------------------------------*/


static void gspq_ess_time(gspq_type pq, dcube *src_c, dcube *dest_c)
{
  pinfo *pi = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi;
  int fb_cnt = gnc_GetGCELL(pq->nc, pq->cell_ref)->register_width;
  int state_out_width = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->out_cnt;
  int inputs = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt;
  int i;
  int is_min = 0, is_max = 0;
  double time, min = 0.0, max = 0.0, diff;
  
  assert( fb_cnt <= state_out_width );
  
  
  /* NOTE: delay of the output lines IS considered in the following loop */
  
  for( i = 0; i < state_out_width; i++ )
  {
    /*
      The following 'if' is executed if 
        1. this is an output value (i >= register_width) or 
        2. the state bit has changed.
      This condition could be stronger: 
        1. the output bit has changed or 
        2. the state bit has changed.
      Well, getting the state's output value is a little bit complicated
      and avoided here.
      Also note that dcGetOut(src_c, i) with i>=register_width would
      access invalid memory.  ... 12 apr 03 why???
    */
    if ( i >= fb_cnt || dcGetOut(src_c, i) != dcGetOut(dest_c, i) )
    {
      if ( i >= fb_cnt )
      {
        time = gspq_GetTime(pq, 1, inputs + i);

        gnc_Log(pq->nc, 0, "Delay calculation: Delay on line '%s' "
           "is %lg.",
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
           time
        );
      }
      else
      {
        time = gspq_GetTime(pq, 1, inputs + i);
        
        gnc_Log(pq->nc, 0, "Delay calculation: Delay on line '%s'/'%s' "
           "is %lg/%lg.",
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs - fb_cnt + i),
           gspq_GetTime(pq, 1, inputs + i),
           gspq_GetTime(pq, 1, inputs - fb_cnt  + i)
        );
      }
      

      if ( is_min == 0 )
      {
        min = time;
        is_min = 1;
      }
      if ( min > time )
        min = time;
        
      if ( is_max == 0 )
      {
        max = time;
        is_max = 1;
      }
      if ( max < time )
        max = time;
    }
  }

  gnc_Log(pq->nc, 0, "Delay calculation: Stable delay %lg, index time 0: %lg, 1: %lg.",
     pq->time, pq->indexed_time[0],  pq->indexed_time[1]
  );

  if ( max < pq->indexed_time[0] )
  {
    gnc_Log(pq->nc, 0, "Delay calculation: Index time 0 (%lg) used as maximum (> %lg).",
       pq->indexed_time[0],  max
    );
    max = pq->indexed_time[0];
  }
  
  diff = max - min;
  
  gnc_Log(pq->nc, 2, "Delay calculation: For this transition all lines are valid after %lg.",
     max
  );

  /*
    Now we know the maximum delay to either the next state or the output
    values. Any feed-back line that changes its value must not be faster 
    than this maximum delay. So the feed-back lines must be delayed
    by the difference to the maximum delay.
    At the moment I think, that this procedure also avoids essential
    hazards on output lines.
  */

  
  for( i = 0; i < pi->out_cnt; i++ )
  {
    if ( dcGetOut(src_c, i) != dcGetOut(dest_c, i) )
    {
      /* this is the conservative value */
      /* time = gspq_GetTime(pq, 1, inputs + i); */
      /* we do use this value, because the door element serves as a delay */
      time = gspq_GetTime(pq, 1, inputs - fb_cnt + i);
      gspq_ApplyMaxTime(pq, 2, inputs + i, max-time);

      gnc_Log(pq->nc, 1, "Delay calculation: Required delay between '%s' (%lg) and '%s' (%lg) "
         "is %lg - %lg = %lg.",
         gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
         gspq_GetTime(pq, 1, inputs + i),
         gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs - fb_cnt + i),
         gspq_GetTime(pq, 1, inputs - fb_cnt + i),
         max, 
         time,
         max-time
      );

      gnc_Log(pq->nc, 1, "Delay calculation: Current additional delay for feed-back line '%s' "
         "is %lg.",
         gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
         gspq_GetTime(pq, 2, inputs + i)
      );

    }
  }
}


static int xbm_ess_cb(xbm_type x, void *data, dcube *n1, dcube *n2, dcube *n3)
{
  gspq_type pq = (gspq_type)data;


  pq->is_calc_max_in_out_dly = 0;

  gnc_SetAllCellNetLogicVal(pq->nc, pq->cell_ref, GLV_UNKNOWN);
  gnc_SetAllCellPortLogicVal(pq->nc, pq->cell_ref, GLV_UNKNOWN);


  gspq_ApplyCellXBMSimulationTransitionState(pq, n1);

  {
    int net_ref = -1;
    glv_struct *lv;

    while( gnc_LoopCellNet(pq->nc, pq->cell_ref, &net_ref) != 0 )
    {
      lv = gnc_GetCellNetLogicVal(pq->nc, pq->cell_ref, net_ref);
      if ( lv->logic_value == GLV_UNKNOWN )
      {
        gnc_Log(pq->nc, 0, "Simulation: UNKNOWN value for net %d.",
          net_ref);
      }
    }
  }


  if ( gspq_CompareCellXBMStateForSimulation(pq, n1) == 0 )
    return 0;

  pq->is_calc_max_in_out_dly = 1;
  gspq_ApplyCellXBMSimulationTransition(pq, n2);

  {
    int net_ref = -1;
    glv_struct *lv;

    while( gnc_LoopCellNet(pq->nc, pq->cell_ref, &net_ref) != 0 )
    {
      lv = gnc_GetCellNetLogicVal(pq->nc, pq->cell_ref, net_ref);
      if ( lv->logic_value == GLV_UNKNOWN )
      {
        gnc_Log(pq->nc, 0, "Simulation: UNKNOWN value for net %d.",
          net_ref);
      }
    }
  }

  if ( gspq_CompareCellXBMStateForSimulation(pq, n2) == 0 )
    return 0;


  gspq_ess_time(pq, n1, n2);
  return 1;
}


int gspq_InsertXBMFeedbackDelay(gspq_type pq, xbm_type x)
{
  int inputs = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt;
  int fb_cnt = gnc_GetGCELL(pq->nc, pq->cell_ref)->register_width;
  int edge_id;
  int i;
  double time;
  double path_delay;
  double logic_delay = 0.0;
  double max_path_delay = 0.0;
  int ess_cnt = 0, total_cnt = 0;
  int input_port;

  /* only for calculating the fundamental delay */
  logic_delay = gnc_GetCellTotalMaxDelay(pq->nc, pq->cell_ref);
  

  if ( xbm_CalcEssentialHazards(x, xbm_ess_cb, pq) == 0 )
    return 0;

  if ( x->total_cnt > 0 )
    gnc_Log(pq->nc, 4, "XBM: %d of %d transitions (%.1f%%) could generate an essential hazard.", 
      x->ess_cnt, x->total_cnt, (double)((double)x->ess_cnt*100.0)/(double)x->total_cnt);
  
  for( i = 0; i < fb_cnt; i++ )
  {
    time = gspq_GetTime(pq, 2, inputs + i);
    gnc_Log(pq->nc, 3, "Delay calculation: Required delay for feed-back line '%s' "
       "is %lg.",
       gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
       time
    );
    input_port = inputs - fb_cnt + i;
    if ( dclIsDCInVar(x->pi_machine, x->cl_machine, input_port) != 0 )
    {
      gnc_Log(pq->nc, 4, "XBM: Unused feedback-line '%s', creation of feedback suppressed.",
        gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs+i));
    } 
    else
    {
      if ( gnc_InsertPortFeedbackDelay(pq->nc, pq->cell_ref, input_port, time, &path_delay) == 0 )
        return 0;
      if ( max_path_delay < path_delay )
        max_path_delay = path_delay;
    }
  }

  gnc_Log(pq->nc, 4, "Asynchronous FSM: Maximum feed-back delay is %lf.", 
    max_path_delay );

  gnc_Log(pq->nc, 4, "Asynchronous FSM: Logic delay (longest path) is %lf.", 
    logic_delay );

  gnc_Log(pq->nc, 5, "Asynchronous FSM: Estimated (longest path) 'fundamental mode' delay is %lf.", 
    max_path_delay+2*logic_delay );
  
  gnc_Log(pq->nc, 4, "Asynchronous FSM: Total cell area after delay correction is %lf.",
    gnc_CalcCellNetNodeArea(pq->nc, pq->cell_ref));

  return 1;
}

int gnc_InsertXBMFeedbackDelay(gnc nc, int cell_ref, xbm_type x)
{
  gspq_type pq;
  pq = gspq_Open(nc, cell_ref);
  if ( pq != NULL )
  {
    pq->is_stop_at_fsm_input = 1;
    if ( gspq_InsertXBMFeedbackDelay(pq, x) != 0 )
    {
      gspq_Close(pq);
      return 1;
    }
    gspq_Close(pq);
  }
  return 0;
}

/*---------------------------------------------------------------------------*/

/*

  external delay: A delay which is required if an output is feedback into
  the input of the circuit. This value is usually smaller than the 
  overall fundamental mode delay (due to the internal delay)

*/

static int gspq_fm_time(gspq_type pq, xbm_type x, int tr_pos, int var_in_idx, dcube *src_c, dcube *dest_c)
{
  pinfo *pi = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi;
  int fb_cnt = gnc_GetGCELL(pq->nc, pq->cell_ref)->register_width;
  int state_out_width = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->out_cnt;
  int inputs = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt;
  int i;
  int is_min = 0, is_max = 0;
  double time, min = 0.0, max = 0.0, diff;
  const char *invarstr = xbm_GetVarNameStrByIndex(x, XBM_DIRECTION_IN, var_in_idx);
  char pm = dcGetIn(dest_c, var_in_idx)==2?'+':(dcGetIn(dest_c, var_in_idx)==1?'-':'?');
  
  assert( fb_cnt <= state_out_width );
  
  
  /* NOTE: delay of the output lines IS considered in the following loop */
  
  for( i = 0; i < state_out_width; i++ )
  {
    if ( i >= fb_cnt || dcGetOut(src_c, i) != dcGetOut(dest_c, i) )
    {
      if ( i >= fb_cnt )
      {
        time = gspq_GetTime(pq, 1, inputs + i);

        gnc_Log(pq->nc, 0, "Fundamental mode dly calc: Delay on line '%s' "
           "is %lg.",
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
           time
        );
      }
      else
      {
        time = gspq_GetTime(pq, 1, inputs + i);
        
        gnc_Log(pq->nc, 0, "Fundamental mode dly calc: Delay on line '%s'/'%s' "
           "is %lg/%lg.",
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs - fb_cnt + i),
           gspq_GetTime(pq, 1, inputs + i),
           gspq_GetTime(pq, 1, inputs - fb_cnt  + i)
        );
      }
      

      if ( is_min == 0 )
      {
        min = time;
        is_min = 1;
      }
      if ( min > time )
        min = time;
        
      if ( is_max == 0 )
      {
        max = time;
        is_max = 1;
      }
      if ( max < time )
        max = time;
    }
  }

  gnc_Log(pq->nc, 0, "Fundamental mode dly calc: Stable delay %lg, index time 0: %lg, 1: %lg.",
     pq->time, pq->indexed_time[0],  pq->indexed_time[1]
  );

  if ( max < pq->indexed_time[0] )
  {
    gnc_Log(pq->nc, 0, "Fundamental mode dly calc: Index time 0 (%lg) used as maximum (> %lg).",
       pq->indexed_time[0],  max
    );
    max = pq->indexed_time[0];
  }

  if ( max < pq->indexed_time[1] )
  {
    gnc_Log(pq->nc, 0, "Fundamental mode dly calc: Index time 1 (%lg) used as maximum (> %lg).",
       pq->indexed_time[1],  max
    );
    max = pq->indexed_time[1];
  }
  
  diff = max - min;
  
  gnc_Log(pq->nc, 3, "Fundamental mode dly calc: "
     "'%s' -> '%s' %8s%c settling time %6lg.",
     xbm_GetStNameStr(x, xbm_GetTrSrcStPos(x, tr_pos)),
     xbm_GetStNameStr(x, xbm_GetTrDestStPos(x, tr_pos)),
     invarstr,
     pm,
     max
  );

  for( i = 0; i < state_out_width; i++ )
  {
    if ( dcGetOut(src_c, i) != dcGetOut(dest_c, i) )
    {
      if ( i >= fb_cnt )
      {

        gnc_Log(pq->nc, 2, "Fundamental mode dly calc: "
           "'%s' -> '%s' %8s%c -> %8s%c  cell dly %6.4lg   req ext dly %6.4lg.",
           xbm_GetStNameStr(x, xbm_GetTrSrcStPos(x, tr_pos)),
           xbm_GetStNameStr(x, xbm_GetTrDestStPos(x, tr_pos)),
           invarstr,
           pm, 
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
           dcGetOut(dest_c, i)==0?'-':'+',
           gspq_GetTime(pq, 1, inputs + i), 
           max-gspq_GetTime(pq, 1, inputs + i)
        );
        
        /* minimum logical delay */
        gspq_ApplyMaxMTime(pq, var_in_idx, inputs + i, max-gspq_GetTime(pq, 1, inputs + i));
        /* maximum settle time on an input */
        gspq_ApplyMaxTime(pq, 2, var_in_idx, max);
        /* minimum reaction time on an output */
        gspq_ApplyMaxTime(pq, 2, inputs + i, -gspq_GetTime(pq, 1, inputs + i));
      }
      else
      {
        time = gspq_GetTime(pq, 1, inputs + i);

        gnc_Log(pq->nc, 2, "Fundamental mode dly calc: "
           "'%s' -> '%s' %8s%c -> %8s%c  cell dly %6.4lg   req ext dly %6.4lg.",
           xbm_GetStNameStr(x, xbm_GetTrSrcStPos(x, tr_pos)),
           xbm_GetStNameStr(x, xbm_GetTrDestStPos(x, tr_pos)),
           invarstr,
           pm, 
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
           dcGetOut(dest_c, i)==0?'-':'+',
           gspq_GetTime(pq, 1, inputs + i), 
           max-gspq_GetTime(pq, 1, inputs + i));

        /* minimum logical delay */
        gspq_ApplyMaxMTime(pq, var_in_idx, inputs + i, max-gspq_GetTime(pq, 1, inputs + i));
        /* maximum settle time on an input */
        gspq_ApplyMaxTime(pq, 2, var_in_idx, max);
        /* minimum reaction time on an output */
        gspq_ApplyMaxTime(pq, 2, inputs + i, -gspq_GetTime(pq, 1, inputs + i));

        gnc_Log(pq->nc, 2, "Fundamental mode dly calc: "
           "'%s' -> '%s' %8s%c -> %8s%c  cell dly %6.4lg   req ext dly %6.4lg.",
           xbm_GetStNameStr(x, xbm_GetTrSrcStPos(x, tr_pos)),
           xbm_GetStNameStr(x, xbm_GetTrDestStPos(x, tr_pos)),
           invarstr,
           pm, 
           gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs - fb_cnt + i),
           dcGetOut(dest_c, i)==0?'-':'+',
           gspq_GetTime(pq, 1, inputs - fb_cnt + i), 
           max-gspq_GetTime(pq, 1, inputs - fb_cnt + i));

        /* minimum logical delay */
        gspq_ApplyMaxMTime(pq, var_in_idx, inputs - fb_cnt + i, max-gspq_GetTime(pq, 1, inputs - fb_cnt + i));
        /* maximum settle time on an input */
        gspq_ApplyMaxTime(pq, 2, var_in_idx, max);
        /* minimum reaction time on an output */
        gspq_ApplyMaxTime(pq, 2, inputs - fb_cnt + i, -gspq_GetTime(pq, 1, inputs - fb_cnt + i));
      }
      

      if ( is_min == 0 )
      {
        min = time;
        is_min = 1;
      }
      if ( min > time )
        min = time;
        
      if ( is_max == 0 )
      {
        max = time;
        is_max = 1;
      }
      if ( max < time )
        max = time;
    }
  }

  return 1;
}

static int xbm_fm_cb(xbm_type x, void *data, int tr_pos, int var_in_idx, dcube *n1, dcube *n2)
{
  gspq_type pq = (gspq_type)data;

  pq->is_stop_at_fsm_input = 0;
  pq->is_calc_max_in_out_dly = 0;

  gnc_SetAllCellNetLogicVal(pq->nc, pq->cell_ref, GLV_UNKNOWN);
  gnc_SetAllCellPortLogicVal(pq->nc, pq->cell_ref, GLV_UNKNOWN);

  gspq_ApplyCellXBMSimulationTransitionState(pq, n1);

  if ( gspq_CompareCellXBMStateForSimulation(pq, n1) == 0 )
    return 0;

  pq->is_calc_max_in_out_dly = 1;
  gspq_ApplyCellXBMSimulationTransition(pq, n2);

  if ( gspq_CompareCellXBMStateForSimulation(pq, n2) == 0 )
    return 0;


  return gspq_fm_time(pq, x, tr_pos, var_in_idx, n1, n2);
  
}

int gspq_GetFundamentalModeXBM(gspq_type pq, xbm_type x)
{
  int fb_cnt = gnc_GetGCELL(pq->nc, pq->cell_ref)->register_width;
  int state_out_width = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->out_cnt;
  int inputs = gnc_GetGCELL(pq->nc, pq->cell_ref)->pi->in_cnt;
  int i, j;
  double max = 0.0;

  gspq_ClearPool(pq, 2);

  if ( xbm_DoTransitions(x, xbm_fm_cb, pq) == 0 )
    return -1.0;

  for( i = 0; i < inputs-fb_cnt; i++ )
  {
    gnc_Log(pq->nc, 3, "Fundamental mode dly calc: "
      "Maximum settle time after input change on %8s is %6.4lg",
      xbm_GetVarNameStrByIndex(x, XBM_DIRECTION_IN, i),
      gspq_GetTime(pq, 2, i)
    );
    if ( max < gspq_GetTime(pq, 2, i) )
      max = gspq_GetTime(pq, 2, i);
  }

  gnc_Log(pq->nc, 3, "Fundamental mode dly calc: "
      "Overall settle time (fundamental mode delay) is       %6.4lg",  max);
      
  for( i = 0; i < state_out_width; i++ )
  {
    for( j = 0; j < inputs-fb_cnt; j++ )
    {
      /* if ( gspq_GetMTime(pq, j, i) != 0.0 ) */
      {
        gnc_Log(pq->nc, 3, "Fundamental mode dly calc: "
          "Required external delay for %8s --> %8s is  %6.4lg",
          gnc_GetCellPortName(pq->nc, pq->cell_ref, inputs + i),
          gnc_GetCellPortName(pq->nc, pq->cell_ref, j),
          gspq_GetMTime(pq, j, i)
        );
      }
    }
  }
  
  gnc_Log(pq->nc, 5, "Asynchronous FSM: Estimated (simulation) 'fundamental mode' delay is %lf.", 
    max );
  
  return 1;
}

double gnc_GetFundamentalModeXBM(gnc nc, int cell_ref, xbm_type x)
{
  double fundamental_mode = -1.0;
  gspq_type pq;
  pq = gspq_Open(nc, cell_ref);
  if ( pq != NULL )
  {
    /* pq->is_stop_at_fsm_input = 1; */

    fundamental_mode = gspq_GetFundamentalModeXBM(pq, x);
    gspq_Close(pq);
  }
  return fundamental_mode;
}

