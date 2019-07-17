/*

  pluc.c
  
*/

#include <stdio.h>
#include "dcube.h"
#include "cmdline.h"
/*==================================================*/
/* global variables */
char c_file_name[1024] = "";

pinfo pi, pi2;
dclist cl_on, cl_dc, cl2_on, cl2_dc;

/*==================================================*/
void log(const char *format, ...)
{
  va_list va;
  va_start(va, format);
  vprintf(format, va);
  puts("");
  va_end(va);
}

void err(const char *format, ...)
{
  va_list va;
  va_start(va, format);
  vprintf(format, va);
  puts("");
  va_end(va);
}


/*==================================================*/
int pluc_init(void)
{
  log("Init");
  if ( pinfoInit(&pi) == 0 )
    return 0;
  if ( pinfoInit(&pi2) == 0 )
    return 0;
  if ( dclInitVA(4, &cl_on, &cl_dc, &cl2_on, &cl2_dc) == 0 )
    return 0;
  return 1;
}

int pluc_read(void)
{
  int i;
  log("Read (files: %d)", cl_file_cnt);
  for( i = 0; i < cl_file_cnt; i++ )
  {
    pinfoDestroy(&pi2);
    if ( pinfoInit(&pi2) == 0 )
      return 0;
    if ( dclImport(&pi2, cl2_on, cl2_dc, cl_file_list[i]) == 0 )
    {
      err("Import error with file %s", cl_file_list[i]);
      return 0;
    }
    pinfoMerge(&pi, cl_on, &pi2, cl2_on);
    
  }
  return 1;
}

/*==================================================*/
int pluc(void)
{
  if ( pluc_init() == 0 )
    return 0;
  if ( pluc_read() == 0 )
    return 0;
  dclShow(&pi, cl_on);
  dclShow(&pi2, cl2_on);
  return 1;
}




/*==================================================*/
/* commandline handling & help */

cl_entry_struct cl_list[] =
{
  { CL_TYP_STRING,  "oc-write C code", c_file_name, 1024 },
  //{ CL_TYP_ON,      "greedy-use heuristic cover algorithm", &greedy,  0 },
  CL_ENTRY_LAST
};

void help(const char *pn)
{
    printf("Usage: %s [options] <input files> \n", pn);
    puts("options:");
    cl_OutHelp(cl_list, stdout, " ", 11);
}


/*==================================================*/
/* main procedure */

int main(int argc, char **argv)
{
  if ( cl_Do(cl_list, argc, argv) == 0 )
  {
    puts("cmdline error");
    exit(1);
  }

  if ( cl_file_cnt < 1 )
  {
    help(argv[0]);
  }
  else
  {
    pluc();
  }
}

