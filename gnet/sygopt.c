/*

  sygopt.c
  
  synthesis: generic optimiziation
  
  These optimizations are only performed on generic elements

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
#include <assert.h>
#include "mwc.h"

/*
  precondition:
  the output of o_node_ref is input of i_node_ref.
  This net is given by 'con_net_ref' and this net must
  only contain two ports.
*/
static int gnc_OptGenericMergeCells(gnc nc, int cell_ref, int o_node_ref, int i_node_ref, int con_net_ref, int g_id)
{
  b_il_type net_refs;
  int out_net_ref = -1;
  
  {
    int o_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, o_node_ref);
    int i_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, i_node_ref);
    gcell o_cell = gnc_GetGCELL(nc, o_cell_ref);
    gcell i_cell = gnc_GetGCELL(nc, i_cell_ref);
    assert(o_cell->id == i_cell->id);
  }
  
  gnc_DelCellNet(nc, cell_ref, con_net_ref);
  
  
  net_refs = b_il_Open();
  b_il_Clear(net_refs);
  
  if ( gnc_AddCellNodeTypeNet(nc, cell_ref, o_node_ref, GPORT_TYPE_IN, net_refs) == 0 )
    return b_il_Close(net_refs), 0;
  if ( gnc_AddCellNodeTypeNet(nc, cell_ref, i_node_ref, GPORT_TYPE_IN, net_refs) == 0 )
    return b_il_Close(net_refs), 0;
  out_net_ref = gnc_GetCellNodeFirstOutputNet(nc, cell_ref, i_node_ref);
  if ( out_net_ref < 0 )
    return b_il_Close(net_refs), 0;
    
  gnc_DelCellNode(nc, cell_ref, o_node_ref);
  gnc_DelCellNode(nc, cell_ref, i_node_ref);

  if ( gnc_SynthGenericGate(nc, cell_ref, g_id, net_refs, out_net_ref) < 0 )
    return b_il_Close(net_refs), 0;
  
  return b_il_Close(net_refs), 1;
}

int gnc_OptGenericGateMergeNode(gnc nc, int cell_ref, int node_ref, int g_id)
{
  b_il_type net_refs;
  gnode node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
  gcell cell = gnc_GetGCELL(nc, gnc_GetCellNodeCell(nc, cell_ref, node_ref));
  int i, cnt;
  int drv_node_ref;
  int drv_cell_ref;
  
  assert(node != NULL);
  assert(cell != NULL);
  
  if ( cell->id != g_id )
    return 1;
  
  net_refs = b_il_Open();
  if ( net_refs == NULL )
  {
    gnc_Error(nc, "gnc_MergeGenericGate: Out of Memory (b_il_Open).");
    return 0;
  }
  
  if ( gnc_GetCellNodeInputNet(nc, cell_ref, node_ref, net_refs) == 0 )
    return b_il_Close(net_refs), 0;
  
  cnt = b_il_GetCnt(net_refs);
  for( i = 0; i < cnt; i++ )
  {
    if ( gnc_GetCellNetPortCnt(nc, cell_ref, b_il_GetVal(net_refs, i)) == 2 )
    {
      drv_node_ref = gnc_FindCellNetOutputNode(nc, cell_ref, b_il_GetVal(net_refs, i));
      if ( drv_node_ref >= 0 )
      {
        drv_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, drv_node_ref);
        if ( gnc_GetCellId(nc, drv_cell_ref) == g_id )
        {
          if ( gnc_OptGenericMergeCells(nc, cell_ref, drv_node_ref, node_ref,  b_il_GetVal(net_refs, i), g_id) == 0 )
            return b_il_Close(net_refs), 0;
          return b_il_Close(net_refs), 1;
        }
      }
    }
  }
  node->flag = 1;
  return b_il_Close(net_refs), 1;
}


int gnc_OptGenericMerge(gnc nc, int cell_ref)
{
  int is_finished = 1;
  int node_ref;
  gnode node;
  int cnt = 0;
  
  gnc_ClearCellNodeFlags(nc, cell_ref);
  
  do
  {
    is_finished = 1;
    node_ref = -1;
    while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
    {
      node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
      if ( node->flag == 0 )
      {
        if ( gnc_GetCellId(nc, node->cell_ref) == GCELL_ID_G_AND )
        {
          is_finished = 0;
          if ( gnc_OptGenericGateMergeNode(nc, cell_ref, node_ref, GCELL_ID_G_AND) == 0 )
            return 0;
        }
        else if ( gnc_GetCellId(nc, node->cell_ref) == GCELL_ID_G_OR )
        {
          is_finished = 0;
          if ( gnc_OptGenericGateMergeNode(nc, cell_ref, node_ref, GCELL_ID_G_OR) == 0 )
            return 0;
        }
      }
    }
    cnt++;
  } while( is_finished == 0 && cnt < 1000);
  
  if ( cnt == 1000 )
    gnc_Log(nc, 4, "Synthesis: Generic cell merge has been aborted.");
  
  return 1;
}

/*---------------------------------------------------------------------------*/

/* 
  creates a new node that is a copy of dest_node_ref.
  dest_node_ref is deleted .
  sub_node_ref is not changed.
  the output of sub_node_ref will drive the new node.
  the new node has inputs(dest_node_ref)-inputs(sub_node_ref)+1 inputs.
  
  precondition: all inputs to sub_node_ref must also be inputs to dest_node_ref
*/

int gnc_ReplaceInputs(gnc nc, int cell_ref, int dest_node_ref, int sub_node_ref)
{
  b_il_type dest, sub;
  int sub_out_net_ref;
  int dest_cell_ref;
  int sub_cell_ref;
  int dest_out_net_ref;
  int g_id;
  int i;

  dest_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, dest_node_ref);
  sub_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, sub_node_ref);
  g_id = gnc_GetCellId(nc, dest_cell_ref);
  if ( g_id != gnc_GetCellId(nc, sub_cell_ref) )
  {
    gnc_Error(nc, "gnc_ReplaceInputs: Cell ID's do not match.");
    return 0;
  }
  
  if ( g_id != GCELL_ID_G_AND && g_id != GCELL_ID_G_OR )
  {
    gnc_Error(nc, "gnc_ReplaceInputs: Not a generic cell.");
    return 0;
  }
  
  if ( b_il_OpenMultiple(2, &dest, &sub) == 0 )
  {
    gnc_Error(nc, "gnc_ReplaceInputs: Out of Memory (b_il_OpenMultiple).");
    return 0;
  }

  if ( gnc_GetCellNodeInputNet(nc, cell_ref, dest_node_ref, dest) == 0 )
    return b_il_Close(dest), b_il_Close(sub), 0;

  if ( gnc_GetCellNodeInputNet(nc, cell_ref, sub_node_ref, sub) == 0 )
    return b_il_Close(dest), b_il_Close(sub), 0;
  
  dest_out_net_ref = gnc_GetCellNodeFirstOutputNet(nc, cell_ref, dest_node_ref);
  if ( dest_out_net_ref < 0 )
    return b_il_Close(dest), b_il_Close(sub), 0;

  sub_out_net_ref = gnc_GetCellNodeFirstOutputNet(nc, cell_ref, sub_node_ref);
  if ( sub_out_net_ref < 0 )
    return b_il_Close(dest), b_il_Close(sub), 0;
    
  for( i = 0; i < b_il_GetCnt(sub); i++ )
    b_il_DelByVal(dest, b_il_GetVal(sub, i));

  if ( b_il_Add(dest, sub_out_net_ref) < 0 )
    return b_il_Close(dest), b_il_Close(sub), 0;

  gnc_DelCellNode(nc, cell_ref, dest_node_ref);

  if ( gnc_SynthGenericGate(nc, cell_ref, g_id, dest, dest_out_net_ref) < 0 )
    return b_il_Close(dest), b_il_Close(sub), 0;
  
  return b_il_Close(dest), b_il_Close(sub), 1;
}

/*---------------------------------------------------------------------------*/

int gnc_OptGeneric(gnc nc, int cell_ref)
{
  return gnc_OptGenericMerge(nc, cell_ref);
}

