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
void pluc_log(const char *format, ...)
{
  va_list va;
  va_start(va, format);
  vprintf(format, va);
  puts("");
  va_end(va);
}

void pluc_err(const char *format, ...)
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
  pluc_log("Init");
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
  pluc_log("Read (files: %d)", cl_file_cnt);
  for( i = 0; i < cl_file_cnt; i++ )
  {
    pinfoDestroy(&pi2);
    if ( pinfoInit(&pi2) == 0 )
      return 0;
    if ( dclImport(&pi2, cl2_on, cl2_dc, cl_file_list[i]) == 0 )
    {
      pluc_err("Import pluc_error with file %s", cl_file_list[i]);
      return 0;
    }
    pinfoMerge(&pi, cl_on, &pi2, cl2_on);
    
  }
  return 1;
}

/*==================================================*/

int pluc_current_lut_number = 0;

const char *pluc_get_lut_output_name(int pos)
{
  static char s[32];
  sprintf(s, "LUT%02d", pos);
  return s;
}

int pluc_map_cof(pinfo *pi, dclist cl, dcube *cof, int depth)
{
  int i;
  int none_dc_cnt;
  
  dclist cl_left, cl_right;
  int new_left_lut;
  int new_right_lut;
  
  /* get two cubes for the later split (if required) */
  dcube *cofactor_left = &(pi->stack1[depth]);
  dcube *cofactor_right = &(pi->stack2[depth]);
  
  /* calculate the number of variabels which are not fully DC */
  /* Variables which are DC in all cubes must be ignored (infact they could be deleted) */
  none_dc_cnt = 0;
  for( i = 0; i < pi->in_cnt; i++ )
  {
    if ( dclIsDCInVar(pi, cl, i) == 0 )
      none_dc_cnt++;
  }
  
  /* if the number of variables (which are not DC) is lower than 6, then we are done */
  if ( none_dc_cnt <= 5 )
  {
    printf("leaf (depth=%d)\n", depth);
    
    dclShow(pi, cl);
    return 1;
  }
  
  /* find a suitable variable for splitting */
  for( i = 0; i < pi->in_cnt; i++ )
  {
    if ( dclIsDCInVar(pi, cl, i) == 0 )
    {
      break;
    }
  }

  if ( i >= pi->in_cnt )
    return 0;
  
  
  /* for a full split we need two more luts: get the number for these luts */
  new_left_lut = pluc_current_lut_number+0;
  new_right_lut = pluc_current_lut_number+1;
  
  /* occupy the lut by advancing the current lut number */
  pluc_current_lut_number += 2;
  
  
  /* create a new boolean function which connects the later functions */
  {
    pinfo *pi_connect = pinfoOpen();
    dclist cl_connect;
      
    

    if ( pinfoAddOutLabel(pi_connect, pinfoGetOutLabel(pi, 0)) < 0 )
      return pinfoClose(pi_connect), 0;
    
    if ( pinfoAddInLabel(pi_connect, pinfoGetInLabel(pi, i)) < 0 )
      return pinfoClose(pi_connect), 0;
    
    if ( pinfoAddInLabel(pi_connect, pluc_get_lut_output_name(new_left_lut)) < 0 )
      return pinfoClose(pi_connect), 0;

    if ( pinfoAddInLabel(pi_connect, pluc_get_lut_output_name(new_right_lut)) < 0 )
      return pinfoClose(pi_connect), 0;

    if ( dclInit(&cl_connect) == 0 )
      return pinfoClose(pi_connect), 0;
      
    
    dcSetTautology(pi, pi_connect->tmp+17);
    dcSetIn(pi_connect->tmp+17, 0, 2);
    dcSetIn(pi_connect->tmp+17, 1, 2);
    if ( dclAdd(pi_connect, cl_connect, pi_connect->tmp+17) < 0 )
      return pinfoClose(pi_connect), dclDestroy(cl_connect), 0;
    
    dcSetTautology(pi, pi_connect->tmp+17);
    dcSetIn(pi_connect->tmp+17, 0, 1);
    dcSetIn(pi_connect->tmp+17, 2, 1);
    if ( dclAdd(pi_connect, cl_connect, pi_connect->tmp+17) < 0 )
      return pinfoClose(pi_connect), dclDestroy(cl_connect), 0;

    printf("connector (depth=%d)\n", depth);
    dclShow(pi_connect, cl_connect);
    pinfoClose(pi_connect);
    dclDestroy(cl_connect);
  }
  
  /* construct the cofactor cubes: split happens against variable i (as derived above) */
  dcCopy(pi, cofactor_left, cof);
  dcCopy(pi, cofactor_right, cof);
  dcInSetAll(pi, cofactor_left, CUBE_IN_MASK_DC);
  dcInSetAll(pi, cofactor_right, CUBE_IN_MASK_DC);
  dcSetIn(cofactor_left, i, 2);
  dcSetIn(cofactor_right, i, 1);
  
  
  //int dcGetBinateInVarCofactor(pinfo *pi, dcube *r, dcube *rinv, dclist cl, dcube *cof)
  //if ( dcGetNoneDCInVarCofactor(pi, cofactor_left, cofactor_right, cl, cof) == 0 )
  //  return 0;

  if ( dclInitVA(2, &cl_left, &cl_right) == 0 )
    return 0;
  
  if ( dclSCCCofactor(pi, cl_left, cl, cofactor_left) == 0 )
    return dclDestroyVA(2, cl_left, cl_right), 0;
    
  if ( dclSCCCofactor(pi, cl_right, cl, cofactor_right) == 0 )
    return dclDestroyVA(2, cl_left, cl_right), 0;

  pinfoSetOutLabel(pi, 0, pluc_get_lut_output_name(new_left_lut));
  if ( pluc_map_cof(pi, cl_left, cofactor_left, depth+1) == 0 )
    return dclDestroyVA(2, cl_left, cl_right), 0;
  
  pinfoSetOutLabel(pi, 0, pluc_get_lut_output_name(new_right_lut));
  if ( pluc_map_cof(pi, cl_right, cofactor_right, depth+1) == 0 )
    return dclDestroyVA(2, cl_left, cl_right), 0;
  
  return dclDestroyVA(2, cl_left, cl_right), 1;
  
}


int pluc_map(void)
{
  int result;
  dcube *cof = pi.tmp+2;
  dcSetTautology(&pi, cof);
  result = pluc_map_cof(&pi, cl_on, cof, 0);
  return result;
}

/*==================================================*/
int pluc(void)
{
  if ( pluc_init() == 0 )
    return 0;
  if ( pluc_read() == 0 )
    return 0;
  if ( pluc_map() == 0 )
    return 0;
  //dclShow(&pi, cl_on);
  //dclShow(&pi2, cl2_on);
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
    puts("cmdline pluc_error");
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

