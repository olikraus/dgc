/*

  xbm2pla.c
  
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
  
  
  THIS IS JUST A TEST APPLICATION... NOTING USEFUL JET....
    
*/

#include "config.h"
#include "cmdline.h"
#include "xbm.h"

int is_fbo = 0;
int is_mis = 0;
int is_sync = 0;
int log_level = 4;
int is_show_walk = 0;
long tb_reset_type = XBM_RESET_LOW;

char kiss_file_name[1024] = "";
char vcg_file_name[1024] = "";
char dot_file_name[1024] = "";
char bex_file_name[1024] = "";
char pla_file_name[1024] = "";
char eqn_file_name[1024] = "";
int is_auto_ddc_level = 1;
int is_auto_ddc_edge = 0;
int is_auto_ddc_mix = 0; 
char tb_vhdl_file_name[1024] = "";
char tb_dut_name[1024] = "mydesign";
long tb_wait_time_ns = 100;
char normal_partitions_file_name[1024] = "";
char prime_partitions_file_name[1024] = "";
char minimized_partitions_file_name[1024] = "";
char state_encoding_file_name[1024] = "";
char import_state_encoding_file_name[1024] = "";
char import_machine_function_file_name[1024] = "";

cl_entry_struct cl_list[] =
{
  { CL_TYP_GROUP,   "Output", NULL, 0 },
  { CL_TYP_STRING,  "ok-write KISS file", kiss_file_name, 1024 },
  { CL_TYP_STRING,  "ovcg-write VCG file", vcg_file_name, 1024 },
  { CL_TYP_STRING,  "odot-write DOT file", dot_file_name, 1024 },
  { CL_TYP_STRING,  "ob-write BEX file", bex_file_name, 1024 },
  { CL_TYP_STRING,  "op-write PLA file", pla_file_name, 1024 },
  { CL_TYP_STRING,  "oeqn-write EQN (3D) file", eqn_file_name, 1024 },
  { CL_TYP_GROUP,   "Options", NULL, 0 },
  { CL_TYP_LONG,    "ll-log level, 1: all messages, 7: no messages", &log_level, 0 },
  { CL_TYP_ON,      "fbo-enable feed-back optimization", &is_fbo,  0 },
  { CL_TYP_OFF,     "nofbo-disable feed-back optimization", &is_fbo,  0 },
  { CL_TYP_ON,      "mis-enable state minimization", &is_mis,  0 },
  { CL_TYP_OFF,     "nomis-disable state minimization", &is_mis,  0 },
  { CL_TYP_ON,      "sync-synchronous machines", &is_sync,  0 },
  { CL_TYP_OFF,     "async-asynchronous machines", &is_sync,  0 },
  { CL_TYP_GROUP,   "Advanced options", NULL, 0 },
  { CL_TYP_ON,      "ddclevel-apply 'directed don't care' to all level variables", &is_auto_ddc_level,  0 },
  { CL_TYP_OFF,     "noddclevel-do not apply 'directed don't care' to all level variables", &is_auto_ddc_level,  0 },
  { CL_TYP_ON,      "ddcedge-apply 'directed don't care' to all edge variables", &is_auto_ddc_edge,  0 },
  { CL_TYP_OFF,     "noddcedge-do not apply 'directed don't care' to all edge variables", &is_auto_ddc_edge,  0 },
  { CL_TYP_ON,      "ddcmix-apply 'directed don't care' to all edge AND level variables", &is_auto_ddc_mix,  0 },
  { CL_TYP_OFF,     "noddcmix-do not apply 'directed don't care' to all edge AND level variables", &is_auto_ddc_mix,  0 },
  { CL_TYP_GROUP,   "Testbench generation", NULL, 0 },
  { CL_TYP_STRING,  "tbvhdl-write VHDL testbench", tb_vhdl_file_name, 1024 },
  { CL_TYP_STRING,  "tbdut-DUT/component name", tb_dut_name, 1024 },
  { CL_TYP_SET,     "tbrsl-Generate low aktive reset", &tb_reset_type, XBM_RESET_LOW },
  { CL_TYP_SET,     "tbrsh-Generate high aktive reset", &tb_reset_type,  XBM_RESET_HIGH},
  { CL_TYP_LONG,    "tbwait-Argument (nanoseconds) for the 'wait' statement.", &tb_wait_time_ns,  0},
  { CL_TYP_GROUP,   "Debug", NULL, 0 },
  { CL_TYP_STRING,  "donp-write partitions", normal_partitions_file_name, 1024 },
  { CL_TYP_STRING,  "dopp-write prime partitions", prime_partitions_file_name, 1024 },
  { CL_TYP_STRING,  "domp-write minimized partitions", minimized_partitions_file_name, 1024 },
  { CL_TYP_STRING,  "dose-write state encoding", state_encoding_file_name, 1024 },
  { CL_TYP_STRING,  "imse-import state encoding", import_state_encoding_file_name, 1024 },
  { CL_TYP_STRING,  "immf-import machine function", import_machine_function_file_name, 1024 },
  { CL_TYP_ON,      "walk-show walk through all states", &is_show_walk,  0 },
  CL_ENTRY_LAST
};

int doall(xbm_type x)
{
  int opt = 0;
  
  x->log_level = log_level;
  
  x->is_auto_ddc_level = is_auto_ddc_level;
  x->is_auto_ddc_edge = is_auto_ddc_edge;
  x->is_auto_ddc_mix = is_auto_ddc_mix; 
  
  
  if ( is_fbo != 0 )
    opt |= XBM_BUILD_OPT_FBO;

  if ( is_mis != 0 )
    opt |= XBM_BUILD_OPT_MIS;
    
  if ( is_sync != 0 )
    opt |= XBM_BUILD_OPT_SYNC;
  
  if ( xbm_ReadBMS(x, cl_file_list[0]) == 0 )
    return 0;
  
  if ( kiss_file_name[0] != '\0' )
  {
    xbm_WriteKISS(x, kiss_file_name);
    if ( normal_partitions_file_name[0] == '\0' &&
         minimized_partitions_file_name[0] == '\0' &&
         bex_file_name[0] == '\0' &&
         pla_file_name[0] == '\0' &&
         eqn_file_name[0] == '\0' )
      return 1;
  }
  
 if ( vcg_file_name[0] != '\0' )
  {
    xbm_WriteVCG(x, vcg_file_name);
    if ( normal_partitions_file_name[0] == '\0' &&
         minimized_partitions_file_name[0] == '\0' &&
         bex_file_name[0] == '\0' &&
         pla_file_name[0] == '\0' &&
         eqn_file_name[0] == '\0' )
      return 1;
  }
  
   if ( dot_file_name[0] != '\0' )
  {
    xbm_WriteDOT(x, dot_file_name);
    if ( normal_partitions_file_name[0] == '\0' &&
         minimized_partitions_file_name[0] == '\0' &&
         bex_file_name[0] == '\0' &&
         pla_file_name[0] == '\0' &&
         eqn_file_name[0] == '\0' )
      return 1;
  }
  if ( normal_partitions_file_name[0] != '\0' )
    xbm_SetPLAFileNameXBMPartitions(x, normal_partitions_file_name);

  if ( prime_partitions_file_name[0] != '\0' )
    xbm_SetPLAFileNamePrimePartitions(x, prime_partitions_file_name);
    
  if ( minimized_partitions_file_name[0] != '\0' )
    xbm_SetPLAFileNameMinPartitions(x, minimized_partitions_file_name);

  if ( import_state_encoding_file_name[0] != '\0' )
    xbm_SetImportFileNameStateCodes(x, import_state_encoding_file_name);

  if ( import_machine_function_file_name[0] != '\0' )
    xbm_SetImportFileNamePLAMachine(x, import_machine_function_file_name);

  if ( xbm_Build(x, opt) == 0 )
    return 0;

  if ( state_encoding_file_name[0] != '\0' )
    xbm_WriteEncoding(x, state_encoding_file_name);
  
  if ( bex_file_name[0] != '\0' )
    xbm_WriteBEX(x, bex_file_name);

  if ( pla_file_name[0] != '\0' )
    xbm_WritePLA(x, pla_file_name);

  if ( eqn_file_name[0] != '\0' )
    xbm_Write3DEQN(x, eqn_file_name);

  if ( tb_vhdl_file_name[0] != '\0' )
  {
    x->tb_reset_type = tb_reset_type;
    x->tb_wait_time_ns = tb_wait_time_ns;
    xbm_SetVHDLComponentName(x, tb_dut_name);
    xbm_WriteTestbenchVHDL(x, tb_vhdl_file_name);
  }
  
 

  if ( pla_file_name[0] == '\0' && bex_file_name[0] == '\0' && eqn_file_name[0] == '\0' )
  {
    if ( x->cl_machine != NULL && x->pi_machine != NULL )
    {
      printf(".i %d\n", x->pi_machine->in_cnt);
      printf(".o %d\n", x->pi_machine->out_cnt);
      printf(".p %d\n", dclCnt(x->cl_machine));
      dclShow(x->pi_machine, x->cl_machine);
    }
  }
  
  if ( is_show_walk != 0 )
    xbm_ShowWalk(x);
    
  return 1;
}

int main(int argc, char *argv[])
{
  xbm_type x;

  if ( cl_Do(cl_list, argc, argv) == 0 )
  {
    puts("cmdline error");
    exit(1);
  }

  if ( cl_file_cnt < 1 )
  {
    puts("XBM converter");
    puts("Creates a boolean function from an extended burst mode description.");
    puts(COPYRIGHT);
    puts(FREESOFT);
    puts(REDIST);
    printf("Usage: %s [options] <bms/xbm file>\n", argv[0]);
    puts("options:");
    cl_OutHelp(cl_list, stdout, " ", 14);
    exit(1);
  }

  x = xbm_Open();
  if ( x != NULL )
  {
    doall(x);
    xbm_Close(x);
  }
  
  return 0;
}

