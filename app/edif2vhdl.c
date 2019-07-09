/*

  edif2vhdl.c
  
  experimental edif converter

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
#include <string.h>
#include "gnet.h"
#include "edif.h"
#include "cmdline.h"
#include "b_ff.h"
#include "config.h"

char vhdl_file_name[1024] = "";
char vhdl_file_name_x[1024+10] = "";
char edif_file_name[1024] = "";
char edif_view_name[1024] = "symbol";
char vhdl_entity_name[1024] = "myentity";
char vhdl_arch_name[1024] = "netlist";
char vhdl_lib_name[1024] = "xyzlib";
long log_level = 4;
int is_edif_external = 1;   /* not used */
int is_edif_lib_cells = 1;  /* not used */
int is_edif_lib_nets = 1;   /* not used */
int is_edif_assign_name = 0;

cl_entry_struct cl_list[] =
{
  { CL_TYP_PATH,  "ov-Name of VHDL outputfile", vhdl_file_name, 1022 },
  { CL_TYP_NAME,    "entity-VHDL entity name", vhdl_entity_name, 1022 },
  { CL_TYP_NAME,    "arch-VHDL architecture name", vhdl_arch_name, 1022 },
  { CL_TYP_NAME,    "vlib-VHDL library (use ...)", vhdl_lib_name, 1022 },
  { CL_TYP_PATH,  "oe-Name of EDIF outputfile", edif_file_name, 1022 },
  { CL_TYP_NAME,    "view-EDIF view name", edif_view_name, 1022 },
  { CL_TYP_LONG,    "ll-log level, 1: all messages, 7: no messages", &log_level, 0 },
  CL_ENTRY_LAST
};

int main(int argc, char **argv)
{
  int i;
  gnc nc;
  int edif_option;
  int cell_ref;
  int cnt;

  if ( cl_Do(cl_list, argc, argv) == 0 )
  {
    puts("cmdline error");
    exit(1);
  }

  if ( cl_file_cnt < 1 )
  {
    char *s;
    puts("EDIF - VHDL (experimental) conversion utility");
    puts(COPYRIGHT);
    puts(FREESOFT);
    puts(REDIST);
    printf("Usage: %s [options] <edif files>\n", argv[0]);
    puts("options:");
    cl_OutHelp(cl_list, stdout, " ", 15);
    s = b_get_ff_searchpath();
    if ( s != NULL )
    {
      puts("Searchpath:");
      puts(s);
      free(s);
    }
    exit(1);
  }  

  edif_option = 0;
  if ( is_edif_external != 0 )    edif_option |= EDIF_OPT_READ_EXTERNAL;
  if ( is_edif_lib_cells != 0 )   edif_option |= EDIF_OPT_READ_LIBRARY_CELLS;
  if ( is_edif_lib_nets != 0 )    edif_option |= EDIF_OPT_READ_LIBRARY_NETS;
  if ( is_edif_assign_name != 0 ) edif_option |= EDIF_OPT_ASSIGN_NAME;
  
  nc = gnc_Open();
  if ( nc != NULL )
  {
    nc->log_level = log_level;
    
    /* read EDIF */
  
    for( i = 0; i < cl_file_cnt; i++ )
    {
      gnc_Log(nc, 6, "Reading EDIF file '%s'.", cl_file_list[i]);
      gnc_ReadEdif(nc, cl_file_list[i], edif_option);
      gnc_Log(nc, 6, "Done reading '%s'.", cl_file_list[i]);
      
    }

    /* do some output */

    if ( edif_file_name[0] != '\0' )
    {
      gnc_Log(nc, 6, "Writing EDIF-file'%s'.", edif_file_name);
      if ( gnc_WriteEdif(nc, edif_file_name, edif_view_name) == 0 )
      {
        gnc_Error(nc, "Can not write EDIF-file '%s'.", edif_file_name);
      }
    }

    if ( vhdl_file_name[0] != '\0' )
    {

      cell_ref = -1;
      cnt = 0;
      while( gnc_LoopCell(nc, &cell_ref) )
      {
        if ( gnc_IsTopLevelCell(nc, cell_ref) != 0 )
        {
          strcpy(vhdl_file_name_x, vhdl_file_name);
          if ( cnt > 0 )
            sprintf(vhdl_file_name_x+strlen(vhdl_file_name_x), ".%d", cnt);
          gnc_Log(nc, 6, "Writing VHDL-file '%s'.", vhdl_file_name);
          if ( gnc_WriteVHDL(nc, cell_ref, vhdl_file_name, 
            vhdl_entity_name, 
            vhdl_arch_name, 
            0, 0, 
            vhdl_lib_name) == 0 )
          {
            gnc_Error(nc, "Can not write VHDL-file '%s'.", edif_file_name);
          }
          cnt++;
        }
      }
    }

    gnc_Close(nc);
  }
  return 1;
}
