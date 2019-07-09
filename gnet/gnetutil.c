/*

  gnetutil.c

  some more or less general, more complex or perhaps special utility functions

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




/* returns cell_ref */
int gnc_AddEmptyCell(gnc nc)
{
  int cell_ref;
  char *name = "__empty__";
  
  if ( gnc_GetCellById(nc, GCELL_ID_EMPTY) >= 0 )
    return gnc_GetCellById(nc, GCELL_ID_EMPTY);
  
  assert( gnc_FindCell(nc, name, NULL) < 0 );
    
  cell_ref = gnc_AddCell(nc, name, GNET_HIDDEN_LIB_NAME);
  if ( cell_ref >= 0 )
  {
    if ( gnc_AddCellPort(nc, cell_ref, GPORT_TYPE_IN, "in") >= 0 )
    {
      if ( gnc_AddCellPort(nc, cell_ref, GPORT_TYPE_OUT, "out") >= 0 )
      {
        gnc_SetCellId(nc, cell_ref, GCELL_ID_EMPTY);
        gnc_SetCellArea(nc, cell_ref, 0.0);
        nc->bbb_disable[GCELL_ID_EMPTY] = 0;
        nc->bbb_cell_ref[GCELL_ID_EMPTY] = cell_ref;
        return cell_ref;
      }
    }
    gnc_DelCell(nc, cell_ref);
  }
  return -1;
}

/* net2_ref is deleted */
/* returns 0 on error */
/* TODO: use gnlMergeNets */
int gnc_MergeNets(gnc nc, int cell_ref, int net1_ref, int net2_ref)
{
  int join_ref = -1;
  int node_ref;
  int port_ref;
  while( gnc_LoopCellNetJoin(nc, cell_ref, net2_ref, &join_ref) )
  {
    node_ref = gnc_GetCellNetNode(nc, cell_ref, net2_ref, join_ref);
    port_ref = gnc_GetCellNetPort(nc, cell_ref, net2_ref, join_ref);
    if ( gnc_AddCellNetJoin(nc, cell_ref, net1_ref, node_ref, port_ref, 0) < 0 )
      return 0;    
  }
  gnc_DelCellNet(nc, cell_ref, net2_ref);
  return 1;
}


b_il_type gnc_GetCellListById(gnc nc, ...)
{
  int id;
  va_list va;
  
  b_il_type il;
  il = b_il_Open();
  if ( il == NULL )
  {
    gnc_Error(nc, "gnc_GetCellList: Out of Memory (b_il_Open).");
    return 0;
  }
  
  va_start(va, nc);
  for(;;)
  {
    id = va_arg(va, int);
    if ( id == GCELL_ID_UNKNOWN )
      break;
    if ( gnc_GetCellById(nc, id) >= 0 )
      if ( b_il_Add(il, gnc_GetCellById(nc, id)) < 0 )
      {
        va_end(va);
        b_il_Close(il);
        gnc_Error(nc, "gnc_GetCellList: Out of Memory (b_il_Add).");
        return 0;
      }
  }
  va_end(va);
  return il;
}

/*
void gnc_RateCellRefs(gnc nc,  b_il_type cell_refs, int *in_cnt_p, int *out_cnt_p, double *area_p, int *max_in_p, int *min_in_p)
{
  int in_cnt = 0;
  int out_cnt = 0;
  int min_in_cnt = 30000;
  int max_in_cnt = 0;
  double area = 0.0;
  int i, cnt = b_il_GetCnt(cell_refs);
  int cell_ref;
  gcell cell;

  for( i = 0; i < cnt; i++ )
  {
    cell_ref = b_il_GetVal(cell_refs, i);
    cell = gnc_GetGCELL(nc, cell_ref);
    in_cnt += cell->in_cnt;
    out_cnt += cell->out_cnt;
    area += cell->area;
    
    if ( min_in_cnt >  cell->in_cnt )
      min_in_cnt =  cell->in_cnt;
    if ( max_in_cnt < cell->in_cnt )
      max_in_cnt = cell->in_cnt;
  }

  if ( in_cnt_p != NULL )
    *in_cnt_p = in_cnt;
  if ( out_cnt_p != NULL )
    *out_cnt_p = out_cnt;
  if ( area_p != NULL )
    *area_p = area;
  if ( min_in_p != NULL )
    *min_in_p = min_in_cnt;
  if ( max_in_p != NULL )
    *max_in_p = max_in_cnt;
}
*/


int gnc_FindGenericCell(gnc nc, int g_id, int in_cnt)
{
  int cell_ref = -1;
  gcell cell;
  while( gnc_LoopCell(nc, &cell_ref) != 0 )
  {
    cell = gnc_GetGCELL(nc, cell_ref);
    if ( cell->in_cnt == in_cnt && cell->id == g_id )
      return cell_ref;
  }
  return -1;
}

int gnc_GetGenericCell(gnc nc, int g_id, int in_cnt)
{
  int cell_ref;
  char s[64];
  int i;
  cell_ref = gnc_FindGenericCell(nc, g_id, in_cnt);
  if ( cell_ref >= 0 )
    return cell_ref;

  switch(g_id)  
  {
    case GCELL_ID_G_INV:
      sprintf(s, "__generic_inv__");
      break;
    case GCELL_ID_G_AND:
      sprintf(s, "__generic_and_%d__", in_cnt);
      break;
    case GCELL_ID_G_OR:
      sprintf(s, "__generic_or_%d__", in_cnt);
      break;
    case GCELL_ID_G_REGISTER:
      sprintf(s, "__generic_register_%d__", in_cnt);
      break;
    case GCELL_ID_G_DOOR:
      sprintf(s, "__generic_door_%d__", in_cnt);
      break;
    default:
      return -1;
  }

  cell_ref = gnc_AddCell(nc, s, GNET_GENERIC_LIB_NAME);
  if ( cell_ref < 0 )
    return -1;
    
  gnc_GetGCELL(nc, cell_ref)->id = g_id;

  if ( g_id == GCELL_ID_G_REGISTER || g_id == GCELL_ID_G_DOOR )
  {
    for( i = 0; i < in_cnt; i++ )
    {
      sprintf(s, GPORT_OUT_PREFIX "%d", i);
      if ( gnc_SetCellPort(nc, cell_ref, i, GPORT_TYPE_OUT, s) == 0 )
        return gnc_DelCell(nc, cell_ref), -1;
      gnc_SetCellPortFn(nc, cell_ref, i, GPORT_FN_Q, 0);
    }
    for( i = 0; i < in_cnt; i++ )
    {
      sprintf(s, GPORT_IN_PREFIX "%d", i);
      if ( gnc_SetCellPort(nc, cell_ref, i+in_cnt, GPORT_TYPE_IN, s) == 0 )
        return gnc_DelCell(nc, cell_ref), -1;
      gnc_SetCellPortFn(nc, cell_ref, i, GPORT_FN_Q, 0);
    }
  }
  else
  {
    if ( gnc_AddCellPort(nc, cell_ref, GPORT_TYPE_OUT, GPORT_OUT_PREFIX "0") < 0 )
      return gnc_DelCell(nc, cell_ref), -1;

    for( i = 0; i < in_cnt; i++ )
    {
      sprintf(s, GPORT_IN_PREFIX "%d", i);
      if ( gnc_AddCellPort(nc, cell_ref, GPORT_TYPE_IN, s) < 0 )
        return gnc_DelCell(nc, cell_ref), -1;
    }
  }
    
  
  return cell_ref;
}

int gnc_ApplyRegisterResetCode(gnc nc, int cell_ref, pinfo *pi, dcube *code)
{
  int i, cnt;

  if ( code == NULL )
  {
    gnc_Error(nc, "gnc_ApplyRegisterResetCode: No reset.");
    return 0;
  }

  i = -1;
  cnt = 0;
  /* clear all output ports lines */
  while( gnc_LoopCellPort(nc, cell_ref, &i) != 0 )
  {
    if ( gnc_GetCellPortType(nc, cell_ref, i) == GPORT_TYPE_IN )
    {
      if ( cnt != i-pi->out_cnt )
      {
        gnc_Error(nc, "gnc_ApplyRegisterResetCode: Input lines expected last.");
        return 0;
      }
      cnt++;
      gnc_SetCellPortFn(nc, cell_ref, i, GPORT_FN_Q, 0);
    }
  }
    
  if ( cnt != pi->out_cnt )
  {
    gnc_Error(nc, "gnc_ApplyRegisterResetCode: Register- and code-width do not match (%d != %d, %s).",
      cnt, pi->out_cnt, 
      gnc_GetCellName(nc, cell_ref)==NULL?"?":gnc_GetCellName(nc, cell_ref));
    return 0;
  }
    
  for( i = 0; i < cnt; i++ )
    gnc_SetCellPortFn(nc, cell_ref, i+pi->out_cnt, GPORT_FN_LOGIC, dcGetOut(code, i));

  return 1;
}


void gnc_DeleteUnusedNodes(gnc nc, int cell_ref)
{
  int port_ref = -1; 
  int net_ref = -1;
  int join_ref = -1;
  int node_ref = -1;
  int is_delete;
  
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
  {
    port_ref = -1;
    net_ref = -1;
    join_ref = -1;
    is_delete = 1;
    while ( gnc_LoopCellNodeOutputs(nc, cell_ref, node_ref, &port_ref, &net_ref, &join_ref) != 0 )
    {
      if ( gnc_GetCellNetPortCnt(nc, cell_ref, net_ref) > 1 )
      {
        is_delete = 0;
        break;
      }
    }
    if ( is_delete != 0 )
    {
      gnc_DelCellNode(nc, cell_ref, node_ref);
    }
  }
}

void gnc_SetCellNodeData(gnc nc, int cell_ref, int node_ref, int data)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return;
  gnlGetGNODE(nl, node_ref)->data = data;
}

int gnc_GetCellNodeData(gnc nc, int cell_ref, int node_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return -1;
  return gnlGetGNODE(nl, node_ref)->data;
}

/* make a copy of all nodes in src, store the new node_id inside src */
static int gnc_copy_nodes_and_set_data(gnc nc, int dest_cell_ref, int src_cell_ref)
{
  int node_ref = -1;
  int new_node_ref;
  int node_cell_ref;
  while( gnc_LoopCellNode(nc, src_cell_ref, &node_ref) != 0 )
  {
    node_cell_ref = gnc_GetCellNodeCell(nc, src_cell_ref, node_ref);
    /* the name can not be reused, because automatic generated node names might */
    /* already exist */
    new_node_ref = gnc_AddCellNode(nc, dest_cell_ref, NULL, node_cell_ref);
    if ( new_node_ref < 0 )
      return 0;
      
    if ( 0 >= nc->log_level )
    {
      gnc_Log(nc, 0, "Flatten: Add node '%s' from cell '%s' to cell '%s' (%d -> %d).",
        gnc_GetCellName(nc, node_cell_ref)==NULL?"<unknown>":gnc_GetCellName(nc, node_cell_ref),
        gnc_GetCellName(nc, src_cell_ref)==NULL?"<unknown>":gnc_GetCellName(nc, src_cell_ref),
        gnc_GetCellName(nc, dest_cell_ref)==NULL?"<unknown>":gnc_GetCellName(nc, dest_cell_ref),
        node_ref, new_node_ref
        );
    }
      
    gnc_SetCellNodeData(nc, src_cell_ref, node_ref, new_node_ref);
  }
  return 1;
}

static int gnc_copy_nets_and_use_data(gnc nc, int dest_cell_ref, int src_node_ref, int src_cell_ref)
{
  int net_ref = -1;
  int new_net_ret;
  int join_ref;
  int node_ref;
  int port_ref;
  int is_unique;
  while( gnc_LoopCellNet(nc, src_cell_ref, &net_ref) != 0 )
  {
    /* if the net contains connections to the parent... do the slow way */
    if ( gnc_FindCellNetJoin(nc, src_cell_ref, net_ref, -1, -1) >= 0 )
      is_unique = 1;
      
    new_net_ret = gnc_AddCellNet(nc, dest_cell_ref, NULL);
    if ( new_net_ret < 0 )
      return 0;
    join_ref = -1;
    while( gnc_LoopCellNetJoin(nc, src_cell_ref, net_ref, &join_ref) != 0 )
    {
      /* get the pair from the source net */
      port_ref = gnc_GetCellNetPort(nc, src_cell_ref, net_ref, join_ref);
      node_ref = gnc_GetCellNetNode(nc, src_cell_ref, net_ref, join_ref);
      /* translate the pair */
      if ( node_ref < 0 )
        node_ref = src_node_ref;
      else
        node_ref = gnc_GetCellNodeData(nc, src_cell_ref, node_ref);
      if ( gnc_AddCellNetJoin(nc, dest_cell_ref, new_net_ret, node_ref, port_ref, is_unique) < 0 )
        return 0;
    }
  }
  return 1;
}

int gnc_ReplaceCellNode(gnc nc, int cell_ref, int node_ref)
{
  int node_cell_ref;
  if ( cell_ref < 0 || node_ref < 0 )
    return 0;
  node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
  if ( gnc_GetGNL(nc, node_cell_ref, 0) == NULL )
    return 1; /* no error: cell has no netlist */    
    
  gnc_Log(nc, 1, "Flatten: Replace Node %d of cell '%s' with cell '%s'",
    node_ref, 
    gnc_GetCellName(nc, cell_ref)==NULL?"<unknown>":gnc_GetCellName(nc, cell_ref),
    gnc_GetCellName(nc, node_cell_ref)==NULL?"<unknown>":gnc_GetCellName(nc, node_cell_ref)
    );
  
  if ( gnc_copy_nodes_and_set_data(nc, cell_ref, node_cell_ref) == 0 )
    return 0;

  if ( gnc_copy_nets_and_use_data(nc, cell_ref, node_ref, node_cell_ref) == 0 )
    return 0;
    
  gnc_DelCellNode(nc, cell_ref, node_ref);
    
  return 1;
}

/* replace all references to node_cell_ref by the corresponding net */
int gnc_ReplaceCellCellRef(gnc nc, int cell_ref, int node_cell_ref)
{
  int node_ref;
  int is_replace;

  if ( gnc_GetGNL(nc, node_cell_ref, 0) == NULL )
    return 0;
  
  do
  {
    is_replace = 0;
    node_ref = -1;
    while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
      if ( gnc_GetCellNodeCell(nc, cell_ref, node_ref) == node_cell_ref )
      {
        if ( gnc_ReplaceCellNode(nc, cell_ref, node_ref) == 0 )
          return 0;
        /* start a new loop... because the node list has been changed */
        is_replace = 1;
        break; 
      }
  } while( is_replace != 0 );
  return 1;
}

/*!
  \ingroup gncnet

  A netlist contains nodes and nets.  
  This function replaces all nodes of the netlist of \a cell_ref 
  by the netlist of the node if possible. If a netlist only contains
  basic building blocks, nothing is done, because basic building blocks
  usually do not contain any netlists.
  
  The function is called by synthesis procedures for hierarchical
  descriptions if the 
  option \c GNC_HL_OPT_FLATTEN is set.
  
  \note Some export functions require a none hierarchical netlist. 
    This function can be used to ensure this.
  
  \pre The cell \a cell_ref must have a valid netlist.
  
  
  
  
  \param nc A pointer to a gnc structure.
  \param cell_ref The handle of a cell (for example the value returned by 
    gnc_SynthByFile()).
  \param is_delete If set to a none zero value: Delete cells that 
    are not referenced any more.
  
  \return 0 if an error occured.
  
  \see gnc_Open()
  \see gnc_WriteXNF()
  \see gnc_WriteVHDL()
  \see gnc_SynthByFile()
*/
int gnc_FlattenCell(gnc nc, int cell_ref, int is_delete)
{
  int node_ref;
  int is_replace;
  int node_cell_ref;
  
  do
  {
    is_replace = 0;
    node_ref = -1;
    while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
    {
      node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
      if ( gnc_GetGNL(nc, node_cell_ref, 0) != NULL )
      {
        if ( gnc_ReplaceCellCellRef(nc, cell_ref, node_cell_ref) == 0 )
          return 0;
        is_replace = 1;
        
        if ( is_delete != 0 )
          if ( gnc_IsTopLevelCell(nc, node_cell_ref) != 0 )
            gnc_DelCell(nc, node_cell_ref);
        
        /* start a new loop... because the node list has been changed */
        break; 
      }
    }
  } while( is_replace != 0 );
  return 1;
}




/* replace all references to cell_ref by inserting the corresponding net */
int gnc_ReplaceCellRef(gnc nc, int cell_ref)
{
  int ref;
  if ( gnc_GetGNL(nc, cell_ref, 0) == NULL )
    return 0; /* error: cell has no netlist */

  gnc_Log(nc, 2, "Flatten: Replace all references to cell '%s'.",
    gnc_GetCellName(nc, cell_ref)==NULL?"<unknown>":gnc_GetCellName(nc, cell_ref)
    );
  
  ref = -1;
  while( gnc_LoopCell(nc, &ref) != 0 )
    if ( gnc_GetGNL(nc, ref, 0) != NULL && ref != cell_ref )
      if ( gnc_ReplaceCellCellRef(nc, ref, cell_ref) == 0 )
        return 0;
  
  return 1;
}

int gnc_Flatten(gnc nc, int is_delete)
{
  int cell_ref = -1;
  int cnt = 0;
  gnc_Log(nc, 3, "Flatten: Started.");
  while( gnc_LoopCell(nc, &cell_ref) != 0 )
    if ( gnc_GetGNL(nc, cell_ref, 0) != NULL )
      if ( gnc_IsTopLevelCell(nc, cell_ref) == 0 )
      {
        cnt++;
        if ( gnc_ReplaceCellRef(nc, cell_ref) == 0 )
          return 0;
        if ( is_delete != 0 )
          gnc_DelCell(nc, cell_ref);
      }
      
  gnc_Log(nc, 3, "Flatten: Ended (%d cell%s replaced).",
    cnt, cnt==1?"":"s");
    
  return 1;
}

