/*

  pluc.c
  
  f(..., x, ...) = x && f(..., 1, ... ) || !x && f(..., 0, ... )
  
  
  init	
    setup of all the data structures
  read
    parse files and merge everyting into a single huge boolean problem
  map
    cut down the boolean problem and map the problem into the available luts
  
*/

/*

  TODO:
    The LUT can be placed anywere (except for the first four LUTs which could drive the FFs)
    There is probably no need to assign an internal name, LUTs can be allocated directly to the target LUT, only one problem:
    There must be two lists (or at least two counter) to count the FFs and the real LUTs

*/


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "dcube.h"
#include "cmdline.h"

/*==================================================*/
struct _pluc_lut_struct
{
  pinfo pi;			/* the label of the output will be the name of expected signal */
  dclist dcl;
  /* obsolete, the LUT name can always be derived from the position in the list */
  // char *user_out_name;	/* the original output signal name, given by the user */
  int is_placed;			/* whether this LUT is placed */
};
typedef struct _pluc_lut_struct pluc_lut_t;


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
  uint32_t idx;			/* index for base (byte offset)*/
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
  int8_t is_blocked;		/* shared resourced can be blocked, once they are used */
  int8_t is_lut_in;
  int8_t is_lut_out;
  
};
typedef struct _pluc_wire_struct pluc_wire_t;


/*==================================================*/
/* global variables */

char c_file_name[1024] = "";
int cmdline_listmap = 0;
char cmdline_output[1024] = "";
char cmdline_input[1024] = "";

/* 
 read
    parse files and merge everyting into a single huge boolean problem
*/
pinfo pi, pi2;
dclist cl_on, cl_dc, cl2_on, cl2_dc;

/*
  map
    cut down the boolean problem and map the problem into the available luts
*/
#define PLUC_LUT_MAX 128
#define PLUC_FF_MAX 4
pluc_lut_t pluc_lut_list[PLUC_LUT_MAX];
int pluc_lut_cnt = PLUC_FF_MAX;
int pluc_ff_cnt = 0;

int pluc_internal_cnt = 0;


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
 { PLUC_WIRE_PLU_IN(from), PLUC_WIRE_LUT_IN(lut, in), {{1, 0, idx, 0, v}, {0, 0, 0, 0, 0}}, 0, 0, 1, 0 } ,

#define LPC804_CONNECT_PLU_OUT_GPIO(out, gpio, o) \
 { PLUC_WIRE_PLU_OUT(out), #gpio, {{7, 1, 0x180, 3<<(out*2+12), o<<(out*2+12)}, {0, 0, 0, 0, 0}}, 0, 0, 0, 0 } ,

#define PLUC_CONNECT_LUT_OUT_LUT_IN(from, lut, in, idx, v) \
 { PLUC_WIRE_LUT_OUT(from), PLUC_WIRE_LUT_IN(lut, in), {{1, 0, idx, 0, v}, {0, 0, 0, 0, 0}}, 0, 0, 1, 0 } ,

#define LPC804_CONNECT_LUT_OUT_PLU_OUT(lutout, pluout) \
 { PLUC_WIRE_LUT_OUT(lutout), PLUC_WIRE_PLU_OUT(pluout), {{1, 0, 0xc00+pluout*4, 0, lutout}, {0, 0, 0, 0, 0}}, 0, 0, 0, 1 } ,

#define PLUC_CONNECT_FF_OUT_LUT_IN(from, lut, in, idx, v) \
 { PLUC_WIRE_FF_OUT(from), PLUC_WIRE_LUT_IN(lut, in), {{1, 0, idx, 0, v}, {0, 0, 0, 0, 0}}, 0, 0, 1, 0 } ,

#define LPC804_CONNECT_FF_OUT_PLU_OUT(ffout, pluout) \
 { PLUC_WIRE_FF_OUT(ffout), PLUC_WIRE_PLU_OUT(pluout), {{1, 0, 0xc00+pluout*4, 0, ffout+26}, {0, 0, 0, 0, 0}}, 0, 0, 0, 0 } ,

#define LPC804_CONNECT_GPIO_PLU_IN(gpio, in, o) \
 { #gpio, PLUC_WIRE_PLU_IN(in), {{7, 1, 0x180, 3<<(in*2), o<<(in*2)}, {0, 0, 0, 0, 0}}, 0, 0, 0, 0 } ,

#define LPC804_CONNECT_LUT_OUT_FF_OUT(lut, ff) \
 { PLUC_WIRE_LUT_OUT(lut), PLUC_WIRE_FF_OUT(ff), {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 0, 0, 0, 1	 } ,


#define PLUC_CONNECT_NONE() \
 { NULL, NULL, {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}}, 0, 0, 0, 0 }

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
/*=== init ===*/
/*==================================================*/
int pluc_init(void)
{
  int i;
  pluc_log("Init");  
  
  /*  read */
  
  if ( pinfoInit(&pi) == 0 )
    return 0;
  if ( pinfoInit(&pi2) == 0 )
    return 0;
  if ( dclInitVA(4, &cl_on, &cl_dc, &cl2_on, &cl2_dc) == 0 )
    return 0;

  /* map */

  for( i = 0; i < PLUC_LUT_MAX; i++ )
  {
    if ( pinfoInit(&(pluc_lut_list[i].pi) ) == 0 )
      return 0;
    if ( dclInit( &(pluc_lut_list[i].dcl) ) == 0 )
      return 0;
    //pluc_lut_list[i].user_out_name = NULL;
    pluc_lut_list[i].is_placed = 0;
  }
  
  pluc_lut_cnt = PLUC_FF_MAX;		// we could assign 0, if there is no state machine
    
  return 1;
}

/*==================================================*/
/*=== read ===*/
/*==================================================*/
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
/*=== map ===*/
/*==================================================*/

/* return an internal unique name for the LUT, the internal name starts with a "." */
const char *pluc_get_lut_output_internal_name(int pos)
{
  static char s[32];
  sprintf(s, ".LUT%02d", pos);
  return s;
}

/* return the LUT name, as used in the wire table...  */
const char *pluc_get_lut_output_name(int pos)
{
  static char s[32];
  sprintf(s, "LUT%d", pos);
  return s;
}

/* remove a don't care column, which may appear during the map algorithm */
void pluc_remove_dc(pinfo *pi, dclist cl)
{
  int i;
  
  i = 0;
  while( i < pinfoGetInCnt(pi) )
  {
    if ( dclIsDCInVar(pi, cl, i) != 0 )
    {
      dclDeleteIn(pi, cl, i);
      pinfoDeleteInLabel(pi, i);      
    }
    else
    {
      i++;
    }    
  }
}



/*
  add a (reduced) problem into the lut list and increase the number of
  luts in the list.

  All don't cares are removed before the LUT is added (pluc_remove_dc). 
*/
int pluc_add_lut(pinfo *pi, dclist cl)
{
  
  if ( pinfoCopy( &(pluc_lut_list[pluc_lut_cnt].pi), pi) == 0 )
    return 0;

  if ( dclCopy(pi, pluc_lut_list[pluc_lut_cnt].dcl, cl) == 0 )
    return 0;
  
  pluc_remove_dc(&(pluc_lut_list[pluc_lut_cnt].pi), pluc_lut_list[pluc_lut_cnt].dcl);

  //if ( pluc_lut_list[pluc_lut_cnt].user_out_name != NULL )
  //  free(pluc_lut_list[pluc_lut_cnt].user_out_name);
  //pluc_lut_list[pluc_lut_cnt].user_out_name = NULL;
  pluc_lut_list[pluc_lut_cnt].is_placed = 0;
   
   pluc_lut_cnt++;
   
   return 1;
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
    
    if ( pluc_add_lut(pi, cl) == 0 )
      return 0;
    pluc_log("Map: Leaf fn (in-cnt %d) added to LUT table (index %d), output '%s'", none_dc_cnt, pluc_lut_cnt-1, pinfoGetOutLabel(pi, 0));
    
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
  new_left_lut = pluc_internal_cnt+0;
  new_right_lut = pluc_internal_cnt+1;
  pluc_internal_cnt += 2;
  
  
  /* create a new boolean function which connects the later functions */
  {
    pinfo *pi_connect = pinfoOpen();
    dclist cl_connect;
      
    

    if ( pinfoAddOutLabel(pi_connect, pinfoGetOutLabel(pi, 0)) < 0 )
      return pinfoClose(pi_connect), 0;
    
    if ( pinfoAddInLabel(pi_connect, pinfoGetInLabel(pi, i)) < 0 )
      return pinfoClose(pi_connect), 0;
    
    if ( pinfoAddInLabel(pi_connect, pluc_get_lut_output_internal_name(new_left_lut)) < 0 )
      return pinfoClose(pi_connect), 0;

    if ( pinfoAddInLabel(pi_connect, pluc_get_lut_output_internal_name(new_right_lut)) < 0 )
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
    
    if ( pluc_add_lut(pi_connect, cl_connect) == 0 )
      return pinfoClose(pi_connect), dclDestroy(cl_connect), 0;

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

  pinfoSetOutLabel(pi, 0, pluc_get_lut_output_internal_name(new_left_lut));
  if ( pluc_map_cof(pi, cl_left, cofactor_left, depth+1) == 0 )
    return dclDestroyVA(2, cl_left, cl_right), 0;
  
  pinfoSetOutLabel(pi, 0, pluc_get_lut_output_internal_name(new_right_lut));
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
/*=== route ===*/
/*==================================================*/



/*==================================================*/
/* Wire replace procedures */

int pluc_pinfo_replace_in_label(pinfo *pi, const char *search, const char *replace)
{
  int pos;
  pos = pinfoFindInLabelPos(pi, search);
  if ( pos < 0 )
    return 1;
  return pinfoSetInLabel(pi, pos, replace);
}

int pluc_replace_all_lut_in(const char *search, const char *replace)
{
  int i;
  //pluc_log("Route: Replace internal signal %s --> %s", search, replace);
  for( i = 0; i < pluc_lut_cnt; i++ )
  {
    if ( pluc_pinfo_replace_in_label( &(pluc_lut_list[i].pi), search, replace ) == 0 )
      return 0;
  }
  return 1;
}

int pluc_replace_internal_lut_signals(void)
{
  int i;
  const char *s;
  
  pluc_log("Route: Replace internal signal names");
  
  for( i = 0; i < pluc_lut_cnt; i++ )
  {
    s = pinfoGetOutLabel(&(pluc_lut_list[i].pi), 0);
    if ( s != NULL )
    {
      if ( s[0] == '.' )
      {
	if ( pluc_replace_all_lut_in(s, pluc_get_lut_output_name(i)) == 0 )
	  return 0;
	if ( pinfoSetOutLabel(&(pluc_lut_list[i].pi), 0, pluc_get_lut_output_name(i)) == 0 )
	  return 0;
      }
    }
  }
  return 1;
}

/*==================================================*/
/* Route find procedures */

/* array, which stores the current route chain, generated by  "pluc_calc_route_chain" */
#define PLUC_ROUTE_CHAIN_MAX 16
int pluc_route_chain_list[PLUC_ROUTE_CHAIN_MAX];		/* points into wire table */
int pluc_route_chain_cnt = 0;

/* mark the current list in the wire table */
void pluc_mark_route_chain_wire_list(void)
{
  int i, j;
  for( i = 0; i < pluc_route_chain_cnt; i++ )
  {
    /* mark the route as used */
    lpc804_wire_table[pluc_route_chain_list[i]].is_used = 1;
    
    /* mark all other routs, leading to the same resource, as blocked */
    j = 0;
    while( lpc804_wire_table[j].from != NULL )
    {
      if ( j != pluc_route_chain_list[i] )
      {
	if ( strcmp(lpc804_wire_table[pluc_route_chain_list[i]].to, lpc804_wire_table[j].to) == 0 )
	{
	  lpc804_wire_table[j].is_blocked = 1;
	  //pluc_log("Route: Blocked %s -> %s", lpc804_wire_table[j].from, lpc804_wire_table[j].to);
	}
      }
      j++;
    }
   
  }
}

int pluc_find_to(const char *s)
{
    int i = 0;
    while( lpc804_wire_table[i].from != NULL )
    {
      if ( strcmp(lpc804_wire_table[i].to, s) == 0 )
	return i;
      i++;
    }
    return -1;
}

int pluc_find_from(const char *s)
{
    int i = 0;
    while( lpc804_wire_table[i].from != NULL )
    {
      if ( strcmp(lpc804_wire_table[i].from, s) == 0 )
	return i;
      i++;
    }
    return -1;
}



/*

  OBSOLETE

  calculate a route from the provided input or output node 
  result is stored in
    - pluc_route_chain_list
    - pluc_route_chain_cnt

  Args:
    s:	port name
    is_in_to_out: route direction 0: from out to in, 1: from in to out
*/
int pluc_calc_route_chain(const char *s, int is_in_to_out)
{
  int i;
  
  
  pluc_route_chain_cnt = 0;
  i = 0;
  while( i < PLUC_ROUTE_CHAIN_MAX )
  {
    if ( is_in_to_out ) 
      pluc_route_chain_list[i] = pluc_find_from(s);
    else
      pluc_route_chain_list[i] = pluc_find_to(s);
    if ( pluc_route_chain_list[i] < 0 )
    {
      /* not found */
      pluc_err("'%s' unknown", s);
      return 0;
    }
    pluc_log("Route: From %s to %s", lpc804_wire_table[pluc_route_chain_list[i]].from, lpc804_wire_table[pluc_route_chain_list[i]].to);

    if ( is_in_to_out ) 
    {
      if ( lpc804_wire_table[pluc_route_chain_list[i]].is_lut_in != 0 )
      {
	i++;
	break;
      }
    }
    else
    {
      if ( lpc804_wire_table[pluc_route_chain_list[i]].is_lut_out != 0 )
      {
	i++;
	break;
      }
    }

    
    
    if ( is_in_to_out ) 
      s = lpc804_wire_table[pluc_route_chain_list[i]].to;
    else
      s = lpc804_wire_table[pluc_route_chain_list[i]].from;
    i++;
  }

  pluc_route_chain_cnt = i;  
  return 1;
}

int _pluc_calc_from_to_sub(const char *from, const char *to)
{
  int i = 0;

  //pluc_log("from_to: %s-->%s", from, to);
  while( lpc804_wire_table[i].from != NULL )
  {
    if ( lpc804_wire_table[i].is_blocked == 0 )
    {
      if ( strcmp(lpc804_wire_table[i].from, from) == 0 )
      {
	if ( strcmp(lpc804_wire_table[i].to, to) == 0 )
	{
	  /* found leaf node */
	  pluc_route_chain_list[pluc_route_chain_cnt++] = i;
	  pluc_log("from_to: %s-->%s", from, to);
	  return 1;
	}
	if ( _pluc_calc_from_to_sub(lpc804_wire_table[i].to, to) != 0 )
	{
	  pluc_route_chain_list[pluc_route_chain_cnt++] = i;
	  pluc_log("from_to: %s-->%s", from, to);
	  return 1;
	}
      }
    }
    i++;
  }
  return 0;
}

/*
  calculate the chain from the given wire to the provided wire.
  fills the pluc_route_chain_list array and also updates pluc_route_chain_cnt 
  During the calculation any is_used connections are ignored.
*/
int pluc_calc_from_to(const char *from, const char *to)
{
  pluc_route_chain_cnt = 0;
  return _pluc_calc_from_to_sub(from, to);
}

/*
  All required luts are stored in pluc_lut_list.
  However luts are not yet placed and connected.
  This function will place the external luts (those which are 
  connected to a none-internal signal) in the correct lut.
  Internl signals names start with a ".".
*/
int pluc_route_external_connected_luts(void)
{
  int i;
  const char *s;
  pluc_log("Route: LUTs with external signals", pluc_lut_cnt);
  for( i = 0; i < pluc_lut_cnt; i++ )
  {
    if ( pluc_lut_list[i].is_placed == 0 )
    {
      s = pinfoGetOutLabel( &(pluc_lut_list[i].pi), 0);
      if ( s != NULL )
      {
	if ( s[0] != '.' )
	{
	  
	  if ( strcmp( pluc_get_lut_output_name(i), pinfoGetOutLabel(&(pluc_lut_list[i].pi), 0) ) != 0 )
	  {
	    if ( pluc_calc_from_to(pluc_get_lut_output_name(i), pinfoGetOutLabel(&(pluc_lut_list[i].pi), 0) ) != 0 )
	    {
	      
	      pluc_log("Route: Path found from LUT%d to %s ", i, pinfoGetOutLabel(&(pluc_lut_list[i].pi), 0));
	      /* TODO: We must search from LUT i to output pinfoGetOutLabel(&(pluc_lut_list[i].pi), 0) */
	      
	      
	      //if ( pluc_calc_route_chain(s, 0) == 0 )	// WRONG function call
	      //{
	      //  pluc_err("No route found from '%s' to any LUT", s);
	      //  return 0;
	      //}
	      pluc_mark_route_chain_wire_list();		/* route found.. mark it! */
	      
	      //if ( pluc_lut_list[i].user_out_name != NULL )
	      //  free(pluc_lut_list[i].user_out_name);
	      //pluc_lut_list[i].user_out_name = strdup(s);
	      //if ( pluc_lut_list[i].user_out_name == NULL )
	      //  return pluc_err("memory error"), 0;
	      
	      
	      //pluc_log("Output '%s' connected to LUT%d", pinfoGetOutLabel(&(pluc_lut_list[i].pi), 0), i);
	      
	      
	      pluc_lut_list[i].is_placed = 1;			/* mark the LUT as done */
	    }
	    else
	    {
	      pluc_err("Route: No route found from LUT%d to %s", i, pinfoGetOutLabel(&(pluc_lut_list[i].pi), 0));
	    }
	  } // LUT output is identical to LUT name
	} // external connection
      } // s != NULL
    } // is_placed == 0
  }
  return 1;
}

/*
  works, but...:
  TODO:
  This procedure must be more clever. 
  
  PIO0_12    -> PLUINPUT4 --> LUT5
  But later, ther should be another connection
  PIO0_12    -> PLUINPUT4 --> LUT6
  which will fail, because PIO0_12    -> PLUINPUT4 is already used.
  However, this is wrong, because the source has not changed and PIO0_12    -> PLUINPUT4 can be reused.

  Eigentlich müsste es doch so sein: Wenn es mehrere übergänge zu dem selben Ziel gibt, müssten diese unkenntlich gemacht werden
  also alles andere ausser 
    PIO0_12    -> PLUINPUT4
  das ebenfalls nach PLUINPUT4 geht, müsste deaktiviert werden (darf nicht mehr verwendet werden)
*/

int pluc_route_lut_input(void)
{
  int i, j;
  char in[32];
  pinfo *pi;

  pluc_log("Route: LUT input route");
  
  for( i = 0; i < pluc_lut_cnt; i++ )
  {
    pi = &(pluc_lut_list[i].pi);
    for( j = 0; j < pinfoGetInCnt(pi); j++ )
    {
	
      sprintf(in, "%s_INP%d", pluc_get_lut_output_name(i), j);
      if ( pluc_calc_from_to(pinfoGetInLabel(pi, j), in ) != 0 )
      {
	  pluc_log("Route: LUT input path found from %s to %s ", pinfoGetInLabel(pi, j), in);
	  pluc_mark_route_chain_wire_list();		/* route found.. mark it! */
      }
      else
      {
	pluc_err("Route: No LUT input path found from %s to %s", pinfoGetInLabel(pi, j), in);
	return 0;
      }
    }
  }
  return 1;
}


int pluc_route(void)
{
  /* rename the internal signal name to normal LUT names */
  /* this is actually a result of the previous step, but is done here instead */
  if ( pluc_replace_internal_lut_signals() == 0 )
    return 0;

  /* connect LUT output signal, ignore LUTs where the output signal is identical with the LUT name */
  if ( pluc_route_external_connected_luts() == 0 )
    return 0;

  if ( pluc_route_lut_input() == 0 )
    return 0;
  
  for( int i = 0; i < pluc_lut_cnt; i++ )
  {
    printf("pluc_lut_list entry %d:\n", i);
    dclShow(&(pluc_lut_list[i].pi), pluc_lut_list[i].dcl);    
  }
  
  
  return 1;
}

/*==================================================*/
/* code generation */
/*

  Return the configuration value for the LUT.
  All unused colums had been previously removed (see pluc_remove_dc), so that the LUT has five or lesser inputs.


*/

void pluc_out(const char *s)
{
  printf("%s", s);
}

void pluc_out_regop(pluc_regop_t *regop)
{
  static char s[1024];
  uint32_t adr = 0L;
  switch(regop->base)
  {
    case 0:	adr = 0x40028000;	break;
    case 1:	adr = 0x4000c000;	break;
    default: assert(0); break;
  }
  adr += regop->idx;
  
  assert((regop->idx & 3) == 0);
  
  switch(regop->op)
  {
    case 0:
      s[0] = '\0';
      break;
    case 1:
      sprintf(s, "*(uint32_t *)0x%08x = 0x%08xUL;", adr, regop->or_value);
      break;
    case 2:
      sprintf(s, "*(uint32_t *)0x%08x &= 0x%08xUL;", adr, regop->and_value);
      break;
    case 3:
      sprintf(s, "*(uint32_t *)0x%08x &= ~0x%08xUL;", adr, regop->and_value);
      break;
    case 4:
      sprintf(s, "*(uint32_t *)0x%08x |= 0x%08xUL;", adr, regop->or_value);
      break;
    case 5:
      sprintf(s, "*(uint32_t *)0x%08x |= ~0x%08xUL;", adr, regop->or_value);
      break;
    case 6:
      sprintf(s, "*(uint32_t *)0x%08x &= 0x%08xUL; *(uint32_t *)0x%08x |= 0x%08xUL;", adr, regop->and_value, adr, regop->or_value);
      break;
    case 7:
      sprintf(s, "*(uint32_t *)0x%08x &= ~0x%08xUL; *(uint32_t *)0x%08x |= 0x%08xUL;", adr, regop->and_value, adr, regop->or_value);
      break;
  }
  
  pluc_out("\t");
  pluc_out(s);
  pluc_out("\n");
}

void pluc_out_wire(pluc_wire_t *wire)
{
  static char s[1024];
  int i;
  sprintf("\t/* %s --> %s */\n", wire->from, wire->to);
  pluc_out(s);
  for( i = 0; i < PLUC_WIRE_REGOP_CNT; i++ )
  {
    pluc_out_regop(wire->regop+i);
  }
}



uint32_t pluc_get_lut_config_value(int lut)
{
  dcube *input;
  uint32_t result;
  int in_cnt;
  int i;
  int bit_cnt;
  
  in_cnt = pinfoGetInCnt(&(pluc_lut_list[lut].pi));
  assert( in_cnt <= 5 );
  
  bit_cnt = 1<<in_cnt;
  
  input = &(pluc_lut_list[lut].pi.tmp[10]);
  
  /* prepare the counter, all DC except for the inputs (which are set to 0) */
  dcInSetAll(&(pluc_lut_list[lut].pi), input, CUBE_IN_MASK_DC);
  for( i = 0; i < in_cnt; i++ )
    dcSetIn(input, i, 1);
  
  /* calculate the truth table for each input combination */
  result = 0;
  for( i = 0; i < bit_cnt; i++ )
  {
    dclResult(&(pluc_lut_list[lut].pi), input, pluc_lut_list[lut].dcl);
    if ( dcGetOut(input, 0) != 0 )
      result |= 1;
    
    result <<= 1;
    dcInc(&(pluc_lut_list[lut].pi), input);
  }
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
  if ( pluc_route() == 0 )
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
  { CL_TYP_STRING,  "testoutroute-Find a route from given output to a LUT", cmdline_output, 1024 },
  { CL_TYP_STRING,  "testinroute-Find a route from given input to a LUT", cmdline_input, 1024 },
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
      printf("%05d: %c/%c %-10s -> %-10s\n", i, lpc804_wire_table[i].is_lut_in?'i':'-', lpc804_wire_table[i].is_lut_out?'o':'-', lpc804_wire_table[i].from, lpc804_wire_table[i].to );
      i++;
    }
    exit(0);
  }
  
  if ( cmdline_output[0] != '\0' )
  {
    pluc_calc_route_chain(cmdline_output, 0);
    exit(1);
  }

  if ( cmdline_input[0] != '\0' )
  {
    pluc_calc_route_chain(cmdline_input, 1);
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

