/*

  dglc.c
  
  digital gate library compiler

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
#include "gnet.h"
#include "cmdline.h"
#include "b_ff.h"
#include "config.h"

char out_file_name[1024] = "default.dgl";
char library_name[256] = "";
long log_level = 4;
int do_id = 0;

cl_entry_struct cl_list[] =
{
  { CL_TYP_STRING,  "o-output filename", out_file_name, 1024 },
  { CL_TYP_STRING,  "lib-rename library", library_name, 256 },
  { CL_TYP_LONG,    "ll-log level, 1: all messages, 7: no messages", &log_level, 0 },
  { CL_TYP_ON,      "id-Identify the logical function of the cells", &do_id, 0 },
  CL_ENTRY_LAST
};


int main(int argc, char **argv)
{
  gnc nc;
  int i;
  char *file_lib_name;
  char *user_lib_name;

  if ( cl_Do(cl_list, argc, argv) == 0 )
  {
    puts("cmdline error");
    exit(1);
  }

  if ( cl_file_cnt < 1 )
  {
    char *s;
    puts("Digital Gate Library Compiler");
    puts(COPYRIGHT);
    puts(FREESOFT);
    puts(REDIST);
    printf("Usage: %s [options] <library files>\n", argv[0]);
    puts("options:");
    cl_OutHelp(cl_list, stdout, " ", 11);
    s = b_get_ff_searchpath();
    if ( s != NULL )
    {
      puts("Searchpath:");
      puts(s);
      free(s);
    }
    exit(1);
  }  
  
  if ( cl_file_cnt > 1 && library_name[0] == '\0' )
  {
    puts("Use -l <libname> to assign a common name for all provided files.");
    puts("Use -o <filename> for the output file.");
    exit(1);
  }
  
  if ( library_name[0] == '\0' )
    user_lib_name = NULL;
  else
    user_lib_name = library_name;
  
  nc = gnc_Open();
  if ( nc != NULL )
  {
    nc->log_level = log_level;
  
    for( i = 0; i < cl_file_cnt; i++ )
    {
      gnc_Log(nc, 6, "Reading library '%s'.", cl_file_list[i]);
      file_lib_name = gnc_ReadLibrary(nc, cl_file_list[i], user_lib_name, 0);
      if( file_lib_name == NULL )
      {
        gnc_Error(nc, "Failed for input file '%s'.", cl_file_list[i]);
        break;
      }
    }
    if ( i >= cl_file_cnt )
    {
      if ( do_id != 0 )
      {
        gnc_Log(nc, 6, "Cell identification.");
        
        /* FIXME: use gnc_ApplyBBBs, but this would apply the empty cell */
        /* and the empty cell generates a read error... */
        if ( gnc_IdentifyAndMarkBBBs(nc, 1) == 0 )
        {
          gnc_Error(nc, "Identify error!");
        }
      }
    
      gnc_Log(nc, 6, "Using library name '%s' for output file '%s'.", file_lib_name, out_file_name);
      if ( gnc_WriteBinaryLibrary(nc, out_file_name, file_lib_name) == 0 )
      {
        gnc_Error(nc, "Error writing file '%s'.", out_file_name);
      }
      gnc_Log(nc, 6, "Done.");
    }
    gnc_Close(nc);
  }
  exit(1);
  return 1;
}
