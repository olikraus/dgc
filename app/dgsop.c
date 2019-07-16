/*

  Digital Gate: Sum of Product
  
  implements an espresso clone

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
#include "dcube.h"
#include "cmdline.h"
#include "b_ff.h"
#include "config.h"

char pla_file_name[1024] = "";
char bex_file_name[1024] = "";
char fn_cube_str[1024] = "";
long log_level = 4;
long command = 0;
int is_quiet = 0, greedy = 0;
int is_bcp = 0;
int is_pos = 0;
int is_literal = 0;

cl_entry_struct cl_list[] =
{
  { CL_TYP_GROUP,   "Output", NULL, 0 },
  { CL_TYP_STRING,  "op-write PLA file", pla_file_name, 1024 },
  { CL_TYP_STRING,  "ob-write BEX file", bex_file_name, 1024 },
/*  { CL_TYP_LONG,    "ll-log level, 1: all messages, 7: no messages", &log_level, 0 }, */
  { CL_TYP_GROUP,   "Commands", NULL, 0 },
  { CL_TYP_SET,     "nop-Do nothing", &command, -1 },
  { CL_TYP_SET,     "min-Minimize", &command, 0 },
  { CL_TYP_SET,     "primes-Generate all prime implicants", &command, 1 },
  { CL_TYP_SET,     "eq-Compare 1st and 2nd file", &command, 2 },
  { CL_TYP_SET,     "sub-Subtract 2nd from 1st file", &command, 3 },
  { CL_TYP_STRING,  "fn-Calculate output values of the input cube <str>", fn_cube_str, 1024 },
  { CL_TYP_SET,     "info-Information about a file", &command, 4 },
  { CL_TYP_SET,     "dcexpand-Expand don't cares", &command, 5 },
  { CL_TYP_SET,     "merge-Merge two files", &command, 6 },
  { CL_TYP_GROUP,   "Additional options", NULL, 0 },
  { CL_TYP_ON,      "greedy-use heuristic cover algorithm", &greedy,  0 },
  { CL_TYP_ON,      "literal-weight function is 'number of literals'", &is_literal,  0 },
  { CL_TYP_ON,      "bcp-use binate cover algorithm for minimize command", &is_bcp,  0 },
  { CL_TYP_ON,      "pos-assume 'product of sums' for the 1st input file", &is_pos, 0 },
  { CL_TYP_ON,      "b-Batch operation, be quiet", &is_quiet, 0 },
  CL_ENTRY_LAST
};


int doall(void)
{
  pinfo pi;
  pinfo pi2;
  dclist cl_on, cl_dc, cl2_on, cl2_dc;
  const char *t;

  if ( pinfoInit(&pi) == 0 )
    return 0;
  if ( dclInitVA(4, &cl_on, &cl_dc, &cl2_on, &cl2_dc) == 0 )
    return 0;
    
  if ( is_quiet == 0 )
  {
    if ( pinfoInitProgress(&pi) == 0 )
      return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
  }
  
  if ( dclImport(&pi, cl_on, cl_dc, cl_file_list[0]) == 0 )
  {
    t = cl_file_list[0];
    /* if ( dclReadPLAStr(&pi, cl_on, cl_dc, &t) == 0 ) */
    {
      printf("import error (%s)\n", cl_file_list[0]);
      return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
    }
  }
  
  if ( cl_file_cnt >= 2 )
  {
    if ( pinfoInit(&pi2) == 0 )
      return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
    if ( dclImport(&pi2, cl2_on, cl2_dc, cl_file_list[1]) == 0 )
    {
      t = cl_file_list[1];
      if ( dclReadPLAStr(&pi2, cl2_on, cl2_dc, &t) == 0 )
      {
        printf("import error (%s)\n", cl_file_list[1]);
        return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), pinfoDestroy(&pi2), 0;
      }
    }
    
    if ( command != 6 )
    {
      if ( pi2.in_cnt != pi.in_cnt || pi2.out_cnt != pi.out_cnt)
      {
	printf("files must have the same numbers of inputs and outputs\n");
	return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), pinfoDestroy(&pi2), 0;
      }
    }
  }
  
  
  if ( is_pos != 0 )
  {
    dclConvertFromPOS(&pi, cl_on);
  }
  
  if ( fn_cube_str[0] != '\0' )
    command = -2;

  switch(command)  
  {
    case -1:
      break;
    case 0:
      if ( is_bcp == 0 )
      {
        if ( dclMinimizeDC(&pi, cl_on, cl_dc, greedy, is_literal) == 0 )
        {
          puts("error: minimize");
          return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
        }
      }
      else
      {
        if ( greedy != 0 )
          puts("warning: -greedy not supported for -bcp");

        if ( is_literal != 0 )
          puts("warning: -literal not supported for -bcp");
        
        if ( dclMinimizeDCWithBCP(&pi, cl_on, cl_dc) == 0 )
        {
          puts("error: minimize");
          return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
        }
      }
      break;
    case 1:
      if ( dclPrimesDC(&pi, cl_on, cl_dc) == 0 )
      {
        puts("error: primes");
        return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
      }
      break;
    case 2:
      if ( dclCnt(cl_dc) != 0 || dclCnt(cl2_dc) != 0 )
      {
        puts("error: can not compare problems with don't cares.");
        return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
      }
      if ( dclIsEquivalent(&pi, cl_on, cl2_on) == 0 )
      {
        dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc);
        pinfoDestroy(&pi);
        if ( is_quiet == 0 )
          puts("not equal");
        exit(2);
      }
      else
      {
        dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc);
        pinfoDestroy(&pi);
        if ( is_quiet == 0 )
          puts("equal");
        exit(1);
      }
      break;
    case 3:
      if ( dclCnt(cl_dc) != 0 || dclCnt(cl2_dc) != 0 )
      {
        puts("error: can not subtract problems with don't cares.");
        return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
      }
      if ( dclSubtract(&pi, cl_on, cl2_on) == 0 )
      {
        dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc);
        return 0;
      }
      break;
    case 4:
      printf("SOP format:\n");
      printf("  Cubes:    %d\n", dclCnt(cl_on));
      printf("  Literals: %d\n", dclGetLiteralCnt(&pi, cl_on));
      break;
    case 5:
      if ( dclCnt(cl_dc) != 0  )
      {
        puts("error: don't care set not supported.");
        return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
      }
      dclDontCareExpand(&pi, cl_on);
      break;
    case 6:
      if ( pinfoMerge(&pi, cl_on, &pi2, cl2_on) == 0 )
      {
	puts("Merge failed.");
        dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc);
        return 0;
      }
      
      pinfoCntDCList(&pi, cl_on, &(pi.tmp[0]));
      printf("none DC in var count: %d\n", pinfoGetNoneDCInVarCnt(&pi));
      
      break;
    default:
      if ( fn_cube_str[0] != '\0' )
      {
        dcube *c = &(pi.tmp[10]);
        if ( dcSetInByStr(&pi, c, fn_cube_str) == 0 )
        {
          printf("error: '%s' is an illegal cube\n", fn_cube_str);
          return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
        }
        dclClear(cl2_on);
        dclClear(cl2_dc);
        if ( dclAdd(&pi, cl2_on, c) < 0 )
          return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
        dclDontCareExpand(&pi, cl2_on);
        {
          int i;
          for( i = 0; i < dclCnt(cl2_on); i++ )
            dclGetOutput(&pi, cl_on, dclGet(cl2_on, i));
        }
        dclClear(cl_on);
        dclClear(cl_dc);
        if ( dclCopy(&pi, cl_on, cl2_on) == 0 )
          return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
        /*
        if ( dclAdd(&pi, cl_on, c) < 0 )
          return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
        */
      }
      
      break;
  }
  
  if ( pla_file_name[0] != '\0' )
  {
    if ( dclWritePLA(&pi, cl_on, pla_file_name) == 0 )
    {
      puts("export error");
      return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
    }
  }

  if ( bex_file_name[0] != '\0' )
  {
    if ( dclWriteBEX(&pi, cl_on, bex_file_name) == 0 )
    {
      puts("export error");
      return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 0;
    }
  }
  
  if ( pla_file_name[0] == '\0' && bex_file_name[0] == '\0' )
  {
    printf(".i %d\n", pi.in_cnt);
    printf(".o %d\n", pi.out_cnt);
    printf(".p %d\n", dclCnt(cl_on));
    dclShow(&pi, cl_on);
  }

  return dclDestroyVA(4, cl_on, cl_dc, cl2_on, cl2_dc), pinfoDestroy(&pi), 1;
}


int main(int argc, char **argv)
{
  if ( cl_Do(cl_list, argc, argv) == 0 )
  {
    puts("cmdline error");
    exit(3);
  }

  if ( cl_file_cnt < 1 )
  {
    char *s;
    puts("Digital Gate: Sum of Product Minimization");
    /*
    puts(COPYRIGHT);
    puts(FREESOFT);
    puts(REDIST);
    */
    printf("Usage: %s [options] <1st input file> <2nd input file>\n", argv[0]);
    puts("options:");
    cl_OutHelp(cl_list, stdout, " ", 11);
    s = b_get_ff_searchpath();
    if ( s != NULL )
    {
      puts("Searchpath:");
      puts(s);
      free(s);
    }
    puts("Return value:");
    puts(" 4: Help.");
    puts(" 3: Error occured.");
    puts(" 2: -eq operation is false (not equal).");
    puts(" 1: -eq operation is true (equal).");
    puts(" 0: Operation is successful.");
    exit(4);
  }  
  
  if ( doall() == 0 )
  {
    exit(3);
  }
    
  return 0;
}

