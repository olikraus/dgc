/*

  fsmvhd.c

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

#include <stdio.h>
#include <assert.h>
#include "fsm.h"
#include "fsmtest.h"
#include "mwc.h"


/*---------------------------------------------------------------------------*/

static int fsm_WriteVHDLNewLine(fsm_type fsm, FILE *fp, int indent)
{
  fprintf(fp, "\n");
  while( indent-- > 0 )
    fprintf(fp, "  ");
  return 1;
}

/*---------------------------------------------------------------------------*/

static int fsm_WriteVHDLMachineInputSignalFP(fsm_type fsm, FILE *fp, int in)
{
  b_sl_type bs;
  bs = pinfoGetInLabelList(fsm->pi_machine);
  if ( bs == NULL )
    fprintf(fp, "i%d", in);
  else
    fprintf(fp, "%s", b_sl_GetVal(bs, in));
  return 1;
}

static int fsm_WriteVHDLMachineOutputSignalFP(fsm_type fsm, FILE *fp, int out)
{
  b_sl_type bs;
  bs = pinfoGetOutLabelList(fsm->pi_machine);
  if ( bs == NULL )
    fprintf(fp, "i%d", out);
  else
    fprintf(fp, "%s", b_sl_GetVal(bs, out));
  return 1;
}

static int fsm_WriteVHDLMachineInternalInputSignalFP(fsm_type fsm, FILE *fp, int out)
{
  fprintf(fp, "i");
  return fsm_WriteVHDLMachineInputSignalFP(fsm, fp, out);
}

static int fsm_WriteVHDLMachineInternalOutputSignalFP(fsm_type fsm, FILE *fp, int out)
{
  fprintf(fp, "i");
  return fsm_WriteVHDLMachineOutputSignalFP(fsm, fp, out);
}

static int fsm_WriteVHDLMachinePortFP(fsm_type fsm, FILE *fp, char *clock, char *reset, int opt)
{
  int i;
  int is_first;
  int in_cnt = fsm->pi_machine->in_cnt;
  int out_cnt = fsm->pi_machine->out_cnt;
  int code_width =  fsm_GetCodeWidth(fsm);
  
  
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "PORT(");
  is_first = 1;
  for( i = 0; i < in_cnt-code_width; i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fsm_WriteVHDLMachineInputSignalFP(fsm, fp, i);
  }
  
  fprintf(fp, " : IN STD_LOGIC;");
  fsm_WriteVHDLNewLine(fsm, fp, 2);


  is_first = 1;
  for( i = in_cnt-code_width; i < in_cnt; i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fsm_WriteVHDLMachineInputSignalFP(fsm, fp, i);
  }

  for( i = 0; i < out_cnt; i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fsm_WriteVHDLMachineOutputSignalFP(fsm, fp, i);
  }
  fprintf(fp, " : OUT STD_LOGIC;");

  fsm_WriteVHDLNewLine(fsm, fp, 2);
  
  is_first = 1;
  if ( is_first == 0 )
    fprintf(fp, ", ");
  is_first = 0;
  fprintf(fp, "%s", reset);
  
  if ( clock != NULL && clock[0] != '\0' )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fprintf(fp, "%s", clock);
  }
  
  fprintf(fp, " : IN STD_LOGIC);");

  return 1;
}

static int fsm_WriteVHDLInternalOutputSignalListFP(fsm_type fsm, FILE *fp, int opt, int is_int)
{
  int is_first = 1;
  int i;
  int out_cnt = fsm->pi_machine->out_cnt;

  for( i = 0; i < out_cnt; i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fsm_WriteVHDLMachineInternalOutputSignalFP(fsm, fp, i);
  }

  return 1;
}

static int fsm_WriteVHDLInternalInputSignalListFP(fsm_type fsm, FILE *fp, int opt, int is_int)
{
  int is_first = 1;
  int i;
  int in_cnt = fsm->pi_machine->in_cnt;
  int code_width =  fsm_GetCodeWidth(fsm);

  for( i = in_cnt-code_width; i < in_cnt; i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fsm_WriteVHDLMachineInternalInputSignalFP(fsm, fp, i);
  }

  return 1;
}


/*---------------------------------------------------------------------------*/

static int fsm_WriteVHDLDependSignalFP(fsm_type fsm, FILE *fp, int dep, int opt)
{
  int in_cnt = fsm->pi_machine->in_cnt;
  int code_width =  fsm_GetCodeWidth(fsm);
  if ( dep < in_cnt-code_width )
    return fsm_WriteVHDLMachineInputSignalFP(fsm, fp, dep);
  return fsm_WriteVHDLMachineInternalInputSignalFP(fsm, fp, dep);
}

static int fsm_WriteVHDLPosNegDependSignalFP(fsm_type fsm, FILE *fp, int dep, int neg, int opt)
{
  if ( neg != 0 )
    fprintf(fp, "(NOT ");
  fsm_WriteVHDLDependSignalFP(fsm, fp, dep, opt);
  if ( neg != 0 )
    fprintf(fp, ")");
  return 1;
}

static int fsm_WriteVHDLMachineCubeFP(fsm_type fsm, FILE *fp, dcube *c, int opt)
{
  int i, cnt = fsm->pi_machine->in_cnt;
  int s;
  int is_first = 1;
  fprintf(fp, "(");
  for( i = 0; i < cnt; i++ )
  {
    s = dcGetIn(c, i);
    if ( s == 1 || s == 2 )
    {
      if ( is_first == 0 )
        fprintf(fp, " AND ");
      is_first = 0;
      if ( fsm_WriteVHDLPosNegDependSignalFP(fsm, fp, i, s==1?1:0, opt) == 0 )
        return 0;
    }
  }
  fprintf(fp, ")");
  return 1;
}

int fsm_WriteVHDLEntityFP(fsm_type fsm, FILE *fp, char *entity, char *clock, char *reset, int opt)
{

  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "LIBRARY ieee;");
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "USE ieee.std_logic_1164.all;");
  fsm_WriteVHDLNewLine(fsm, fp, 0);


  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "ENTITY %s IS", entity);
  if ( fsm_WriteVHDLMachinePortFP(fsm, fp, clock, reset, opt) == 0 )
    return 0;
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "END %s;", entity);
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  return 1;
}

static int fsm_WriteVHDLResetAssignment(fsm_type fsm, FILE *fp, int indent)
{
  int i;
  dcube *c;
  int in_cnt = fsm->pi_machine->in_cnt;
  int code_width =  fsm_GetCodeWidth(fsm);
  
  if ( fsm->reset_node_id < 0 )
    return 1;

  c = fsm_GetNodeCode(fsm, fsm->reset_node_id);
  for( i = in_cnt-code_width; i < in_cnt; i++ )
  {
    fsm_WriteVHDLNewLine(fsm, fp, indent);
    fsm_WriteVHDLMachineInternalInputSignalFP(fsm, fp, i);
    fprintf(fp, " <= '%c';", dcGetOut(c, i)+'0');
  }
  return 1;
}

static int fsm_WriteVHDLAssignmentZZZ(fsm_type fsm, FILE *fp, int indent)
{
  int i;
  int in_cnt = fsm->pi_machine->in_cnt;
  int code_width =  fsm_GetCodeWidth(fsm);
  for( i = 0; i < code_width; i++ )
  {
    fsm_WriteVHDLNewLine(fsm, fp, indent);
    fsm_WriteVHDLMachineInternalInputSignalFP(fsm, fp, in_cnt-code_width+i);
    fprintf(fp, " <= ");
    fsm_WriteVHDLMachineInternalOutputSignalFP(fsm, fp, i);
    fprintf(fp, ";");
  }
  return 1;
}

static int fsm_WriteVHDLAssignmentInOut(fsm_type fsm, FILE *fp, int indent, int opt)
{
  int i;
  int in_cnt = fsm->pi_machine->in_cnt;
  int out_cnt = fsm->pi_machine->out_cnt;
  int code_width =  fsm_GetCodeWidth(fsm);

  for( i = in_cnt-code_width; i < in_cnt; i++ )
  {
    fsm_WriteVHDLNewLine(fsm, fp, indent);
    fsm_WriteVHDLMachineInputSignalFP(fsm, fp, i);
    fprintf(fp, " <= ");
    fsm_WriteVHDLMachineInternalInputSignalFP(fsm, fp, i);
    fprintf(fp, ";");
  }

  for( i = 0; i < out_cnt; i++ )
  {
    fsm_WriteVHDLNewLine(fsm, fp, indent);
    fsm_WriteVHDLMachineOutputSignalFP(fsm, fp, i);
    fprintf(fp, " <= ");
    fsm_WriteVHDLMachineInternalOutputSignalFP(fsm, fp, i);
    fprintf(fp, ";");
  }

  return 1;
}


static int fsm_WriteVHDLMachineFunction(fsm_type fsm, FILE *fp, int indent, int start, int end, int opt)
{
  int c_idx, c_cnt = dclCnt(fsm->cl_machine);
  int i;
  int is_first;

  for( i = start; i < end; i++ )
  {
    fsm_WriteVHDLNewLine(fsm, fp, indent);
    fsm_WriteVHDLMachineInternalOutputSignalFP(fsm, fp, i);
    fprintf(fp, " <= ");
    fsm_WriteVHDLNewLine(fsm, fp, indent+1);
    is_first = 1;
    for( c_idx = 0; c_idx < c_cnt; c_idx++ )
    {
      if ( dcGetOut(dclGet(fsm->cl_machine, c_idx), i) != 0 )
      {
        if ( is_first == 0 )
        {
          fprintf(fp, " OR ");  
          fsm_WriteVHDLNewLine(fsm, fp, indent+1);
        }
        is_first = 0;
        fsm_WriteVHDLMachineCubeFP(fsm, fp, dclGet(fsm->cl_machine, c_idx), opt);
      }
    }
    fprintf(fp, ";");
  }
  return 1;
}

int fsm_WriteVHDLArchitectureFP(fsm_type fsm, FILE *fp, char *entity, char *arch, char *clock, char *reset, int opt)
{ 
  int reset_value = 1;
  if ( (opt & FSM_VHDL_OPT_CLR_ON_LOW) == FSM_VHDL_OPT_CLR_ON_LOW )
    reset_value = 0;

  assert(fsm->code_width > 0);
  
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "ARCHITECTURE %s OF %s IS", arch, entity);
  
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "SIGNAL ");
  fsm_WriteVHDLInternalInputSignalListFP(fsm, fp, opt, 1);
  fprintf(fp, " : STD_LOGIC;");

  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "SIGNAL ");
  fsm_WriteVHDLInternalOutputSignalListFP(fsm, fp, opt, 1);
  fprintf(fp, " : STD_LOGIC;");

  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "BEGIN");


  fsm_WriteVHDLMachineFunction(fsm, fp, 1, 0, fsm->code_width, opt);
  
  if ( clock != NULL && clock[0] != '\0' )
  {
    fsm_WriteVHDLNewLine(fsm, fp, 1);
    fprintf(fp, "PROCESS(%s,%s)", clock, reset);

    fsm_WriteVHDLNewLine(fsm, fp, 1);
    fprintf(fp, "BEGIN");

    fsm_WriteVHDLNewLine(fsm, fp, 2);
    fprintf(fp, "IF %s = '%d' THEN", reset, reset_value);
    fsm_WriteVHDLResetAssignment(fsm, fp, 3);

    fsm_WriteVHDLNewLine(fsm, fp, 2);
    fprintf(fp, "ELSIF %s = '1' AND %s'EVENT THEN", clock, clock);

    fsm_WriteVHDLAssignmentZZZ(fsm, fp, 3);
    
    fsm_WriteVHDLNewLine(fsm, fp, 2);
    fprintf(fp, "END IF;");
    fsm_WriteVHDLNewLine(fsm, fp, 1);
    fprintf(fp, "END PROCESS;");
  }
  else
  {
    int i;
    fsm_WriteVHDLNewLine(fsm, fp, 1);
    fprintf(fp, "PROCESS(%s", reset);
    
    for( i = 0; i < fsm_GetCodeWidth(fsm); i++ )
    {
      fprintf(fp, ", ");
      fsm_WriteVHDLMachineInternalOutputSignalFP(fsm, fp, i);
    }
    
    fprintf(fp, ")");
    
    fsm_WriteVHDLNewLine(fsm, fp, 1);
    fprintf(fp, "BEGIN");

    fsm_WriteVHDLNewLine(fsm, fp, 2);

    fprintf(fp, "IF %s = '%d' THEN", reset, reset_value);

    fsm_WriteVHDLResetAssignment(fsm, fp, 3);
    fsm_WriteVHDLNewLine(fsm, fp, 2);
    fprintf(fp, "ELSE");

    fsm_WriteVHDLAssignmentZZZ(fsm, fp, 3);

    fsm_WriteVHDLNewLine(fsm, fp, 2);
    fprintf(fp, "END IF;");
    fsm_WriteVHDLNewLine(fsm, fp, 1);
    fprintf(fp, "END PROCESS;");

  }

  fsm_WriteVHDLAssignmentInOut(fsm, fp, 1, opt); 

  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "END %s;", arch);
  fsm_WriteVHDLNewLine(fsm, fp, 0);

  return 1;
}

int fsm_WriteVHDLConfigurationFP(fsm_type fsm, FILE *fp, char *entity, char *arch)
{
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "CONFIGURATION cfg_%s OF %s IS", entity, entity);
    fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "FOR %s", arch);
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "END FOR;");
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "END cfg_%s;", entity);
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  return 1;
}


int fsm_WriteVHDL(fsm_type fsm, char *filename, char *entity, char *arch, char *clock, char *reset, int opt)
{
  FILE *fp;

  if ( fsm->reset_node_id < 0 )
    return fsm_Log(fsm, "FSM: Writing VHDL code requires a reset state (abort)."), 0;

  fsm_Log(fsm, "FSM: Writing VHDL code to '%s'.", filename);
  if ( clock != NULL && clock[0] != '\0' )
  {
    fsm_Log(fsm, "FSM: Clock lines are generated for '%s'.", entity);
  }
  else
  {
    fsm_Log(fsm, "FSM: No clock lines are generated for '%s' (no label specified).", entity);
  }
  


  fp = fopen(filename, "w");
  if ( fp == NULL )
    return 0;
  fsm_WriteVHDLEntityFP(fsm, fp, entity, clock, reset, opt);
  fsm_WriteVHDLArchitectureFP(fsm, fp, entity, arch, clock, reset, opt);
  fsm_WriteVHDLConfigurationFP(fsm, fp, entity, arch);
  fclose(fp);
  return 1;
}

/*---------------------------------------------------------------------------*/


int fsm_WriteVHDLTBEntityFP(fsm_type fsm, FILE *fp, char *tb_entity)
{

  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "LIBRARY ieee;");
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "USE ieee.std_logic_1164.all;");
  fsm_WriteVHDLNewLine(fsm, fp, 0);

  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "ENTITY %s IS", tb_entity);
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "END %s;", tb_entity);
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  return 1;
}

static int fsm_WriteVHDLTBPortMap(fsm_type fsm, FILE *fp, char *clock, char *reset, int opt)
{
  int is_first = 1;
  int i;
  
  for( i = 0; i < fsm_GetInCnt(fsm); i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fprintf(fp, "x(%d)", i);
  }

  for( i = 0; i < fsm_GetCodeWidth(fsm); i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fprintf(fp, "z(%d)", i);
  }

  for( i = 0; i < fsm_GetCodeWidth(fsm); i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fprintf(fp, "zz(%d)", i);
  }

  for( i = 0; i < fsm->pi_machine->out_cnt - fsm_GetCodeWidth(fsm); i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fprintf(fp, "y(%d)", i);
  }

  if ( is_first == 0 )
    fprintf(fp, ", ");
  is_first = 0;
  fprintf(fp, "%s", reset);

  if ( clock != NULL && clock[0] != '\0' )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    fprintf(fp, "%s", clock);
  }

  return 1;
}


struct device
{
  FILE *fp;
  int opt; 
  char *reset;
  char *clock;
  int hcp_ns; /* half clock period (nano seconds) */
  
  int edge_cnt; 
};

int fsm_WriteVHDLTBInput(fsm_type fsm, FILE *fp, dcube *c)
{
  int i;
  fsm_WriteVHDLNewLine(fsm, fp, 2);
  fprintf(fp, "x <= \"");
  for( i = 0; i < fsm_GetInCnt(fsm); i++ )
    putc((int)(unsigned char)("-01-"[dcGetIn(c, i)]), fp);
  fprintf(fp, "\";");
  return 1;
}


int fsm_WriteVHDLTBClockGoHigh(fsm_type fsm, FILE *fp, char *clock, int ns)
{
  fsm_WriteVHDLNewLine(fsm, fp, 2);
  fprintf(fp, "WAIT FOR %d ns;", ns);
  fsm_WriteVHDLNewLine(fsm, fp, 2);
  fprintf(fp, "%s <= '1';", clock);
  return 1;
}

int fsm_WriteVHDLTBClockGoLow(fsm_type fsm, FILE *fp, char *clock, int ns)
{
  fsm_WriteVHDLNewLine(fsm, fp, 2);
  fprintf(fp, "WAIT FOR %d ns;", ns);
  fsm_WriteVHDLNewLine(fsm, fp, 2);
  fprintf(fp, "%s <= '0';", clock);
  return 1;
}

int fsm_WriteVHDLTBStateAssert(fsm_type fsm, FILE *fp, int edge_id, int opt)
{
  int src_node, dest_node;
  char *src_str;
  char *dest_str;
  
  dest_node = fsm_GetEdgeDestNode(fsm, edge_id);
  src_node = fsm_GetEdgeSrcNode(fsm, edge_id);
  
  dest_str = fsm_GetNodeName(fsm, dest_node);
  src_str = fsm_GetNodeName(fsm, src_node);
  
  if ( dest_str == NULL ) dest_str = "";
  if ( src_str == NULL ) src_str = "";
  
  
  fsm_WriteVHDLNewLine(fsm, fp, 2);
  fprintf(fp, "ASSERT (");
  
  fprintf(fp, "zz");
  fprintf(fp, " = \"%s\") ", 
    dcOutToStr(fsm_GetCodePINFO(fsm), fsm_GetNodeCode(fsm, dest_node), ""));
    
 fprintf(fp, "REPORT \"transition %s -> %s failed\" ", src_str, dest_str);
 fprintf(fp, "SEVERITY ERROR;");
 
 return 1;
}

int fsm_WriteVHDLTBClockDevice(void *data, fsm_type fsm, int msg, int arg, dcube *c)
{
  struct device *d = (struct device *)data;
  switch(msg)
  {
    case FSM_TEST_MSG_START:
      break;
    case FSM_TEST_MSG_END:
      break;
    case FSM_TEST_MSG_RESET:
      d->edge_cnt = 0;
      fsm_WriteVHDLNewLine(fsm, d->fp, 2);
      if ( (d->opt & FSM_VHDL_OPT_CLR_ON_LOW) == FSM_VHDL_OPT_CLR_ON_LOW )
        fprintf(d->fp, "%s <= '0';", d->reset);  
      else
        fprintf(d->fp, "%s <= '1';", d->reset);  

      if ( d->clock != NULL && d->clock[0] != '\0' )
      {
        fsm_WriteVHDLNewLine(fsm, d->fp, 2);
        fprintf(d->fp, "%s <= '0';", d->clock);
      }
      if ( c != NULL )
        fsm_WriteVHDLTBInput(fsm, d->fp, c);
      fsm_WriteVHDLTBClockGoHigh(fsm, d->fp, d->clock, d->hcp_ns);
      fsm_WriteVHDLTBClockGoLow(fsm, d->fp, d->clock, d->hcp_ns);
      fsm_WriteVHDLNewLine(fsm, d->fp, 2);
      if ( (d->opt & FSM_VHDL_OPT_CLR_ON_LOW) == FSM_VHDL_OPT_CLR_ON_LOW )
        fprintf(d->fp, "%s <= '1';", d->reset);  
      else
        fprintf(d->fp, "%s <= '0';", d->reset);  
      break;
    case FSM_TEST_MSG_GO_EDGE:
    case FSM_TEST_MSG_DO_EDGE:
      fsm_WriteVHDLTBInput(fsm, d->fp, c);
      fsm_WriteVHDLTBClockGoHigh(fsm, d->fp, d->clock, d->hcp_ns);
      fsm_WriteVHDLTBClockGoLow(fsm, d->fp, d->clock, d->hcp_ns);
      fsm_WriteVHDLTBStateAssert(fsm, d->fp, arg, d->opt);
      break;
  }
  return 1;
}

int fsm_WriteVHDLTBGlitchCntClr(fsm_type fsm, FILE *fp, int indent, int opt)
{
  int i;
  int out_cnt = fsm->pi_machine->out_cnt - fsm_GetCodeWidth(fsm);
  if ( (opt&FSM_VHDL_OPT_CHK_GLITCH) != 0 )
  {
    if ( out_cnt != 0 )
    {
      fsm_WriteVHDLNewLine(fsm, fp, indent);
      fprintf(fp, "glitch_y := \"");
      for( i = 0; i < out_cnt; i++ )
        putc((int)(unsigned char)'0', fp);
      fprintf(fp, "\";");
    }

    fsm_WriteVHDLNewLine(fsm, fp, indent);
    fprintf(fp, "glitch_z := \"");
    for( i = 0; i < fsm_GetCodeWidth(fsm); i++ )
      putc((int)(unsigned char)'0', fp);
    fprintf(fp, "\";");

    fsm_WriteVHDLNewLine(fsm, fp, indent);
    fprintf(fp, "glitch_zz := \"");
    for( i = 0; i < fsm_GetCodeWidth(fsm); i++ )
      putc((int)(unsigned char)'0', fp);
    fprintf(fp, "\";");
  }
  return 1;
}

int fsm_WriteVHDLTBNoClockDevice(void *data, fsm_type fsm, int msg, int arg, dcube *c)
{
  struct device *d = (struct device *)data;
  switch(msg)
  {
    case FSM_TEST_MSG_START:
      break;
    case FSM_TEST_MSG_END:
      break;
    case FSM_TEST_MSG_RESET:
      d->edge_cnt = 0;
      fsm_WriteVHDLNewLine(fsm, d->fp, 2);
      
      if ( (d->opt & FSM_VHDL_OPT_CLR_ON_LOW) == FSM_VHDL_OPT_CLR_ON_LOW )
        fprintf(d->fp, "%s <= '0';", d->reset);  
      else
        fprintf(d->fp, "%s <= '1';", d->reset);  

      if ( d->clock != NULL && d->clock[0] != '\0' )
      {
        fsm_WriteVHDLNewLine(fsm, d->fp, 2);
        fprintf(d->fp, "%s <= '0';", d->clock);
      }
      if ( c != NULL )
        fsm_WriteVHDLTBInput(fsm, d->fp, c);
      fsm_WriteVHDLNewLine(fsm, d->fp, 2);
      fprintf(d->fp, "WAIT FOR %d ns;", d->hcp_ns*2);
      fsm_WriteVHDLTBGlitchCntClr(fsm, d->fp, 2, d->opt);
      fsm_WriteVHDLNewLine(fsm, d->fp, 2);
      if ( (d->opt & FSM_VHDL_OPT_CLR_ON_LOW) == FSM_VHDL_OPT_CLR_ON_LOW )
        fprintf(d->fp, "%s <= '1';", d->reset);  
      else
        fprintf(d->fp, "%s <= '0';", d->reset);  
      fsm_WriteVHDLNewLine(fsm, d->fp, 2);
      fprintf(d->fp, "WAIT FOR %d ns;", d->hcp_ns*2);
      fsm_WriteVHDLTBGlitchCntClr(fsm, d->fp, 2, d->opt);
      break;
    case FSM_TEST_MSG_GO_EDGE:
    case FSM_TEST_MSG_DO_EDGE:
      fsm_WriteVHDLTBInput(fsm, d->fp, c);
      fsm_WriteVHDLNewLine(fsm, d->fp, 2);
      fprintf(d->fp, "WAIT FOR %d ns;", d->hcp_ns*2);
      fsm_WriteVHDLTBGlitchCntClr(fsm, d->fp, 2, d->opt);
      fsm_WriteVHDLTBStateAssert(fsm, d->fp, arg, d->opt);
      break;
  }
  return 1;
}


int fsm_WriteVHDLTBDoTestSequence(fsm_type fsm, FILE *fp, fsmtl_type tl, char *clock, char *reset, int opt, int ns)
{
  struct device d;
  d.fp = fp;
  d.opt = opt;
  d.reset = reset;
  d.clock = clock;
  d.edge_cnt = 0;
  d.hcp_ns = ns;

  if ( clock != NULL && clock[0] != '\0' )
    fsmtl_SetDevice(tl, fsm_WriteVHDLTBClockDevice, &d);
  else
    fsmtl_SetDevice(tl, fsm_WriteVHDLTBNoClockDevice, &d);
  return fsmtl_Do(tl);
}


int fsm_WriteVHDLTBArchitectureFP(fsm_type fsm, FILE *fp, fsmtl_type tl, char *tb_entity, char *entity, char *clock, char *reset, int opt, int ns)
{
  char *tb_arch = "test";
  int out_cnt = fsm->pi_machine->out_cnt - fsm_GetCodeWidth(fsm);
  
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "ARCHITECTURE %s OF %s IS", tb_arch , tb_entity);
  
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "COMPONENT %s", entity);
  fsm_WriteVHDLMachinePortFP(fsm, fp, clock, reset, opt);
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "END COMPONENT;");

  
  /* write all signals */
  
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "SIGNAL x : STD_LOGIC_VECTOR(0 to %d);", fsm_GetInCnt(fsm)-1);
  if ( out_cnt != 0 )
  {
    fsm_WriteVHDLNewLine(fsm, fp, 1);
    fprintf(fp, "SIGNAL y : STD_LOGIC_VECTOR(0 to %d);", out_cnt-1);
    if ( (opt&FSM_VHDL_OPT_CHK_GLITCH) != 0 )
    {
      fsm_WriteVHDLNewLine(fsm, fp, 1);
      fprintf(fp, "SHARED VARIABLE glitch_y : STD_LOGIC_VECTOR(0 to %d);", fsm_GetOutCnt(fsm)-1);
    }
  }
  
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "SIGNAL z : STD_LOGIC_VECTOR(0 to %d);", 
    fsm_GetCodeWidth(fsm)-1);

  if ( (opt&FSM_VHDL_OPT_CHK_GLITCH) != 0 )
  {
    fsm_WriteVHDLNewLine(fsm, fp, 1);
    fprintf(fp, "SHARED VARIABLE glitch_z : STD_LOGIC_VECTOR(0 to %d);", 
      fsm_GetCodeWidth(fsm)-1);
  }

  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "SIGNAL zz : STD_LOGIC_VECTOR(0 to %d);", fsm_GetCodeWidth(fsm)-1);

  if ( (opt&FSM_VHDL_OPT_CHK_GLITCH) != 0 )
  {
    fsm_WriteVHDLNewLine(fsm, fp, 1);
    fprintf(fp, "SHARED VARIABLE glitch_zz : STD_LOGIC_VECTOR(0 to %d);", fsm_GetCodeWidth(fsm)-1);
  }
      
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "SIGNAL %s : STD_LOGIC;", reset);
  if ( clock != NULL && clock[0] != '\0' )
  {
    fsm_WriteVHDLNewLine(fsm, fp, 1);
    fprintf(fp, "SIGNAL %s : STD_LOGIC;", clock);
  }
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "BEGIN");

  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "DUT: %s PORT MAP(", entity);
  fsm_WriteVHDLTBPortMap(fsm, fp, clock, reset, opt);
  fprintf(fp, ");");
  
  
  /* optional VHDL processes for glitch detection */
  
  if ( (opt&FSM_VHDL_OPT_CHK_GLITCH) != 0 )
  {
    int i;
    for( i = 0; i < out_cnt; i++ )
    {
      fsm_WriteVHDLNewLine(fsm, fp, 0);
      fsm_WriteVHDLNewLine(fsm, fp, 1);
      fprintf(fp, "PROCESS(y(%d))", i);
      fsm_WriteVHDLNewLine(fsm, fp, 1);
      fprintf(fp, "BEGIN");
      fsm_WriteVHDLNewLine(fsm, fp, 2);
      fprintf(fp, "IF ( y(%d) = '0' OR y(%d) = '1' ) THEN", i, i);
      fsm_WriteVHDLNewLine(fsm, fp, 3);
      fprintf(fp, "ASSERT ( glitch_y(%d) /= '1' ) REPORT \"Glitch on output y(%d) found.\" SEVERITY ERROR;",
        i, i);
      fsm_WriteVHDLNewLine(fsm, fp, 3);
      fprintf(fp, "glitch_y(%d) := '1';", i);
      fsm_WriteVHDLNewLine(fsm, fp, 2);
      fprintf(fp, "END IF;");
      fsm_WriteVHDLNewLine(fsm, fp, 1);
      fprintf(fp, "END PROCESS;");
    }

    for( i = 0; i < fsm_GetCodeWidth(fsm); i++ )
    {
      fsm_WriteVHDLNewLine(fsm, fp, 0);
      fsm_WriteVHDLNewLine(fsm, fp, 1);
      fprintf(fp, "PROCESS(z(%d))", i);
      fsm_WriteVHDLNewLine(fsm, fp, 1);
      fprintf(fp, "BEGIN");
      fsm_WriteVHDLNewLine(fsm, fp, 2);
      fprintf(fp, "IF ( z(%d) = '0' OR z(%d) = '1' ) THEN", i, i);
      fsm_WriteVHDLNewLine(fsm, fp, 3);
      fprintf(fp, "ASSERT ( glitch_z(%d) /= '1' ) REPORT \"Glitch on feedback z(%d) found.\" SEVERITY ERROR;",
        i, i);
      fsm_WriteVHDLNewLine(fsm, fp, 3);
      fprintf(fp, "glitch_z(%d) := '1';", i);
      fsm_WriteVHDLNewLine(fsm, fp, 2);
      fprintf(fp, "END IF;");
      fsm_WriteVHDLNewLine(fsm, fp, 1);
      fprintf(fp, "END PROCESS;");
    }

    for( i = 0; i < fsm_GetCodeWidth(fsm); i++ )
    {
      fsm_WriteVHDLNewLine(fsm, fp, 0);
      fsm_WriteVHDLNewLine(fsm, fp, 1);
      fprintf(fp, "PROCESS(zz(%d))", i);
      fsm_WriteVHDLNewLine(fsm, fp, 1);
      fprintf(fp, "BEGIN");
      fsm_WriteVHDLNewLine(fsm, fp, 2);
      fprintf(fp, "IF ( zz(%d) = '0' OR zz(%d) = '1' ) THEN", i, i);
      fsm_WriteVHDLNewLine(fsm, fp, 3);
      fprintf(fp, "ASSERT ( glitch_zz(%d) /= '1' ) REPORT \"Glitch on feedback z(%d) found.\" SEVERITY ERROR;",
        i, i);
      fsm_WriteVHDLNewLine(fsm, fp, 3);
      fprintf(fp, "glitch_zz(%d) := '1';", i);
      fsm_WriteVHDLNewLine(fsm, fp, 2);
      fprintf(fp, "END IF;");
      fsm_WriteVHDLNewLine(fsm, fp, 1);
      fprintf(fp, "END PROCESS;");
    }
  }

  
  /* write the testbench for the design */
  
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "PROCESS");
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "BEGIN");
  fsm_WriteVHDLTBDoTestSequence(fsm, fp, tl, clock, reset, opt, ns);
  fsm_WriteVHDLNewLine(fsm, fp, 2);
  fprintf(fp, "WAIT;");
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "END PROCESS;");
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "END test;");
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  
  return 1;
}

int fsm_WriteVHDLTBConfigurationFP(fsm_type fsm, FILE *fp, char *tb_entity)
{
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "CONFIGURATION cfg_%s OF %s IS", tb_entity, tb_entity);
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "FOR test");
  fsm_WriteVHDLNewLine(fsm, fp, 1);
  fprintf(fp, "END FOR;");
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  fprintf(fp, "END cfg_%s;", tb_entity);
  fsm_WriteVHDLNewLine(fsm, fp, 0);
  return 1;
}

int fsm_WriteVHDLTBFP(fsm_type fsm, FILE *fp, fsmtl_type tl, char *tb_entity, 
  char *entity, char *clock, char *reset, int opt, int ns)
{
  fsm_WriteVHDLTBEntityFP(fsm, fp, tb_entity);
  fsm_WriteVHDLTBArchitectureFP(fsm, fp, tl, tb_entity, entity, clock, reset, opt, ns);
  fsm_WriteVHDLTBConfigurationFP(fsm, fp, tb_entity);
  return 1;
}

int fsm_WriteVHDLTB(fsm_type fsm, char *filename, fsmtl_type tl, char *tb_entity, 
  char *entity, char *clock, char *reset, int opt, int ns)
{
  FILE *fp;
  
  fp = fopen(filename, "w");
  if ( fp == NULL )
    return 0;
    
  fsm_WriteVHDLTBFP(fsm, fp, tl, tb_entity, entity, clock, reset, opt, ns);
  
  fclose(fp);
  
  return 1;
}

