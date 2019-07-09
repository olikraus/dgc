/*

  DIGITAL GATE COMPILER
  
  dgc.c
  
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

#include "cmdline.h"
#include "gnet.h"
#include <stdio.h>
#include <string.h>
#include "b_ff.h"
#include "config.h"
#include "dgd_opt.h"
#include "b_time.h"

char cell_name[1024] = "mydesign";
char target_library_name[1024] = "mylib";
char import_lib_name[1024] = "default.genlib";
char sim_lib_name[1024] = "vhdlgatelib";
char delay_cell_name[1024] = "";
char out_edif_name[1024] = "";
char out_vhdl_name[1024] = "";
char out_xnf_name[1024] = "";
char out_bex_name[1024] = "";
char out_pla_name[1024] = "";
char cell_library_name[1024] = "gatelib";
char view_name[1024] = "symbol";
int is_async = 0;
long log_level = 4;
long reset_type = GNC_HL_OPT_CLR_LOW;
long state_encoding = GNC_HL_OPT_ENC_SIMPLE;
long delay_safety = 10L;

int t_is_neca = 1;
int t_is_generic = 1;
int t_is_library = 1;
int t_is_multi_level = 1;
int t_is_cross_output = 1;
int t_is_2l_min = 1;
int t_is_delay = 1;
int t_is_old_delay_method = 0;
int t_is_delay_path = 0;
int t_is_flatten = 1;
int t_is_rso = 0;
int t_is_fbo = 0;
int t_is_cap_report = 0;
int t_is_bm_check = 0;
int t_is_min_state = 0;
int t_is_old_min_state = 0;

int is_synopsis_msg = 0;

char disable_cells[1024*2] = "";

cl_entry_struct cl_list[] =
{
  { CL_TYP_GROUP,   "General", NULL, 0 },
  { CL_TYP_PATH,    "lib-Gate library file (synopsys '.lib' or genlib file)", import_lib_name, 1022 },
  { CL_TYP_GROUP,   "Synthesis", NULL, 0 },
  { CL_TYP_STRING,  "dcl-Disable cells", disable_cells, 1024*2-2 },
  { CL_TYP_ON,      DGD_HL_NO_CLOCK "-Generate asynchrounous state machine", &is_async, 0 },
  { CL_TYP_SET,     DGD_HL_CLR_LOW "-Generate low aktive reset", &reset_type, GNC_HL_OPT_CLR_LOW },
  { CL_TYP_SET,     DGD_HL_CLR_HIGH "-Generate high aktive reset", &reset_type, GNC_HL_OPT_CLR_HIGH },
  { CL_TYP_SET,     DGD_HL_NO_RESET "-Do not generate a reset signal", &reset_type, GNC_HL_OPT_NO_RESET },
  { CL_TYP_ON,      DGD_HL_RSO "-Reset of output lines (only XBM)", &t_is_rso, 0 },
  { CL_TYP_OFF,     DGD_HL_NO_RSO "-Disable reset of output lines (only XBM)", &t_is_rso, 0 },
  { CL_TYP_ON,      DGD_HL_FBO "-Enable use of output lines for encoding (only asynchronous machines)", &t_is_fbo, 0 },
  { CL_TYP_OFF,     DGD_HL_NO_FBO "-Disable use of output lines for encoding (only asynchronous machines)", &t_is_fbo, 0 },
  { CL_TYP_ON,      DGD_NECA "-Enable Net Cache Optimization", &t_is_neca, 0 },
  { CL_TYP_OFF,     DGD_NO_NECA "-Disable Net Cache Optimization", &t_is_neca, 0 },
  { CL_TYP_ON,      DGD_GENERIC "-Enable Generic Cell Optimization", &t_is_generic, 0 },
  { CL_TYP_OFF,     DGD_NO_GENERIC "-Disable Generic Cell Optimization", &t_is_generic, 0 },
  { CL_TYP_ON,      DGD_LIBRARY "-Enable Technology Optimization", &t_is_library, 0 },
  { CL_TYP_OFF,     DGD_NO_LIBRARY "-Disable Technology Optimization", &t_is_library, 0 },
  { CL_TYP_ON,      DGD_LEVELS "-Enable Multi Level Optimization", &t_is_multi_level, 0 },
  { CL_TYP_OFF,     DGD_NO_LEVELS "-Disable Multi Level Optimization", &t_is_multi_level, 0 },
  { CL_TYP_ON,      DGD_OUTPUTS "-Enable Cross Output Optimization", &t_is_cross_output, 0 },
  { CL_TYP_OFF,     DGD_NO_OUTPUTS "-Disable Cross Output Optimization", &t_is_cross_output, 0 },
  { CL_TYP_ON,      "optm-Enable 2-Level Minimization", &t_is_2l_min, 0 },
  { CL_TYP_OFF,     "noptm-Disable 2-Level Minimization", &t_is_2l_min, 0 },
  { CL_TYP_ON,      "dlycor-Enable delay correction", &t_is_delay, 0 },
  { CL_TYP_OFF,     "nodlycor-Disable delay correction", &t_is_delay, 0 },
  { CL_TYP_ON,      "olddlycor-Enable old delay correction method", &t_is_old_delay_method, 0 },
  { CL_TYP_OFF,     "noolddlycor-Disable delay correction", &t_is_old_delay_method, 0 },
  { CL_TYP_ON,      "dlypath-Enable delay path construction", &t_is_delay_path, 0 },
  { CL_TYP_OFF,     "nodlypath-Disable delay path construction", &t_is_delay_path, 0 },
  { CL_TYP_ON,      DGD_HL_FLATTEN "-Enable flatten of DGD files", &t_is_flatten, 0 },
  { CL_TYP_OFF,     DGD_HL_NO_FLATTEN "-Disable flatten of DGD files", &t_is_flatten, 0 },
  { CL_TYP_ON,      DGD_HL_MIN_STATE "-Enable state minimization", &t_is_min_state, 0 },
  { CL_TYP_ON,      "oldmis" "-Enable state minimization, old method", &t_is_old_min_state, 0 },
  { CL_TYP_OFF,     DGD_HL_NO_MIN_STATE "-Disable state minimization", &t_is_min_state, 0 },
  { CL_TYP_ON,      "synoplibmsg" "-Enable error messages during library import", &is_synopsis_msg, 0 },
  { CL_TYP_LONG,    "dlysafety-additional delay safety (per cent)", &delay_safety, 0 },
  
  

  { CL_TYP_SET,     "encsimple-Use simple state encoding", &state_encoding, GNC_HL_OPT_ENC_SIMPLE },
  { CL_TYP_SET,     "encfi-Use 'fan in' state encoding", &state_encoding, GNC_HL_OPT_ENC_FAN_IN },
  { CL_TYP_SET,     "encica-Use 'all input constrains' state encoding", &state_encoding, GNC_HL_OPT_ENC_IC_ALL },
  { CL_TYP_SET,     "encicp-Use 'input constraints partially' state encoding", &state_encoding, GNC_HL_OPT_ENC_IC_PART },

  { CL_TYP_GROUP,   "Export netlist", NULL, 0 },
  { CL_TYP_PATH,    "oe-Generate EDIF netlist", out_edif_name, 1022 },
  { CL_TYP_PATH,    "ov-Generate VHDL netlist", out_vhdl_name, 1022 },
  { CL_TYP_PATH,    "ox-Generate XNF (Xilinx netlist format)", out_xnf_name, 1022 },
  { CL_TYP_NAME,    "tcell-VHDL: Entity name, EDIF: Cell name", cell_name, 1022 },
  { CL_TYP_NAME,    "vlib-VHDL: Additional library name for the vhdl export", sim_lib_name, 1022 },
  { CL_TYP_NAME,    "tlib-EDIF: Name of design library", target_library_name, 1022 },
  { CL_TYP_NAME,    "view-EDIF: Name of view of the netlist", view_name, 1022 },
  { CL_TYP_NAME,    "clib-EDIF: Name of the the cell library", cell_library_name, 1022 },
  { CL_TYP_GROUP,   "Export boolean function", NULL, 0 },
  { CL_TYP_PATH,    "op-Write PLA file", out_pla_name, 1022 },
  { CL_TYP_PATH,    "ob-Write BEX file (boolean expression)", out_bex_name, 1022 },
  /*
  { CL_TYP_LONG,    "d-delay (ns) of the wait statement of the optional testbench for the VHDL netlist", &wait_ns, 0 },
  { CL_TYP_ON,      "min-perform 2-level minimization on input file", &minimize, 0 },
  */
  { CL_TYP_GROUP,   "Additional", NULL, 0 },
  { CL_TYP_ON,      "bmc-Force burst-mode verification", &t_is_bm_check, 0 },
  
  { CL_TYP_NAME,    "dlycell-Library name for the delay cell of asynchronous", delay_cell_name, 1022 },
  
  { CL_TYP_LONG,    "ll-Log level, 0: all messages, 7: no messages", &log_level, 0 },
  { CL_TYP_ON,      "cap-Calculate charge/dischage capacitance (see log output)", &t_is_cap_report, 0 },
  CL_ENTRY_LAST
};


int do_all(gnc nc)
{
  int cell_ref;
  int synth_option = 0;
  int hl_option = 0;
  
  b_time_start();
  
  nc->log_level = log_level;
  nc->syparam.delay_safety = 1.0 + ((double)delay_safety)/100.0;
  gnc_Log(nc, 4, "Using delay safety factor %.2lf", nc->syparam.delay_safety);

  gnc_DisableStrListBBB(nc, disable_cells);
  
  gnc_Log(nc, 6, "Reading library '%s'.", import_lib_name);
  if ( gnc_ReadLibrary(nc, import_lib_name, cell_library_name, is_synopsis_msg) == NULL )
    return 0;
    
  gnc_Log(nc, 6, "Gate identification.");
  if ( gnc_ApplyBBBs(nc, 0) == 0 )
    return 0; /* there might be missing gates */

  if ( delay_cell_name[0] != '\0' )
  {
    nc->async_delay_cell_ref = gnc_FindCell(nc, delay_cell_name, cell_library_name);
    if ( nc->async_delay_cell_ref < 0 )
    {
      gnc_Error(nc, "Delay cell '%s' not found.", delay_cell_name);
      return 0;
    }
    else
    {
      if ( gnc_GetCellPortCnt(nc, nc->async_delay_cell_ref) != 2 )
      {
        gnc_Error(nc, "Delay cell '%s' not valid.", delay_cell_name);
        nc->async_delay_cell_ref = -1;
        return 0;
      }
      else
      {
        gnc_Log(nc, 5, "Cell '%s' used as delay cell.", delay_cell_name);
      }
    }
  }


  if ( out_vhdl_name[0] != '\0' || out_xnf_name[0] != '\0' )
  {
    gnc_Log(nc, 6, "VHDL or XNF output requires '-" DGD_HL_FLATTEN "'. Flatten enabled.");
    t_is_flatten = 1;
  }
  
  if ( t_is_fbo != 0 && t_is_rso != 0 )
  {
    gnc_Log(nc, 6,  "'-" DGD_HL_FBO "' includes '-" DGD_HL_RSO "'. '-" DGD_HL_RSO "' not required.");
    t_is_rso = 0;
  }
  
  if ( t_is_rso != 0 && reset_type == GNC_HL_OPT_NO_RESET )
  {
    gnc_Log(nc, 6,  "'-" DGD_HL_RSO "' requires a reset signal. '-" DGD_HL_RSO "' disabled.");
    t_is_rso = 0;
  }
  
  if ( t_is_neca )
    synth_option |= GNC_SYNTH_OPT_NECA;
  if ( t_is_generic )
    synth_option |= GNC_SYNTH_OPT_GENERIC;
  if ( t_is_library )
    synth_option |= GNC_SYNTH_OPT_LIBARY;
  if ( t_is_multi_level )
    synth_option |= GNC_SYNTH_OPT_LEVELS;
  if ( t_is_delay_path )
    synth_option |= GNC_SYNTH_OPT_DLYPATH;
  if ( t_is_cross_output )
    synth_option |= GNC_SYNTH_OPT_OUTPUTS;
  

  hl_option |= reset_type;
  hl_option |= state_encoding;
  if ( is_async == 0 )
    hl_option |= GNC_HL_OPT_CLOCK;
  if ( t_is_2l_min )
    hl_option |= GNC_HL_OPT_MINIMIZE;
  if ( t_is_delay == 0 )
    hl_option |= GNC_HL_OPT_NO_DELAY;
  if ( t_is_flatten )
    hl_option |= GNC_HL_OPT_FLATTEN;
  if ( t_is_rso )
    hl_option |= GNC_HL_OPT_RSO;
  if ( t_is_fbo )
    hl_option |= GNC_HL_OPT_FBO;
  if ( t_is_cap_report )
    hl_option |= GNC_HL_OPT_CAP_REPORT;
  if ( t_is_bm_check )
    hl_option |= GNC_HL_OPT_BM_CHECK;
  if ( t_is_min_state )
    hl_option |= GNC_HL_OPT_MIN_STATE;
  if ( t_is_old_min_state )
    hl_option |= GNC_HL_OPT_OLD_MIN_STATE;
    
  if ( t_is_old_delay_method )
    hl_option |= GNC_HL_OPT_USE_OLD_DLY;
  
  gnc_Log(nc, 6, "Synthesis of file '%s'.", cl_file_list[0]);
  
  cell_ref = gnc_SynthByFile(nc, cl_file_list[0], cell_name, target_library_name, hl_option, synth_option);
  if ( cell_ref < 0 )
  {
    gnc_Error(nc, "Synthesis failed.");
    return 0;
  }
  else
  {
    gnc_Log(nc, 6, "Synthesis of file '%s' done, elapsed time %s.", cl_file_list[0], b_time_end());

    if ( out_edif_name[0] != '\0' && cell_ref >= 0 )
    {
      gnc_Log(nc, 6, "Write EDIF file '%s'.", out_edif_name);
      if ( gnc_WriteEdif(nc, out_edif_name, view_name) == 0 )
        return 0;
    }
    if ( out_vhdl_name[0] != '\0' && cell_ref >= 0 )
    {
      gnc_Log(nc, 6, "Write VHDL file '%s'.", out_vhdl_name);
      if ( gnc_WriteVHDL(nc, cell_ref, out_vhdl_name, 
        cell_name, "netlist", GNC_WRITE_VHDL_UPPER_KEY, -1, sim_lib_name) == 0 )
      {
        gnc_Log(nc, 6, "Failed with file '%s'.", out_vhdl_name);
        return 0;
      }
    }
    
    if ( out_xnf_name[0] != '\0' && cell_ref >= 0 )
    {
      gnc_Log(nc, 6, "Write XNF file '%s'.", out_xnf_name);
      if ( gnc_WriteXNF(nc, cell_ref, out_xnf_name) == 0 )
      {
        gnc_Log(nc, 6, "Failed with file '%s'.", out_xnf_name);
        return 0;
      }
    }

    if ( out_bex_name[0] != '\0' && cell_ref >= 0 )
    {
      if ( gnc_GetGCELL(nc, cell_ref)->pi != NULL &&
           gnc_GetGCELL(nc, cell_ref)->cl_on != NULL )
      {
        gnc_Log(nc, 6, "Write BEX file '%s'.", out_bex_name);
        if ( gnc_WriteCellBEX(nc, cell_ref, out_bex_name) == 0 )
        {
          gnc_Log(nc, 6, "Failed with file '%s'.", out_bex_name);
          return 0;
        }
      }
      else
      {
        gnc_Error(nc, "Can not write BEX file (no SOP specification available).");
      }
    }

    if ( out_pla_name[0] != '\0' && cell_ref >= 0 )
    {
      if ( gnc_GetGCELL(nc, cell_ref)->pi != NULL &&
           gnc_GetGCELL(nc, cell_ref)->cl_on != NULL )
      {
        gnc_Log(nc, 6, "Write PLA file '%s'.", out_pla_name);
        if ( gnc_WriteCellPLA(nc, cell_ref, out_pla_name) == 0 )
        {
          gnc_Log(nc, 6, "Failed with file '%s'.", out_pla_name);
          return 0;
        }
      }
      else
      {
        gnc_Error(nc, "Can not write PLA file (no SOP specification available).");
      }
    }

  }
    
 
  return 1;
}

int main(int argc, char **argv)
{
  gnc nc;
  if ( cl_Do(cl_list, argc, argv) == 0 )
  {
    puts("cmdline error");
    exit(1);
  }

  if ( cl_file_cnt < 1 )
  {
    char *s;
    puts("Digital Gate Compiler");
    puts(COPYRIGHT);
    puts(FREESOFT);
    puts(REDIST);
    printf("Usage: %s [options] <input>\n", argv[0]);
    puts("<input> KISS, BMS, PLA, BEX, NEX or DGD file");
    puts("options:");
    cl_OutHelp(cl_list, stdout, " ", 16);
    s = b_get_ff_searchpath();
    if ( s != NULL )
    {
      puts("Searchpath:");
      printf(" %s\n",s);
      free(s);
    }

    puts("Syntax for '-dcl <list>'");
    puts("  list ::= {<cell>}");
    puts("  cell ::= [and|nand|or|nor][<number>]");
    puts("EDIF --> Cadence");
    puts("  -clib: The name of the library that contains the layout view");
    puts("         of the individal gates (should match the correct library).");
    puts("  -lib:  File with the description of the function of each gate.");
    puts("         The name of the gates must match the names in '-clib'.");
    puts("  -view: Cadence celltype (should be 'symbol'?).");
    puts("  -tlib: Toplevel library name for Cadence (user def'ed, but should");
    puts("         be created inside cadence first).");
    exit(1);
  }
  
  if ( strcmp(out_edif_name, cl_file_list[0]) == 0 )
  {
    printf("EDIF file must differ from input file.\n");
    exit(1);
  }
  
  if ( strcmp(out_vhdl_name, cl_file_list[0]) == 0 )
  {
    printf("VHDL file must differ from input file.\n");
    exit(1);
  }

  if ( strcmp(out_xnf_name, cl_file_list[0]) == 0 )
  {
    printf("XNF file must differ from input file.\n");
    exit(1);
  }
  
  nc = gnc_Open();
  if ( nc != NULL )
  {
    if ( do_all(nc) == 0 )
    {
      gnc_Close(nc);
      return 1;
    }
  }
  
  gnc_Close(nc);
  return 0;
} 


