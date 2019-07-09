/*

  syxbm.c
  
  synthesis of finite state machines by using the xbm structure

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
#include "xbm.h"
#include "mwc.h"

/*
  fn: one of GPORT_FN_CLR or GPORT_FN_SET
  is_data_inv: whether the door cell requires inverted input.
  
  special version for pure output port  
*/
/* returns node_ref */
int gnc_SynthXBMOutputDoor(gnc nc, int cell_ref, int door_cell_ref, int in_net, int out_net, 
  int fn, int is_ctl_inv, int is_data_inv)
{
  int door_port_ref;
  int door_node_ref;
  int net_ref;

  int parent_port_ref = -1;
  int parent_join_ref;
  
  /* find the output parent port */
  parent_join_ref = gnc_FindCellNetJoin(nc, cell_ref, out_net, -1, -1);
  if ( parent_join_ref < 0 )
  {
    gnc_Error(nc, "Synthesis: Not an output port.");
    return -1;
  }
  parent_port_ref = gnc_GetCellNetPort(nc, cell_ref, in_net, parent_join_ref);
    
  /* create a new and gate */
  door_node_ref = gnc_AddCellNode(nc, cell_ref, NULL, door_cell_ref);
  if ( door_node_ref < 0 )
    return -1;
    
  /* find the data port */
  door_port_ref = gnc_GetNthPort(nc, door_cell_ref, 0, GPORT_TYPE_IN);
  if ( door_port_ref < 0 )
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
    if ( gnc_AddCellNetJoin(nc, cell_ref, in_net, door_node_ref, door_port_ref, 0) < 0 )
      return -1;
  }
  else
  {
    int inv_cell_ref = gnc_GetCellById(nc, GCELL_ID_INV);
    int inv_node_ref;
    int inv_in_port_ref;
    int inv_out_port_ref;

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

    /* and the at last, connect other and of the net to data port of the door */
    if ( gnc_AddCellNetJoin(nc, cell_ref, in_net, door_node_ref, door_port_ref, 0) < 0 )
      return -1;
      
    /* the net between the inverter and the door should be valid */
    assert( gnc_FindCellNetJoinByDriver(nc, cell_ref, in_net) >= 0 );      
  }

  /* find the output port */
  door_port_ref = gnc_GetNthPort(nc, door_cell_ref, 0, GPORT_TYPE_OUT);
  if ( door_port_ref < 0 )
  {
    gnc_Error(nc, "gnc_GetNthPort: output port not found (???).");
    return -1;
  }
  
  /* connect the door output port to the output net */
  if ( gnc_AddCellNetJoin(nc, cell_ref, out_net, door_node_ref, door_port_ref, 0) < 0 )
    return -1;
  /* out net should be valid... */
  assert( gnc_FindCellNetJoinByDriver(nc, cell_ref, out_net) >= 0 );
    

  /* find the control port, if the function is valid */
  if ( fn != GPORT_FN_NONE )
  {
    door_port_ref = gnc_GetNthPort(nc, door_cell_ref, 1, GPORT_TYPE_IN);
    if ( door_port_ref < 0 )
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
    if ( gnc_AddCellNetJoin(nc, cell_ref, net_ref, door_node_ref, door_port_ref, 0) < 0 )
      return -1;
    /* ctrl net must be valid */
    assert( gnc_FindCellNetJoinByDriver(nc, cell_ref, net_ref) >= 0 );
  }
  
  return door_node_ref;
}


/*
                  state vars        output ports
                                  reset    feed-back
                                  
                    nrs             nrs       no
  -rso              nrs             nrs       no
  -fbo              nrs             nrs       yes
  -rsl              rsl             nrs       no
  -rsl -fbo         rsl             rsl       yes
  -rsl -rso         rsl             rsl       no
  -rsl -fbo -rso    rsl             rsl       yes
  
  
  reset         value    door       control   data
  low active      0      AND2       non-inv   non-inv
  high active     0      AND2         inv     non-inv
  low active      1      OR2        non-inv   non-inv
  high active     1      OR2          inv     non-inv
  
              
*/

/* synthesis reset logic for output lines */
static int gnc_SynthXBMOutputReset(gnc nc, xbm_type x, int cell_ref)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  int i, port_ref;
  int pos;
  int reset_value;
  int in_net_ref;
  int out_net_ref;


  int is_ctl_inv = 0;
  int is_data_inv = 0;
  int door_cell_ref = -1;
  int alt_door_cell_ref = -1;

  int fn = GPORT_FN_CLR;
  
  if ( gnc_FindCellPortByFn(nc, cell_ref, GPORT_FN_CLR) < 0 )
    return 1;   /* no reset line */
    
  if ( x->is_fbo != 0 )
    return 1;   /* reset is already done */
    
  pos = -1;
  while( xbm_LoopVar(x, &pos) != 0 )
  {
    if ( xbm_GetVar(x, pos)->direction == XBM_DIRECTION_OUT )
    {
      gnc_Log(nc, 3, "Synthesis: Reset for output port '%s' (pi in %d, rw %d, pi out %d).",
        xbm_GetVarName2Str(x, pos),
        cell->pi->in_cnt, cell->register_width, cell->pi->out_cnt);

      for( i = cell->register_width; i < cell->pi->out_cnt; i++ )
      {
        port_ref = i + cell->pi->in_cnt;
        if ( strcmp( gnc_GetCellPortName(nc, cell_ref, port_ref), 
                     xbm_GetVarName2Str(x, pos) ) == 0 ) 
          break;
      }
      if ( i >= cell->pi->out_cnt )
        return gnc_Error(nc, "Synthesis: Output port '%s' not found (rso).", xbm_GetVarName2Str(x, pos)), 0;

      in_net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, port_ref);
      if ( in_net_ref < 0 )
        return gnc_Error(nc, "Synthesis: Output port '%s' (port_ref %d) not connected (rso).", 
          xbm_GetVarName2Str(x, pos), port_ref), 0;

      /* disconnect the output port of the parent cell */
      gnc_DelCellNetJoinByPort(nc, cell_ref, in_net_ref, -1, port_ref);

      /* create a new net for the disconnected output port */
      out_net_ref = gnc_GetDigitalCellNetByPort(nc, cell_ref, port_ref, 0, 0);
      if ( out_net_ref < 0 )
        return gnc_Error(nc, "Synthesis: Output port '%s' (port_ref %d) not connected (rso).", 
          xbm_GetVarName2Str(x, pos), port_ref), 0;
          
      gnc_Log(nc, 0, "Synthesis: innet %d, outnet %d", in_net_ref, out_net_ref);
      
      reset_value = xbm_GetVar(x, pos)->reset_value;

      if ( reset_value == 0 )
      {
        is_ctl_inv = !0;
        is_data_inv = 0;
        door_cell_ref = gnc_GetCellById(nc, GCELL_ID_AND2);
        alt_door_cell_ref = gnc_GetCellById(nc, GCELL_ID_NOR2);
        
        gnc_Log(nc, 1, "Synthesis: Reset state of output '%s' is 0.", 
          xbm_GetVarName2Str(x, pos));
        gnc_Log(nc, 0, "Synthesis: Reset gate AND2 (%d), alternate gate NOR2 (%d)", 
          door_cell_ref, alt_door_cell_ref);

      }
      else
      {
        is_ctl_inv = 0;
        is_data_inv = 0;
        door_cell_ref = gnc_GetCellById(nc, GCELL_ID_OR2);
        alt_door_cell_ref = gnc_GetCellById(nc, GCELL_ID_NAND2);

        gnc_Log(nc, 1, "Synthesis: Reset state of output '%s' is 1.", 
          xbm_GetVarName2Str(x, pos));
        gnc_Log(nc, 0, "Synthesis: Reset gate OR2 (%d), alternate gate NAND2 (%d)", 
          door_cell_ref, alt_door_cell_ref);
      }

      if ( door_cell_ref < 0 && alt_door_cell_ref < 0 )
      {
        if ( reset_value == 0 )
          gnc_Error(nc, "Synthesis: No suitable reset gate found (AND2 or NOR2).");
        else
          gnc_Error(nc, "Synthesis: No suitable reset gate found (OR2 or NAND2).");
        return 0;
      }

      if ( door_cell_ref < 0 )
      {
        door_cell_ref = alt_door_cell_ref;
        is_ctl_inv = !is_ctl_inv;
        is_data_inv = !is_data_inv;
      }

      if ( gnc_GetCellName(nc, door_cell_ref) != NULL )
      {
        gnc_Log(nc, 2, "Synthesis: Reset gate for output '%s' is '%s' (feedback line %sinverted, control line %sinverted).", 
          xbm_GetVarName2Str(x, pos),
          gnc_GetCellName(nc, door_cell_ref),
          is_data_inv==0?"not ":"",
          is_ctl_inv==0?"not ":""
          );
      }

      /*
      if ( gnc_SynthDoor(nc, cell_ref, door_cell_ref, in_net_ref, out_net_ref, fn, is_ctl_inv, is_data_inv) < 0 )
        return 0;
      */
      if ( gnc_SynthXBMOutputDoor(nc, cell_ref, door_cell_ref, in_net_ref, out_net_ref, fn, is_ctl_inv, is_data_inv) < 0 )
        return 0;
    }
  }

  assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);
  
  return 1;
}

/* handles synthesis of XBM state variables */
/* State variable create two output port. There is an internal chain */
/* of inverter pairs between these two ports. */
static int gnc_SynthXBMDelayVariables(gnc nc, xbm_type x, int cell_ref)
{
  int pos_in = -1;
  int pos_out = 0;
  int port_ref;
  double dly;

  /* xbm_GetVarNameStr() returns the first name */
  /* xbm_GetVarName2Str() returns the second name of a xbm variable */
  /* the second name might contain an additional postfix */
  /* usually the second name should be used for any further processing */
  
  while( xbm_LoopVar(x, &pos_in) != 0 )
  {
    if ( xbm_GetVar(x, pos_in)->direction == XBM_DIRECTION_IN )
    {
      pos_out = xbm_FindVar(x, XBM_DIRECTION_OUT, xbm_GetVarNameStr(x, pos_in));
      
      /* state variables are detected by checking if the variable exists twice */
      /* as an input and as an output port */
      
      if ( pos_out >= 0 )
      {
        dly = xbm_GetVar(x, pos_in)->delay;
        gnc_Log(nc, 3, "XBM: Delay ports (%s, %s) found (delay: %lfns).",
          xbm_GetVarName2Str(x, pos_in), xbm_GetVarName2Str(x, pos_out), 
          xbm_GetVar(x, pos_in)->delay);
   
        if ( gnc_BuildDelayPath(nc, cell_ref, dly, 
            xbm_GetVarName2Str(x, pos_out), 
            xbm_GetVarName2Str(x, pos_in)) == 0 )
          return 0;

        port_ref = gnc_FindCellPort(nc, cell_ref, xbm_GetVarName2Str(x, pos_in));
        if ( port_ref >= 0 )
          gnc_SetCellPortType(nc, cell_ref, port_ref, GPORT_TYPE_OUT);

      }
    }
  }

  assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);

  return 1;
}

/*--------------------------------------------------------------------------*/
/* 
  a special test for the martin padeffke problem: 
  is there an output port which is feed back, but should not...
*/
static int gnc_CheckXBMOutputConnections(gnc nc, xbm_type x, int cell_ref)
{
  int in_cnt = gnc_GetGCELL(nc, cell_ref)->pi->in_cnt;
  int out_cnt = gnc_GetGCELL(nc, cell_ref)->pi->out_cnt;
  int o;
  int net_ref;
  int join_ref;
  int port_ref;
  int node_ref;
  int port_type;  /* GPORT_TYPE_BI, GPORT_TYPE_IN, GPORT_TYPE_OUT */

  int parent_cnt;
  int fan_in;
  int feedback_cnt = xbm_GetFeedbackWidth(x);
  const char *outputname;

  gnc_Log(nc, 2, "XBM: Output check, outputs %d, feedbacks %d.",
    out_cnt, feedback_cnt);
  
  for( o = 0; o < out_cnt; o++ )
  {
    net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, o+in_cnt);
    
    parent_cnt = 0;
    fan_in = 0;
    
    join_ref = -1;
    while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
    {
      port_ref = gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref);
      node_ref = gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
      port_type = gnc_GetCellNetPortType(nc, cell_ref, net_ref, join_ref);
      
      if ( node_ref < 0 )
        parent_cnt++;
        
      if ( port_type == GPORT_TYPE_IN )
        fan_in++;
    }
    
    outputname = gnc_GetCellPortName(nc, cell_ref, o+in_cnt);
    
    gnc_Log(nc, 2, "XBM: Output check, output name '%s', fan in %d, total output connections %d.",
      outputname, fan_in, parent_cnt);
      
    if ( o < feedback_cnt )
    {
      /* a feed-back signal should have a connection... */
      if ( fan_in == 0 )
        gnc_Error(nc, "XBM: Output check, feed back '%s' is not connected.", 
          outputname);
    }
    else
    {
      if ( fan_in > 0 )
        gnc_Log(nc, 6, "XBM: Output check warning, feed back '%s' has additional connection.", 
          outputname);
    }
      
  }
  return 1;
}



/*--------------------------------------------------------------------------*/

static void gnc_xbm_log_cb(void *data, int ll, const char *fmt, va_list va)
{
  gnc_LogVA((gnc)data, ll, fmt, va);
}


/* rewritten gnc_ApplyRegisterResetCode() for XBM */

int gnc_ApplyXBMRegisterResetCode(gnc nc, xbm_type x, int cell_ref)
{
  int i;
  int icnt, ocnt;
  
  if ( x->reset_st_pos < 0 )
    return 0;

  i = -1;
  icnt = 0;
  ocnt = 0;
  while( gnc_LoopCellPort(nc, cell_ref, &i) != 0 )
  {
    if ( gnc_GetCellPortType(nc, cell_ref, i) == GPORT_TYPE_IN )
    {
      icnt++;
      gnc_SetCellPortFn(nc, cell_ref, i, GPORT_FN_Q, 0);
    }
    else
    {
      ocnt++;
    }
  }

  assert(icnt == xbm_GetFeedbackWidth(x));
  assert(ocnt == xbm_GetFeedbackWidth(x));
    
    
  for( i = 0; i < xbm_GetFeedbackWidth(x); i++ )
  {
    assert(gnc_GetCellPortType(nc, cell_ref, i+xbm_GetFeedbackWidth(x)) == GPORT_TYPE_IN);
    if ( i < xbm_GetPiCode(x)->out_cnt )
    {
      gnc_SetCellPortFn(nc, cell_ref, 
          i+xbm_GetFeedbackWidth(x), 
          GPORT_FN_LOGIC, 
          dcGetOut(&(xbm_GetSt(x, x->reset_st_pos)->code), i));
    }
    else  /* assuming is_fbo != 0 */
    {
      assert(x->is_fbo != 0);
      gnc_SetCellPortFn(nc, cell_ref, 
          i+xbm_GetFeedbackWidth(x), 
          GPORT_FN_LOGIC, 
          dcGetOut(&(xbm_GetSt(x, x->reset_st_pos)->out), i-xbm_GetPiCode(x)->out_cnt));
    }
  }

  return 1;
}

/* this is mostly a copy of gnc_SynthAsynchronousFSM() */
/* returns cell_ref */
/* synth_opt = GNC_SYNTH_OPT_DEFAULT */
/* hl_opt = GNC_HL_CLR_HIGH | GNC_HL_CLR_LOW */
/*
  
  important calls:
    1. xbm_Build
    2. gnc_AddCell
    3. gnc_SetFSMDCLWithoutClock/gnc_SetFSMDCLWithClock
    4. gnc_SynthGenericDoor/gnc_SynthRegisterDoor
  
*/
static int gnc_synth_XBM(gnc nc, xbm_type x, const char *cell_name, const char *lib_name, int hl_opt, int synth_opt)
{
  int cell_ref;
  int reset_is_neg;
  int reset_fn;
  int xbm_opt = 0;
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
      gnc_Error(nc, "Specified FSM synthesis options are not valid.");
      return -1;
  } 
  
  if ( (hl_opt & GNC_HL_OPT_FBO) != 0 )
    xbm_opt |= XBM_BUILD_OPT_FBO;
    
  if ( (hl_opt & GNC_HL_OPT_CLOCK) == GNC_HL_OPT_CLOCK )
    xbm_opt |= XBM_BUILD_OPT_SYNC;
    
    
  if ( (hl_opt & GNC_HL_OPT_MIN_STATE) != 0 || 
       (hl_opt & GNC_HL_OPT_OLD_MIN_STATE) != 0 )
  {
    xbm_opt |= XBM_BUILD_OPT_MIS;
  }

  gnc_Log(nc, 5, "XBM (%s): Building control function.", cell_name);

  xbm_SetLogCB(x, gnc_xbm_log_cb, nc);
  if ( xbm_Build(x, xbm_opt) == 0 )
  {
    gnc_Error(nc, "XBM (%s): Can not create transition function.", cell_name);
    return xbm_ClrLogCB(x), -1;
  }
  xbm_ClrLogCB(x);

  gnc_Log(nc, 5, "XBM (%s): Synthesis started.", cell_name);
  
  cell_ref = gnc_AddCell(nc, cell_name, lib_name);
  if ( cell_ref < 0 )
    return -1;

  if ( (hl_opt & GNC_HL_OPT_CLOCK) == GNC_HL_OPT_CLOCK )
  {
    gnc_Log(nc, 5, "XBM (%s): Synchronous state machine.", cell_name);
    if ( gnc_SetFSMDCLWithClock(nc, cell_ref, 
                  x->pi_machine, 
                  x->cl_machine, 
                  xbm_GetFeedbackWidth(x), 
                  reset_fn, reset_is_neg) == 0  )
      return gnc_DelCell(nc, cell_ref), -1;
  }
  else
  {
    gnc_Log(nc, 5, "XBM (%s): Asynchronous state machine.", cell_name);
    if ( gnc_SetFSMDCLWithoutClock(nc, cell_ref, 
                  x->pi_machine,
                  x->cl_machine, 
                  xbm_GetFeedbackWidth(x), 
                  reset_fn, reset_is_neg) == 0  )
      return gnc_DelCell(nc, cell_ref), -1;
  }

  gnc_Log(nc, 5, "XBM (%s): Machine has %d input%s, %d output%s and %d feed-back line%s.", 
    cell_name,
    x->inputs,
    x->inputs==1?"":"s",
    x->outputs,
    x->outputs==1?"":"s",
    xbm_GetFeedbackWidth(x),
    xbm_GetFeedbackWidth(x)==1?"":"s"
    );


  if ( (hl_opt & GNC_HL_OPT_CLOCK) == GNC_HL_OPT_CLOCK )
  {
    /* create a register-cell and implement a node with it */
    register_node_ref = gnc_SynthGenericRegister(nc, cell_ref, xbm_GetFeedbackWidth(x));
    if ( register_node_ref < 0 )
      return gnc_DelCell(nc, cell_ref), -1;
  }
  else
  {
    /* create a register-cell and implement a node with it */
    register_node_ref = gnc_SynthGenericDoor(nc, cell_ref, xbm_GetFeedbackWidth(x));
    if ( register_node_ref < 0 )
      return gnc_DelCell(nc, cell_ref), -1;
  }
  
    
  /* assign the reset code to the cell... well it would have been better to */
  /* assign it to the node... or one should ensure that the register cell is */
  /* unique... ok... in this case a register cell is only used temporary... */

  if ( x->reset_st_pos < 0 )
  {
    gnc_Error(nc, "XBM (%s): Reset state not available.", 
      cell_name);
    return 0;
  }
  
  if ( gnc_ApplyXBMRegisterResetCode(nc, x,
      gnc_GetCellNodeCell(nc, cell_ref, register_node_ref)) == 0 )
    return 0;

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


  if ( (hl_opt & GNC_HL_OPT_CLOCK) == 0 )
  {
    if ( (hl_opt & GNC_HL_OPT_NO_DELAY) == GNC_HL_OPT_NO_DELAY )
    {
      gnc_Log(nc, 5, "XBM (%s): Delay correction skipped.", cell_name);
    }
    else
    {

      /* do a correction of the path delays */

      if ( (hl_opt & GNC_HL_OPT_USE_OLD_DLY) == GNC_HL_OPT_USE_OLD_DLY )
      {
        gnc_Log(nc, 5, "XBM (%s): Delay analysis and correction (old method).", cell_name);
        if ( gnc_InsertFeedbackDelayElements(nc, cell_ref) == 0 )
          return gnc_DelCell(nc, cell_ref), -1;
      }
      else
      {
        gnc_Log(nc, 5, "XBM (%s): Delay analysis and correction.", cell_name);
        if ( gnc_InsertXBMFeedbackDelay(nc, cell_ref, x) == 0 )
          return gnc_DelCell(nc, cell_ref), -1;
      }
      
      gnc_GetFundamentalModeXBM(nc, cell_ref, x);
    }
  }

  if ( (hl_opt & GNC_HL_OPT_RSO) != 0 )
    if ( gnc_SynthXBMOutputReset(nc, x, cell_ref) == 0 )
      return 0;

  if ( gnc_SynthRemoveEmptyCells(nc, cell_ref) == 0 )
    return 0;

  if ( gnc_CheckXBMOutputConnections(nc, x, cell_ref) == 0 )
    return 0;

  if ( gnc_SynthXBMDelayVariables(nc, x, cell_ref) == 0 )
    return 0;
    
  gnc_Log(nc, 5, "XBM (%s): Synthesis finished.", cell_name);

  if ( (hl_opt & GNC_HL_OPT_CLOCK) == 0 )
  {
    if ( (hl_opt & GNC_HL_OPT_NO_DELAY) == GNC_HL_OPT_NO_DELAY )
    {
      gnc_Log(nc, 5, "XBM (%s): Design might be incorrect because delay correction was disabled.", cell_name);
    }
  }

  if ( (hl_opt & GNC_HL_OPT_CLOCK) == GNC_HL_OPT_CLOCK )
  {
    double m = gnc_GetCellTotalMaxDelay(nc, cell_ref);
    if ( m >= 0.0 )
      gnc_Log(nc, 5, "XBM (%s): Estimated logic delay (without flip-flops) is %lg.", 
        cell_name, m);
  }

  return cell_ref;
}

/*--------------------------------------------------------------------------*/
/* XBM Capacitance analysis */

static int xbm_cap_analysis(xbm_type x, void *data, int tr_pos, dcube *cs, dcube *ct, int is_reset, int is_unique)
{
  gspq_type pq = (gspq_type)data;

  gnc_Log(pq->nc, 3, "Analysis: Transition '%s' %s '%s'.",
    xbm_GetStNameStr(x, xbm_GetTrSrcStPos(x, tr_pos)),
    is_unique!=0?"-->":"==>",
    xbm_GetStNameStr(x, xbm_GetTrDestStPos(x, tr_pos)));

  gnc_Log(pq->nc, 2, "Analysis: stable %s  transition %s  reset %d  unique %d.",
    dcToStr(x->pi_machine, cs, " ", ""),
    dcToStr2(x->pi_machine, ct, " ", ""),
    is_reset, is_unique);

  pq->is_cap_analysis = 1;
  
  if ( is_reset != 0 )
  {
    gnc_SetAllCellNetLogicVal(pq->nc, pq->cell_ref, GLV_UNKNOWN);
    gnc_SetAllCellPortLogicVal(pq->nc, pq->cell_ref, GLV_UNKNOWN);

    pq->is_cap_analysis = 0;
    if ( gspq_ApplyCellXBMSimulationTransitionResetState(pq, cs) == 0 )
      return 0;
    if ( gspq_CompareCellXBMStateForSimulation(pq, cs) == 0 )
      return 0;
    pq->is_cap_analysis = 1;
  }

  if ( is_unique == 0 )
    pq->is_cap_analysis = 0;
    
  if ( gspq_ApplyCellXBMSimulationTransition(pq, ct) == 0 )
    return 0;
  if ( gspq_CompareCellXBMStateForSimulation(pq, ct) == 0 )
    return 0;

  pq->is_cap_analysis = 1;
  
  return 1;
}

int gnc_CalculateXBMTransitionCapacitance(gnc nc, int cell_ref, xbm_type x)
{
  gspq_type pq = gspq_Open(nc, cell_ref);
  if ( pq == NULL )
    return 0;

  gnc_Log(nc, 4, "Analysis: Capacitance analysis started.");

  gspq_EnableCapacitanceAnalysis(pq);

  if ( xbm_DoWalk(x, xbm_cap_analysis, pq) == 0 )
    return gspq_Close(pq), 0;
    
  gnc_Log(nc, 6, "Analysis: Average charge/dischage capacitance per transition is %lg (%d transitions).", 
    gspq_GetAveragetNetChangeCapacitance(pq), pq->cap_analysis_transition_cnt);

  gnc_Log(nc, 6, "Analysis: Average charge/dischage capacitance of FF per transition is %lg (%d transitions).", 
    pq->ff_capacitance_sum/pq->cap_analysis_transition_cnt, pq->cap_analysis_transition_cnt);

  return gspq_Close(pq), 1;
}


/*--------------------------------------------------------------------------*/

/* returns cell_ref */
/* synth_opt = GNC_SYNTH_OPT_DEFAULT */
/* hl_opt = GNC_HL_CLR_HIGH | GNC_HL_CLR_LOW | GNC_HL_OPT_CLOCK */

int gnc_SynthXBM(gnc nc, xbm_type x, const char *cell_name, const char *lib_name, int hl_opt, int synth_opt)
{
  int r;
  r = gnc_synth_XBM(nc, x, cell_name, lib_name, hl_opt, synth_opt);
  if ( r < 0 )
    return -1;
    
  if ( (hl_opt & GNC_HL_OPT_CAP_REPORT) == GNC_HL_OPT_CAP_REPORT )
  {
    if ( gnc_CalculateXBMTransitionCapacitance(nc, r, x) == 0 )
      return gnc_DelCell(nc, r), -1;
  }

  return r;
}


/* returns cell_ref */
/* synth_opt = GNC_SYNTH_OPT_DEFAULT */
int gnc_SynthXBMByFile(gnc nc, const char *import_file_name, const char *cell_name, const char *lib_name, int fsm_opt, int synth_opt)
{
  xbm_type x;
  int cell_ref;
  x = xbm_Open();
  if ( x == NULL )
    return -1;
  x->log_level = nc->log_level;
  xbm_SetLogCB(x, gnc_xbm_log_cb, nc);
  if ( xbm_ReadBMS(x, import_file_name) == 0 )
  {
    xbm_ClrLogCB(x);
    gnc_Error(nc, "Can not read file '%s'", import_file_name);
    return xbm_Close(x), -1;
  }
  xbm_ClrLogCB(x);
  cell_ref = gnc_SynthXBM(nc, x, cell_name, lib_name, fsm_opt, synth_opt);
  if ( cell_ref < 0 )
    return xbm_Close(x), -1;
  return xbm_Close(x), cell_ref;
}


/*---------------------------------------------------------------------------*/
