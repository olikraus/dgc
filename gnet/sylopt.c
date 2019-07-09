/*

  sylopt.c
  
  synthesis: library optimization
  
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
  
  
  Optimizations:
    - Remove inverter-inverter pairs
    - replace a gate-inverter pair by a single gate
    - replace inverter-gate combination if usefull
    - replace more than one inverter at a net by only one inverter
  

*/

#include "gnet.h"
#include <assert.h>
#include <stdlib.h>
#include "mwc.h"


int gnc_ReplaceInverterInverter(gnc nc, int cell_ref)
{
  int node_ref;         /* the investigated node */
  int node_cell_ref;    /* the node's cell reference */
  int net_ref;          /* the input net of the inverter */
  int input_node_ref;   /* the leading node */
  int input_node_cell_ref;
  int f_net_ref;
  int l_net_ref;
  node_ref = -1;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
  {
    if ( gnc_IsCellNodeDoNotTouch(nc, cell_ref, node_ref) != 0 )
      continue;
    node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
    if ( gnc_GetCellId(nc, node_cell_ref) == GCELL_ID_INV )
    {
      net_ref = gnc_GetCellNodeFirstInputNet(nc, cell_ref, node_ref);
      if ( net_ref >= 0 )
      {
        input_node_ref = gnc_FindCellNetOutputNode(nc, cell_ref, net_ref);
        if ( input_node_ref >= 0 )
        {
          if ( gnc_IsCellNodeDoNotTouch(nc, cell_ref, input_node_ref) == 0 )
          {
            input_node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, input_node_ref);
            if ( gnc_GetCellId(nc, input_node_cell_ref) == GCELL_ID_INV )
            {
              f_net_ref = gnc_GetCellNodeFirstInputNet(nc, cell_ref, input_node_ref);
              l_net_ref = gnc_GetCellNodeFirstOutputNet(nc, cell_ref, node_ref);
              gnc_DelCellNode(nc, cell_ref, node_ref);
              if ( gnc_MergeNets(nc, cell_ref, f_net_ref, l_net_ref) == 0 )
                return 0;
              if ( gnc_GetCellNetPortCnt(nc, cell_ref, net_ref) == 1 )
                gnc_DelCellNode(nc, cell_ref, input_node_ref);

              node_ref = -1;  /* start from the beginning */
            }
          }
        }
      }
    }
  }
  return 1;
}

/* replace dublicate invertes at a net */
int gnc_ReplaceNetInverters(gnc nc, int cell_ref)
{
  int net_ref = -1;
  int join_ref;
  int first_inv_node_ref;
  int first_inv_out_net_ref;
  int node_ref;
  int out_net_ref;
  int node_cell_ref;
  int id;
  while( gnc_LoopCellNet(nc, cell_ref, &net_ref) != 0 )
  {
    join_ref = -1;
    first_inv_node_ref = -1;
    while(gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref))
    {
      node_ref = gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
      
      if ( node_ref < 0 )
        continue;           /* ignore parents */

      if ( gnc_IsCellNodeDoNotTouch(nc, cell_ref, node_ref) != 0 )
        continue;           /* ignore do-not-touch elements */

      node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
      id = gnc_GetCellId(nc, node_cell_ref);
      if ( id == GCELL_ID_INV )
      {
        /* inverter node found */
        /* if this is the first one found, store it */
        /* delete all following inverters */
      
        out_net_ref = gnc_GetCellNodeFirstOutputNet(nc, cell_ref, node_ref);
        if ( first_inv_node_ref < 0 )
        {
          first_inv_node_ref = node_ref;
          first_inv_out_net_ref = out_net_ref;
        }
        else
        {
          if ( gnc_MergeNets(nc, cell_ref, first_inv_out_net_ref, out_net_ref) == 0 )
            return 0;
          gnc_DelCellNode(nc, cell_ref, node_ref);
        }
      }
    }
  } 
  return 1;
}

/* tries to remove an inverter after a gate */
/* inv_node_ref should point to an inverter */
int gnc_DelNodeInverter(gnc nc, int cell_ref, int inv_node_ref)
{
  int node_ref;
  int node_cell_ref;
  int old_id;
  int new_id;
  int net_ref;
  int inv_node_cell_ref;
  int inv_out_net_ref;
  const char *gate_name;
  
  if ( gnc_IsCellNodeDoNotTouch(nc, cell_ref, inv_node_ref) != 0 )
    return 1;
  
  inv_node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, inv_node_ref);
  if ( gnc_GetCellId(nc, inv_node_cell_ref) != GCELL_ID_INV )
    return 1;
  
  net_ref = gnc_GetCellNodeFirstInputNet(nc, cell_ref, inv_node_ref);
  if ( net_ref < 0 )
    return 1;
  if ( gnc_GetCellNetPortCnt(nc, cell_ref, net_ref) > 2 )
    return 1;
  node_ref = gnc_FindCellNetOutputNode(nc, cell_ref, net_ref);
  if ( node_ref < 0 )
    return 1;
  
  node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
  old_id = gnc_GetCellId(nc, node_cell_ref);
  if ( old_id <= GCELL_ID_INV )
    return 1;

  new_id = GCELL_ID_UNKNOWN;
  if ( (old_id&3) == 0 )      /* AND --> NAND */
  {
    new_id = (old_id&~3)|1;
    gate_name = "NAND";
  }
  else if ( (old_id&3) == 1 ) /* NAND --> AND */
  {
    new_id = (old_id&~3)|0;
    gate_name = "AND";
  }
  else if ( (old_id&3) == 2 ) /* OR --> NOR */
  {
    new_id = (old_id&~3)|3;
    gate_name = "NOR";
  }
  else if ( (old_id&3) == 3 ) /* NOR --> OR */
  {
    new_id = (old_id&~3)|2;
    gate_name = "OR";
  }
  
  if ( gnc_GetCellById(nc, new_id) < 0 )
    return 1;

  inv_out_net_ref = gnc_GetCellNodeFirstOutputNet(nc, cell_ref, inv_node_ref);
  if ( inv_out_net_ref < 0 )
    return 1;
    
  /* delete inv_out_net_ref, add contents to net_ref */
  if ( gnc_MergeNets(nc, cell_ref, net_ref, inv_out_net_ref) == 0 )
    return 0;
   

  /* read the new cell */   
  {
    b_il_type il = b_il_Open();
    if ( il == NULL )
      return 0;
    if ( gnc_GetCellNodeInputNet(nc, cell_ref, node_ref, il) == 0 )
      return b_il_Close(il), 0;

    if ( gnc_SynthDigitalCell(nc, cell_ref, gnc_GetCellById(nc, new_id), il, net_ref) < 0 )
      return b_il_Close(il), 0;
    
    b_il_Close(il);
  }
  
  /* delete the inverter and the old cell */
  gnc_DelCellNode(nc, cell_ref, inv_node_ref);
  gnc_DelCellNode(nc, cell_ref, node_ref);

  gnc_Log(nc, 0, "Synthesis: Gate-Inverter pair replaced (new gate: %s).", gate_name);
  
  return 1;
}

int gnc_ReplaceGateInverter(gnc nc, int cell_ref)
{
  int node_ref = -1;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
  {
    if ( gnc_DelNodeInverter(nc, cell_ref, node_ref) == 0 )
      return 0;
  }
  return 1;
}

int gnc_CountInputs(gnc nc, int cell_ref, int node_ref, int *inv_cnt, int *gate_cnt, int *parent_cnt)
{
  int port_ref = -1; 
  int net_ref = -1;
  int join_ref = -1;
  int in_node_ref;
  int in_cell_ref;
  *inv_cnt = 0;
  *gate_cnt = 0;
  *parent_cnt = 0;
  
  while( gnc_LoopCellNodeInputs(nc, cell_ref, node_ref, &port_ref, &net_ref, &join_ref) != 0 )
  {
    in_node_ref = gnc_FindCellNetNodeByDriver(nc, cell_ref, net_ref);
    if ( in_node_ref < 0 )
    {
      (*parent_cnt)++;
    }
    else
    {
      in_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, in_node_ref);
      if ( gnc_GetCellId(nc, in_cell_ref) == GCELL_ID_INV )
        (*inv_cnt)++;
      else
        (*gate_cnt)++;

      /*
      gnc_Log(nc, 0, "Synthesis: Node '%s' has input from node '%s'.", 
        gnc_GetCellNodeCellName(nc, cell_ref, node_ref)==NULL?"?":
        gnc_GetCellNodeCellName(nc, cell_ref, node_ref),
        gnc_GetCellNodeCellName(nc, cell_ref, in_node_ref)==NULL?"?":
        gnc_GetCellNodeCellName(nc, cell_ref, in_node_ref)
      );
      */

    }
  }
  return 1;
}

/* requires: gnc_ReplaceInverterInverter() */
int gnc_InvertInputs(gnc nc, int cell_ref, int node_ref, int *is_synth)
{
  int port_ref = -1; 
  int net_ref = -1;
  int join_ref = -1;
  int new_in_net_ref;
  int in_port_ref;
  int in_node_ref;
  int in_cell_ref;
  int old_id;
  int new_id;
  int out_net_ref;
  b_il_type net_refs;
  int new_node_ref;

  *is_synth = 0;
  
  if ( gnc_IsCellNodeDoNotTouch(nc, cell_ref, node_ref) != 0 )
    return 1;

  old_id = gnc_GetCellId(nc, gnc_GetCellNodeCell(nc, cell_ref, node_ref));
  if ( old_id <= GCELL_ID_INV )
    return 0;

  new_id = GCELL_ID_UNKNOWN;
  if ( (old_id&3) == 0 )      /* AND --> NOR */
    new_id = (old_id&~3)|3;
  else if ( (old_id&3) == 1 ) /* NAND --> OR */
    new_id = (old_id&~3)|2;
  else if ( (old_id&3) == 2 ) /* OR --> NAND */
    new_id = (old_id&~3)|1;
  else if ( (old_id&3) == 3 ) /* NOR --> AND */
    new_id = (old_id&~3)|0;

  assert(new_id >= GCELL_ID_UNKNOWN);
  assert(new_id < GCELL_ID_MAX);
  if ( gnc_GetCellById(nc, new_id) < 0 )
    return 1;   /* the new id does not exist, can not convert (is_synth = 0) */


  if ( old_id >= GCELL_ID_G_INV )
  {
    gnc_Error(nc, "gnc_InvertInputs: Generic cell %d found\n", old_id);
    return 0;
  }
  /* assert(old_id < GCELL_ID_G_INV); */
  
  net_refs = b_il_Open();
  if ( net_refs == NULL )
    return 0;
  
  gnc_Log(nc, 1, "Synthesis: Invert inputs of cell '%s' (node %d).", 
    gnc_GetCellNodeCellName(nc, cell_ref, node_ref)==NULL?"?":
    gnc_GetCellNodeCellName(nc, cell_ref, node_ref),
    node_ref);
    
  while( gnc_LoopCellNodeInputs(nc, cell_ref, node_ref, &port_ref, &net_ref, &join_ref) != 0 )
  {
    in_node_ref = gnc_FindCellNetNodeByDriver(nc, cell_ref, net_ref);
    if ( in_node_ref < 0 )
    {
      /* the net has not output port, well it must be driven by the */
      /* input port of the parent cell */
      in_port_ref = gnc_FindCellNetPortByDriver(nc, cell_ref, net_ref);
      if ( gnc_GetCellPortType(nc, cell_ref, in_port_ref) == GPORT_TYPE_IN )
      {
        new_in_net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, in_port_ref, 1, 0);
      }
      else
      {
        in_node_ref = gnc_FindCellNetNodeByDriver(nc, cell_ref, net_ref);
        gnc_Error(nc, "gnc_InvertInputs: Internal Error (Parent input not connected: %s, node %d, net %d).",
          gnc_GetCellName(nc, gnc_GetCellNodeCell(nc, cell_ref, node_ref)), node_ref, net_ref);
        return 0;
      }
    }
    else
    {
      in_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, in_node_ref);
      if ( gnc_GetCellId(nc, in_cell_ref) == GCELL_ID_INV )
      {
        new_in_net_ref = gnc_GetCellNodeFirstInputNet(nc, cell_ref, in_node_ref);
      }
      else
      {
        new_in_net_ref = gnc_GetInvertedNet(nc, cell_ref, net_ref, 0);
      }
      if ( new_in_net_ref < 0 )
      {
        gnc_Error(nc, "gnc_InvertInputs: Failed.");
        return b_il_Close(net_refs), 0;
      }
    }
    if ( new_in_net_ref >= 0 )
      if ( b_il_Add(net_refs, new_in_net_ref) < 0 )
      {
        gnc_Error(nc, "gnc_InvertInputs: Out of Memory (b_il_Add).");
        return b_il_Close(net_refs), 0;
      }
  }
  
  out_net_ref = gnc_GetCellNodeFirstOutputNet(nc, cell_ref, node_ref);
  if ( out_net_ref < 0 )
    return b_il_Close(net_refs), 0;

  new_node_ref = gnc_SynthDigitalCell(nc, cell_ref, gnc_GetCellById(nc, new_id), net_refs, out_net_ref);
  if ( new_node_ref < 0 )
    return b_il_Close(net_refs), 0;

  *is_synth = 1;

  gnc_DelCellNode(nc, cell_ref, node_ref);
  
  
  /*
  if ( gnc_GetCellNetPortCnt(nc, cell_ref, out_net_ref) == 2 )
  {
    int tmp_node_ref = gnc_FindFirstCellNetInputNode(nc, cell_ref, net_ref);
    if ( tmp_node_ref >= 0 )
    {
      if ( gnc_DelNodeInverter(nc, cell_ref, tmp_node_ref) == 0 )
        return b_il_Close(net_refs), 0;
    }
  }
  */

  return b_il_Close(net_refs), 1;
}

/* optimize a node by complementing the inputs if the number of inverted */
/* is larger, than the number of inverted outputs */
int gnc_OptNode(gnc nc, int cell_ref, int node_ref, int *is_synth)
{
  int inv_cnt, gate_cnt, parent_cnt;
  *is_synth = 0;
  if ( gnc_CountInputs(nc, cell_ref, node_ref, &inv_cnt, &gate_cnt, &parent_cnt) == 0 )
    return 0;

  gnc_Log(nc, 0, "Synthesis: Consider node '%s' (%d) with %d inverter, %d gate and %d parent connections.", 
    gnc_GetCellNodeCellName(nc, cell_ref, node_ref)==NULL?"?":
    gnc_GetCellNodeCellName(nc, cell_ref, node_ref),
    node_ref, inv_cnt, gate_cnt, parent_cnt);

  if ( inv_cnt > gate_cnt+parent_cnt )
  {

    gnc_Log(nc, 1, "Synthesis: Optimize node '%s' (%d) with %d inverted and %d non-inverted inputs.", 
      gnc_GetCellNodeCellName(nc, cell_ref, node_ref)==NULL?"?":
      gnc_GetCellNodeCellName(nc, cell_ref, node_ref),
      node_ref, inv_cnt, gate_cnt+parent_cnt);

    if ( gnc_InvertInputs(nc, cell_ref, node_ref, is_synth) == 0 )
      return 0;

    gnc_DeleteUnusedNodes(nc, cell_ref);

    if ( *is_synth != 0 )
      if ( gnc_ReplaceGateInverter(nc, cell_ref) == 0 )
        return 0;
  }
    
  return 1;
}

int gnc_OptNodes(gnc nc, int cell_ref)
{
  int node_ref;
  int is_synth;
  int cnt = 0;

  is_synth = 1;

  while( is_synth != 0 )
  {
    node_ref = -1;
    is_synth = 0;
    while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
    {
      if ( gnc_OptNode(nc, cell_ref, node_ref, &is_synth) == 0 )
        return 0;
      if ( is_synth != 0 )
      {
        cnt++;
        break;
      }
    }
  }

  gnc_Log(nc, 3, "Synthesis: %d gate%s optimized.", cnt, cnt==1?"":"s");

  return 1;
}


int gnc_OptLibrary(gnc nc, int cell_ref)
{
  gnc_DeleteUnusedNodes(nc, cell_ref);

  if ( gnc_ReplaceInverterInverter(nc, cell_ref) == 0 )
    return 0;

  if ( gnc_ReplaceGateInverter(nc, cell_ref) == 0 )
    return 0;

  if ( gnc_OptNodes(nc, cell_ref) == 0 )
    return 0;

  return 1;
}



#ifdef __OBSOLETE_CODE

/* fill 'net_refs' with all net's that are connected to the output ports */
int gnc_GetCellOutputNet(gnc nc, int cell_ref, b_il_type net_refs)
{
  int port_ref = -1;
  int net_ref;
  b_il_Clear(net_refs);
  while( gnc_LoopCellPort(nc, cell_ref, &port_ref) != 0 )
  {
    if ( gnc_GetCellPortType(nc, cell_ref, port_ref) == GPORT_TYPE_OUT )
    {
      net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, port_ref);
      if ( net_ref >= 0 )
      {
        if ( b_il_Add(net_refs, net_ref) < 0 )
        {
          gnc_Error(nc, "gnc_GetCellOutputNet: Out of Memory (b_il_Add).");
          return 0;
        }
      }
    }
  }
  return 1;
}

/* fill 'node_refs' with all those nodes, that drive a net that is */
/* connected to one of the output ports */
int gnc_GetCellOutputNodes(gnc nc, int cell_ref, b_il_type node_refs)
{
  int i, cnt;
  
  /* put all 'net_ref's into the 'node_refs' list that are connected to */
  /* an output port of the parent cell */
  
  if ( gnc_GetCellOutputNet(nc, cell_ref, node_refs) == 0 )
    return 0;
    
  /* replace the 'net_ref's with the 'node_ref' of the node that drives */
  /* the net */
    
  cnt = b_il_GetCnt(node_refs);
  for( i = 0; i < cnt; i++ )
  {
    b_il_SetVal(node_refs, i, gnc_FindCellNetNodeByDriver(nc, cell_ref, b_il_GetVal(node_refs, i)));
  }
  return 1;
}

static int gnc_FindInvertNode(gnc nc, int cell_ref, b_il_type node_refs, int start)
{
  int i, cnt;
  int node_ref;
  int node_cell_ref;
  int id;
  gnode node;
  cnt = b_il_GetCnt(node_refs);
  for( i = start; i < cnt; i++ )
  {
    node_ref = b_il_GetVal(node_refs, i);
    if ( node_ref >= 0 )
    {
      node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
      if ( node->flag == 0 )
      {
        node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
        id = gnc_GetCellId(nc, node_cell_ref);
        if ( id == GCELL_ID_INV )
          return i;
        if ( id > GCELL_ID_INV )
          if ( (id&1) != 0 )
            return i;
      }
    }
  }
  return -1;
}

/* generates a list of nodes, which are not inverters and which */
/* drive some of the output ports */
int gnc_GetNoneInvertNodeList(gnc nc, int cell_ref, b_il_type node_refs)
{
  int start, pos;
  int node_ref;
  

  gnc_ClearCellNodeFlags(nc, cell_ref);
  
  if ( gnc_GetCellOutputNodes(nc, cell_ref, node_refs) == 0 )
    return 0;

  pos = 0;
  for(;;)
  {  
    pos = gnc_FindInvertNode(nc, cell_ref, node_refs, pos);
    if ( pos < 0 )
      break;
    node_ref = b_il_GetVal(node_refs, pos);
    gnc_GetCellGNODE(nc, cell_ref, node_ref)->flag = 1;
    b_il_DelByPos(node_refs, pos);
    if ( gnc_AddCellNodeInputNodes(nc, cell_ref, node_ref, node_refs) == 0 )
      return 0;
  }
  
  return 1;
}

int gnc_OptGates(gnc nc, int cell_ref, b_il_type node_refs, int *is_optimized, int *synth_cnt)
{
  int cnt = b_il_GetCnt(node_refs);
  int i;
  int node_ref;
  int node_cell_ref;
  int id;
  int is_synth = 0;
  
  *is_optimized = 0;
  for( i = 0; i < cnt; i++ )
  {  
    node_ref = b_il_GetVal(node_refs, i);
    if ( node_ref < 0 )
      continue;
      
    node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
    if ( node_cell_ref < 0  )
      continue;
      
    id = gnc_GetCellId(nc, node_cell_ref);
    if ( id <= GCELL_ID_INV )
      continue;

    if ( (id&1) != 0 )
      continue;

    if ( id >= GCELL_ID_DFF_Q )
      continue;

    if ( gnc_InvertInputs(nc, cell_ref, node_ref, &is_synth) == 0 )
      return 0;
    if ( is_synth != 0 )
    {
      *is_optimized = 1;
      (*synth_cnt)++;
    }
  }
  
  return 1;
}
  

int gnc_OptLibrary(gnc nc, int cell_ref)
{
  int node_ref = -1;
  int inv_cnt = 0;
  int gate_cnt = 0;
  int parent_cnt = 0;
  int is_optimized;
  b_il_type il;
  int synth_cnt = 0;
  
  gnc_DeleteUnusedNodes(nc, cell_ref);

  if ( gnc_ReplaceInverterInverter(nc, cell_ref) == 0 )
    return 0;

  if ( gnc_ReplaceGateInverter(nc, cell_ref) == 0 )
    return 0;

  il = b_il_Open();


  /*
  if ( gnc_ReplaceNetInverters(nc, cell_ref) == 0 )
    return 0;
  */

  /*
  if ( gnc_ReplaceGateInverter(nc, cell_ref) == 0 )
    return b_il_Close(il), 0;
  */

  for(;;)
  {  

    gnc_GetNoneInvertNodeList(nc, cell_ref, il);

    synth_cnt = 0;
    if ( gnc_OptGates(nc, cell_ref, il, &is_optimized, &synth_cnt) == 0 )
      return b_il_Close(il), 0;
    if ( is_optimized == 0 )
      break;
    gnc_Log(nc, 2, "Synthesis: %d library cell%s optimized.", synth_cnt, synth_cnt==1?"":"s");

    gnc_DeleteUnusedNodes(nc, cell_ref);

    if ( gnc_ReplaceInverterInverter(nc, cell_ref) == 0 )
      return b_il_Close(il), 0;
      
    /*
    if ( gnc_ReplaceGateInverter(nc, cell_ref) == 0 )
      return b_il_Close(il), 0;
    */
  }
  
  /*
  if ( gnc_ReplaceGateInverter(nc, cell_ref) == 0 )
    return 0;
  */

  if ( gnc_ReplaceNetInverters(nc, cell_ref) == 0 )
    return 0;

  b_il_Close(il);
  return 1;
}

#endif /* __OBSOLETE_CODE */
