/*

  gnetdcex.c
  
  Extract a boolean syntax tree from a netlist.
  Do a formal verification.
  
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
#include "dcex.h"

static dcexn gnc_get_dcex_by_node(gnc nc, int cell_ref, int node_ref);

static int translate_id(int id)
{
  if ( id == GCELL_ID_G_INV )
    return DCEXN_NOT;
  if ( id == GCELL_ID_INV )
    return DCEXN_NOT;
  if ( id == GCELL_ID_G_AND )
    return DCEXN_AND;
  if ( id == GCELL_ID_G_OR )
    return DCEXN_OR;
  if ( id >= GCELL_ID_AND2 && id <= GCELL_ID_NOR8 )
  {
    if ( (id&2) == 0 )
      return DCEXN_AND;
    return DCEXN_OR;
  }
  return -1;
}

static int is_inverted_gate(int id)
{
  if ( id >= GCELL_ID_AND2 && id <= GCELL_ID_NOR8 )
    if ( (id&1) != 0 )
      return 1;
  return 0;
}

static dcexn gnc_get_dcex_node(gnc nc, int id)
{
  dcexn n = dcexnOpen();
  if ( n == NULL )
  {
    gnc_Error(nc, "GNC2DCEX: Out of Memory (dcexnOpen).");
    return NULL;
  }
  n->data = id;
  return n;
}

static dcexn gnc_get_dcex_by_net(gnc nc, int cell_ref, int net_ref)
{
  int node_ref;
  int join_ref;
  int min_parent_port_ref = gnc_GetCellPortMax(nc, cell_ref)+1;
  int min_parent_join_ref = -1;
  pinfo *pi = gnc_GetGCELL(nc, cell_ref)->pi;
  int input_cnt = gnc_GetCellPortCnt(nc, cell_ref);
  
  if ( pi != NULL )
    input_cnt = pi->in_cnt;
  
  /* again, an new search procedure */
  join_ref = -1;
  node_ref = -1;
  while(gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0)
  {
    /* Parent ports are sorted and start with input values going to output */
    /* values. Now find the minimum ref number of a parent port */
    if ( gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref) < 0 )
      if ( gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref) < input_cnt )
        if ( min_parent_port_ref > gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref) )
        {
          min_parent_port_ref = gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref);
          min_parent_join_ref = join_ref;
        }

    /* also find the node that drives the net */        
    if ( gnc_GetCellNetPortType(nc, cell_ref, net_ref, join_ref) == GPORT_TYPE_OUT )
      if ( gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref) >= 0 )
        node_ref = gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
  }
  
  /* valid parent ports have priority (we would find door and ff elements */
  /* otherwise) */
  
  if ( min_parent_port_ref != gnc_GetCellPortMax(nc, cell_ref)+1 )
  {
    /* port_ref = gnc_GetCellNetPort(nc, cell_ref, net_ref, min_parent_join_ref); */

    gnc_Log(nc, 0, "Verify: Terminal port %d (%s).", min_parent_port_ref,
      gnc_GetCellPortName(nc, cell_ref, min_parent_port_ref)==NULL?"<unknown>":gnc_GetCellPortName(nc, cell_ref, min_parent_port_ref));
    
    switch(gnc_GetCellPortFn(nc, cell_ref, min_parent_port_ref))
    {
      case GPORT_FN_CLR:
        if ( gnc_IsCellPortInverted(nc, cell_ref, min_parent_port_ref) != 0 )
          return gnc_get_dcex_node(nc, DCEXN_ONE);
        return gnc_get_dcex_node(nc, DCEXN_ZERO);
      case GPORT_FN_SET:
        if ( gnc_IsCellPortInverted(nc, cell_ref, min_parent_port_ref) != 0 )
          return gnc_get_dcex_node(nc, DCEXN_ONE);
        return gnc_get_dcex_node(nc, DCEXN_ZERO);
    }
    return gnc_get_dcex_node(nc, min_parent_port_ref + DCEXN_STR_OFFSET);
  }
  
  /* ok, we should have a gate that drives the net */
  if ( node_ref >= 0 )
  {
    return gnc_get_dcex_by_node(nc, cell_ref, node_ref);
  }
  
  gnc_Error(nc, "GNC2DCEX: Net not driven?");
  return NULL;
}

static dcexn gnc_get_dcex_by_node(gnc nc, int cell_ref, int node_ref)
{
  int node_cell_ref;
  int id;
  int port_ref;
  int net_ref;
  int join_ref;
  dcexn n;
  dcexn *np;
  node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
  
  gnc_Log(nc, 0, "Verify: Conversion of node %d (%s).", node_ref,
    gnc_GetCellNameStr(nc, node_cell_ref));
  
  id = gnc_GetCellId(nc, node_cell_ref);
  if ( id < 0 )
  {
    gnc_Error(nc, "GNC2DCEX: Node without valid id found (use gnc_FlattenCell()).");
    return NULL;
  }
  
  id = gnc_GetCellId(nc, node_cell_ref);
  id = translate_id(id);
  if ( id < 0 )
  {
    gnc_Error(nc, "GNC2DCEX: Can not translate node.");
    return NULL;
  }
  
  if ( is_inverted_gate(gnc_GetCellId(nc, node_cell_ref)) != 0 )
  {
    n = gnc_get_dcex_node(nc, DCEXN_NOT);
    if ( n == NULL )
      return NULL;
    n->down = gnc_get_dcex_node(nc, id);
    if ( n->down == NULL )
    {
      dcexnClose(n);
      return NULL;
    }
    np = &(n->down->down);
  }
  else
  {
    n = gnc_get_dcex_node(nc, id);
    np = &(n->down);
  }
  
  port_ref = -1;
  net_ref = -1;
  join_ref = -1;
  while( gnc_LoopCellNodeInputs(nc, cell_ref, node_ref, &port_ref, &net_ref, &join_ref) != 0 )
  {
    *np = gnc_get_dcex_by_net(nc, cell_ref, net_ref);
    if ( *np == NULL )
    {
      dcexnClose(n);
      return NULL;
    }
    np = &((*np)->next);
  }
  
  return n;
}


static dcexn gnc_get_dcex_by_port(gnc nc, int cell_ref, int port_ref)
{
  gcell cell;
  dcexn n;
  int net_ref;
  
  cell = gnc_GetGCELL(nc, cell_ref);
  
  if ( cell->pi == NULL )
  {
    gnc_Error(nc, "GNC2DCEX: Problem info strucutre required.");
    return NULL;
  }
  
  n = gnc_get_dcex_node(nc, DCEXN_ASSIGN);
  if ( n != NULL )
  {
    n->down = gnc_get_dcex_node(nc, port_ref + DCEXN_STR_OFFSET);
    if ( n->down != NULL )
    {
      net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, port_ref);
      if ( net_ref >= 0 )
      {
        if ( gnc_IsCellPortInverted(nc, cell_ref, port_ref) != 0 && 
             port_ref >= cell->pi->in_cnt &&
             port_ref < cell->pi->in_cnt + cell->register_width )
        {
          gnc_Log(nc, 1, "Verify: Inverted function at port %d (%s).", 
            port_ref,
            gnc_GetCellPortName(nc, cell_ref, port_ref)==NULL?"<unknown>":
            gnc_GetCellPortName(nc, cell_ref, port_ref));
          n->down->next = gnc_get_dcex_node(nc, DCEXN_NOT);
          if ( n->down->next != NULL )
          {
            n->down->next->down = gnc_get_dcex_by_net(nc, cell_ref, net_ref);
            if ( n->down->next->down != NULL )
              return n;
          }
        }
        else
        {     
          gnc_Log(nc, 1, "Verify: None-inverted function at port %d (%s).", 
            port_ref,
            gnc_GetCellPortName(nc, cell_ref, port_ref)==NULL?"<unknown>":
            gnc_GetCellPortName(nc, cell_ref, port_ref));
          n->down->next = gnc_get_dcex_by_net(nc, cell_ref, net_ref);
          if ( n->down->next != NULL )
            return n;
        }
      }
      else
      {
        gnc_Error(nc, "GNC2DCEX: Output port not connected.");
      }
    }
    dcexnClose(n);
  }
  return NULL;  
}

/* create a syntax tree from a netlist */
/* assumes that the netlist has been created from */
/* gnc_GetGCELL(nc, cell_ref)->pi and gnc_GetGCELL(nc, cell_ref)->cl_on */
static dcexn gnc_get_dcex_tree(gnc nc, int cell_ref)
{
  gcell cell;
  dcexn n;
  dcexn *np;
  int port_ref;

  cell = gnc_GetGCELL(nc, cell_ref);
  
  if ( cell->pi == NULL )
  {
    gnc_Error(nc, "GNC2DCEX: Problem info strucutre required.");
    return NULL;
  }
  
  n = gnc_get_dcex_node(nc, DCEXN_CMDLIST);
  if ( n == NULL )
    return NULL;
  
  np = &(n->down);
  
  for( port_ref = 0; port_ref < cell->pi->out_cnt; port_ref++ )
  {
    *np = gnc_get_dcex_by_port(nc, cell_ref, port_ref+cell->pi->in_cnt);
    if ( *np == NULL )
    {
      dcexnClose(n);
      return NULL;
    }
    np = &((*np)->next);
  }
  
  return n;
}

/* assumes, that dcex is empty */
static int gnc_build_dcex_str_refs(gnc nc, int cell_ref, dcex_type dcex)
{
  gcell cell;
  int i;
  char s[32];
  const char *t;

  cell = gnc_GetGCELL(nc, cell_ref);
  
  if ( cell->pi == NULL )
  {
    gnc_Error(nc, "GNC2DCEX: Problem info strucutre required.");
    return 0;
  }
  
  for( i = 0; i < cell->pi->in_cnt; i++ )
  {
    t = pinfoGetInLabel(cell->pi, i);
    if ( t == NULL )
    {
      sprintf(s, GPORT_IN_PREFIX "%d", i);
      t = s;
    }
    if ( dcexSetStrRef(dcex, i, i, t) == 0 )
      return 0;
    if ( b_sl_Set(dcex->in_variables, i, t) == 0 )
      return 0;
  }

  for( i = 0; i < cell->pi->out_cnt; i++ )
  {
    t = pinfoGetOutLabel(cell->pi, i);
    if ( t == NULL )
    {
      sprintf(s, GPORT_OUT_PREFIX "%d", i);
      t = s;
    }
    if ( dcexSetStrRef(dcex, cell->pi->in_cnt + i, i, t) == 0 )
      return 0;
    if ( b_sl_Set(dcex->out_variables, i, t) == 0 )
      return 0;
  }
  
  return 1;
}

static int gnc_do_verification(gnc nc, int cell_ref, dcex_type dcex, pinfo *pi, dclist cl)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  dcexn n;
  
  gnc_Log(nc, 3, "Verify: Building port list.");
  
  if ( gnc_build_dcex_str_refs(nc, cell_ref, dcex) == 0 )
  {
    gnc_Error(nc, "Verify: Building port list failed.");
    return 0;
  }

  gnc_Log(nc, 3, "Verify: Building syntax tree.");
    
  n = gnc_get_dcex_tree(nc, cell_ref);
  if ( n == NULL )
  {
    gnc_Error(nc, "Verify: Building syntax tree failed.");
    return 0;
  }

  gnc_Log(nc, 3, "Verify: SOP conversion.");

  n = dcexReduceNot(dcex, n);
  if ( n == NULL )
  {
    gnc_Error(nc, "Verify: SOP conversion failed.");
    return 0;
  }
  dcexMarkSide(dcex, n, 0);
  if ( dcexToDCL(dcex, pi, cl, n) == 0 )
  {
    gnc_Error(nc, "Verify: SOP conversion failed.");
    dcexnClose(n);
    return 0;
  }

  dcexnClose(n);

  gnc_Log(nc, 3, "Verify: Equivalence check.");
  
  if ( pi->in_cnt != cell->pi->in_cnt )
  {
    gnc_Error(nc, "Verify: Input signal mismatch.");
    return 0;
  }

  if ( pi->out_cnt != cell->pi->out_cnt )
  {
    gnc_Error(nc, "Verify: Output signal mismatch.");
    return 0;
  }
  
  if ( dclIsEquivalent(pi, cl, cell->cl_on) == 0 )
  {
    gnc_Error(nc, "Verify: Design verification failed. Please send a bug report.");
    gnc_LogDCL(nc, 4, "Verify: ", 2, "Specification", cell->pi, cell->cl_on, "Netlist", pi, cl);
    return 0;    
  }

  gnc_Log(nc, 3, "Verify: Successfully done.");
  
  return 1;
}

int gnc_DoVerification(gnc nc, int cell_ref)
{
  int ret;
  dcex_type dcex;
  pinfo pi;
  dclist cl;
  dcex = dcexOpen();
  if ( dcex != NULL )
  {
    if ( pinfoInit(&pi) != 0 )
    {
      if ( dclInit(&cl) != 0 )
      {
        ret = gnc_do_verification(nc, cell_ref, dcex, &pi, cl);
        dclDestroy(cl);
      }
      pinfoDestroy(&pi);
    }
    dcexClose(dcex);
  }
   
  return 1;
  return ret;
}

