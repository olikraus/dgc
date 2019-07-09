/*

  simfsm.c

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
#include "config.h"
#include "fsm.h"
#include "dgd_opt.h"
#include "fsmtest.h"
#include "fsmenc.h"
#include "cmdline.h"
#include <stdio.h>
#include <string.h>

char out_vhdl_name[1024] = "";
char out_vhdltb_name[1024] = "";
char vhdl_entity_name[1024] = "mydesign";
char vhdltb_entity_name[1024] = "testbench";
char vhdl_arch_name[1024] = "rtl";
char vhdl_clock_name[1024] = "clk";
char vhdl_reset_name[1024] = "clr";
/*
 * int is_state_port = 0;
 * int is_state_dub = 0;
 */
int is_glitch_detection = 0;
long encoding = 0;
long ns = 20;
long reset_type = 1;
int t_is_min_state = 0;

cl_entry_struct cl_list[] =
{
/*
  { CL_TYP_SET,     "enclin-use linear state encoding", &encoding, 0 },
  { CL_TYP_SET,     "encas-use (old) asynchronous state encoding", &encoding, 1 },
  { CL_TYP_SET,     "encbm-use burst machine asynchronous state encoding", &encoding, 2 },
  { CL_TYP_SET,     "encao-use burst machine asynchronous state encoding with output optimization", &encoding, 3 },
*/  
  { CL_TYP_ON,      DGD_HL_MIN_STATE "-Enable state minimization", &t_is_min_state, 0 },
  { CL_TYP_OFF,     DGD_HL_NO_MIN_STATE "-Disable state minimization", &t_is_min_state, 0 },
/*
  { CL_TYP_ON,      "zport-include state signals into the port of the VHDL model", &is_state_port, 0 },
  { CL_TYP_ON,      "zdub-use two sets of state signals", &is_state_dub, 0 },
*/
/*
  { CL_TYP_ON,      "glitch-add VHDL code for glitch detection", &is_glitch_detection, 0 },
*/
  { CL_TYP_STRING,  "ovtb-generate VHDL testbench", out_vhdltb_name, 1022 },
  { CL_TYP_STRING,  "ov-generate VHDL model", out_vhdl_name, 1022 }, 
  { CL_TYP_STRING,  "tbe-entity name for the VHDL testbench", vhdltb_entity_name, 1022 },
  { CL_TYP_STRING,  "e-entity name for the VHDL model", vhdl_entity_name, 1022 },
  { CL_TYP_STRING,  "a-architecture name for the VHDL model", vhdl_arch_name, 1022 },
  { CL_TYP_STRING,  "c-name of the clock line for VHDL", vhdl_clock_name, 1022 },
  { CL_TYP_STRING,  "r-name of the reset line for VHDL", vhdl_reset_name, 1022 },
  { CL_TYP_LONG,    "p-length of clock period in nanoseconds", &ns, 0 },
  { CL_TYP_SET,     "rsl-Generate low aktive reset", &reset_type, 1 },
  { CL_TYP_SET,     "rsh-Generate high aktive reset", &reset_type, 0 },
  CL_ENTRY_LAST
};


int do_all(fsm_type fsm)
{
  int ret = 0;
  int vhdl_opt = 0;
  int is_tl = 1;
  
/*
 *   if ( is_state_port ) 
 *     vhdl_opt |= FSM_VHDL_OPT_STATE_PORT;
 *     
 *   if ( is_state_dub )
 *     vhdl_opt |= FSM_VHDL_OPT_STATE_DUB;
 */
    
  if ( is_glitch_detection )
    vhdl_opt |= FSM_VHDL_OPT_CHK_GLITCH;
    
  if ( reset_type == 1 )
    vhdl_opt |= FSM_VHDL_OPT_CLR_ON_LOW;
    

  if ( t_is_min_state != 0 )
    if ( encoding == 3 )  /* fbo */
    {
      if ( fsm_MinimizeStates(fsm, 1, 0) == 0 )
        return 0;
    }
    else
    {
      if ( fsm_MinimizeStates(fsm, 0, 0) == 0 )
        return 0;
    }
  
  switch ( encoding )
  {
    case 0:
      ret = fsm_BuildClockedMachine(fsm, FSM_ENCODE_SIMPLE);
      break;
    case 1:
      ret = fsm_BuildAsynchronousMachine(fsm, 0);
      break;
    case 2:
      ret = fsm_BuildBurstModeMachine(fsm, 0, FSM_ASYNC_ENCODE_ZERO_MAX);
      break;
    case 3:
      ret = fsm_BuildBurstModeMachine(fsm, 0, FSM_ASYNC_ENCODE_WITH_OUTPUT);
      vhdl_opt |= FSM_VHDL_OPT_FBO;
      break;
  }
    
  if ( ret == 0 )
  {
    puts("Construction of the 'next step' and 'output' function failed.");
    return 0;
  }
  
  if ( out_vhdl_name[0] != '\0' )
  {
    if ( fsm_WriteVHDL(fsm, out_vhdl_name, vhdl_entity_name, vhdl_arch_name, vhdl_clock_name, vhdl_reset_name, vhdl_opt) == 0 )
    {
      printf("Generation fo VHDL model (file '%s') failed.\n", out_vhdl_name);
    }
  }

  if ( is_tl != 0 )
  {
    fsmtl_type tl;
    tl = fsmtl_Open(fsm);
    if ( tl != NULL )
    {
      fsmtl_AddAllEdgeSequence(tl);
      if ( out_vhdltb_name[0] != '\0' )
      {
        fsm_WriteVHDLTB(fsm, out_vhdltb_name, tl, vhdltb_entity_name, 
          vhdl_entity_name, vhdl_clock_name, vhdl_reset_name, vhdl_opt, ns/2);
      }
      fsmtl_Close(tl);
    }
    else
    {
      puts("Memory error (fsmtl_Open).");
    }
  }
  
  return 1;
}

int main(int argc, char **argv)
{
  fsm_type fsm;
  
  if ( cl_Do(cl_list, argc, argv) == 0 )
  {
    puts("cmdline error");
    exit(1);
  }

  if ( cl_file_cnt < 1 )
  {
    puts("Digital Gate: Create files for VHDL-simulation.");
    puts(COPYRIGHT);
    puts(FREESOFT);
    puts(REDIST);
    printf("Usage: simfsm [options] <input>\n");
    puts("<input> KISS or BMS file");
    puts("options:");
    cl_OutHelp(cl_list, stdout, " ", 11);
/*    
    puts("notes:");
    puts("  -c     should be used if the vhdl code should make ");
    puts("         use of a clock signal.");
    puts("  Combination of -enclin, -encas, -encbm and -c clk:");
    puts("    -enclin -encas -encbm -encao -c clk");
    puts("     yes     no     no     no     no    : Illegal asynchronous machine.");
    puts("     no      yes    no     no     no    : Asynchronous VHDL machine.");
    puts("     no      no     yes    yes    no    : Burst Mode asynchronous VHDL machine.");
    puts("     yes     no     no     no     yes   : Normal synchronous machine.");
    puts("     no      yes    no     no     yes   : Large synchronous machine.");
    puts("     no      no     yes    yes    yes   : Large synchronous machine.");
    puts("  Asynchronous state machines:");
    puts("     'dgc -async'             use -encbm");
    puts("     'dgc -async -fbo'        use -encao");
    puts("     'dgc -async -fbo -mis'   use -encao -mis");
    puts("  VHDL simulation of dgc results:");
*/
/*    puts("    Use same VHDL interface: -zdub -zport"); */
/*
    puts("    Use identical design NAME:  dgc ... -tcell NAME ...");
    puts("                                simfsm ... -e NAME ...");
    puts("    If the name of the testbench is TBNAME (-tbe TBNAME) use the");
    puts("    configuration cfg_TBNAME for the VHDL simulation.");
*/
    
    exit(1);
  }
  
  if ( strcmp(out_vhdl_name, cl_file_list[0]) == 0 )
  {
    printf("VHDL file must differ from input file.\n");
    exit(1);
  }
  
  fsm = fsm_Open(fsm);
  if ( fsm == NULL )
  {
    puts("Out of memory.");
    exit(1);
  }

  if ( fsm_Import(fsm, cl_file_list[0]) != 0 )
  {
    do_all(fsm);
  }
  else
  {
    printf("Can not read '%s'.\n", cl_file_list[0]);
  }
  
  fsm_Close(fsm);
  
  return 1;
} 


