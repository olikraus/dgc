/*

  wiga.h
  
  wide gate implementation

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

#ifndef _WIGA_H
#define _WIGA_H

#include "b_pl.h"
#include "b_il.h"
#include "b_dic.h"


#ifndef _GNC_TYPE
#define _GNC_TYPE
typedef struct _gnc_struct *gnc;
#endif


/* wide gate level */
struct _wgl_struct
{
  b_il_type cells;      /* contains cell_refs */
  int in_cnt;
  double area;
};
typedef struct _wgl_struct wgl_struct;
typedef struct _wgl_struct *wgl_type;

#define wgl_GetCellCnt(wgl) (b_il_GetCnt((wgl)->cells))

#define WIGA_TYPE_AND 0
#define WIGA_TYPE_OR 1

struct _wiga_struct
{
  b_pl_type levels;
  int in_cnt;     /* assigned in wge_create_gate */
  int type;       /* assigned in wge_CreateAndGate, wge_CreateOrGate */
  int is_invert;  /* assigned in wge_CreateNegGate, wge_CreatePureGate */
  double area;
};
typedef struct _wiga_struct wiga_struct;
typedef struct _wiga_struct *wiga_type;

#define wiga_GetWGL(wiga, pos) ((wgl_type)b_pl_GetVal((wiga)->levels, (pos)))
void wiga_Show(wiga_type wiga, gnc nc);


/* wide gate environment */
struct _wge_struct
{
  gnc nc;
  
  b_il_type and_cells;
  b_il_type or_cells;
  b_il_type nand_cells;
  b_il_type nor_cells;
  
  b_il_type and_empty_cells;
  b_il_type or_empty_cells;
  b_il_type nand_inv_cells;
  b_il_type nor_inv_cells;
  
  int is_and_or_logic;
  int is_nand_nor_logic;

  /* variation mit wiederholung */
  int variation_in_cnt;
  int is_best;
  double rate_best;
  b_il_type variation_best_list;
  b_il_type variation_list;
  
  double rate_area;
  int rate_min_in_cnt;
  int rate_max_in_cnt;
  
  /* cache */
  b_dic_type wiga_cache;

  /* synthesis */
  b_il_type out_net_refs;
  b_il_type in_net_refs;
  b_il_type in2_net_refs;
  
};
typedef struct _wge_struct wge_struct;
typedef struct _wge_struct *wge_type;
  
wge_type wge_Open(gnc nc);
void wge_Close(wge_type wge);

int wge_DoVariation(wge_type wge, int in_cnt, b_il_type cells);

wiga_type wge_CreateAndGate(wge_type wge, int in_cnt);
wiga_type wge_CreateOrGate(wge_type wge, int in_cnt);

wiga_type wge_GetGate(wge_type wge, int in_cnt, int type);
int wge_SynthGate(wge_type wge, int cell_ref, int type, 
  b_il_type in_net_refs, int *p_out, int *n_out);

int wge_IsAND(wge_type wge, int id);
int wge_IsNAND(wge_type wge, int id);
int wge_IsOR(wge_type wge, int id);
int wge_IsNOR(wge_type wge, int id);

#endif /* _WIGA_H */
