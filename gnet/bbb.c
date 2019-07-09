/*

  BBB.C

  Basic Building Blocks for gnet
  
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

/*! \defgroup gncbbb gnc Basic Building Blocks */


#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dcube.h"
#include "gnet.h"
#include "mwc.h"

#define BBB_PLA_MAX 8
struct _bbb_desc_struct
{
  int id;
  char *desc;
  int info;
  pinfo *pi;
  dclist cl[BBB_PLA_MAX];
  const char **pla_list[BBB_PLA_MAX];
};
typedef struct _bbb_desc_struct bbb_desc_struct;

#define BBB_FF_PORTS_MAX 8
struct _bbb_ff_struct
{
  int id;
  char *desc;
  int cnt;
  int type[BBB_FF_PORTS_MAX];
  int fn[BBB_FF_PORTS_MAX];
  int is_inverted[BBB_FF_PORTS_MAX];
};
typedef struct _bbb_ff_struct bbb_ff_struct;

/*----- INV -----*/

const char *bbb_pla_inv[] = 
{
  ".i 1",
  ".o 1",
  "0 1",
  NULL
};

bbb_desc_struct bbb_desc_inv = 
{
  GCELL_ID_INV,
  "inverter",
  0, 
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_inv, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- BUF -----*/

const char *bbb_pla_buf[] = 
{
  ".i 1",
  ".o 1",
  "1 1",
  NULL
};

bbb_desc_struct bbb_desc_buf = 
{
  GCELL_ID_BUF,
  "buffer",
  0, 
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_buf, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- AND2 -----*/

const char *bbb_pla_and2[] = 
{
  ".i 2",
  ".o 1",
  "11 1",
  NULL
};

bbb_desc_struct bbb_desc_and2 = 
{
  GCELL_ID_AND2,
  "2-input and",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_and2, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NAND2 -----*/

const char *bbb_pla_nand2[] = 
{
  ".i 2",
  ".o 1",
  "00 1",
  "01 1",
  "10 1",
  NULL
};

bbb_desc_struct bbb_desc_nand2 = 
{
  GCELL_ID_NAND2,
  "2-input nand",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nand2, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- OR2 -----*/

const char *bbb_pla_or2[] = 
{
  ".i 2",
  ".o 1",
  "01 1",
  "10 1",
  "11 1",
  NULL
};

bbb_desc_struct bbb_desc_or2 = 
{
  GCELL_ID_OR2,
  "2-input or",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_or2, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NOR2 -----*/

const char *bbb_pla_nor2[] = 
{
  ".i 2",
  ".o 1",
  "00 1",
  NULL
};

bbb_desc_struct bbb_desc_nor2 = 
{
  GCELL_ID_NOR2,
  "2-input nor",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nor2, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- AND3 -----*/

const char *bbb_pla_and3[] =
{
  ".i 3",
  ".o 1",
  "111 1",
  NULL
};

bbb_desc_struct bbb_desc_and3 = 
{
  GCELL_ID_AND3,
  "3-input and",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_and3, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NAND3 -----*/

const char *bbb_pla_nand3[] =
{
  ".i 3",
  ".o 1",
  "000 1",
  "001 1",
  "010 1",
  "100 1",
  "011 1",
  "101 1",
  "110 1",
  NULL
};

bbb_desc_struct bbb_desc_nand3 = 
{
  GCELL_ID_NAND3,
  "3-input nand",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nand3, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- OR3 -----*/

const char *bbb_pla_or3[] = 
{
  ".i 3",
  ".o 1",
  "001 1",
  "010 1",
  "100 1",
  "011 1",
  "101 1",
  "110 1",
  "111 1",
  NULL
};

bbb_desc_struct bbb_desc_or3 = 
{
  GCELL_ID_OR3,
  "3-input or",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_or3, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NOR3 -----*/

const char *bbb_pla_nor3[] =
{
  ".i 3",
  ".o 1",
  "000 1",
  NULL
};

bbb_desc_struct bbb_desc_nor3 = 
{
  GCELL_ID_NOR3,
  "3-input nor",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nor3, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- AND4 -----*/

const char *bbb_pla_and4[] =
{
  ".i 4",
  ".o 1",
  "1111 1",
  NULL
};

bbb_desc_struct bbb_desc_and4 = 
{
  GCELL_ID_AND4,
  "4-input and",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_and4, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NAND4 -----*/

const char *bbb_pla_nand4[] =
{
  ".i 4",
  ".o 1",
  "0--- 1",
  "-0-- 1",
  "--0- 1",
  "---0 1",
  NULL
};

bbb_desc_struct bbb_desc_nand4 = 
{
  GCELL_ID_NAND4,
  "4-input nand",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nand4, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- OR4 -----*/

const char *bbb_pla_or4[] = 
{
  ".i 4",
  ".o 1",
  "1--- 1",
  "-1-- 1",
  "--1- 1",
  "---1 1",
  NULL
};

bbb_desc_struct bbb_desc_or4 = 
{
  GCELL_ID_OR4,
  "4-input or",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_or4, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NOR4 -----*/

const char *bbb_pla_nor4[] =
{
  ".i 4",
  ".o 1",
  "0000 1",
  NULL
};

bbb_desc_struct bbb_desc_nor4 = 
{
  GCELL_ID_NOR4,
  "4-input nor",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nor4, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- AND5 -----*/

const char *bbb_pla_and5[] =
{
  ".i 5",
  ".o 1",
  "11111 1",
  NULL
};

bbb_desc_struct bbb_desc_and5 = 
{
  GCELL_ID_AND5,
  "5-input and",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_and5, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NAND5 -----*/

const char *bbb_pla_nand5[] =
{
  ".i 5",
  ".o 1",
  "0---- 1",
  "-0--- 1",
  "--0-- 1",
  "---0- 1",
  "----0 1",
  NULL
};

bbb_desc_struct bbb_desc_nand5 = 
{
  GCELL_ID_NAND5,
  "5-input nand",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nand5, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- OR5 -----*/

const char *bbb_pla_or5[] = 
{
  ".i 5",
  ".o 1",
  "1---- 1",
  "-1--- 1",
  "--1-- 1",
  "---1- 1",
  "----1 1",
  NULL
};

bbb_desc_struct bbb_desc_or5 = 
{
  GCELL_ID_OR5,
  "5-input or",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_or5, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NOR5 -----*/

const char *bbb_pla_nor5[] =
{
  ".i 5",
  ".o 1",
  "00000 1",
  NULL
};

bbb_desc_struct bbb_desc_nor5 = 
{
  GCELL_ID_NOR5,
  "5-input nor",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nor5, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- AND6 -----*/

const char *bbb_pla_and6[] =
{
  ".i 6",
  ".o 1",
  "111111 1",
  NULL
};

bbb_desc_struct bbb_desc_and6 = 
{
  GCELL_ID_AND6,
  "6-input and",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_and6, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NAND6 -----*/

const char *bbb_pla_nand6[] =
{
  ".i 6",
  ".o 1",
  "0----- 1",
  "-0---- 1",
  "--0--- 1",
  "---0-- 1",
  "----0- 1",
  "-----0 1",
  NULL
};

bbb_desc_struct bbb_desc_nand6 = 
{
  GCELL_ID_NAND6,
  "6-input nand",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nand6, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- OR6 -----*/

const char *bbb_pla_or6[] =
{
  ".i 6",
  ".o 1",
  "1----- 1",
  "-1---- 1",
  "--1--- 1",
  "---1-- 1",
  "----1- 1",
  "-----1 1",
  NULL
};

bbb_desc_struct bbb_desc_or6 = 
{
  GCELL_ID_OR6,
  "6-input or",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_or6, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NOR6 -----*/

const char *bbb_pla_nor6[] =
{
  ".i 6",
  ".o 1",
  "000000 1",
  NULL
};

bbb_desc_struct bbb_desc_nor6 = 
{
  GCELL_ID_NOR6,
  "6-input nor",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nor6, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- AND7 -----*/

const char *bbb_pla_and7[] =
{
  ".i 7",
  ".o 1",
  "1111111 1",
  NULL
};

bbb_desc_struct bbb_desc_and7 = 
{
  GCELL_ID_AND7,
  "7-input and",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_and7, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NAND7 -----*/

const char *bbb_pla_nand7[] =
{
  ".i 7",
  ".o 1",
  "0------ 1",
  "-0----- 1",
  "--0---- 1",
  "---0--- 1",
  "----0-- 1",
  "-----0- 1",
  "------0 1",
  NULL
};

bbb_desc_struct bbb_desc_nand7 =
{
  GCELL_ID_NAND7,
  "7-input nand",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nand7, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- OR7 -----*/

const char *bbb_pla_or7[] =
{
  ".i 7",
  ".o 1",
  "1------ 1",
  "-1----- 1",
  "--1---- 1",
  "---1--- 1",
  "----1-- 1",
  "-----1- 1",
  "------1 1",
  NULL
};

bbb_desc_struct bbb_desc_or7 = 
{
  GCELL_ID_OR7,
  "7-input or",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_or7, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NOR7 -----*/

const char *bbb_pla_nor7[] =
{
  ".i 7",
  ".o 1",
  "0000000 1",
  NULL
};

bbb_desc_struct bbb_desc_nor7 = 
{
  GCELL_ID_NOR7,
  "7-input nor",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nor7, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- AND8 -----*/

const char *bbb_pla_and8[] =
{
  ".i 8",
  ".o 1",
  "11111111 1",
  NULL
};

bbb_desc_struct bbb_desc_and8 = 
{
  GCELL_ID_AND8,
  "8-input and",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_and8, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NAND8 -----*/

const char *bbb_pla_nand8[] =
{
  ".i 8",
  ".o 1",
  "0------- 1",
  "-0------ 1",
  "--0----- 1",
  "---0---- 1",
  "----0--- 1",
  "-----0-- 1",
  "------0- 1",
  "-------0 1",
  NULL
};

bbb_desc_struct bbb_desc_nand8 =
{
  GCELL_ID_NAND8,
  "8-input nand",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nand8, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- OR8 -----*/

const char *bbb_pla_or8[] =
{
  ".i 8",
  ".o 1",
  "1------- 1",
  "-1------ 1",
  "--1----- 1",
  "---1---- 1",
  "----1--- 1",
  "-----1-- 1",
  "------1- 1",
  "-------1 1",
  NULL
};

bbb_desc_struct bbb_desc_or8 = 
{
  GCELL_ID_OR8,
  "8-input or",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_or8, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----- NOR8 -----*/

const char *bbb_pla_nor8[] =
{
  ".i 8",
  ".o 1",
  "00000000 1",
  NULL
};

bbb_desc_struct bbb_desc_nor8 = 
{
  GCELL_ID_NOR8,
  "8-input nor",
  0,
  NULL,
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  { bbb_pla_nor8, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/*----------------------------------------------------------------*/

bbb_desc_struct *bbb_desc_list[] = 
{
  &bbb_desc_inv,
  &bbb_desc_buf,

  &bbb_desc_and2,
  &bbb_desc_nand2,
  &bbb_desc_or2,
  &bbb_desc_nor2,

  &bbb_desc_and3,
  &bbb_desc_nand3,
  &bbb_desc_or3,
  &bbb_desc_nor3,

  &bbb_desc_and4,
  &bbb_desc_nand4,
  &bbb_desc_or4,
  &bbb_desc_nor4,

  &bbb_desc_and5,
  &bbb_desc_nand5,
  &bbb_desc_or5,
  &bbb_desc_nor5,

  &bbb_desc_and6,
  &bbb_desc_nand6,
  &bbb_desc_or6,
  &bbb_desc_nor6,

  &bbb_desc_and7,
  &bbb_desc_nand7,
  &bbb_desc_or7,
  &bbb_desc_nor7,

  &bbb_desc_and8,
  &bbb_desc_nand8,
  &bbb_desc_or8,
  &bbb_desc_nor8,

  NULL
};

/*----------------------------------------------------------------*/

bbb_ff_struct bbb_ff_dff_q_nq = 
{
  GCELL_ID_DFF_Q_NQ,
  "DFF with Q and QN",
  4,
  { GPORT_TYPE_IN, GPORT_TYPE_IN, GPORT_TYPE_OUT, GPORT_TYPE_OUT, 0, 0, 0, 0 },
  { GPORT_FN_D,    GPORT_FN_CLK,  GPORT_FN_Q,     GPORT_FN_Q,     0, 0, 0, 0 },
  { 0,             0,             0,              1,              0, 0, 0, 0 }
};

bbb_ff_struct bbb_ff_dff_q_nq_lc = 
{
  GCELL_ID_DFF_Q_NQ_LC,
  "DFF with Q and QN, clear on low",
  5,
  { GPORT_TYPE_IN, GPORT_TYPE_IN, GPORT_TYPE_OUT, GPORT_TYPE_OUT, GPORT_TYPE_IN, 0, 0, 0 },
  { GPORT_FN_D,    GPORT_FN_CLK,  GPORT_FN_Q,     GPORT_FN_Q,     GPORT_FN_CLR,  0, 0, 0 },
  { 0,             0,             0,              1,              1,             0, 0, 0 }
};

bbb_ff_struct bbb_ff_dff_q_nq_hc = 
{
  GCELL_ID_DFF_Q_NQ_HC,
  "DFF with Q and QN, clear on high",
  5,
  { GPORT_TYPE_IN, GPORT_TYPE_IN, GPORT_TYPE_OUT, GPORT_TYPE_OUT, GPORT_TYPE_IN, 0, 0, 0 },
  { GPORT_FN_D,    GPORT_FN_CLK,  GPORT_FN_Q,     GPORT_FN_Q,     GPORT_FN_CLR,  0, 0, 0 },
  { 0,             0,             0,              1,              0,             0, 0, 0 }
};


bbb_ff_struct bbb_ff_dff_q_nq_ls = 
{
  GCELL_ID_DFF_Q_NQ_LS,
  "DFF with Q and QN, set on low",
  5,
  { GPORT_TYPE_IN, GPORT_TYPE_IN, GPORT_TYPE_OUT, GPORT_TYPE_OUT, GPORT_TYPE_IN, 0, 0, 0 },
  { GPORT_FN_D,    GPORT_FN_CLK,  GPORT_FN_Q,     GPORT_FN_Q,     GPORT_FN_SET,  0, 0, 0 },
  { 0,             0,             0,              1,              1,             0, 0, 0 }
};

bbb_ff_struct bbb_ff_dff_q_nq_hs = 
{
  GCELL_ID_DFF_Q_NQ_HS,
  "DFF with Q and QN, set on high",
  5,
  { GPORT_TYPE_IN, GPORT_TYPE_IN, GPORT_TYPE_OUT, GPORT_TYPE_OUT, GPORT_TYPE_IN, 0, 0, 0 },
  { GPORT_FN_D,    GPORT_FN_CLK,  GPORT_FN_Q,     GPORT_FN_Q,     GPORT_FN_SET,  0, 0, 0 },
  { 0,             0,             0,              1,              0,             0, 0, 0 }
};




bbb_ff_struct bbb_ff_dff_q = 
{
  GCELL_ID_DFF_Q,
  "DFF with Q",
  3,
  { GPORT_TYPE_IN, GPORT_TYPE_IN, GPORT_TYPE_OUT, 0, 0, 0, 0, 0 },
  { GPORT_FN_D,    GPORT_FN_CLK,  GPORT_FN_Q,     0, 0, 0, 0, 0 },
  { 0,             0,             0,              0, 0, 0, 0, 0 }
};

bbb_ff_struct bbb_ff_dff_q_lc = 
{
  GCELL_ID_DFF_Q_LC,
  "DFF with Q, clear on low",
  4,
  { GPORT_TYPE_IN, GPORT_TYPE_IN, GPORT_TYPE_OUT, GPORT_TYPE_IN, 0, 0, 0, 0 },
  { GPORT_FN_D,    GPORT_FN_CLK,  GPORT_FN_Q,     GPORT_FN_CLR,  0, 0, 0, 0 },
  { 0,             0,             0,              1,             0, 0, 0, 0 }
};

bbb_ff_struct bbb_ff_dff_q_hc =
{
  GCELL_ID_DFF_Q_HC,
  "DFF with Q, clear on high",
  4,
  { GPORT_TYPE_IN, GPORT_TYPE_IN, GPORT_TYPE_OUT, GPORT_TYPE_IN, 0, 0, 0, 0 },
  { GPORT_FN_D,    GPORT_FN_CLK,  GPORT_FN_Q,     GPORT_FN_CLR,  0, 0, 0, 0 },
  { 0,             0,             0,              0,             0, 0, 0, 0 }
};


bbb_ff_struct bbb_ff_dff_q_ls =
{
  GCELL_ID_DFF_Q_LS,
  "DFF with Q, set on low",
  4,
  { GPORT_TYPE_IN, GPORT_TYPE_IN, GPORT_TYPE_OUT, GPORT_TYPE_IN, 0, 0, 0, 0 },
  { GPORT_FN_D,    GPORT_FN_CLK,  GPORT_FN_Q,     GPORT_FN_SET,  0, 0, 0, 0 },
  { 0,             0,             0,              1,             0, 0, 0, 0 }
};

bbb_ff_struct bbb_ff_dff_q_hs =
{
  GCELL_ID_DFF_Q_HS,
  "DFF with Q, set on high",
  4,
  { GPORT_TYPE_IN, GPORT_TYPE_IN, GPORT_TYPE_OUT, GPORT_TYPE_IN, 0, 0, 0, 0 },
  { GPORT_FN_D,    GPORT_FN_CLK,  GPORT_FN_Q,     GPORT_FN_SET,  0, 0, 0, 0 },
  { 0,             0,             0,              0,             0, 0, 0, 0 }
};


bbb_ff_struct *bbb_ff_list[] = 
{
  &bbb_ff_dff_q_nq,
  &bbb_ff_dff_q_nq_lc,
  &bbb_ff_dff_q_nq_hc,
  &bbb_ff_dff_q_nq_ls,
  &bbb_ff_dff_q_nq_hs,
  &bbb_ff_dff_q,
  &bbb_ff_dff_q_lc,
  &bbb_ff_dff_q_hc,
  &bbb_ff_dff_q_ls,
  &bbb_ff_dff_q_hs,

  NULL
};

/*----------------------------------------------------------------*/

static int bbb_check_cl(pinfo *pi, dclist cl, pinfo *pi_r, dclist cl_r)
{
  if ( pi->in_cnt != pi_r->in_cnt || pi->out_cnt != pi_r->out_cnt )
    return 0;
  /*
  puts("--- chk ---");
  dclShow(pi, cl);
  puts("--- ref ---");
  dclShow(pi_r, cl_r);
  */
  return dclIsEquivalent(pi, cl, cl_r);
}

static int bbb_check_desc(bbb_desc_struct *bbbd, pinfo *pi_r, dclist cl_r)
{
  int i;
  for( i = 0; i < BBB_PLA_MAX; i++ )
  {
    if ( bbbd->pla_list[i] != NULL )
    {
      if ( bbb_check_cl(bbbd->pi, bbbd->cl[i], pi_r, cl_r) != 0 )
        return bbbd->id;
    }
  }
  return GCELL_ID_UNKNOWN;
}

void bbb_destroy_dclists()
{
  int i = 0;
  int j;
  while(bbb_desc_list[i] != NULL)
  {
    if ( bbb_desc_list[i]->pi != NULL )
    {
      for( j = 0; j < BBB_PLA_MAX; j++ )
        if ( bbb_desc_list[i]->pla_list[j] != NULL )
          dclDestroy(bbb_desc_list[i]->cl[j]);
      pinfoClose(bbb_desc_list[i]->pi);
    }
    i++;
  }
}

int bbb_init_dclists()
{
  int i = 0;
  int j;
  int k;
  while(bbb_desc_list[i] != NULL)
  {
    bbb_desc_list[i]->pi = pinfoOpen();
    if ( bbb_desc_list[i]->pi == NULL )
      return 0;
    for( j = 0; j < BBB_PLA_MAX; j++ )
    {
      if ( bbb_desc_list[i]->pla_list[j] != NULL )
      {
        if ( dclInit(&(bbb_desc_list[i]->cl[j])) == 0 )
        {
          for(k = 0; k < j; k++ )
            if ( bbb_desc_list[i]->pla_list[k] != NULL )
              dclDestroy(bbb_desc_list[i]->cl[k]);
          pinfoClose(bbb_desc_list[i]->pi);
          bbb_desc_list[i]->pi = NULL;
          bbb_destroy_dclists();
          return 0;
        }
        if ( dclReadPLAStr(bbb_desc_list[i]->pi, bbb_desc_list[i]->cl[j], NULL, bbb_desc_list[i]->pla_list[j]) == 0 )
        {
          for(k = 0; k <= j; k++ )
            if ( bbb_desc_list[i]->pla_list[k] != NULL )
              dclDestroy(bbb_desc_list[i]->cl[k]);
          pinfoClose(bbb_desc_list[i]->pi);
          bbb_desc_list[i]->pi = NULL;
          bbb_destroy_dclists();
          return 0;
        }
      }
    }
    i++;
  }
  return 1;
}



int bbb_check_desc_list(pinfo *pi_r, dclist cl_r)
{
  int i = 0;
  int id;
  while(bbb_desc_list[i] != NULL)
  {
    id = bbb_check_desc(bbb_desc_list[i], pi_r, cl_r);
    if ( id != GCELL_ID_UNKNOWN )
      return i;
    i++;
  }
  return -1;
}

static int bbb_is_ff_port(bbb_ff_struct *bbbff, gport port)
{
  int i;
  for( i = 0; i < bbbff->cnt; i++ )
  {
    if ( port->type == bbbff->type[i] && 
         port->fn == bbbff->fn[i] &&
         port->is_inverted == bbbff->is_inverted[i] )
      return 1;
  }
  return 0;
}

static int bbb_check_ff(bbb_ff_struct *bbbff, gcell cell)
{
  int pos = -1;
  if ( bbbff->cnt != b_set_Cnt(cell->port_set) )
    return GCELL_ID_UNKNOWN;
  while( gcellLoopPortRef(cell, &pos) != 0 )
  {
    if ( bbb_is_ff_port(bbbff, gcellGetGPORT(cell,pos)) == 0 )
      return GCELL_ID_UNKNOWN;
  }
  return bbbff->id;
}

static int bbb_check_ff_list(gcell cell)
{
  int i = 0;
  int id;
  while(bbb_ff_list[i] != NULL)
  {
    id = bbb_check_ff(bbb_ff_list[i], cell);
    if ( id != GCELL_ID_UNKNOWN )
      return i;
    i++;
  }
  return -1;
}

/* if force is != 0, this will force a check if a valid id exists */
int gnc_IdentifyAndMarkBBBs(gnc nc, int force)
{
  int cell_pos;
  int bbb_pos;
  gcell cell;
  
  if ( bbb_init_dclists() == 0 )
    return 0;
  
  cell_pos = -1;
  while( gnc_LoopCell(nc, &cell_pos) )
  {
    cell = gnc_GetGCELL(nc, cell_pos);
    
    if ( force != 0 || cell->id == GCELL_ID_UNKNOWN )
    {
      /* printf("checking cell %s\n", cell->name); */
      if ( cell->pi != NULL && cell->cl_on != NULL )
      {
        bbb_pos = bbb_check_desc_list(cell->pi, cell->cl_on);
        if ( bbb_pos >= 0 )
        {
          cell->id = bbb_desc_list[bbb_pos]->id;
          gcellSetDescription(cell, bbb_desc_list[bbb_pos]->desc);
          gnc_Log(nc, 1, "Identification: Cell '%s' is '%s' (size %lf).", cell->name, cell->desc, cell->area);
        }
      }
      else
      {
        bbb_pos = bbb_check_ff_list(cell);
        if ( bbb_pos >= 0 )
        {
          cell->id = bbb_ff_list[bbb_pos]->id;
          gcellSetDescription(cell, bbb_ff_list[bbb_pos]->desc);
          gnc_Log(nc, 1, "Identification: FF '%s' is '%s' (size %lf).", cell->name, cell->desc, cell->area);
        }
      }
    }
  }
  
  bbb_destroy_dclists();
  return 1;
}

/* returns cell_ref */
static int gnc_find_bbb(gnc nc, int id)
{
  int cell_pos;
  gcell cell;

  int curr_cell_ref = -1;
  double cell_area;
  size_t name_length;
  
  
  
  cell_pos = -1;
  while( gnc_LoopCell(nc, &cell_pos) )
  {
    cell = gnc_GetGCELL(nc, cell_pos);
    if ( cell->id == id )
    {
      if ( curr_cell_ref < 0 )
      {
        curr_cell_ref = cell_pos;
        cell_area = cell->area;
        name_length = strlen(cell->name);
      }
      else
      {
        if ( cell_area > cell->area )
        {
          curr_cell_ref = cell_pos;
          cell_area = cell->area;
          name_length = strlen(cell->name);
        }
        else if ( cell_area == cell->area )
        {
          if ( name_length > strlen(cell->name) )
          {
            curr_cell_ref = cell_pos;
            cell_area = cell->area;
            name_length = strlen(cell->name);
          }
        }
      }
    }
  }
  if ( curr_cell_ref >= 0 )
  {
    cell = gnc_GetGCELL(nc, curr_cell_ref);
    gnc_Log(nc, 3, "Selection: Cell %s ('%s', size %lf) selected.", cell->name, cell->desc, cell->area);
  }
  return curr_cell_ref;
}

int gnc_UpdateWideGateEnvironment(gnc nc)
{
  if ( nc->wge != NULL )
    wge_Close(nc->wge);
  nc->wge = wge_Open(nc);
  if ( nc->wge == NULL )
  {
    gnc_Error(nc, "Can not create technology database.");
    return 0;
  }
  return 1;
}

/* requires a previous call to gnc_IdentifyAndMarkBBBs */
void gnc_BuildBBBQuickAccessList(gnc nc)
{
  int i;
  nc->bbb_cell_ref[0] = -1;
  /* start with 1, skip GCELL_ID_UNKNOWN! */
  for( i = 1; i < GCELL_ID_MAX; i++ )
    if ( nc->bbb_disable[i] != 0 )
      nc->bbb_cell_ref[i] = -1;
    else
      nc->bbb_cell_ref[i] = gnc_find_bbb(nc, i);
}

/*---------------------------------------------------------------------------*/

void gnc_DisableBBB(gnc nc, int id)
{
  assert(id >= 0);
  assert(id < GCELL_ID_MAX);
  nc->bbb_cell_ref[id] = -1;
  nc->bbb_disable[id] = 1;
}

void gnc_EnableBBB(gnc nc, int id)
{
  assert(id >= 0);
  assert(id < GCELL_ID_MAX);
  nc->bbb_cell_ref[id] = -1;
  nc->bbb_disable[id] = 1;
}

int gnc_StrToId(gnc nc, const char *s)
{
  int id = -1;
  if ( strncmp(s, "inv", 3) == 0 )
    id = GCELL_ID_INV;
  else if ( strncmp(s, "and", 3) == 0 )
    id = (atoi(s+3)-1)*4;
  else if ( strncmp(s, "nand", 4) == 0 )
    id = (atoi(s+4)-1)*4+1;
  else if ( strncmp(s, "or", 2) == 0 )
    id = (atoi(s+2)-1)*4+2;
  else if ( strncmp(s, "nor", 3) == 0 )
    id = (atoi(s+3)-1)*4+3;
  if ( id < 0 )
    return -1;
  if ( id >= GCELL_ID_MAX )
    return -1;
  return id;
}

void gnc_DisableStrBBB(gnc nc, const char *sid)
{
  int id = gnc_StrToId(nc, sid);
  if ( id < 0 )
    return;
  gnc_DisableBBB(nc, id);
}

void gnc_EnableStrBBB(gnc nc, const char *sid)
{
  int id = gnc_StrToId(nc, sid);
  if ( id < 0 )
    return;
  gnc_EnableBBB(nc, id);
}

void gnc_DisableStrListBBB(gnc nc, const char *sl)
{
  size_t pos = 0;
  char *sep = " ";
  pos += strspn(sl+pos, sep);
  while( sl[pos] != '\0' )
  {
    gnc_DisableStrBBB(nc, sl+pos);
    pos += strcspn(sl+pos, sep);
    pos += strspn(sl+pos, sep);
  }
}

/*---------------------------------------------------------------------------*/

/*!
  \ingroup gncbbb

  Some cells of a gnc object serve as \em basic \em building \em blocks.
  Those cells have an additional property set: They have a valid
  functional cell identifier (cell-id).
  
  This function applies such a cell-id to all cells (if possible)
  of the gnc object. If the argument \a force is not \c 0, than
  all cell-id's are recalculated. If \a force is \c 0, cells
  with a valid cell-id are untouched.
  
  \param nc A pointer to a gnc structure.
  \param force Control the recalculation of cell-id's
    - 0: Cells with valid cell-id are untouched.
    - not 0: All cell-id's are recalculated.

  \return 0 if an error occured.
    
  \see gnc_Open()
  \see gnc_SynthByFile()
*/
int gnc_ApplyBBBs(gnc nc, int force)
{
  if ( gnc_IdentifyAndMarkBBBs(nc, force) == 0 )
    return 0;
  gnc_BuildBBBQuickAccessList(nc);
  if ( gnc_UpdateWideGateEnvironment(nc) == 0 )
    return 0;
  return 1;
}

/*---------------------------------------------------------------------------*/

int gnc_GetCellById(gnc nc, int id)
{
  assert(id >= 0 && id < GCELL_ID_MAX);
  
  if ( nc->bbb_disable[id] != 0 )
  {
    assert(nc->bbb_cell_ref[id] < 0);
    return -1;
  }

  return nc->bbb_cell_ref[id];
}

/*---------------------------------------------------------------------------*/



