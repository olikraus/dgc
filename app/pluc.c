/*

  pluc.c
  
  f(..., x, ...) = x && f(..., 1, ... ) || !x && f(..., 0, ... )
  
*/

#include <stdio.h>
#include <stdint.h>
#include "dcube.h"
#include "cmdline.h"

/*==================================================*/
#define PLUC_REGOP_NOT_USED 0
#define PLUC_REGOP_ASSIGN_OR 1
#define PLUC_REGOP_AND 2
#define PLUC_REGOP_NAND 3
#define PLUC_REGOP_OR 4
#define PLUC_REGOP_NOR 5
#define PLUC_REGOP_AND_OR 6
#define PLUC_REGOP_NAND_OR 7

#define PLUC_BASE_PLU 0
#define PLUC_BASE_SWM 1


struct _pluc_regop_struct
{
  int8_t op;			/* 0: not used, 1: assign "or value" */
					/* 2: "&=" "and_value", 3: "&= ~" "and_value", 4: "|=" "or_value", 5: "|= ~" "or_value" */
					/* 6: "&=and_value" "|=or_value", 7: "&=~and_value |=or_value */ 
  int8_t base;			/* not really a base address,but a index for the base address, 0: PLU, 1: SWM  */
					/* 0: 0x40028000 PLU   */
					/* 1: 0x4000c000 SWD  */
  uint32_t idx;			/* index for base */
  uint32_t and_value;	/* and value */
  uint32_t or_value;		/* or value */
  
};

typedef struct _pluc_regop_struct pluc_regop_t;

#define PLUC_WIRE_REGOP_CNT 2
struct _pluc_wire_struct
{
  const char *from;
  const char *to;
  pluc_regop_t regop[PLUC_WIRE_REGOP_CNT];
  int8_t is_used;
};
typedef struct _pluc_wire_struct pluc_wire_t;



/*==================================================*/
/* global variables */
char c_file_name[1024] = "";
int cmdline_listmap = 0;

pinfo pi, pi2;
dclist cl_on, cl_dc, cl2_on, cl2_dc;


/*==================================================*/
/* LPC804 routing information */
/* 
  Routing is based on the signal/wire name 
  LUTx_INPy			y input pin y for lut xx
  LUTx				x lut output
  FFx				x flip flip output, note that LUT0 output is automatically mapped to FF0
  
*/
#define PLUC_WIRE_STRING(x) #x

#define PLUC_WIRE_LUT_OUT(x)  PLUC_WIRE_STRING(LUT ## x)
#define PLUC_WIRE_FF_OUT(x)  PLUC_WIRE_STRING(FF ## x)
#define PLUC_WIRE_LUT_IN(lut, in) PLUC_WIRE_STRING( LUT ## lut ## _INP ## in)
#define PLUC_WIRE_PLU_IN(in) PLUC_WIRE_STRING( PLUINPUT ## in)
#define PLUC_WIRE_PLU_OUT(out) PLUC_WIRE_STRING( PLUOUT ## out)

#define PLUC_CONNECT_PLU_IN_LUT_IN(from, lut, in, idx, v) \
 { PLUC_WIRE_PLU_IN(from), PLUC_WIRE_LUT_IN(lut, in), {{1, 0, idx, 0, v}, {0, 0, 0, 0, 0}}, 0 } ,

#define LPC804_CONNECT_PLU_OUT_GPIO(out, gpio, o) \
 { PLUC_WIRE_PLU_OUT(out), #gpio, {{7, 1, 0x180, 3<<(out*2+12), o<<(out*2+12)}, {0, 0, 0, 0, 0}}, 0 } ,

#define PLUC_CONNECT_LUT_OUT_LUT_IN(from, lut, in, idx, v) \
 { PLUC_WIRE_LUT_OUT(from), PLUC_WIRE_LUT_IN(lut, in), {{1, 0, idx, 0, v}, {0, 0, 0, 0, 0}}, 0 } ,

#define LPC804_CONNECT_LUT_OUT_PLU_OUT(lutout, pluout) \
 { PLUC_WIRE_LUT_OUT(lutout), PLUC_WIRE_PLU_OUT(pluout), {{1, 0, 0xc00+pluout*4, 0, lutout}, {0, 0, 0, 0, 0}}, 0 } ,

#define PLUC_CONNECT_FF_OUT_LUT_IN(from, lut, in, idx, v) \
 { PLUC_WIRE_FF_OUT(from), PLUC_WIRE_LUT_IN(lut, in), {{1, 0, idx, 0, v}, {0, 0, 0, 0, 0}}, 0 } ,

#define LPC804_CONNECT_FF_OUT_PLU_OUT(ffout, pluout) \
 { PLUC_WIRE_FF_OUT(ffout), PLUC_WIRE_PLU_OUT(pluout), {{1, 0, 0xc00+pluout*4, 0, ffout+26}, {0, 0, 0, 0, 0}}, 0 } ,

#define LPC804_CONNECT_GPIO_PLU_IN(gpio, in, o) \
 { #gpio, PLUC_WIRE_PLU_IN(in), {{7, 1, 0x180, 3<<(in*2), o<<(in*2)}, {0, 0, 0, 0, 0}}, 0 } ,

#define LPC804_CONNECT_LUT_OUT_FF_OUT(lut, ff) \
 { PLUC_WIRE_LUT_OUT(lut), PLUC_WIRE_FF_OUT(ff), {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 0 } ,


#define PLUC_CONNECT_NONE() \
 { NULL, NULL, {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 0 }

/* from LUT, dest-LUT, input of dest-LUT, index, mux value */

#define LPC804_CONNECT_LUT_IN(lut, in, idx) \
  PLUC_CONNECT_PLU_IN_LUT_IN(0, lut, in, idx, 0) \
  PLUC_CONNECT_PLU_IN_LUT_IN(1, lut, in, idx, 1) \
  PLUC_CONNECT_PLU_IN_LUT_IN(2, lut, in, idx, 2) \
  PLUC_CONNECT_PLU_IN_LUT_IN(3, lut, in, idx, 3) \
  PLUC_CONNECT_PLU_IN_LUT_IN(4, lut, in, idx, 4) \
  PLUC_CONNECT_PLU_IN_LUT_IN(5, lut, in, idx, 5) \
  PLUC_CONNECT_LUT_OUT_LUT_IN(0, lut, in, idx, 6) \
  PLUC_CONNECT_LUT_OUT_LUT_IN(1, lut, in, idx, 7) \
  PLUC_CONNECT_LUT_OUT_LUT_IN(2, lut, in, idx, 8)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(3, lut, in, idx, 9)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(4, lut, in, idx, 10)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(5, lut, in, idx, 11)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(6, lut, in, idx, 12)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(7, lut, in, idx, 13)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(8, lut, in, idx, 14)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(9, lut, in, idx, 15)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(10, lut, in, idx, 16)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(11, lut, in, idx, 17)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(12, lut, in, idx, 18)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(13, lut, in, idx, 19)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(14, lut, in, idx, 20)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(15, lut, in, idx, 21)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(16, lut, in, idx, 22)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(17, lut, in, idx, 23)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(18, lut, in, idx, 24)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(19, lut, in, idx, 25)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(20, lut, in, idx, 26)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(21, lut, in, idx, 27)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(22, lut, in, idx, 28)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(23, lut, in, idx, 29)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(24, lut, in, idx, 30)  \
  PLUC_CONNECT_LUT_OUT_LUT_IN(25, lut, in, idx, 31) \
  PLUC_CONNECT_FF_OUT_LUT_IN(0, lut, in, idx, 32) \
  PLUC_CONNECT_FF_OUT_LUT_IN(1, lut, in, idx, 33) \
  PLUC_CONNECT_FF_OUT_LUT_IN(2, lut, in, idx, 34) \
  PLUC_CONNECT_FF_OUT_LUT_IN(3, lut, in, idx, 35)

/* dest-LUT, input of dest-LUT, index input mux */
#define LPC804_CONNECT_LUT(lut) \
  LPC804_CONNECT_LUT_IN(lut, 0, lut*0x020+0x000) \
  LPC804_CONNECT_LUT_IN(lut, 1, lut*0x020+0x004) \
  LPC804_CONNECT_LUT_IN(lut, 2, lut*0x020+0x008) \
  LPC804_CONNECT_LUT_IN(lut, 3, lut*0x020+0x00c) \
  LPC804_CONNECT_LUT_IN(lut, 4, lut*0x020+0x010) \
  LPC804_CONNECT_LUT_OUT_PLU_OUT(lut, 0) \
  LPC804_CONNECT_LUT_OUT_PLU_OUT(lut, 1) \
  LPC804_CONNECT_LUT_OUT_PLU_OUT(lut, 2) \
  LPC804_CONNECT_LUT_OUT_PLU_OUT(lut, 3) \
  LPC804_CONNECT_LUT_OUT_PLU_OUT(lut, 4) \
  LPC804_CONNECT_LUT_OUT_PLU_OUT(lut, 5) \
  LPC804_CONNECT_LUT_OUT_PLU_OUT(lut, 6) \
  LPC804_CONNECT_LUT_OUT_PLU_OUT(lut, 7)


#define LPC804_CONNECT_FF(ff) \
  LPC804_CONNECT_FF_OUT_PLU_OUT(ff, 0) \
  LPC804_CONNECT_FF_OUT_PLU_OUT(ff, 1) \
  LPC804_CONNECT_FF_OUT_PLU_OUT(ff, 2) \
  LPC804_CONNECT_FF_OUT_PLU_OUT(ff, 3) \
  LPC804_CONNECT_FF_OUT_PLU_OUT(ff, 4) \
  LPC804_CONNECT_FF_OUT_PLU_OUT(ff, 5) \
  LPC804_CONNECT_FF_OUT_PLU_OUT(ff, 6) \
  LPC804_CONNECT_FF_OUT_PLU_OUT(ff, 7) \
  LPC804_CONNECT_LUT_OUT_FF_OUT(ff, ff)

pluc_wire_t lpc804_wire_table[] = {
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_00, 0, 0)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_08, 0, 1)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_17, 0, 2)
  
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_01, 1, 0)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_09, 1, 1)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_18, 1, 2)
  
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_02, 2, 0)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_10, 2, 1)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_19, 2, 2)
  
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_03, 3, 0)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_11, 3, 1)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_20, 3, 2)
  
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_04, 4, 0)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_12, 4, 1)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_21, 4, 2)
  
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_05, 5, 0)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_13, 5, 1)
  LPC804_CONNECT_GPIO_PLU_IN(PIO0_22, 5, 2)

  LPC804_CONNECT_PLU_OUT_GPIO(0, PIO0_07, 0)
  LPC804_CONNECT_PLU_OUT_GPIO(0, PIO0_14, 1)
  LPC804_CONNECT_PLU_OUT_GPIO(0, PIO0_23, 2)

  LPC804_CONNECT_PLU_OUT_GPIO(1, PIO0_08, 0)
  LPC804_CONNECT_PLU_OUT_GPIO(1, PIO0_15, 1)
  LPC804_CONNECT_PLU_OUT_GPIO(1, PIO0_24, 2)

  LPC804_CONNECT_PLU_OUT_GPIO(2, PIO0_09, 0)
  LPC804_CONNECT_PLU_OUT_GPIO(2, PIO0_16, 1)
  LPC804_CONNECT_PLU_OUT_GPIO(2, PIO0_25, 2)

  LPC804_CONNECT_PLU_OUT_GPIO(3, PIO0_10, 0)
  LPC804_CONNECT_PLU_OUT_GPIO(3, PIO0_17, 1)
  LPC804_CONNECT_PLU_OUT_GPIO(3, PIO0_26, 2)

  LPC804_CONNECT_PLU_OUT_GPIO(4, PIO0_11, 0)
  LPC804_CONNECT_PLU_OUT_GPIO(4, PIO0_18, 1)
  LPC804_CONNECT_PLU_OUT_GPIO(4, PIO0_27, 2)

  LPC804_CONNECT_PLU_OUT_GPIO(5, PIO0_12, 0)
  LPC804_CONNECT_PLU_OUT_GPIO(5, PIO0_19, 1)
  LPC804_CONNECT_PLU_OUT_GPIO(5, PIO0_28, 2)

  LPC804_CONNECT_PLU_OUT_GPIO(6, PIO0_13, 0)
  LPC804_CONNECT_PLU_OUT_GPIO(6, PIO0_20, 1)
  LPC804_CONNECT_PLU_OUT_GPIO(6, PIO0_29, 2)

  LPC804_CONNECT_PLU_OUT_GPIO(7, PIO0_14, 0)
  LPC804_CONNECT_PLU_OUT_GPIO(7, PIO0_21, 1)
  LPC804_CONNECT_PLU_OUT_GPIO(7, PIO0_30, 2)

  LPC804_CONNECT_LUT_OUT_PLU_OUT(0, 0)
  LPC804_CONNECT_LUT_OUT_PLU_OUT(1, 0)
  LPC804_CONNECT_LUT_OUT_PLU_OUT(2, 0)
  LPC804_CONNECT_LUT_OUT_PLU_OUT(3, 0)
  LPC804_CONNECT_LUT_OUT_PLU_OUT(4, 0)
  LPC804_CONNECT_LUT_OUT_PLU_OUT(5, 0)
  LPC804_CONNECT_LUT_OUT_PLU_OUT(6, 0)
  LPC804_CONNECT_LUT_OUT_PLU_OUT(7, 0)
  LPC804_CONNECT_LUT_OUT_PLU_OUT(8, 0)
  LPC804_CONNECT_LUT_OUT_PLU_OUT(10, 0)

  LPC804_CONNECT_LUT(0)
  LPC804_CONNECT_LUT(1)
  LPC804_CONNECT_LUT(2)
  LPC804_CONNECT_LUT(3)
  LPC804_CONNECT_LUT(4)
  LPC804_CONNECT_LUT(5)
  LPC804_CONNECT_LUT(6)
  LPC804_CONNECT_LUT(7)
  LPC804_CONNECT_LUT(8)
  LPC804_CONNECT_LUT(9)
  LPC804_CONNECT_LUT(10)
  LPC804_CONNECT_LUT(11)
  LPC804_CONNECT_LUT(12)
  LPC804_CONNECT_LUT(13)
  LPC804_CONNECT_LUT(14)
  LPC804_CONNECT_LUT(15)
  LPC804_CONNECT_LUT(16)
  LPC804_CONNECT_LUT(17)
  LPC804_CONNECT_LUT(18)
  LPC804_CONNECT_LUT(19)
  LPC804_CONNECT_LUT(20)
  LPC804_CONNECT_LUT(21)
  LPC804_CONNECT_LUT(22)
  LPC804_CONNECT_LUT(23)
  LPC804_CONNECT_LUT(24)
  LPC804_CONNECT_LUT(25)
  
  LPC804_CONNECT_FF(0)
  LPC804_CONNECT_FF(1)
  LPC804_CONNECT_FF(2)
  LPC804_CONNECT_FF(3)
  
  PLUC_CONNECT_NONE()
};


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
    
    /* combine both functions, consider the case where the output of cl2 is input of cl */
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
  { CL_TYP_ON,      "listmap-list wire mapping", &cmdline_listmap,  0 },
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
  
  if ( cmdline_listmap != 0 )
  {
    int i = 0;
    while( lpc804_wire_table[i].from != NULL )
    {
      printf("%05d: %-10s -> %-10s\n", i, lpc804_wire_table[i].from, lpc804_wire_table[i].to );
      i++;
    }
    exit(0);
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

