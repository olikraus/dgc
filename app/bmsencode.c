/*

  DIGITAL GATE COMPILER
  
  bmsencode.c
  
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
#include "cmdline.h"
#include "config.h"
#include "fsm.h"

int is_mis = 0;
int is_min = 0;
int is_fbo = 0;
int is_dcm = 0;
int is_prim = 0;
int is_verbose = 0;

char prefix[1024] = "";

cl_entry_struct cl_list[] =
{
  { CL_TYP_ON,      "min-Minimize encoding", &is_min,  0 },
  { CL_TYP_ON,      "mis-State reduction", &is_mis, 0 },
  { CL_TYP_ON,      "fbo-Use output lines for encoding", &is_fbo,  0 },
  { CL_TYP_STRING,  "pre-Prefix for output lines", prefix, 1024 },
  { CL_TYP_ON,      "primes-Calculate prime conditions", &is_prim,  0 },
  { CL_TYP_ON,      "v-Print log messages", &is_verbose,  0 },
  { CL_TYP_ON,      "dcm-.", &is_dcm,  0 },
  { CL_TYP_ON,      "a-.", &is_dcm,  0 },
  { CL_TYP_ON,      "r-.", &is_dcm,  0 },
  { CL_TYP_ON,      "e-.", &is_dcm,  0 },
  CL_ENTRY_LAST
};

void logfn(void *data, int ll, char *fmt, va_list va)
{
  if ( is_verbose == 0 )
    return;
  vprintf(fmt, va);
  puts("");
}

void generate_output(pinfo *pi, dclist cl)
{
  int i;
  if ( is_dcm != 0 )
  {
    puts("");
    puts("Minimum cover is");
  }
  for( i = 0; i < dclCnt(cl); i++ )
  {
    if ( is_dcm != 0 )
    {
      printf("%s ; ", dcInToStr(pi, dclGet(cl, i), ""));
      printf("%s\n", dcInToStr(pi, dclGet(cl, i), ""));
    }
    else
    {
      printf("%s%s\n", prefix, dcInToStr(pi, dclGet(cl, i), ""));
    }
  }
}

void doall_fsm(const char *s)
{
  fsm_type fsm;
  fsm = fsm_Open();
  if ( fsm == NULL )
  {
    puts("error: can not open fsm");
    return;
  }
  
  fsm_PushLogFn(fsm, logfn, NULL, 4);
  
  /*  
  if ( fsm_Import(fsm, s) == 0 )
  {
    printf("error: can not read '%s'\n", s);
    return;
  }
  */
  
  if ( fsm_ReadBMS(fsm, s) == 0 )
  {
    printf("error: can not read '%s'\n", s);
    return;
  }

  /*  
  fsm_MinimizeStates(fsm, is_fbo, 0);
  exit(0);
  */

  if ( is_prim != 0 )
  {
    puts("'-primes' not valid for state machines.");
    return;
  }
  
  if ( is_mis != 0 )
    fsm_MinimizeStates(fsm, is_fbo, 1);
    
  if ( fsm_BuildPartitionTable(fsm) == 0 )
    return;
  if ( fsm_SeparateStates(fsm) == 0 )
    return;

  if ( is_fbo != 0 )
    if ( fms_UseOutputFeedbackRemovePartitions(fsm) == 0 )
      return;
  
  if ( is_min != 0 )
    if ( fsm_MinimizePartitionTable(fsm) == 0 )
      return;

  generate_output(fsm->pi_async, fsm->cl_async);
  
  fsm_PopLogFn(fsm);
  fsm_Close(fsm);
}

static void bmsencode_do_msg(void *data, char *fmt, va_list va)
{
}

void doall_dcl(const char *s)
{
  pinfo pi;
  dclist cl;

  if ( pinfoInit(&pi) == 0 )
    return;
    
  if ( dclInit(&cl) == 0 )
  {
    dclDestroy(cl), pinfoDestroy(&pi);
    return;
  }
  
  if ( dclImport(&pi, cl, NULL, s) == 0 )
  {
    dclDestroy(cl), pinfoDestroy(&pi);
    return;
  }
  
  if ( is_min != 0 && is_prim != 0 )
  {
    puts("Use '-min' OR '-primes'.");
    return;
  }

  if ( is_min != 0 )
    if ( dclMinimizeUSTT(&pi, cl, bmsencode_do_msg, NULL, "ENC: ", NULL) == 0 )
    {
      dclDestroy(cl), pinfoDestroy(&pi);
      return;
    }
  
  if ( is_prim != 0 )  
  {
    if ( dclPrimesUSTT(&pi, cl) == 0 )
    {
      dclDestroy(cl), pinfoDestroy(&pi);
      return;
    }
  }

  generate_output(&pi, cl);
  
  dclDestroy(cl);
  pinfoDestroy(&pi);
  
}

void doall(const char *s)
{
  if ( IsValidDCLFile(s) != 0 )
    doall_dcl(s);
  else
    doall_fsm(s);
}

int main(int argc, char **argv)
{
  if ( cl_Do(cl_list, argc, argv) == 0 )
  {
    puts("cmdline error");
    exit(1);
  }
  
  if ( is_dcm != 0 )
    is_min = 1;
  
  if ( cl_file_cnt < 1 )
  {
    char *s;
    puts("Encoding and minimization tool for asynchronous state machines");
    puts(COPYRIGHT);
    puts(FREESOFT);
    puts(REDIST);
    printf("Usage: %s [options] <file>\n", argv[0]);
    puts("options:");
    cl_OutHelp(cl_list, stdout, " ", 14);
    return 1;
  }
  
  
  doall(cl_file_list[0]);
  
  return 0;
}
