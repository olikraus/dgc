/*

  syfsm.c
  
  synthesis of finite state machines

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
#include "fsmenc.h"
#include "mwc.h"

/*

  register_width = 4
  pi->in_cnt = 10
  pi->out_cnt = 7

  the fsm cell has 
    6 inputs (pi->in_cnt - register_width)
    11 outputs (pi->out_cnt + register_width)
  sum: 17
  
  the pinfo structure has
    10 inputs (pi->in_cnt) and
    7 outputs (pi->out_cnt)
  sum: 17
    

  ???

                  i0                         
                  i1                            
                  i2                          
                  i3                            
                  i4                            
                  i5          --------          
               /  o0  ------ + y0  x0 + ------  o4   \
        z^n   |   o1  ------ + y1  x1 + ------  o5    |   z^n+1
              |   o2  ------ + y2  x2 + ------  o6    |
               \  o3  ------ + y3  x3 + ------  o7   /
                              --------          o8   \
                                                o9    |   real outputs
                                                o10  /


*/

/* --- create register --- */

/* assumes, that  gnc_UpdateCellPortsWithPinfo has been called */
/* with the same 'register_width' */

/* returns node_ref */
int gnc_synth_generic_register_or_door(gnc nc, int cell_ref, int register_width, int id)
{
  int register_cell_ref;
  int register_node_ref;
  pinfo *pi;
  int i;
  char in_name[16];
  char out_name[16];
  int node_in_port_ref;
  int node_out_port_ref;
  int in_net_ref;
  int out_net_ref;
  
  register_cell_ref = gnc_GetGenericCell(nc, id, register_width);
  if ( register_cell_ref < 0 )
    return -1;
    
  pi = gnc_GetGCELL(nc, cell_ref)->pi;
  if ( pi == NULL )
  {
    return -1;
  }
  
  register_node_ref = gnc_AddCellNode(nc, cell_ref, NULL, register_cell_ref);
  if ( register_node_ref < 0 )
    return -1;

  /* ignore the cell for optimization */
  /* This is done to get a more readable design and to */
  /* avoid problems for the longest path calculation. */
  /* this statement has no effect at the moment */
  gnc_SetCellNodeDoNotTouch(nc, cell_ref, register_node_ref);
    
  for( i = 0; i < register_width; i++ )
  {
    /* create the name's of the ports of the generic register */
    sprintf(in_name, GPORT_IN_PREFIX "%d", i);
    sprintf(out_name, GPORT_OUT_PREFIX "%d", i);

    /* fetch the port refs */
    node_in_port_ref = gnc_FindCellPort(nc, register_cell_ref, in_name);
    node_out_port_ref = gnc_FindCellPort(nc, register_cell_ref, out_name);
    if ( node_in_port_ref < 0 || node_out_port_ref < 0 )
    {
      gnc_Error(nc, "gnc_synth_generic_register_or_door: Illegal port names (???).");
      return -1;
    }

    /* get the parents net */
    in_net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, pi->in_cnt+i, 0, 1);
    out_net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, pi->in_cnt-register_width+i, 0, 1);
    
    /* add the ports of the register */
    if ( gnc_AddCellNetJoin(nc, cell_ref, in_net_ref, register_node_ref, node_in_port_ref, 0) < 0 )
      return -1;
    if ( gnc_AddCellNetJoin(nc, cell_ref, out_net_ref, register_node_ref, node_out_port_ref, 0) < 0 )
      return -1;
  }
  
  return register_node_ref;
}


/* assumes, that  gnc_UpdateCellPortsWithPinfo has been called */
/* with the same 'register_width' */

/* returns node_ref */
int gnc_SynthGenericRegister(gnc nc, int cell_ref, int register_width)
{
  return gnc_synth_generic_register_or_door(nc, cell_ref, register_width, 
      GCELL_ID_G_REGISTER);
}

/* 
  assumes, that  gnc_UpdateCellPortsWithPinfo has been called
  with the same 'register_width'
  
  returns node_ref 
  
  this function is also used by the xbm synthesis procedures
  
*/
int gnc_SynthGenericDoor(gnc nc, int cell_ref, int register_width)
{
  return gnc_synth_generic_register_or_door(nc, cell_ref, register_width, 
      GCELL_ID_G_DOOR);
}


/* assumes, that  gnc_UpdateCellPortsWithPinfo has been called */
/* with the same 'register_width' */

/* 
  description:
    Synchronous and asynchronous state machines have the same
    interface, but for an asynchronous machine, there is no
    real z^n and z^n-1. Both lines are the same. Indeed: The
    DFF's had to be replaced with a simple wire. This is what
    this function does: Create wire connections between z^n and z^n-1
    instead of DFF's.
  returns: 0 for error.
  
  probably, this function is obsolete, because there should be
  a reset register between these two lines
*/
int gnc_SynthEmptyRegister(gnc nc, int cell_ref, int register_width)
{
  pinfo *pi;
  int i;
  int in_net_ref;
  int out_net_ref;
  
  pi = gnc_GetGCELL(nc, cell_ref)->pi;
  if ( pi == NULL )
  {
    gnc_Error(nc, "gnc_SynthGenericRegister: Unspecified problem info (???).");
    return 0;
  }

  for( i = 0; i < register_width; i++ )
  {
    /* get the parents net */
    in_net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, pi->in_cnt+i, 0, 1);
    out_net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, pi->in_cnt-register_width+i, 0, 1);

    /* create a connection between the two nets */
    if ( gnc_MergeNets(nc, cell_ref, in_net_ref, out_net_ref) == 0 )
      return 0;
  }
  
  return 1;
}

/* returns 0 for error */
/* return !=0 if ports do not match */
/* return !=0 if a connection has been created */
/* 
  p_fn:  parent function
  ff_fn: D-FF function
  
  usually both functions are the same, e.g.
    p_fn == GPORT_FN_CLR and ff_fn == GPORT_FN_CLR
    
  but if the reset code is a 1 at a bit, than this might be different
    p_fn == GPORT_FN_CLR and ff_fn == GPORT_FN_SET
  
*/
int gnc_ConnectFFControlPort(gnc nc, int cell_ref, int node_ref, int p_fn, int ff_fn)
{
  int ff_cell_ref;
  int parent_port_ref;
  int ff_port_ref;
  int net_ref;
  int is_neg;
  ff_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);

  /* get the port ref's that have the requested type */  
  parent_port_ref = gnc_FindCellPortByFn(nc, cell_ref, p_fn);
  ff_port_ref = gnc_FindCellPortByFn(nc, ff_cell_ref, ff_fn);
  
  /* we can only connect if the type of the ports match */
  if ( parent_port_ref < 0 || ff_port_ref < 0 )
    return 1;
    
  /* Check, if one or both of the ports are inverted */
  is_neg = 0;
  if ( gnc_IsCellPortInverted(nc, cell_ref, parent_port_ref) != 0 )
    is_neg ^= 1;
  if ( gnc_IsCellPortInverted(nc, ff_cell_ref, ff_port_ref) != 0 )
    is_neg ^= 1;

  /* get the net with the parent's port */
  net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, parent_port_ref, is_neg, 0);

  if ( gnc_AddCellNetJoin(nc, cell_ref, net_ref, node_ref, ff_port_ref, 0) < 0 )
    return 0;

  return 1;
}

/* returns 0 for error, node_ref is a FF node */
int gnc_ConnectFFControls(gnc nc, int cell_ref, int node_ref, int ff_fn)
{
  if ( gnc_ConnectFFControlPort(nc, cell_ref, node_ref, GPORT_FN_CLK, GPORT_FN_CLK) == 0 )
    return 0;
  if ( ff_fn == GPORT_FN_CLR || ff_fn == GPORT_FN_SET )
    if ( gnc_ConnectFFControlPort(nc, cell_ref, node_ref, GPORT_FN_CLR, ff_fn) == 0 )
      return 0;
  return 1;
}

#define SYFSM_DFF_NO_RESET 0
#define SYFSM_DFF_RESET_LOW 1
#define SYFSM_DFF_RESET_HIGH 2

/* returns cell id for a suitable FF cell */
int gnc_GetFFCellId(gnc nc, int type)
{
  static const int ids0[] = {GCELL_ID_DFF_Q_NQ, GCELL_ID_DFF_Q, -1};
  static const int ids1[] = {GCELL_ID_DFF_Q_NQ_LC, GCELL_ID_DFF_Q_NQ_HC, 
                GCELL_ID_DFF_Q_LC, GCELL_ID_DFF_Q_HC, -1};
  static const int ids2[] = {GCELL_ID_DFF_Q_NQ_LS, GCELL_ID_DFF_Q_NQ_HS, 
                GCELL_ID_DFF_Q_LS, GCELL_ID_DFF_Q_HS, -1};
  const int *ids;
  int i;
  int cell_ref;
  
  switch(type)
  {
    case SYFSM_DFF_NO_RESET:
      ids = ids0;
      break;
    case SYFSM_DFF_RESET_LOW:
      ids = ids1;
      break;
    case SYFSM_DFF_RESET_HIGH:
      ids = ids2;
      break;
    default:
      return -1;
  }
  
  i = 0;  
  while( ids[i] >= 0 )
  {
    cell_ref = gnc_GetCellById(nc, ids[i]);
    if ( cell_ref >= 0 )
      return ids[i];
    i++;
  }
  return -1;
}

/* returns node_ref */
int gnc_SynthDFF(gnc nc, int cell_ref, int id, int in_net, int out_net)
{
  int ff_cell_ref = gnc_GetCellById(nc, id);
  int ff_reset_fn;
  int port_ref;
  int node_ref;
  
  assert(ff_cell_ref >= 0);   /* should have been checked */
  
  gnc_Log(nc, 0, "Synthesis: Using cell D-FF '%s'.", 
    gnc_GetCellNameStr(nc, ff_cell_ref));
  
  /* create a new DFF */
  node_ref = gnc_AddCellNode(nc, cell_ref, NULL, ff_cell_ref);
  if ( node_ref < 0 )
    return -1;
  
  ff_reset_fn = GPORT_FN_LOGIC;  /* no set/reset input */
  if ( id == GCELL_ID_DFF_Q_LC || id == GCELL_ID_DFF_Q_HC ||
       id == GCELL_ID_DFF_Q_NQ_LC || id == GCELL_ID_DFF_Q_NQ_HC )
    ff_reset_fn = GPORT_FN_CLR;  
  if ( id == GCELL_ID_DFF_Q_LS || id == GCELL_ID_DFF_Q_HS ||
       id == GCELL_ID_DFF_Q_NQ_LS || id == GCELL_ID_DFF_Q_NQ_HS )
    ff_reset_fn = GPORT_FN_SET;  
  
  
  /* find the data port */
  port_ref = gnc_FindCellPortByFn(nc, ff_cell_ref, GPORT_FN_D);
  if ( port_ref < 0 )
  {
    gnc_Error(nc, "gnc_AddDFF: GPORT_FN_D port not found (???).");
    return -1;
  }
  
  /* connect the data port to the input net */
  if ( gnc_AddCellNetJoin(nc, cell_ref, in_net, node_ref, port_ref, 0) < 0 )
    return -1;
  
  /* find DFF output port */
  port_ref = gnc_FindCellPortByFn(nc, ff_cell_ref, GPORT_FN_Q);
  if ( port_ref < 0 )
  {
    gnc_Error(nc, "gnc_AddDFF: GPORT_FN_Q port not found (???).");
    return -1;
  }
  
  /* connect the DFF output port to the output net */
  if ( gnc_AddCellNetJoin(nc, cell_ref, out_net, node_ref, port_ref, 0) < 0 )
    return -1;
  
  /* connect the remaining ports of the DFF */
  if ( gnc_ConnectFFControls(nc, cell_ref, node_ref, ff_reset_fn) == 0 )
    return -1;
    
  return node_ref;
}

/*
  called by 
    sygate.c:gnc_SynthGenericNodeToLib()
*/
int gnc_SynthGenericRegisterToLib(gnc nc, int cell_ref, int register_node_ref)
{
  int register_cell_ref;
  int dff_type;
  int ff_cell_id;
  int clr_port_ref;
  int register_width;
  char in_name[16];
  char out_name[16];
  int node_in_port_ref;
  int node_out_port_ref;
  int in_net_ref;
  int out_net_ref;
  int i;
  
  
  register_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, register_node_ref);
  if ( gnc_GetCellId(nc, register_cell_ref) != GCELL_ID_G_REGISTER )
    return 0;
  register_width = gnc_GetCellPortCnt(nc, register_cell_ref);
  assert((register_width&1)==0);
  register_width = register_width / 2;

  /* clr actually means reset line. */
  clr_port_ref = gnc_FindCellPortByFn(nc, cell_ref, GPORT_FN_CLR);
  
  for( i = 0; i < register_width; i++ )
  {
    /* create the name's of the ports of the generic register */
    sprintf(in_name, GPORT_IN_PREFIX "%d", i);
    sprintf(out_name, GPORT_OUT_PREFIX "%d", i);

    /* fetch the port refs */
    node_in_port_ref = gnc_FindCellPort(nc, register_cell_ref, in_name);
    node_out_port_ref = gnc_FindCellPort(nc, register_cell_ref, out_name);
    if ( node_in_port_ref < 0 || node_out_port_ref < 0 )
    {
      gnc_Error(nc, "gnc_SynthGenericRegisterToLib: Illegal port names (???).");
      return 0;
    }
    
    /* get out and input net refs - there might be an easier way to do this */
    in_net_ref = gnc_FindCellNetByPort(nc, cell_ref, register_node_ref, node_in_port_ref);
    out_net_ref = gnc_FindCellNetByPort(nc, cell_ref, register_node_ref, node_out_port_ref);

    if ( clr_port_ref < 0 )
    {
      dff_type = SYFSM_DFF_NO_RESET;
    }
    else
    {
      if ( gnc_IsCellPortInverted(nc, register_cell_ref, node_in_port_ref) == 0 )
      {
        dff_type = SYFSM_DFF_RESET_LOW;
        gnc_Log(nc, 1, "Synthesis: Reset state of D-FF %d is 0.", i);
      }
      else
      {
        dff_type = SYFSM_DFF_RESET_HIGH;
        gnc_Log(nc, 1, "Synthesis: Reset state of D-FF %d is 1.", i);
      }
    }
    
    ff_cell_id = gnc_GetFFCellId(nc, dff_type);
    if ( ff_cell_id < 0 )
    {
      gnc_Error(nc, "Synthesis: No suitable FF with set or reset was found in the library.");
      return 0;
    }
    
    if ( gnc_SynthDFF(nc, cell_ref, ff_cell_id, in_net_ref, out_net_ref) < 0 )
      return 0;
  }

  gnc_DelCellNode(nc, cell_ref, register_node_ref);

  return 1;
}

/* --- tech mapping: door register --- */

/*
  fn: one of GPORT_FN_CLR or GPORT_FN_SET
  is_data_inv: whether the door cell requires inverted input.
*/
/* returns node_ref */
int gnc_SynthDoor(gnc nc, int cell_ref, int door_cell_ref, int in_net, int out_net, 
  int fn, int is_ctl_inv, int is_data_inv)
{
  int port_ref;
  int door_node_ref;
  int net_ref;
  
  /* create a new and gate */
  door_node_ref = gnc_AddCellNode(nc, cell_ref, NULL, door_cell_ref);
  if ( door_node_ref < 0 )
    return -1;
    
  /* ignore the cell for optimization */
  /* This is done to get a more readable design and to   */
  /* avoid problems for the longest path calculation.    */
  /* Removing this statement makes the cycle break fail. */
  gnc_SetCellNodeDoNotTouch(nc, cell_ref, door_node_ref);
  
  /* find the data port */
  port_ref = gnc_GetNthPort(nc, door_cell_ref, 0, GPORT_TYPE_IN);
  if ( port_ref < 0 )
  {
    gnc_Error(nc, "gnc_GetNthPort: input port 0 not found (???).");
    return -1;
  }
  
  /* maybe the data must be inverted */
  if ( is_data_inv == 0 )
  {
    /* thats the easier case: */
    /* connect the data port to the input net */
    gnc_Log(nc, 0, "Synthesis: Data not inverted, no additional inverter required.");
    if ( gnc_AddCellNetJoin(nc, cell_ref, in_net, door_node_ref, port_ref, 0) < 0 )
      return -1;
  }
  else
  {
    int inv_cell_ref = gnc_GetCellById(nc, GCELL_ID_INV);
    int inv_node_ref;
    int inv_in_port_ref;
    int inv_out_port_ref;
    int parent_port_ref = -1;
    int parent_join_ref;

    gnc_Log(nc, 0, "Synthesis: Data must be inverted, inverter required (Input net %d).",
      in_net);
    
    /* create an inverter node */
    if ( inv_cell_ref < 0 )
      return gnc_Error(nc, "Synthesis: Actual door requires data inverter."), -1;
    inv_node_ref = gnc_AddCellNode(nc, cell_ref, NULL, inv_cell_ref);
    if ( inv_node_ref < 0 )
      return -1;
    inv_in_port_ref = gnc_GetNthPort(nc, inv_cell_ref, 0, GPORT_TYPE_IN);
    inv_out_port_ref = gnc_GetNthPort(nc, inv_cell_ref, 0, GPORT_TYPE_OUT);
    assert(inv_in_port_ref >= 0 && inv_out_port_ref >= 0);
    
    /* There is another little problem: The delay calculation  */
    /* requires a exactly one node between the z and zz ports  */
    /* of the parent. We had to ensure, that this is still the */
    /* case. So we remove the parent connection first...       */
    parent_join_ref = gnc_FindCellNetJoin(nc, cell_ref, in_net, -1, -1);
    /* also check if in_net is valid */
    assert( gnc_FindCellNetJoinByDriver(nc, cell_ref, in_net) >= 0 );
    
    /* could there be more than one parent port? I do not think so! */
    
    if ( parent_join_ref >= 0 )
    {
      parent_port_ref = gnc_GetCellNetPort(nc, cell_ref, in_net, parent_join_ref);
      /* remember to rename gnc_DelCellNetPort to gnc_DelCellNetJoin */
      gnc_DelCellNetJoin(nc, cell_ref, in_net, parent_join_ref);
    }
    else
    {
      gnc_Log(nc, 0, "Synthesis: Parent not found...");
      /* something is wrong... but what should we do? */
      /* output reset should be created (gnc_SynthXBMOutputReset) ... */
    }
    
    /* connect the input of the inverter with the input net */
    if ( gnc_AddCellNetJoin(nc, cell_ref, in_net, inv_node_ref, inv_in_port_ref, 0) < 0 )
      return -1;
    /* now in_net should be valid again */
    assert( gnc_FindCellNetJoinByDriver(nc, cell_ref, in_net) >= 0 );
      
    /* create a new input net (that will be inverted) */
    in_net = gnc_AddCellNet(nc, cell_ref, NULL);
    if ( in_net < 0 )
      return -1;
    
    /* connect this new net to the output of the inverter */
    if ( gnc_AddCellNetJoin(nc, cell_ref, in_net, inv_node_ref, inv_out_port_ref, 0) < 0 )
      return -1;

    /* connect the previously remembered parent join to the net */    
    if ( parent_join_ref >= 0 )
      if ( gnc_AddCellNetJoin(nc, cell_ref, in_net, -1, parent_port_ref, 0) < 0 )
        return -1;
      
    /* and the at last, connect other and of the net to data port of the door */
    if ( gnc_AddCellNetJoin(nc, cell_ref, in_net, door_node_ref, port_ref, 0) < 0 )
      return -1;
  }
  
  /* find the output port */
  port_ref = gnc_GetNthPort(nc, door_cell_ref, 0, GPORT_TYPE_OUT);
  if ( port_ref < 0 )
  {
    gnc_Error(nc, "gnc_GetNthPort: output port not found (???).");
    return -1;
  }
  
  /* connect the door output port to the output net */
  if ( gnc_AddCellNetJoin(nc, cell_ref, out_net, door_node_ref, port_ref, 0) < 0 )
    return -1;

  /* find the control port, if the function is valid */
  if ( fn != GPORT_FN_NONE )
  {
    int parent_port_ref;
    port_ref = gnc_GetNthPort(nc, door_cell_ref, 1, GPORT_TYPE_IN);
    if ( port_ref < 0 )
    {
      gnc_Error(nc, "gnc_GetNthPort: input port 1 (control line) not found (???).");
      return -1;
    }

    /* fn is GPORT_FN_CLR or GPORT_FN_SET */
    parent_port_ref = gnc_FindCellPortByFn(nc, cell_ref, fn);
    if ( parent_port_ref < 0 )
      return 0;

    if ( gnc_IsCellPortInverted(nc, cell_ref, parent_port_ref) != 0 )
    {
      gnc_Log(nc, 0, "Synthesis: Low active reset.");
      is_ctl_inv ^= 1;
    }
    else
    {
      gnc_Log(nc, 0, "Synthesis: High active reset.");
    }

    gnc_Log(nc, 0, "Synthesis: Control line is %s for door node %d.", 
      is_ctl_inv==0?"not inverted":"inverted", door_node_ref );

    /* get the net with the parent's control port */
    net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, parent_port_ref, is_ctl_inv, 0);

    /* connect the set/reset line of the parent with the control */
    /* input port of the door */
    if ( gnc_AddCellNetJoin(nc, cell_ref, net_ref, door_node_ref, port_ref, 0) < 0 )
      return -1;
  }
  
  return door_node_ref;
}


/*
  called by 
    sygate.c:gnc_SynthGenericNodeToLib()
    
    
  parent control line (CTL)
    GPORT_FN_CLR:           zero on all states
      is_inverted == 0      aktive high       1 --> 0,  0 --> /x  NOR
      is_inverted == 1      aktive low        0 --> 0,  1 --> x   AND

    GPORT_FN_SET:           one on all states
      is_inverted == 0      aktive high       1 --> 1,  0 --> x   OR
      is_inverted == 1      aktive low        0 --> 1,  1 --> /x  NAND


              +-----+
           ---+ CTL +o----+    +-------+
              +-----+     +----+       !
                               ! DOOR  +----
              +-----+     +----+       !
           ---+DATA +o----+    +-------+
              +-----+     
    
    function  inverted  DOOR  CTL   DATA
    CLR       no        NOR   -     INV
    CLR       yes       NOR   INV   INV
    CLR       no        AND   INV   -
    CLR       yes       AND   -     -
    SET       no        OR    -     -
    SET       yes       OR    INV   -
    SET       no        NAND  INV   INV
    SET       yes       NAND  -     INV
    
    
    The table can be simplified if we assume 'none-inverted'
    functions:
    
    function  inverted  DOOR  CTL   DATA
    CLR       no        NOR   -     INV
    CLR       no        AND   INV   -
    SET       no        OR    -     -
    SET       no        NAND  INV   INV
    
    If the function is inverted, simply invert the ctl input again.
    Indeed this is done in gnc_SynthDoor() if required.
            
            
    gnc_SynthDoor() will build an inverter for the data 
    before the door (depends on is_data_inv). 

              +-----+
    control---+ CTL +-----+
              +-----+     !
                          !
                          !
                          !    +-------+
                          +----+       !
              optional         ! DOOR  +----+---
              +-----+     +----+       !    !
      data ---+ INV +o----+    +-------+    !
              +-----+     !                 !
                          !                 !
                         zoX               ziX 
                         
    zoX and ziX are the parent ports (feed-back line X)
    
*/
int gnc_SynthGenericDoorToLib(gnc nc, int cell_ref, int register_node_ref)
{
  int register_cell_ref;
  int door_cell_ref = -1;
  int alt_door_cell_ref = -1;
  int register_width;
  char in_name[16];
  char out_name[16];
  int node_in_port_ref;
  int node_out_port_ref;
  int in_net_ref;
  int out_net_ref;
  int i;
  int is_ctl_inv = 0;
  int is_data_inv = 0;
  int fn = GPORT_FN_CLR;
  int clr_port_ref;

  register_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, register_node_ref);
  if ( gnc_GetCellId(nc, register_cell_ref) != GCELL_ID_G_DOOR )
    return 0;
    
  /* find the port_ref of the control line of the design */

  /* clr actually means reset line. */
  clr_port_ref = gnc_FindCellPortByFn(nc, cell_ref, GPORT_FN_CLR);
  

  /* replace all generic door elements */
  
  register_width = gnc_GetCellPortCnt(nc, register_cell_ref);
  assert((register_width&1)==0);
  register_width = register_width / 2;
  
  for( i = 0; i < register_width; i++ )
  {
    /* create the name's of the ports of the generic register */
    sprintf(in_name, GPORT_IN_PREFIX "%d", i);
    sprintf(out_name, GPORT_OUT_PREFIX "%d", i);

    /* fetch the port refs */
    node_in_port_ref = gnc_FindCellPort(nc, register_cell_ref, in_name);
    node_out_port_ref = gnc_FindCellPort(nc, register_cell_ref, out_name);
    if ( node_in_port_ref < 0 || node_out_port_ref < 0 )
    {
      gnc_Error(nc, "gnc_SynthGenericDoorToLib: Illegal port names (???).");
      return 0;
    }
    
    /* get out and input net refs - there might be an easier way to do this */
    in_net_ref = gnc_FindCellNetByPort(nc, cell_ref, register_node_ref, node_in_port_ref);
    out_net_ref = gnc_FindCellNetByPort(nc, cell_ref, register_node_ref, node_out_port_ref);
    


    if ( clr_port_ref < 0 )
    {

      /* no reset line */
    
      fn = GPORT_FN_NONE;
    
      is_ctl_inv = 0;
      is_data_inv = 0;
      door_cell_ref = gnc_GetCellById(nc, GCELL_ID_EMPTY);
      alt_door_cell_ref = gnc_GetCellById(nc, GCELL_ID_EMPTY);
    }
    else 
    {
      
      /* there is a reset line */
    
      if ( gnc_IsCellPortInverted(nc, register_cell_ref, node_in_port_ref) == 0 )
      {
        fn = GPORT_FN_CLR;

        is_ctl_inv = !0;
        is_data_inv = 0;
        door_cell_ref = gnc_GetCellById(nc, GCELL_ID_AND2);
        alt_door_cell_ref = gnc_GetCellById(nc, GCELL_ID_NOR2);
        
        gnc_Log(nc, 1, "Synthesis: Reset state of door %d is 0.", i);
        
      }
      else
      {
        fn = GPORT_FN_CLR;

        is_ctl_inv = 0;
        is_data_inv = 0;
        door_cell_ref = gnc_GetCellById(nc, GCELL_ID_OR2);
        alt_door_cell_ref = gnc_GetCellById(nc, GCELL_ID_NAND2);

        gnc_Log(nc, 1, "Synthesis: Reset state of door %d is 1.", i);
      }
    }

  
    /* if none of the two cells exist, we must stop here */
  
    if ( door_cell_ref < 0 && alt_door_cell_ref < 0 )
    {
      if ( clr_port_ref >= 0 )
        gnc_Error(nc, "Synthesis: No suitable door gate found (AND2 or NOR2).");
      else
        gnc_Error(nc, "Synthesis: No suitable door gate found (OR2 or NAND2).");
      return 0;
    }

    /* If the prefered cell does not exist, use the other one */
    /* do not forget to invert the lines */

    if ( door_cell_ref < 0 )
    {
      door_cell_ref = alt_door_cell_ref;
      is_ctl_inv = !is_ctl_inv;
      is_data_inv = !is_data_inv;
    }

    if ( gnc_GetCellName(nc, door_cell_ref) != NULL )
    {
      gnc_Log(nc, 3, "Synthesis: Door gate is '%s' (feedback line %sinverted, control line %sinverted).", 
        gnc_GetCellName(nc, door_cell_ref),
        is_data_inv==0?"not ":"",
        is_ctl_inv==0?"not ":""
        );
    }
  
    /* An inverted data line causes an inverted output on the */
    /* output ports of the next step function. Usually this does */
    /* not cause any problems... ok yes, it should be documented */
    /* But there is also another problem, the verification algorithm */
    /* will fail, if we do not tell the verification procedure, that */
    /* the next step fn is inverted, so this is done here: */

    {
      int join_ref = -1;
      while( gnc_LoopCellNetJoin(nc, cell_ref, in_net_ref, &join_ref) != 0 )
      {
        if ( gnc_GetCellNetNode(nc, cell_ref, in_net_ref, join_ref) == -1 )
        {
          gnc_SetCellPortFn(nc, cell_ref, 
            gnc_GetCellNetPort(nc, cell_ref, in_net_ref, join_ref),  /* port of the parent */
            GPORT_FN_LOGIC, is_data_inv);                         /* inverted or not inverted */
        }
      }
    }

    /* I see that this is a little bit complicated and not very easy */
    /* to understand. But on the other hand, I do not see a way to   */
    /* avoid this problem: At the end, it is caused by the requirement */
    /* that there must be only one gate between two state ports. */
    /* This is required by the longest path procedure, which must */
    /* break the cycles. At the end, I should write a smarter longest */
    /* path calculation, which will allow more than one element. */
  
  
    if ( door_cell_ref >= 0 )
    {
      if ( gnc_SynthDoor(nc, cell_ref, door_cell_ref, in_net_ref, out_net_ref, fn, is_ctl_inv, is_data_inv) < 0 )
        return 0;
    }
    else
    {
      /* 18 okt 2001: merging the net does not help, because */
      /* the cycle analysis requires a physical door element */
      /*
      if ( gnc_MergeNets(nc, cell_ref, in_net_ref, out_net_ref) == 0 )
        return 0;
      */
      /* after all, this branch must never be reached */
      gnc_Error(nc, "Synthesis: No physical door found.");
      return 0;
    }
  }

  gnc_DelCellNode(nc, cell_ref, register_node_ref);

  return 1;
}

/* --- FSM synthesis --- */

/*

  pi/cl_on: transfer function of the state machine
    This function contains also the output values.
    pi->in_cnt == inputs+register_width
    pi->out_cnt == register_width+outputs
  
  register_width: no of bit's to store one state
  
  
  reset_fn: GPORT_FN_CLR, GPORT_FN_SET or -1
  reset_is_neg: 0 clear or set on high, 1 clear or set on low

  Precondition:
    - The cell must not contain any ports
 
  This function will
    - copy pi, cl_on, cl_dc to the internal structures
    - create input, output, state and reset ports for the cell
    - apply names to the ports, according to the information of 'pi'
    
  This function is also used by the XBM synthesis procedures.
 
*/
int gnc_SetFSMDCLWithoutClock(gnc nc, int cell_ref, pinfo *pi, dclist cl_on, 
  int register_width, int reset_fn, int reset_is_neg)
{
  int port_ref;
  
  if ( gnc_SetDCL(nc, cell_ref, pi, cl_on, NULL, register_width) == 0 )
    return 0;
  
  switch( reset_fn )
  {
    case GPORT_FN_CLR:
      gnc_Log(nc, 3, "Synthesis: Add %s CLR line.", reset_is_neg==0?"active high":"active low" );
      port_ref = gnc_AddCellPort(nc, cell_ref, GPORT_TYPE_IN, nc->name_clr);
      if ( port_ref < 0 )
        return 0;
      gnc_SetCellPortFn(nc, cell_ref, port_ref, GPORT_FN_CLR, reset_is_neg);
      break;
    case GPORT_FN_SET:
      gnc_Log(nc, 3, "Synthesis: Add %s SET line.", reset_is_neg==0?"active high":"active low" );
      port_ref = gnc_AddCellPort(nc, cell_ref, GPORT_TYPE_IN, nc->name_set);
      if ( port_ref < 0 )
        return 0;
      gnc_SetCellPortFn(nc, cell_ref, port_ref, GPORT_FN_SET, reset_is_neg);
      break;
  }

  /* setting the register-width is done in SetDCL */
  assert(gnc_GetGCELL(nc, cell_ref)->register_width == register_width);

  return 1;
}

/*
  pi/cl_on: transfer function of the state machine
    This function contains also the output values.
    pi->in_cnt == inputs+register_width
    pi->out_cnt == register_width+outputs
  
  register_width: no of bit's to store one state
  
  
  reset_fn: GPORT_FN_CLR, GPORT_FN_SET or -1
  reset_is_neg: 0 clear or set on high, 1 clear or set on low

  Precondition:
    - The cell must not contain any ports
 
  This function will
    - copy pi, cl_on, cl_dc to the internal structures
    - create input, output, state, reset and clock ports for the cell
    - apply names to the ports, according to the information of 'pi'
*/
int gnc_SetFSMDCLWithClock(gnc nc, int cell_ref, pinfo *pi, dclist cl_on, 
  int register_width, int reset_fn, int reset_is_neg)
{
  int port_ref;
  
  if ( gnc_SetFSMDCLWithoutClock(nc, cell_ref, pi, cl_on, 
            register_width, reset_fn, reset_is_neg) == 0 )
    return 0;
    
  port_ref = gnc_AddCellPort(nc, cell_ref, GPORT_TYPE_IN, nc->name_clk);
  if ( port_ref < 0 )
    return 0;
  gnc_SetCellPortFn(nc, cell_ref, port_ref, GPORT_FN_CLK, 0);
  return 1;
}

static void gnc_fsm_log_fn(void *data, int ll, char *fmt, va_list va)
{
  gnc_LogVA((gnc)data, ll, fmt, va);
}

/* returns cell_ref */
/* synth_opt = GNC_SYNTH_OPT_DEFAULT */
/* fsm_opt = GNC_FSM_CLR_HIGH | GNC_FSM_CLR_LOW */
int gnc_SynthClockedFSM(gnc nc, fsm_type fsm, const char *cell_name, const char *lib_name, int hl_opt, int synth_opt)
{
  int cell_ref;
  int reset_is_neg;
  int reset_fn;
  int encode;
  
  switch(hl_opt&7)
  {
    case GNC_HL_OPT_CLR_HIGH:  reset_fn = GPORT_FN_CLR;  reset_is_neg = 0; break;
    case GNC_HL_OPT_CLR_LOW:   reset_fn = GPORT_FN_CLR;  reset_is_neg = 1; break;
    case GNC_HL_OPT_NO_RESET:  reset_fn = -1;            reset_is_neg = 0; break;
    default:
      gnc_Error(nc, "Spezified FSM synthesis options are not valid.");
      return -1;
  } 

  fsm_PushLogFn(fsm, gnc_fsm_log_fn, nc, 2);

  gnc_Log(nc, 5, "Clocked FSM (%s): Checking FSM.", cell_name);
  
  /*
  if ( fsm_IsValid(fsm) == 0 )
  {
    gnc_Error(nc, "gnc_SynthClockedFSM: FSM not valid.");
    return fsm_PopLogFn(fsm), -1;
  }
  */


  /* this will create the an all zero state code for the reset */
  /* so we must use GPORT_FN_CLR later */
  
  gnc_Log(nc, 5, "Clocked FSM (%s): Building control function.", cell_name);


  switch(hl_opt&GNC_HL_OPT_ENC_MASK)
  {
    case GNC_HL_OPT_ENC_FAN_IN:  encode = FSM_ENCODE_FAN_IN; break;
    case GNC_HL_OPT_ENC_IC_ALL:  encode = FSM_ENCODE_IC_ALL; break;
    case GNC_HL_OPT_ENC_IC_PART: encode = FSM_ENCODE_IC_PART; break;
    case GNC_HL_OPT_ENC_SIMPLE:  encode = FSM_ENCODE_SIMPLE; break;
    default:                     encode = FSM_ENCODE_SIMPLE; break;
  }
    
  if ( (hl_opt & GNC_HL_OPT_MIN_STATE) != 0 || 
       (hl_opt & GNC_HL_OPT_OLD_MIN_STATE) != 0 )
  {
    int is_old = (hl_opt & GNC_HL_OPT_OLD_MIN_STATE) != 0 ? 1 : 0;
    
    gnc_Log(nc, 5, "Clocked FSM (%s): Minimizing states.", cell_name);
    if( fsm_MinimizeStates(fsm, 0, is_old) == 0 )
      return 0;
    if ( fsm_GetGroupCnt(fsm) > 0 )
    {
      if ( fsm_GetNodeCnt(fsm) > fsm_GetGroupCnt(fsm) )
        gnc_Log(nc, 5, "Clocked FSM (%s): Reduced states from %d to %d.", 
          cell_name, fsm_GetNodeCnt(fsm), fsm_GetGroupCnt(fsm));
      else
        gnc_Log(nc, 5, "Clocked FSM (%s): No state reduction possible (%d states).", 
          cell_name, fsm_GetNodeCnt(fsm));
    }
  }

  if ( fsm_BuildClockedMachine(fsm, encode) == 0 )
  {
    gnc_Error(nc, "gnc_SynthClockedFSM: Can not create control function.");
    return fsm_PopLogFn(fsm), -1;
  }

  fsm_PopLogFn(fsm);

  gnc_Log(nc, 5, "Clocked FSM (%s): Synthesis started.", cell_name);
  
  cell_ref = gnc_AddCell(nc, cell_name, lib_name);
  if ( cell_ref < 0 )
    return -1;
  
  /* use GPORT_FN_CLR as reset function, because the reset state has all zero */
  if ( gnc_SetFSMDCLWithClock(nc, cell_ref, 
                fsm->pi_machine, 
                fsm->cl_machine, 
                fsm_GetCodeWidth(fsm), 
                reset_fn, reset_is_neg) == 0  )
    return gnc_DelCell(nc, cell_ref), -1;

  gnc_Log(nc, 5, "Clocked FSM (%s): Machine has %d input%s, %d output%s and %d flipflop%s.", 
    cell_name,
    fsm_GetInCnt(fsm),
    fsm_GetInCnt(fsm)==1?"":"s",
    fsm_GetOutCnt(fsm),
    fsm_GetOutCnt(fsm)==1?"":"s",
    fsm_GetCodeWidth(fsm),
    fsm_GetCodeWidth(fsm)==1?"":"s"
  );

  if ( gnc_SynthGenericRegister(nc, cell_ref, fsm_GetCodeWidth(fsm)) < 0 )
    return gnc_DelCell(nc, cell_ref), -1;

  /* the following check will fail ... so this was removed */
  /* assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0); */

  /* do the remaining stuff... multilevel optimization, tech mapping */  
  /* min_delay is usually not used for finite state machines, set to 0.0 */
  if ( gnc_SynthOnSet(nc, cell_ref, synth_opt, 0.0) == 0 )
    return gnc_DelCell(nc, cell_ref), -1;

  {
    double m = gnc_GetCellTotalMaxDelay(nc, cell_ref);
    if ( m >= 0.0 )
      gnc_Log(nc, 5, "Clocked FSM (%s): Estimated logic delay (without flip-flops) is %lg.", 
        cell_name, m);
  }

  gnc_Log(nc, 5, "Clocked FSM (%s): Synthesis finished.", cell_name);
    
  return cell_ref;
}

/* returns cell_ref */
/* synth_opt = GNC_SYNTH_OPT_DEFAULT */
/* fsm_opt = GNC_HL_CLR_HIGH | GNC_HL_CLR_LOW */
int gnc_SynthAsynchronousFSM(gnc nc, fsm_type fsm, const char *cell_name, const char *lib_name, int hl_opt, int synth_opt)
{
  int cell_ref;
  int reset_is_neg;
  int reset_fn;
  int enc_opt = FSM_ASYNC_ENCODE_ZERO_RESET;
  int register_node_ref;
  int check_level = 1;

  /* if there is a reset line, then (at the moment) gnc REQUIRES a */
  /* reset state with all feedback lines are zero */  
  /* well... this has been changed... :-)  18 jan 2002 */
  switch(hl_opt&7)
  {
    case GNC_HL_OPT_CLR_HIGH:
      reset_fn = GPORT_FN_CLR;  
      reset_is_neg = 0; 
      break;
    case GNC_HL_OPT_CLR_LOW:   
      reset_fn = GPORT_FN_CLR;  
      reset_is_neg = 1; 
      break;
    case GNC_HL_OPT_NO_RESET:  
      reset_fn = -1;            
      reset_is_neg = 0; 
      break;
    default:
      gnc_Error(nc, "Spezified FSM synthesis options are not valid.");
      return -1;
  } 
  
  if ( (hl_opt & GNC_HL_OPT_FBO) == 0 )
    enc_opt = FSM_ASYNC_ENCODE_ZERO_MAX;
  else
    enc_opt = FSM_ASYNC_ENCODE_WITH_OUTPUT;

  gnc_Log(nc, 5, "Asynchronous FSM (%s): Checking FSM.", cell_name);

  fsm_PushLogFn(fsm, gnc_fsm_log_fn, nc, 5);  /* log validation errors to ll 5 */
  if ( fsm_IsValid(fsm) == 0 )
  {
    gnc_Error(nc, "Asynchronous FSM (%s): FSM not valid.", cell_name);
    return fsm_PopLogFn(fsm), -1;
  }
  fsm_PopLogFn(fsm);

  fsm_PushLogFn(fsm, gnc_fsm_log_fn, nc, 2);


  if ( (hl_opt & GNC_HL_OPT_MIN_STATE) != 0 || 
       (hl_opt & GNC_HL_OPT_OLD_MIN_STATE) != 0 )
  {
    int is_old = (hl_opt & GNC_HL_OPT_OLD_MIN_STATE) != 0 ? 1 : 0;
    
    gnc_Log(nc, 5, "Asynchronous FSM (%s): Minimizing states.", cell_name);
    if ( (hl_opt & GNC_HL_OPT_FBO) == 0 )
    {
      if ( fsm_MinimizeStates(fsm, 0, is_old) == 0 )
        return 0;
    }
    else
    {
      if ( fsm_MinimizeStates(fsm, 1, is_old) == 0 )
        return 0;
    }
    if ( fsm_GetGroupCnt(fsm) > 0 )
    {
      if ( fsm_GetNodeCnt(fsm) > fsm_GetGroupCnt(fsm) )
        gnc_Log(nc, 5, "Asynchronous FSM (%s): Reduced states from %d to %d.", 
          cell_name, fsm_GetNodeCnt(fsm), fsm_GetGroupCnt(fsm));
      else
        gnc_Log(nc, 5, "Asynchronous FSM (%s): No state reduction possible (%d states).", 
          cell_name, fsm_GetNodeCnt(fsm));
    }
      
  }

  gnc_Log(nc, 5, "Asynchronous FSM (%s): Building control function.", cell_name);

  if ( (hl_opt & GNC_HL_OPT_BM_CHECK) != 0 )
    check_level = 2;
  
  if ( fsm_BuildBurstModeMachine(fsm, check_level, enc_opt) == 0 )
  {
    gnc_Error(nc, "Asynchronous FSM (%s): Can not create transition function.", cell_name);
    if ( nc->log_level > 2 )
      gnc_Error(nc, "Asynchronous FSM (%s): Set log level to '2' or lower for more details.", cell_name);
    return fsm_PopLogFn(fsm), -1;
  }

  fsm_PopLogFn(fsm);
  
  gnc_Log(nc, 5, "Asynchronous FSM (%s): Synthesis started.", cell_name);
  
  cell_ref = gnc_AddCell(nc, cell_name, lib_name);
  if ( cell_ref < 0 )
    return -1;
  
  /* use GPORT_FN_CLR as reset function, because the reset state has all zero */
  if ( gnc_SetFSMDCLWithoutClock(nc, cell_ref, 
                fsm->pi_machine,
                fsm->cl_machine, 
                fsm_GetCodeWidth(fsm), 
                reset_fn, reset_is_neg) == 0  )
    return gnc_DelCell(nc, cell_ref), -1;

  gnc_Log(nc, 5, "Asynchronous FSM (%s): Machine has %d input%s, %d output%s and %d feedback line%s.", 
    cell_name,
    fsm_GetInCnt(fsm),
    fsm_GetInCnt(fsm)==1?"":"s",
    fsm_GetOutCnt(fsm),
    fsm_GetOutCnt(fsm)==1?"":"s",
    fsm_GetCodeWidth(fsm),
    fsm_GetCodeWidth(fsm)==1?"":"s"
    );

  /* create a register-cell and implement a node with it */
  register_node_ref = gnc_SynthGenericDoor(nc, cell_ref, fsm_GetCodeWidth(fsm));
  if ( register_node_ref < 0 )
    return gnc_DelCell(nc, cell_ref), -1;
    
  /* assign the reset code to the cell... well it would have been better to */
  /* assign it to the node... or one should ensure that the register cell is */
  /* unique... ok... in this case a register cell is only used temporary... */

  if ( fsm_GetResetCode(fsm) != NULL )
  {
    gnc_Log(nc, 3, "Asynchronous FSM (%s): Reset code is '%s'.", 
      cell_name,
      dcToStr(fsm_GetCodePINFO(fsm), fsm_GetResetCode(fsm), " ", "")
      );
  }
  else
  {
    gnc_Error(nc, "Asynchronous FSM (%s): Reset code not available.", 
      cell_name);
    return 0;
  }
  
  if ( gnc_ApplyRegisterResetCode(nc, 
      gnc_GetCellNodeCell(nc, cell_ref, register_node_ref), 
      fsm_GetCodePINFO(fsm), fsm_GetResetCode(fsm)) == 0 )
    return 0;

  /* the following check will fail ... so this was removed */
  /* assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0); */

  /* multilevel optimization, tech mapping */  
  /*
    Usually one should use gnc_SynthOnSet(). Instead gnc_synth_on_set()
    is used here, because this avoids the deletion of __empty__ cells
    which might had been used as door elements for an asynchronous
    circuit without reset line.
    __empty__ cells are deleted after the feedback delay calculation
    (which indeed requires these __empty__ cells).
    The main problem is that longest path calculation is only possible
    for non-cyclic graphs. So the delay calculation had to break up
    the feedback cycles. Now, such a cycle is detected if there is exactly
    one element between the two state lines (zn and zzn). 
    If there is not such an element, it is not clear (?) which part belongs
    to zn and which part belongs to zzn.
    
    The min_delay parameter is set to 0.0. It isn't used anyway.
  */

  if ( gnc_synth_on_set(nc, cell_ref, synth_opt, 0.0, 0) == 0 )
    return gnc_DelCell(nc, cell_ref), -1;


  if ( (hl_opt & GNC_HL_OPT_NO_DELAY) == GNC_HL_OPT_NO_DELAY )
  {
    gnc_Log(nc, 5, "Asynchronous FSM (%s): Delay correction skipped.", cell_name);
  }
  else
  {

    /* do a correction of the path delays */
    
    if ( (hl_opt & GNC_HL_OPT_USE_OLD_DLY) == GNC_HL_OPT_USE_OLD_DLY )
    {
      gnc_Log(nc, 5, "Asynchronous FSM (%s): Delay analysis and correction (old method).", cell_name);
      if ( gnc_InsertFeedbackDelayElements(nc, cell_ref) == 0 )
        return gnc_DelCell(nc, cell_ref), -1;
    }
    else
    {
      gnc_Log(nc, 5, "Asynchronous FSM (%s): Delay analysis and correction.", cell_name);
      if ( gnc_InsertFeedbackDelay(nc, cell_ref, fsm) == 0 )
        return gnc_DelCell(nc, cell_ref), -1;
    }



  }

  if ( gnc_SynthRemoveEmptyCells(nc, cell_ref) == 0 )
    return 0;
    

  gnc_Log(nc, 5, "Asynchronous FSM (%s): Synthesis finished.", cell_name);

  if ( (hl_opt & GNC_HL_OPT_NO_DELAY) == GNC_HL_OPT_NO_DELAY )
  {
    gnc_Log(nc, 5, "Asynchronous FSM (%s): Design might be incorrect because delay correction was disabled.", cell_name);
  }

  return cell_ref;
}

/*--------------------------------------------------------------------------*/

/* returns cell_ref */
/* synth_opt = GNC_SYNTH_OPT_DEFAULT */
/* fsm_opt = GNC_HL_CLR_HIGH | GNC_HL_CLR_LOW | GNC_HL_OPT_CLOCK */

int gnc_SynthFSM(gnc nc, fsm_type fsm, const char *cell_name, const char *lib_name, int fsm_opt, int synth_opt)
{
  int r;
  if ( (fsm_opt & GNC_HL_OPT_CLOCK) == GNC_HL_OPT_CLOCK )
    r = gnc_SynthClockedFSM(nc, fsm, cell_name, lib_name, fsm_opt, synth_opt);
  else
    r = gnc_SynthAsynchronousFSM(nc, fsm, cell_name, lib_name, fsm_opt, synth_opt);
  if ( r < 0 )
    return -1;
    
  
  if ( (fsm_opt & GNC_HL_OPT_CAP_REPORT) == GNC_HL_OPT_CAP_REPORT )
    if ( gnc_CalculateTransitionCapacitance(nc, r, fsm) == 0 )
      return gnc_DelCell(nc, r), -1;

  return r;
}

/*--------------------------------------------------------------------------*/

/* returns cell_ref */
/* synth_opt = GNC_SYNTH_OPT_DEFAULT */
int gnc_SynthFSMByFile(gnc nc, const char *import_file_name, const char *cell_name, const char *lib_name, int fsm_opt, int synth_opt)
{
  fsm_type fsm;
  int cell_ref;
  fsm = fsm_Open();
  if ( fsm == NULL )
    return -1;
  fsm_PushLogFn(fsm, gnc_fsm_log_fn, nc, 5);
  if ( fsm_Import(fsm, import_file_name) == 0 )
  {
    fsm_PopLogFn(fsm);
    gnc_Error(nc, "Can not read file '%s'", import_file_name);
    return fsm_Close(fsm), -1;
  }
  fsm_PopLogFn(fsm);
  cell_ref = gnc_SynthFSM(nc, fsm, cell_name, lib_name, fsm_opt, synth_opt);
  if ( cell_ref < 0 )
    return fsm_Close(fsm), -1;
  return fsm_Close(fsm), cell_ref;
}


