/*

  SYGATE.C

  synthesis functions for elementary gates

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

#include "gnet.h"
#include <stdarg.h>
#include <assert.h>
#include "mwc.h"

/*---------------------------------------------------------------------------*/


int gnc_GetInvertedNet(gnc nc, int cell_ref, int net_ref, int is_generic)
{
  int join_ref;
  int node_ref;
  int port_ref;
  int id = GCELL_ID_INV;
  int inv_cell_ref;

  if ( is_generic != 0 )
  {
    id = GCELL_ID_G_INV;
    inv_cell_ref = gnc_GetGenericCell(nc, id, 1); /* create the cell */
  }
  else
  { 
    id = GCELL_ID_INV;
    inv_cell_ref = gnc_GetCellById(nc, id);
  }
  
  /* find an inverter gate */
  join_ref = gnc_FindCellNetCell(nc, cell_ref, net_ref, inv_cell_ref);
  if ( join_ref >= 0 )
  {
    /* get the node of the inverter */
    node_ref = gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);

    /* the output port of the inverter cell */  
    port_ref = gnc_FindCellPortByType(nc, inv_cell_ref, GPORT_TYPE_OUT);

    /* return the net which is driven by the inverter*/
    net_ref = gnc_FindCellNetByPort(nc, cell_ref, node_ref, port_ref);
    
    assert( gnc_FindCellNetJoinByDriver(nc, cell_ref, net_ref) >= 0 );
    
    return net_ref;
  }
  
  /* create a new inverter node */
  
  node_ref = gnc_AddCellNode(nc, cell_ref, NULL, inv_cell_ref);
  if ( node_ref < 0 )
    return -1;

  /* Add the input of the new invert-node to the net of the port */
  port_ref = gnc_FindCellPortByType(nc, inv_cell_ref, GPORT_TYPE_IN);
  if ( gnc_AddCellNetJoin(nc, cell_ref, net_ref, node_ref, port_ref, 0) < 0 )
    return -1;
  
  /* check the old net */

  assert( gnc_FindCellNetJoinByDriver(nc, cell_ref, net_ref) >= 0 );
  
  /* Create a new net, that is driven by the inverter */
  
  net_ref = gnc_AddCellNet(nc, cell_ref, NULL);

  /* Apply the net */
  
  port_ref = gnc_FindCellPortByType(nc, inv_cell_ref, GPORT_TYPE_OUT);
  
  if ( gnc_AddCellNetJoin(nc, cell_ref, net_ref, node_ref, port_ref, 0) < 0 )
    return -1;

  /* check the new net (the net after the invert-node) */

  assert( gnc_FindCellNetJoinByDriver(nc, cell_ref, net_ref) >= 0 );
  
  return net_ref;
  
}

/*
  assumes that each port of the cell has an associated net.
  If such a net does not exists, this function will create one.
  returns net_ref.
*/
int gnc_GetDigitalCellNetByPort(gnc nc, int cell_ref, int port_ref, int is_neg, int is_generic)
{
  int net_ref;

  net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, port_ref);
  if ( net_ref < 0 )
  {
    net_ref = gnc_AddCellNet(nc, cell_ref, NULL);
    if ( net_ref < 0 )
      return -1;
    if ( gnc_AddCellNetJoin(nc, cell_ref, net_ref, -1, port_ref, 0) < 0 )
      return -1;
  }

#ifndef NDEBUG
  if ( gnc_GetCellPortType(nc, cell_ref, port_ref) == GPORT_TYPE_IN )
  {
    assert( gnc_FindCellNetJoinByDriver(nc, cell_ref, net_ref) >= 0 );
  }
#endif

  if ( is_neg == 0 )
    return net_ref;

/*   this is wrong for state machines...
 *   if ( gnc_GetCellPortType(nc, cell_ref, port_ref) == GPORT_TYPE_OUT )
 *   {
 *     gnc_Error(nc, "gnc_GetDigitalCellNetByPort: Can not invert output port.");
 *     return -1;
 *   }
 */

  return gnc_GetInvertedNet(nc, cell_ref, net_ref, is_generic);
}


/*---------------------------------------------------------------------------*/

static int gnc_ConnectInputs(gnc nc, int cell_ref, b_il_type in_net_refs, int node_ref)
{
  int node_cell_ref = gnc_GetCellByNode(nc, cell_ref, node_ref);
  int port_ref = -1;
  int i = 0;
  int net_ref;
  
  while( gnc_LoopCellPort(nc, node_cell_ref, &port_ref) )
  {
    if ( gnc_GetCellPortType(nc, node_cell_ref, port_ref) == GPORT_TYPE_IN )
    {
      if ( b_il_GetCnt(in_net_refs) <= i )
        return 1;
      net_ref = b_il_GetVal(in_net_refs, i);
      if ( gnc_AddCellNetJoin(nc, cell_ref, net_ref, node_ref, port_ref, 0) < 0 )
      {
        gnc_Error(nc, "gnc_ConnectInputs: Failed.");
        return 0;
      }
      i++;
    }
  }
  
  return 1;
}

/* returns node_ref */
static int gnc_AddCellAndInputs(gnc nc, int cell_ref, int atomic_cell, b_il_type in_net_refs)
{
  int node_ref;
  node_ref = gnc_AddCellNode(nc, cell_ref, NULL, atomic_cell);
  if ( node_ref < 0 )
    return -1;
  if ( gnc_ConnectInputs(nc, cell_ref, in_net_refs, node_ref) == 0 )
    return -1;
  return node_ref;
}

/*
  digital_cell_ref: A new node is created from this cell
  in_net_refs: list of net's that are connected to the input ports of the new node
  out_net_ref: the (single) output of the cell is connected to this net.
               if ( out_net_ref < 0 ) a new net is created.
  
  returns out_net_ref
*/
int gnc_SynthDigitalCell(gnc nc, int cell_ref, int digital_cell_ref, b_il_type in_net_refs, int out_net_ref)
{
  int digital_node_ref;
  int out_digital_port_ref; /* output port of the digital - cell */
  
  digital_node_ref = 
    gnc_AddCellAndInputs(nc, cell_ref, digital_cell_ref, in_net_refs);
  if ( digital_node_ref < 0 )
    return -1;
  
  out_digital_port_ref = 
    gnc_FindCellPortByType(nc, digital_cell_ref, GPORT_TYPE_OUT);
  if ( out_digital_port_ref < 0 )
  {
    gnc_Error(nc, "gnc_SynthDigitalCell: Internal Error");
    return -1;  /* must never occur: all digital cells have a output port */
  }
  
  if ( out_net_ref < 0 )
    out_net_ref = gnc_AddCellNet(nc, cell_ref, NULL);
  if ( out_net_ref < 0 )
    return -1;
    
  if ( gnc_AddCellNetJoin(nc, cell_ref, out_net_ref, digital_node_ref, out_digital_port_ref, 0) < 0 )
    return -1;

  {
    int node_cell_ref;
    node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, digital_node_ref);
    gnc_Log(nc, 0, "Synthesis: Added digital cell '%s' (node %d).", 
      gnc_GetCellNameStr(nc, node_cell_ref),
      digital_node_ref);
  }

  return out_net_ref;
}

/* returns reference to output net */
int gnc_SynthGenericGate(gnc nc, int cell_ref, int g_id, b_il_type in_net_refs, int out_net_ref)
{
  int digital_cell_ref;
  
  if ( g_id == GCELL_ID_G_AND || g_id == GCELL_ID_G_OR )
  {
    if ( b_il_GetCnt(in_net_refs) == 1 && out_net_ref < 0 )
      return b_il_GetVal(in_net_refs, 0);
  }

  if ( g_id == GCELL_ID_G_AND || g_id == GCELL_ID_G_OR || GCELL_ID_G_INV)
  {
    if ( b_il_GetCnt(in_net_refs) == 0 )
    {
      gnc_Error(nc, "gnc_SynthGenericGate: Generic cell without inputs requested.");
      return -1;
    }
  }
  
  digital_cell_ref = gnc_GetGenericCell(nc, g_id, b_il_GetCnt(in_net_refs));
  if ( digital_cell_ref < 0 )
    return -1;
  return gnc_SynthDigitalCell(nc, cell_ref, digital_cell_ref, in_net_refs, out_net_ref);
}

/*---------------------------------------------------------------------------*/

int gnc_SynthGenericNodeToLib(gnc nc, int cell_ref, int node_ref)
{
  int node_cell_ref;
  int gate_type = -1;
  b_il_type net_refs;
  int org_out_net_ref;
  int synth_out_net_ref;
  int is_inv;
  
  node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
  if ( node_cell_ref < 0 )
  {
    gnc_Error(nc, "gnc_SynthGenericNodeToLib: Not a node.");
    return 0;
  }

  switch(gnc_GetCellId(nc, node_cell_ref))
  {
    case GCELL_ID_G_INV:
      is_inv = 1;
      break;
    case GCELL_ID_G_AND:
      is_inv = 0;
      gate_type = GNC_GATE_AND;
      break;
    case GCELL_ID_G_OR:
      is_inv = 0;
      gate_type = GNC_GATE_OR;
      break;
    case GCELL_ID_G_REGISTER:
      /* register replacement has its own function */
      return gnc_SynthGenericRegisterToLib(nc, cell_ref, node_ref); /* syfsm.c */
    case GCELL_ID_G_DOOR:
      /* door replacement has its own function */
      return gnc_SynthGenericDoorToLib(nc, cell_ref, node_ref); /* syfsm.c */
    default:
      return 1;
  }

  net_refs = b_il_Open();
  if ( net_refs == NULL )
  {
    gnc_Error(nc, "gnc_SynthGenericNodeToLib: Out of Memory (b_il_Open).");
    return 0;
  }
    
  
  org_out_net_ref = gnc_GetCellNodeFirstOutputNet(nc, cell_ref, node_ref);
  if ( org_out_net_ref < 0 )
  {
    gnc_Error(nc, "gnc_SynthGenericNodeToLib: No output port.");
    return b_il_Close(net_refs), 0;
  }
  
  if ( gnc_GetCellNodeInputNet(nc, cell_ref, node_ref, net_refs) == 0 )
    return b_il_Close(net_refs), 0;

  if ( b_il_GetCnt(net_refs) == 0 )
  {
    gnc_Error(nc, "gnc_SynthGenericNodeToLib: No input port.");
    return b_il_Close(net_refs), 0;
  }
    
  gnc_DelCellNode(nc, cell_ref, node_ref);
  
  if ( is_inv == 0 )
  {
    if ( gnc_SynthGate(nc, cell_ref, gate_type, net_refs, &synth_out_net_ref, NULL) == 0 )
      return b_il_Close(net_refs), 0;
    if ( gnc_MergeNets(nc, cell_ref, synth_out_net_ref, org_out_net_ref) == 0 )
      return b_il_Close(net_refs), 0;
  }
  else
  {
    int port_ref;
    
    assert(b_il_GetCnt(net_refs) == 1);
  
    node_ref = gnc_AddCellNode(nc, cell_ref, NULL, gnc_GetCellById(nc, GCELL_ID_INV));
    if ( node_ref < 0 )
      return b_il_Close(net_refs), 0;

    port_ref = gnc_FindCellPortByType(nc, gnc_GetCellById(nc, GCELL_ID_INV), GPORT_TYPE_IN);
    if ( gnc_AddCellNetJoin(nc, cell_ref, b_il_GetVal(net_refs, 0), node_ref, port_ref, 0) < 0 )
      return b_il_Close(net_refs), 0;

    port_ref = gnc_FindCellPortByType(nc, gnc_GetCellById(nc, GCELL_ID_INV), GPORT_TYPE_OUT);
    if ( gnc_AddCellNetJoin(nc, cell_ref, org_out_net_ref, node_ref, port_ref, 0) < 0 )
      return b_il_Close(net_refs), 0;
    
  }
  
  return b_il_Close(net_refs), 1;
}

int gnc_ReplaceEmptyNode(gnc nc, int cell_ref, int node_ref)
{
  int in_port_ref, out_port_ref;
  int in_net_ref, out_net_ref;
  int empty_cell_ref;
  
  empty_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
  if ( gnc_GetCellId(nc, empty_cell_ref) != GCELL_ID_EMPTY )
    return 0;
    
  in_port_ref = gnc_FindCellPortByType(nc, empty_cell_ref, GPORT_TYPE_IN);
  out_port_ref = gnc_FindCellPortByType(nc, empty_cell_ref, GPORT_TYPE_OUT);

  in_net_ref = gnc_FindCellNetByPort(nc, cell_ref, node_ref, in_port_ref);
  if ( in_net_ref < 0 )
  {
    gnc_Error(nc, "gnc_ReplaceEmptyNode: Input net of 'empty cell' does not exist.");
    return 0;
  }
  out_net_ref = gnc_FindCellNetByPort(nc, cell_ref, node_ref, out_port_ref);
  if ( out_net_ref < 0 )
  {
    gnc_Error(nc, "gnc_ReplaceEmptyNode: Output net of 'empty cell' does not exist.");
    return 0;
  }

  /* merge both nets, delete out_net_ref */
  if ( gnc_MergeNets(nc, cell_ref, in_net_ref, out_net_ref) == 0 )
    return 0;

  /* do a consistency check on in_net_ref */   
  assert( gnc_FindCellNetJoinByDriver(nc, cell_ref, in_net_ref) >= 0 );

  gnc_DelCellNode(nc, cell_ref, node_ref);

  return 1;
}

int gnc_SynthRemoveEmptyCells(gnc nc, int cell_ref)
{
  int node_ref;
  /* remove __empty__ cells */

  node_ref = -1;
  while(gnc_LoopCellNode(nc, cell_ref, &node_ref)!=0)
    if ( gnc_GetCellId(nc, gnc_GetCellNodeCell(nc, cell_ref, node_ref)) == GCELL_ID_EMPTY )
      if ( gnc_ReplaceEmptyNode(nc, cell_ref, node_ref) == 0 )
        return 0;

  return 1;  
}

int gnc_SynthGenericToLibExceptEmptyCells(gnc nc, int cell_ref)
{
  int node_ref;
  int node_cell_ref;
  
  /* do the synthesis */
  
  node_ref = -1;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
  {
    /* node_cell_ref is only required for the error message. */
    /* It is fetched here, because the node might be deleted. */
    node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
    if ( gnc_SynthGenericNodeToLib(nc, cell_ref, node_ref) == 0 )
    {
      gnc_Error(nc, "Synthesis: Can not convert node %d (%s).",
        node_ref, 
        gnc_GetCellNameStr(nc, node_cell_ref)
        );
      return 0;
    }
  }

  /* check if everything was done */
      
  node_ref = -1;
  while(gnc_LoopCellNode(nc, cell_ref, &node_ref)!=0)
  {
    if ( gnc_GetCellId(nc, gnc_GetCellNodeCell(nc, cell_ref, node_ref)) >= GCELL_ID_G_INV )
    {
      gnc_Error(nc, "generic CELL found (map failed)");
      return 0;
    }
  }
  assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);

  return 1;
}

int gnc_SynthGenericToLib(gnc nc, int cell_ref)
{
  if ( gnc_SynthGenericToLibExceptEmptyCells(nc, cell_ref) == 0 )
    return 0;
  if ( gnc_SynthRemoveEmptyCells(nc, cell_ref) == 0 )
    return 0;
  return 1;
    
}
