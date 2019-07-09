/*

  gnetlv.c
  
  logical values for gates and ports
  
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
#include "fsmtest.h"


/*---------------------------------------------------------------------------*/

void glvInit(glv_struct *glv)
{
  glv->logic_value = GLV_UNKNOWN;
  glv->logic_change_cnt = 0;
}

/* 1 if value has been changed */
int glvSetVal(glv_struct *glv, int val)
{
  if ( glv->logic_value == val )
    return 0;
  glv->logic_value = val;
  glv->logic_change_cnt++;
  return 1;
}

/*---------------------------------------------------------------------------*/


void gnc_ClrCellNodeRecalculate(gnc nc, int cell_ref, int node_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return;
  gnlGetGNODE(nl,node_ref)->recalculate = 0;
}

void gnc_ClrCellRecalculate(gnc nc, int cell_ref)
{
  int node_ref = -1;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
    gnc_ClrCellNodeRecalculate(nc, cell_ref, node_ref);
}


void gnc_SetCellNodeRecalculate(gnc nc, int cell_ref, int node_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return;
  gnlGetGNODE(nl,node_ref)->recalculate = 1;
}

int gnc_IsCellNodeRecalculate(gnc nc, int cell_ref, int node_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return 0;
  return gnlGetGNODE(nl,node_ref)->recalculate;
}

void gnc_CopyCellNodeRecalculateToDeltaCycle(gnc nc, int cell_ref, int node_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return;
  gnlGetGNODE(nl,node_ref)->delta_cycle = 
    gnlGetGNODE(nl,node_ref)->recalculate;
}

int gnc_IsCellNodeDeltaCycle(gnc nc, int cell_ref, int node_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return 0;
  return gnlGetGNODE(nl,node_ref)->delta_cycle;
}

void gnc_CopyCellRecalculateToDeltaCycle(gnc nc, int cell_ref, int *cntp)
{
  int node_ref = -1;
  int cnt = 0;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
  {
    if ( gnc_IsCellNodeRecalculate(nc, cell_ref, node_ref) != 0 )
      cnt++;
    gnc_CopyCellNodeRecalculateToDeltaCycle(nc, cell_ref, node_ref);
  }
  if ( cntp != NULL )
    *cntp = cnt;
}

glv_struct *gnc_GetCellNetLogicVal(gnc nc, int cell_ref, int net_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return NULL;
  return &(gnlGetGNET(nl, net_ref)->lv);
}

void gnc_SetAllCellNetLogicVal(gnc nc, int cell_ref, int val)
{
  int net_ref = -1;
  glv_struct *lv;
  
  while( gnc_LoopCellNet(nc, cell_ref, &net_ref) != 0 )
  {
    lv = gnc_GetCellNetLogicVal(nc, cell_ref, net_ref);
    lv->logic_value = val;
    lv->logic_change_cnt = 0;
  }
}

void gnc_SetAllCellPortLogicVal(gnc nc, int cell_ref, int val)
{
  int port_ref = -1;
  glv_struct *lv;
  
  while( gnc_LoopCellPort(nc, cell_ref, &port_ref) != 0 )
  {
    lv = gnc_GetCellPortLogicVal(nc, cell_ref, port_ref);
    lv->logic_value = val;
    lv->logic_change_cnt = 0;
  }
}

glv_struct *gnc_GetCellPortLogicVal(gnc nc, int cell_ref, int port_ref)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  return &(gcellGetGPORT(cell, port_ref)->lv);
}

static char _gnc_input_values[1024];

/* returns logical value GLV_.... */
static int gnc_CalcLogicValOR(gnc nc, int cell_ref, int node_ref)
{
  int port_ref = -1;
  int net_ref = -1;
  int join_ref = -1;
  int ret_val = GLV_ZERO;
  _gnc_input_values[0] = '\0';
  while( gnc_LoopCellNodeInputs(nc, cell_ref, node_ref, &port_ref, &net_ref, &join_ref) != 0 )
  {
    switch( gnc_GetCellNetLogicVal(nc, cell_ref, net_ref)->logic_value )
    {
      case GLV_UNKNOWN:
        sprintf(_gnc_input_values+strlen(_gnc_input_values), "%d/? ", net_ref);
        ret_val = GLV_UNKNOWN;
        break;
      case GLV_ONE:
        sprintf(_gnc_input_values+strlen(_gnc_input_values), "%d/1 ", net_ref);
        return GLV_ONE;
      default:
        sprintf(_gnc_input_values+strlen(_gnc_input_values), "%d/0 ", net_ref);
        break;
    }
  }
  return ret_val;
}

/* returns logical value GLV_.... */
static int gnc_CalcLogicValAND(gnc nc, int cell_ref, int node_ref)
{
  int port_ref = -1;
  int net_ref = -1;
  int join_ref = -1;
  int ret_val = GLV_ONE;
  _gnc_input_values[0] = '\0';
  while( gnc_LoopCellNodeInputs(nc, cell_ref, node_ref, &port_ref, &net_ref, &join_ref) != 0 )
  {
    switch( gnc_GetCellNetLogicVal(nc, cell_ref, net_ref)->logic_value )
    {
      case GLV_UNKNOWN:
        sprintf(_gnc_input_values+strlen(_gnc_input_values), "%d/? ", net_ref);
        ret_val = GLV_UNKNOWN;
        break;
      case GLV_ZERO:
        sprintf(_gnc_input_values+strlen(_gnc_input_values), "%d/0 ", net_ref);
        return GLV_ZERO;
      default:
        sprintf(_gnc_input_values+strlen(_gnc_input_values), "%d/1 ", net_ref);
        break;
    }
  }
  return ret_val;
}

/* 1 if value has been changed */
/* see also gspq_ProcessEvent */
static int gnc_SetCellNetLogicValue(gnc nc, int cell_ref, int net_ref, int val, int is_force)
{
  int join_ref;
  int node_ref;
  glv_struct *lv;
  int is_changed;
  
  lv = gnc_GetCellNetLogicVal(nc, cell_ref, net_ref);
  is_changed = glvSetVal(lv, val);

  if ( is_force == 0 )
    if( is_changed == 0 )
      return 0;
    
  join_ref = -1;
  while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
  {
    node_ref = gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
    /* ignore the parent cell */
    if ( node_ref >= 0 )
    {
      /* only with input ports ... */
      if ( gnc_GetCellNetPortType(nc, cell_ref, net_ref, join_ref) == GPORT_TYPE_IN )
      {
        gnc_SetCellNodeRecalculate(nc, cell_ref, node_ref);
      }
    }
  }
  
  return 1;
}

static int gnc_CalcLogicValDFF(gnc nc, int cell_ref, int node_ref)
{
  int node_cell_ref;

  int clk_port_ref;
  int clr_port_ref;
  int set_port_ref;
  int d_port_ref;
  
  int clk_inv = 0;
  int clr_inv = 0;
  int set_inv = 0;
  int d_inv = 0;
  
  int clk_net_ref = -1;
  int clr_net_ref = -1;
  int set_net_ref = -1;
  int d_net_ref   = -1;
  
  int clk_val = GLV_UNKNOWN;
  int clr_val = GLV_UNKNOWN;
  int set_val = GLV_UNKNOWN;
  int d_val   = GLV_UNKNOWN;
  
  int d1, d2;
  
  node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
  
  clk_port_ref = gnc_FindCellPortByFn(nc, node_cell_ref, GPORT_FN_CLK);
  if ( clk_port_ref >= 0 )
  {
    clk_inv = gnc_IsCellPortInverted(nc, node_cell_ref, clk_port_ref);
    clk_net_ref = gnc_FindCellNetByPort(nc, cell_ref, node_ref, clk_port_ref);
    clk_val = gnc_GetCellNetLogicVal(nc, cell_ref, clk_net_ref)->logic_value;
    /*
    gnc_Log(nc, 0, "Simulation: Clock port '%s' (%d%s) at net %d of cell '%s' has value '%c'.", 
      gnc_GetCellPortName(nc, node_cell_ref, clk_port_ref)==NULL?"":gnc_GetCellPortName(nc, node_cell_ref, clk_port_ref),
      clk_port_ref,
      clk_inv==0?"":", inverted",
      clk_net_ref, 
      gnc_GetCellName(nc, node_cell_ref)==NULL?"":gnc_GetCellName(nc, node_cell_ref),
      "?01"[clk_val]);
    */
    if ( clk_val != GLV_UNKNOWN && clk_inv != 0 )
      clk_val = (clk_val==GLV_ZERO)?GLV_ONE:GLV_ZERO;
  }
  
  clr_port_ref = gnc_FindCellPortByFn(nc, node_cell_ref, GPORT_FN_CLR);
  if ( clr_port_ref >= 0 )
  {
    clr_inv     = gnc_IsCellPortInverted(nc, node_cell_ref, clr_port_ref);
    clr_net_ref = gnc_FindCellNetByPort(nc, cell_ref, node_ref, clr_port_ref);
    if ( clr_net_ref >= 0 )
    {
      clr_val     = gnc_GetCellNetLogicVal(nc, cell_ref, clr_net_ref)->logic_value;
      if ( clr_val != GLV_UNKNOWN && clr_inv != 0 )
        clr_val = (clr_val==GLV_ZERO)?GLV_ONE:GLV_ZERO;
    }
  }
  
  set_port_ref = gnc_FindCellPortByFn(nc, node_cell_ref, GPORT_FN_SET);
  if ( set_port_ref >= 0 )
  {
    set_inv     = gnc_IsCellPortInverted(nc, node_cell_ref, set_port_ref);
    set_net_ref = gnc_FindCellNetByPort(nc, cell_ref, node_ref, set_port_ref);
    if ( set_net_ref >= 0 )
    {
      set_val     = gnc_GetCellNetLogicVal(nc, cell_ref, set_net_ref)->logic_value;
      if ( set_val != GLV_UNKNOWN && set_inv != 0 )
        set_val = (set_val==GLV_ZERO)?GLV_ONE:GLV_ZERO;
    }
  }
  
  d_port_ref   = gnc_FindCellPortByFn(nc, node_cell_ref, GPORT_FN_D);
  if ( d_port_ref >= 0 )
  {
    d_inv       = gnc_IsCellPortInverted(nc, node_cell_ref, d_port_ref);
    d_net_ref   = gnc_FindCellNetByPort(nc, cell_ref, node_ref, d_port_ref);
    if ( d_net_ref >= 0 )
    {
      d_val       = gnc_GetCellNetLogicVal(nc, cell_ref, d_net_ref)->logic_value;
      if ( d_val != GLV_UNKNOWN && d_inv != 0 )
        d_val = (d_val==GLV_ZERO)?GLV_ONE:GLV_ZERO;
    }
  }

  
  d1 = gnc_GetCellGNODE(nc, cell_ref, node_ref)->data & 1;
  d2 = (gnc_GetCellGNODE(nc, cell_ref, node_ref)->data & 2)>>1;
  
  if ( d_port_ref < 0 || clk_port_ref < 0 )
  {
    gnc_Error(nc, "gnc_CalcLogicValDFF: Not a valid DFF (node %d).", node_ref);
    return -1;
  }
  
  if ( clk_val == GLV_ZERO )
  {
    d1 = (d_val == GLV_ZERO)?0:1;
    sprintf(_gnc_input_values, "d-port %d/%d", d_net_ref, (d_val == GLV_ZERO)?0:1);
    gnc_Log(nc, 0, "Simulation: Node '%s' (cell '%s') new internal value %d.",
      gnc_GetCellNodeName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeName(nc, cell_ref, node_ref),
      gnc_GetCellNodeCellName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeCellName(nc, cell_ref, node_ref),
      d1 );
  }
  if ( clk_val == GLV_ONE )
  {
    d2 = d1;
    sprintf(_gnc_input_values, "internal value %d", d2);
    gnc_Log(nc, 0, "Simulation: Node '%s' (cell '%s') new output value %d.",
      gnc_GetCellNodeName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeName(nc, cell_ref, node_ref),
      gnc_GetCellNodeCellName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeCellName(nc, cell_ref, node_ref),
      d2 );
  }
  if ( set_val == GLV_ONE )
  {
    sprintf(_gnc_input_values, "reset: set %d", set_net_ref);
    gnc_Log(nc, 0, "Simulation: Node '%s' (cell '%s') FF set.",
      gnc_GetCellNodeName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeName(nc, cell_ref, node_ref),
      gnc_GetCellNodeCellName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeCellName(nc, cell_ref, node_ref));
    d1 = 1;
    d2 = 1;
  }
  if ( clr_val == GLV_ONE )
  {
    sprintf(_gnc_input_values, "reset: clr %d", clr_net_ref);
    gnc_Log(nc, 0, "Simulation: Node '%s' (cell '%s') FF reset.",
      gnc_GetCellNodeName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeName(nc, cell_ref, node_ref),
      gnc_GetCellNodeCellName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeCellName(nc, cell_ref, node_ref));
    d1 = 0;
    d2 = 0;
  }
  
  gnc_GetCellGNODE(nc, cell_ref, node_ref)->data = (d2<<1)|d1;
  
  return (d2==0)?GLV_ZERO:GLV_ONE;
}

int gnc_CalculateCellNodeLogicVal(gnc nc, int cell_ref, int node_ref)
{
  int id;
  int val;
  int node_cell_ref;

  node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
  id = gnc_GetCellId(nc, node_cell_ref);
  if ( id >= GCELL_ID_AND2 && id < GCELL_ID_DFF_Q )
  {
    switch(id&3)
    {
      case 0: /* AND */
        val = gnc_CalcLogicValAND(nc, cell_ref, node_ref);
        break;
      case 1: /* NAND */
        val = gnc_CalcLogicValAND(nc, cell_ref, node_ref);
        if ( val == GLV_ZERO )
          val = GLV_ONE;
        else if ( val == GLV_ONE )
          val = GLV_ZERO;
        break;
      case 2: /* OR */
        val = gnc_CalcLogicValOR(nc, cell_ref, node_ref);
        break;
      case 3: /* NOR */
        val = gnc_CalcLogicValOR(nc, cell_ref, node_ref);
        if ( val == GLV_ZERO )
          val = GLV_ONE;
        else if ( val == GLV_ONE )
          val = GLV_ZERO;
        break;
    }
  }
  else if ( id == GCELL_ID_INV )
  {
    val = gnc_CalcLogicValOR(nc, cell_ref, node_ref);
    if ( val == GLV_ZERO )
      val = GLV_ONE;
    else if ( val == GLV_ONE )
      val = GLV_ZERO;
  }
  else if ( id >= GCELL_ID_DFF_Q && id <= GCELL_ID_DFF_Q_NQ_HS )
  {
    val = gnc_CalcLogicValDFF(nc, cell_ref, node_ref);
  }
  else if ( id == GCELL_ID_EMPTY )
  {
    val = gnc_CalcLogicValOR(nc, cell_ref, node_ref);
  }
  else
  {
    /* ignore other nodes... */
    gnc_Log(nc, 3, "Simulation: Node '%s' (cell '%s', id %d) is unknown.",
      gnc_GetCellNodeName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeName(nc, cell_ref, node_ref),
      gnc_GetCellNodeCellName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeCellName(nc, cell_ref, node_ref),
      id
    );
    return GLV_UNKNOWN;
  }

  
  gnc_Log(nc, 0, "Simulation: Node '%s' (cell '%s') has value '%c' (inputs '%s').",
    gnc_GetCellNodeName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeName(nc, cell_ref, node_ref),
    gnc_GetCellNodeCellName(nc, cell_ref, node_ref)==NULL?"":gnc_GetCellNodeCellName(nc, cell_ref, node_ref),
    "?01"[val],
    _gnc_input_values );

  return val;  
}

static int gnc_RecalculateCellNode(gnc nc, int cell_ref, int node_ref)
{
  int net_ref;
  int is_something_done = 0;
  int node_cell_ref;
  int id;
  int val;

  /* yes, the node will be recalculated, so clear the flag */

  gnc_ClrCellNodeRecalculate(nc, cell_ref, node_ref);

  
  /* calculate the new output of the node */
  val = gnc_CalculateCellNodeLogicVal(nc, cell_ref, node_ref);
  
  /* if value has changed, inform other nodes and set is_something_done = 1 */
  /* TODO: CONSIDER DFF with Q and QN ports !!! */
  net_ref = gnc_GetCellNodeFirstOutputNet(nc, cell_ref, node_ref);
  if ( net_ref >= 0 )
  {
    
    is_something_done = gnc_SetCellNetLogicValue(nc, cell_ref, net_ref, val, 0);
  }
  
  return is_something_done;

}

/* returns 1, if another delta cycle is required */
static int gnc_DoDeltaCycle(gnc nc, int cell_ref)
{
  int node_ref;
  int is_something_done = 0;
  int cnt;
  /* all nodes, that should be recalculated, must be processed */
  /* this is handled by the delta_cycle flags */

  gnc_CopyCellRecalculateToDeltaCycle(nc, cell_ref, &cnt);

  gnc_Log(nc, 0, "Simulation: Start of delta cycle (recalculate %d node%s).",
    cnt, cnt==1?"":"s");

  /* recalculate a node, if its delta_cycle flag is set */  
  /* this procedure will also make new assignments to the */
  /* recalculate flag */
  node_ref = -1;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
    if ( gnc_IsCellNodeDeltaCycle(nc, cell_ref, node_ref) != 0 )
      is_something_done |= gnc_RecalculateCellNode(nc, cell_ref, node_ref);
    
  return is_something_done;
}

/* precondition: some recalculate flags are set */
static int gnc_DoDeltaCycleLoop(gnc nc, int cell_ref)
{
  int loop = 0;

  gnc_Log(nc, 0, "Simulation: Start of simulation step.");

  while( gnc_DoDeltaCycle(nc, cell_ref) != 0 && loop < nc->simulation_loop_abort)
    loop++;
  if ( loop == nc->simulation_loop_abort )
  {
    gnc_Error(nc, "gnc_DoSimulate: Maximum loops (%d) reached.", nc->simulation_loop_abort);
    return 0;
  }

  gnc_Log(nc, 0, "Simulation: End of simulation step (loops: %d).", loop);
  
  return 1;
}

/* returns 1, if something has been changed */
int gnc_UseInputPortValues(gnc nc, int cell_ref)
{
  int net_ref;
  int port_ref;
  int val;
  int is_changed = 0;
  
  port_ref = -1;
  while( gnc_LoopCellPort(nc, cell_ref, &port_ref) == 0 )
  {
    if ( gnc_GetCellPortType(nc, cell_ref, port_ref) == GPORT_TYPE_IN )
    {
      val = gnc_GetCellPortLogicVal(nc, cell_ref, port_ref)->logic_value;
      if ( val != GLV_UNKNOWN )
      {
        net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, port_ref);
        if ( net_ref >= 0 )
        {
          is_changed |= gnc_SetCellNetLogicValue(nc, cell_ref, net_ref, val, 0);
        }
      }
      
      gnc_Log(nc, 0, "Simulation: Port '%s' has value '%c'.",
        gnc_GetCellPortName(nc, cell_ref, port_ref)==NULL?"":
          gnc_GetCellPortName(nc, cell_ref, port_ref),
        "?01"[val] );
    }
  }
  return is_changed;
}

void gnc_FillOutputPortValues(gnc nc, int cell_ref)
{
  int net_ref;
  int port_ref;
  glv_struct *src_lv;
  glv_struct *dest_lv;

  port_ref = -1;
  while( gnc_LoopCellPort(nc, cell_ref, &port_ref) != 0 )
  {
    if ( gnc_GetCellPortType(nc, cell_ref, port_ref) == GPORT_TYPE_OUT )
    {
      net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, port_ref);
      if ( net_ref >= 0 )
      {
        src_lv = gnc_GetCellNetLogicVal(nc, cell_ref, net_ref);
        dest_lv = gnc_GetCellPortLogicVal(nc, cell_ref, port_ref);
        *dest_lv = *src_lv;
      }
    }
  }
}

void gnc_PrepareLogicSimulation(gnc nc, int cell_ref)
{
  int net_ref, node_ref;
  net_ref = -1;
  while( gnc_LoopCellNet(nc, cell_ref, &net_ref) != 0 )
    glvInit(gnc_GetCellNetLogicVal(nc, cell_ref, net_ref));
  node_ref = -1;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
    gnc_GetCellGNODE(nc, cell_ref, node_ref)->data = 0;
}

void gnc_ClearLogicSimulationChangeCnt(gnc nc, int cell_ref)
{
  int net_ref;
  net_ref = -1;
  while( gnc_LoopCellNet(nc, cell_ref, &net_ref) != 0 )
    gnc_GetCellNetLogicVal(nc, cell_ref, net_ref)->logic_change_cnt = 0;
}

/* 
  return the number of nets that changed since the last call to 
  gnc_ClearLogicSimulationChangeCnt
*/
int gnc_GetLogicSimulationNetChangeCnt(gnc nc, int cell_ref)
{
  int net_ref;
  int cnt = 0;
  net_ref = -1;
  while( gnc_LoopCellNet(nc, cell_ref, &net_ref) != 0 )
    if ( gnc_GetCellNetLogicVal(nc, cell_ref, net_ref)->logic_change_cnt != 0 )
      cnt++;
  return cnt;
}

double gnc_GetLogicSimulationNetLoadChange(gnc nc, int cell_ref)
{
  int net_ref;
  double cap = 0.0;
  net_ref = -1;
  while( gnc_LoopCellNet(nc, cell_ref, &net_ref) != 0 )
    if ( gnc_GetCellNetLogicVal(nc, cell_ref, net_ref)->logic_change_cnt != 0 )
      cap += gnc_GetCellNetInputLoad(nc, cell_ref, net_ref);
  return cap;
}


int gnc_DoLogicSimulation(gnc nc, int cell_ref)
{
  /* clear recalculate flags */
  gnc_ClrCellRecalculate(nc, cell_ref);
  
  /* use the logic values of the input ports, set some recalculate flags */
  if ( gnc_UseInputPortValues(nc, cell_ref) == 0 )
    return 1;   /* nothing changed */
    
  /* loop until all recalculate flags are cleared */
  if ( gnc_DoDeltaCycleLoop(nc, cell_ref) == 0 )
    return 0;   /* error occured */
  
  /* fill output values with the new values of the net */
  gnc_FillOutputPortValues(nc, cell_ref);
  return 1;
}

/* returns 1, if something has been changed */
static int gnc_UseFSMPortValues(gnc nc, int cell_ref)
{
  int net_ref;
  int port_ref;
  int val;
  int is_changed = 0;
  
  int register_width = gnc_GetGCELL(nc, cell_ref)->register_width; 
  int inputs = gnc_GetGCELL(nc, cell_ref)->pi->in_cnt; 

  port_ref = -1;
  while( gnc_LoopCellPort(nc, cell_ref, &port_ref) != 0 )
  {
    /*
    if ( gnc_GetCellPortType(nc, cell_ref, port_ref) == GPORT_TYPE_IN || 
         port_ref < inputs+register_width)
    */
    if ( gnc_GetCellPortType(nc, cell_ref, port_ref) == GPORT_TYPE_IN )
    {
      val = gnc_GetCellPortLogicVal(nc, cell_ref, port_ref)->logic_value;
      if ( val != GLV_UNKNOWN )
      {
        net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, port_ref);
        if ( net_ref >= 0 )
        {
          is_changed |= gnc_SetCellNetLogicValue(nc, cell_ref, net_ref, val, 0);
        }
        gnc_Log(nc, 0, "Simulation: Port '%s' has value '%c' (net %d).",
          gnc_GetCellPortName(nc, cell_ref, port_ref)==NULL?"":
            gnc_GetCellPortName(nc, cell_ref, port_ref),
          "?01"[val], net_ref );
      }
      else
      {
        gnc_Log(nc, 0, "Simulation: Port '%s' ignored (unknown value).",
          gnc_GetCellPortName(nc, cell_ref, port_ref)==NULL?"":
            gnc_GetCellPortName(nc, cell_ref, port_ref));
      }
    }
    else
    {
      gnc_Log(nc, 0, "Simulation: Port '%s' ignored (current value '%c').",
        gnc_GetCellPortName(nc, cell_ref, port_ref)==NULL?"":
          gnc_GetCellPortName(nc, cell_ref, port_ref),
          "?01"[gnc_GetCellPortLogicVal(nc, cell_ref, port_ref)->logic_value]
          );
    }
  }
  return is_changed;
}

/*
  If the simulation is used for the capacitance calculation, a full
  simulation is not required. Instead one can force several nets
  (the feedback lines of the state machine) to a known value and 
  avoid some additional simulation steps.
  Well... indeed this avoids simulation of flip-flops.

  returns 0 for error  
*/
int gnc_DoFSMLogicSimulation(gnc nc, int cell_ref)
{
  /* clear recalculate flags */
  gnc_ClrCellRecalculate(nc, cell_ref);
  
  /* use the logic values, set some recalculate flags */
  if ( gnc_UseFSMPortValues(nc, cell_ref) == 0 )
    return 1;   /* nothing changed */
    
  /* loop until all recalculate flags are cleared */
  if ( gnc_DoDeltaCycleLoop(nc, cell_ref) == 0 )
    return 0;   /* error occured */
  
  /* fill output values with the new values of the net */
  gnc_FillOutputPortValues(nc, cell_ref);
  return 1;
}

void gnc_ApplyCellFSMInputForSimulation(gnc nc, int cell_ref, pinfo *pi, dcube *c)
{
  int inputs = gnc_GetGCELL(nc, cell_ref)->pi->in_cnt;
  int port_ref;
  int val;
  for( port_ref = 0; port_ref < inputs && port_ref < pi->in_cnt; port_ref++ )
  {
    switch(dcGetIn(c, port_ref))
    {
      case 1:
        val = GLV_ZERO;
        break;
      case 2:
        val = GLV_ONE;
        break;
      default:
        val = GLV_UNKNOWN;
        break;
    }
    
    /* maybe we should consider inverted inputs.... */
    
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, port_ref), val);
  }
}

/* 
  assumes gnc_GetGCELL(nc, cell_ref)->pi for cube c
  handles gnc_GetGCELL(nc, cell_ref)->register_width 
  probably this function can be used for sequential and combinational circuits
*/
void gnc_ApplyCellInputForSimulation(gnc nc, int cell_ref, dcube *c)
{
  int inputs = gnc_GetGCELL(nc, cell_ref)->pi->in_cnt;
  int fb_cnt = gnc_GetGCELL(nc, cell_ref)->register_width;
  int port_ref;
  int val;
  assert(inputs >= fb_cnt);
  for( port_ref = 0; port_ref < inputs-fb_cnt; port_ref++ )
  {
    switch(dcGetIn(c, port_ref))
    {
      case 1:
        val = GLV_ZERO;
        break;
      case 2:
        val = GLV_ONE;
        break;
      default:
        val = GLV_UNKNOWN;
        break;
    }
    
    /* maybe we should consider inverted inputs.... */
    
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, port_ref), val);
  }
}

void gnc_ApplyCellFSMStateForSimulation(gnc nc, int cell_ref, pinfo *pi, dcube *c)
{
  int inputs = gnc_GetGCELL(nc, cell_ref)->pi->in_cnt;
  int register_width = gnc_GetGCELL(nc, cell_ref)->register_width;
  int port_ref;
  int val;
  int inv_val;
  int i;
  for( port_ref = inputs, i = 0; port_ref < inputs+register_width ; port_ref++, i++ )
  {
    switch(dcGetOut(c, i))
    {
      case 0:
        val = GLV_ZERO;
        inv_val = GLV_ONE;
        break;
      case 1:
        val = GLV_ONE;
        inv_val = GLV_ZERO;
        break;
      default:
        val = GLV_UNKNOWN;
        inv_val = GLV_UNKNOWN;
        break;
    }
    if ( gnc_IsCellPortInverted(nc, cell_ref, port_ref) == 0 )
      glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, port_ref), val);
    else
      glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, port_ref), inv_val);
      
    if ( gnc_IsCellPortInverted(nc, cell_ref, port_ref-register_width) == 0 )
      glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, port_ref-register_width), val);
    else
      glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, port_ref-register_width), val);
  }
}

/* 
  assumes gnc_GetGCELL(nc, cell_ref)->pi for cube c
  handles gnc_GetGCELL(nc, cell_ref)->register_width 
  probably this function can be used for sequential and combinational circuits
*/
void gnc_ApplyCellStateForSimulation(gnc nc, int cell_ref, dcube *c)
{
  int inputs = gnc_GetGCELL(nc, cell_ref)->pi->in_cnt;
  int fb_cnt = gnc_GetGCELL(nc, cell_ref)->register_width;
  int port_ref;
  int val;
  int inv_val;
  int i;
  for( port_ref = inputs, i = 0; port_ref < inputs+fb_cnt ; port_ref++, i++ )
  {
    switch(dcGetOut(c, i))
    {
      case 0:
        val = GLV_ZERO;
        inv_val = GLV_ONE;
        assert(dcGetIn(c, port_ref-fb_cnt) == 1);
        break;
      case 1:
        val = GLV_ONE;
        inv_val = GLV_ZERO;
        assert(dcGetIn(c, port_ref-fb_cnt) == 2);
        break;
      default:
        val = GLV_UNKNOWN;
        inv_val = GLV_UNKNOWN;
        break;
    }
    if ( gnc_IsCellPortInverted(nc, cell_ref, port_ref) == 0 )
      glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, port_ref), val);
    else
      glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, port_ref), inv_val);
      
    if ( gnc_IsCellPortInverted(nc, cell_ref, port_ref-fb_cnt) == 0 )
      glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, port_ref-fb_cnt), val);
    else
      glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, port_ref-fb_cnt), val);
  }
}

int gnc_CompareCellFSMStateForSimulation(gnc nc, int cell_ref, pinfo *pi, dcube *c)
{
  int inputs = gnc_GetGCELL(nc, cell_ref)->pi->in_cnt;
  int register_width = gnc_GetGCELL(nc, cell_ref)->register_width;
  int port_ref;
  int val;
  int i;

  if ( 0 >= nc->log_level )
  {
    static char s[100];
    int total = gnc_GetGCELL(nc, cell_ref)->pi->in_cnt
                 + gnc_GetGCELL(nc, cell_ref)->pi->out_cnt;
    for( i = 0; i < total && i < 100-1; i++ )
    {
      switch( gnc_GetCellPortLogicVal(nc, cell_ref, i)->logic_value )
      {
        case GLV_ZERO: s[i] = '0'; break;
        case GLV_ONE: s[i] = '1'; break;
        default: s[i] = '?'; break;
      }
    }
    s[i] = '\0';
    gnc_Log(nc, 1, "Simulation: Current port values are '%s'.", s);
  }

  for( port_ref = inputs-register_width, i = 0; port_ref < inputs ; port_ref++, i++ )
  {
    switch(dcGetOut(c, i))
    {
      case 0:
        val = GLV_ZERO;
        break;
      case 1:
        val = GLV_ONE;
        break;
      default:
        val = GLV_UNKNOWN;
        break;
    }
    if ( gnc_GetCellPortLogicVal(nc, cell_ref, port_ref)->logic_value != val )
    {
      gnc_Error(nc, "gnc_CompareCellFSMStateForSimulation: Simulation result wrong "
        "(port '%s', current '%c', expected '%c', inverted %d).",
        gnc_GetCellPortName(nc, cell_ref, port_ref)==NULL?"":gnc_GetCellPortName(nc, cell_ref, port_ref),
        "?01"[gnc_GetCellPortLogicVal(nc, cell_ref, port_ref)->logic_value],
        "?01"[val],
        gnc_IsCellPortInverted(nc, cell_ref, port_ref));
      return 0;
    }
  }
  gnc_Log(nc, 1, "Simulation: Simulated state valid (%s).", dcOutToStr(pi, c, ""));
  return 1;
}

/*---------------------------------------------------------------------------*/

struct _ctc_struct
{
  gnc nc;
  int cell_ref;
};
typedef struct _ctc_struct ctc_struct;

void gnc_ApplyCellFSMSimulationResetState(gnc nc, int cell_ref, pinfo *pi_in, dcube *c_in, pinfo *pi_z, dcube *c_z)
{
  int clr_port = -1;
  int clk_port = -1;
  int clr_inverted = 0;
  int clk_inverted = 0;
  int clr_val = GLV_ONE;
  int clr_norm_val = GLV_ZERO;
  gnc_ApplyCellFSMInputForSimulation(nc, cell_ref, pi_in, c_in);
  gnc_ApplyCellFSMStateForSimulation(nc, cell_ref, pi_z, c_z);

  /* clr means reset input */
  clr_port = gnc_FindCellPortByFn(nc, cell_ref, GPORT_FN_CLR);
  clk_port = gnc_FindCellPortByFn(nc, cell_ref, GPORT_FN_CLK);
  if ( clr_port >= 0 )
    clr_inverted = gnc_IsCellPortInverted(nc, cell_ref, clr_port);
  if ( clk_port >= 0 )
    clk_inverted = gnc_IsCellPortInverted(nc, cell_ref, clk_port);
  if ( clr_port >= 0 )
    if ( clr_inverted != 0 )
    {
      clr_val = GLV_ZERO;
      clr_norm_val = GLV_ONE;
    }
  if ( clr_port >= 0 )
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clr_port), clr_val);
  if ( clk_port >= 0 )
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clk_port), GLV_ZERO);
    
  gnc_DoFSMLogicSimulation(nc, cell_ref);
  if ( clk_port >= 0 )
  {
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clk_port), GLV_ONE);
    gnc_DoFSMLogicSimulation(nc, cell_ref);
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clk_port), GLV_ZERO);
    gnc_DoFSMLogicSimulation(nc, cell_ref);
  }
  if ( clr_port >= 0 )
  {
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clr_port), clr_norm_val);
    gnc_DoFSMLogicSimulation(nc, cell_ref);
    if ( clk_port >= 0 )
    {
      glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clk_port), GLV_ONE);
      gnc_DoFSMLogicSimulation(nc, cell_ref);
      glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clk_port), GLV_ZERO);
      gnc_DoFSMLogicSimulation(nc, cell_ref);
    }
  }  
}

void gnc_ApplyCellFSMSimulationTransition(gnc nc, int cell_ref, pinfo *pi_in, dcube *c_in, pinfo *pi_z, dcube *c_z, double *cap, double *ff_cap_ptr)
{
  int clr_port = -1;
  int clk_port = -1;
  int clr_inverted = 0;
  int clk_inverted = 0;
  int clr_val = GLV_ONE;
  int clr_norm_val = GLV_ZERO;
  int net_changes;
  double net_cap;
  double ff_cap = 0.0;
  gnc_ApplyCellFSMInputForSimulation(nc, cell_ref, pi_in, c_in);
  gnc_ApplyCellFSMStateForSimulation(nc, cell_ref, pi_z, c_z);

  /* clr means reset input */
  clr_port = gnc_FindCellPortByFn(nc, cell_ref, GPORT_FN_CLR);
  clk_port = gnc_FindCellPortByFn(nc, cell_ref, GPORT_FN_CLK);
  if ( clr_port >= 0 )
    clr_inverted = gnc_IsCellPortInverted(nc, cell_ref, clr_port);
  if ( clk_port >= 0 )
    clk_inverted = gnc_IsCellPortInverted(nc, cell_ref, clk_port);
  if ( clr_port >= 0 )
    if ( clr_inverted != 0 )
    {
      clr_val = GLV_ZERO;
      clr_norm_val = GLV_ONE;
    }
  if ( clr_port >= 0 )
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clr_port), clr_norm_val);
  if ( clk_port >= 0 )
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clk_port), GLV_ZERO);
    
  gnc_ClearLogicSimulationChangeCnt(nc, cell_ref);
  gnc_DoFSMLogicSimulation(nc, cell_ref);

  net_changes = gnc_GetLogicSimulationNetChangeCnt(nc, cell_ref);
  net_cap = gnc_GetLogicSimulationNetLoadChange(nc, cell_ref);
  if ( clk_port >= 0 )
  {
    gnc_ClearLogicSimulationChangeCnt(nc, cell_ref);
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clk_port), GLV_ONE);
    gnc_DoFSMLogicSimulation(nc, cell_ref);
    ff_cap += gnc_GetLogicSimulationNetLoadChange(nc, cell_ref);

    gnc_ClearLogicSimulationChangeCnt(nc, cell_ref);
    glvSetVal(gnc_GetCellPortLogicVal(nc, cell_ref, clk_port), GLV_ZERO);
    gnc_DoFSMLogicSimulation(nc, cell_ref);
    ff_cap += gnc_GetLogicSimulationNetLoadChange(nc, cell_ref);
  }

  gnc_Log(nc, 3, "Simulation: Net changes: %d, charge/dischage capacitance: %lg (= %lg + %lg).",
    net_changes, net_cap+ff_cap, net_cap, ff_cap);

  if ( cap != NULL )
    *cap = net_cap+ff_cap;
    
  if ( ff_cap_ptr != NULL )
    *ff_cap_ptr = ff_cap;
}


int ctc_device(void *data, fsm_type fsm, int msg, int arg, dcube *c)
{
  int src_id;
  int dest_id;
  ctc_struct *ctc = (ctc_struct *)data;
  int cell_ref = ctc->cell_ref;
  gnc nc = ctc->nc;
  static int cap_cnt, ff_cap_cnt;
  static double cap, cap_sum;
  static double ff_cap, ff_cap_sum;
  
  switch(msg)
  {
    case FSM_TEST_MSG_RESET:
      /* printf("reset %s\n", fsm_GetNodeNameStr(fsm, arg)); */
      if ( c == NULL )
        return 0;
      gnc_ApplyCellFSMSimulationResetState(nc, cell_ref, 
        fsm_GetConditionPINFO(fsm), 
        c, 
        fsm_GetCodePINFO(fsm), 
        fsm_GetNodeCode(fsm, arg)
        );
      if ( gnc_CompareCellFSMStateForSimulation(nc, cell_ref, 
        fsm_GetCodePINFO(fsm), fsm_GetNodeCode(fsm, arg)) == 0 )
        return 0;
      break;
    case FSM_TEST_MSG_GO_EDGE:
    case FSM_TEST_MSG_DO_EDGE:
      src_id = fsm_GetEdgeSrcNode(fsm, arg);
      dest_id = fsm_GetEdgeDestNode(fsm, arg);
      gnc_Log(nc, 3, "Simulation: Transition %s (%d) -> %s (%d).",
        fsm_GetNodeNameStr(fsm, src_id), src_id, 
        fsm_GetNodeNameStr(fsm, dest_id), dest_id);
      gnc_ApplyCellFSMSimulationTransition(nc, cell_ref, 
        fsm_GetConditionPINFO(fsm), 
        c, 
        fsm_GetCodePINFO(fsm), 
        fsm_GetNodeCode(fsm, src_id),
        &cap,
        &ff_cap
      );
      if ( msg == FSM_TEST_MSG_DO_EDGE && src_id != dest_id )
      {
        cap_sum += cap;
        cap_cnt++;
      }
      if ( msg == FSM_TEST_MSG_DO_EDGE )
      {
        ff_cap_sum += ff_cap;
        ff_cap_cnt++;
      }
      if ( gnc_CompareCellFSMStateForSimulation(nc, cell_ref, 
        fsm_GetCodePINFO(fsm), fsm_GetNodeCode(fsm, dest_id)) == 0 )
        return 0;
      break;
    case FSM_TEST_MSG_START:
      gnc_PrepareLogicSimulation(nc, cell_ref);
      gnc_Log(nc, 2, "Simulation: Transition walk started.");
      cap_cnt = 0;
      cap_sum = 0.0;
      break;
    case FSM_TEST_MSG_END:
      gnc_Log(nc, 2, "Simulation: Transition walk finished.");
      if ( cap_cnt != 0 )
        gnc_Log(nc, 6, "Analysis: Average charge/dischage capacitance per transition is %lg (%d transitions).", 
          cap_sum/(double)cap_cnt, cap_cnt);
      if ( ff_cap_cnt != 0 )
        gnc_Log(nc, 6, "Analysis: Average charge/dischage capacitance of FF per transition is %lg (%d transitions).", 
          ff_cap_sum/(double)ff_cap_cnt, ff_cap_cnt);
      break;
    
  }
  return 1;

}

static void gnetlv_fsm_log_fn(void *data, int ll, char *fmt, va_list va)
{
  /*
  gnc_LogVA((gnc)data, 2, fmt, va);
  */
}

/* precondition: cell_ref contains a netlist of fsm */
int gnc_CalculateTransitionCapacitance(gnc nc, int cell_ref, fsm_type fsm)
{
  ctc_struct ctc;
  fsmtl_type tl = fsmtl_Open(fsm);
  if ( tl == NULL )
    return 0;

  gnc_Log(nc, 4, "Analysis: Power/Capacitance.");
  
  if ( fsm->reset_node_id < 0 )
  {
    gnc_Error(nc, "Analysis: Missing reset state (abort).");
    return fsmtl_Close(tl), 0;
  }

  fsm_PushLogFn(fsm, gnetlv_fsm_log_fn, nc, 2);
  
  
  if ( fsmtl_AddAllEdgeSequence(tl) == 0 )
  {
    gnc_Error(nc, "Analysis: Power/Capacitance: sequence calculation failed.");
    return fsmtl_Close(tl), fsm_PopLogFn(fsm), 0;
  }

  ctc.nc = nc;
  ctc.cell_ref = cell_ref;
  
  fsmtl_SetDevice(tl, ctc_device, &ctc);
  if ( fsmtl_Do(tl) == 0 )
  {
    gnc_Error(nc, "Analysis: Power/Capacitance: simulation failed.");
    return fsmtl_Close(tl), fsm_PopLogFn(fsm), 1;
  }
  
  return fsmtl_Close(tl), fsm_PopLogFn(fsm), 1;
}

