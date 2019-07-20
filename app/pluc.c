/*

  pluc.c
  
  f(..., x, ...) = x && f(..., 1, ... ) || !x && f(..., 0, ... )
  
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

int pluc_current_lut_number = 0;

int pluc_get_lut_output_name(int pos)
{
  const char s[32];
  sprintf("LUT%02", pos);
  return s;
}

int pluc_map_cof(pinfo *pi, dclist cl, dcube *cof, int depth)
{
  int i;
  
  dclist cl_left, cl_right;
  dcube *cofactor_left = &(pi->stack1[depth]);
  dcube *cofactor_right = &(pi->stack2[depth]);
  
  
  if ( pi->in_cnt <= 5 )
  {
    puts("leaf:");
    
    dclShow(pi, cl);
    return 1;
  }
  
  for( i = 0; i < pi->in_cnt; i++ )
  {
    if ( dclIsDCInVar(pi, cl, i) == 0 )
    {
      break;
    }
  }

  if ( i >= pi->in_cnt )
    return 0;
  
  {
    pinfo *pi2 = pinfoOpen();
    dclist cl2;
      

    if ( pinfoAddOutLabel(pi2, pinfoGetOutLabel(pi, 0)) == 0 )
      return pinfoClose(pi2), 0;
    
    if ( pinfoAddInLabel(pi2, pinfoGetInLabel(pi, i)) == 0 )
      return pinfoClose(pi2), 0;
    
    if ( pinfoAddInLabel(pi2, pluc_get_lut_output_name(pluc_current_lut_number)) == 0 )
      return pinfoClose(pi2), 0;

    if ( pinfoAddInLabel(pi2, pluc_get_lut_output_name(pluc_current_lut_number+1)) == 0 )
      return pinfoClose(pi2), 0;

    if ( dclInit(&cl2) == 0 )
      return pinfoClose(pi2), 0;
      
    
    dcSetTautology(pi, pi2->tmp+17);
    dcSetIn(pi2->tmp+17, 0, 2);
    dcSetIn(pi2->tmp+17, 1, 2);
    if ( dclAdd(pi2, cl2, pi2->tmp+17) == 0 )
      return pinfoClose(pi2), dclDestroy(cl2), 0;
    
    dcSetTautology(pi, pi2->tmp+17);
    dcSetIn(pi2->tmp+17, 0, 1);
    dcSetIn(pi2->tmp+17, 2, 1);
    if ( dclAdd(pi2, cl2, pi2->tmp+17) == 0 )
      return pinfoClose(pi2), dclDestroy(cl2), 0;

    puts("connector:");
    dclShow(pi2, cl2);
    pinfoClose(pi2);
    dclDestroy(cl2);
  }
  
  dcCopy(pi, cofactor_left, cof);
  dcCopy(pi, cofactor_right, cof);
  dcInSetAll(pi, cofactor_left, CUBE_IN_MASK_DC);
  dcInSetAll(pi, cofactor_right, CUBE_IN_MASK_DC);
  dcSetIn(cofactor_left, i, 2);
  dcSetIn(cofactor_right, i, 1);
  
  
  //int dcGetBinateInVarCofactor(pinfo *pi, dcube *r, dcube *rinv, dclist cl, dcube *cof)
  if ( dcGetNoneDCInVarCofactor(pi, cofactor_left, cofactor_right, cl, cof) == 0 )
    return 0;

  if ( dclInitVA(2, &cl_left, &cl_right) == 0 )
    return 0;

  if ( dclSCCCofactor(pi, cl_left, cl, cofactor_left) == 0 )
    return dclDestroyVA(2, cl_left, cl_right), 0;
    
  if ( dclSCCCofactor(pi, cl_right, cl, cofactor_right) == 0 )
    return dclDestroyVA(2, cl_left, cl_right), 0;

  if ( pluc_map_cof(pi, cl_left, cofactor_left, depth+1) == 0 )
    return dclDestroyVA(2, cl_left, cl_right), 0;
  
  if ( pluc_map_cof(pi, cl_right, cofactor_right, depth+1) == 0 )
    return dclDestroyVA(2, cl_left, cl_right), 0;
  
  return dclDestroyVA(2, cl_left, cl_right), 1;
  
}


int pluc_map(void)
{
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

