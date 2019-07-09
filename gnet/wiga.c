/*

  wiga.c
  
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

#include "wiga.h"
#include "gnet.h"
#include <stdlib.h>
#include <assert.h>
#include "mwc.h"
  
/*---------------------------------------------------------------------------*/

wgl_type wgl_Open()
{
  wgl_type wgl;
  wgl = (wgl_type)malloc(sizeof(struct _wgl_struct));
  if ( wgl != NULL )
  {
    wgl->area = 0.0;
    wgl->in_cnt = 0;
    wgl->cells = b_il_Open();
    if ( wgl->cells != NULL )
    {
      return wgl;
    }
    free(wgl);
  }
  return wgl;
}

void wgl_Close(wgl_type wgl)
{
  b_il_Close(wgl->cells);
  free(wgl);
}

int wgl_Show(wgl_type wgl, gnc nc)
{
  int i, cnt = b_il_GetCnt(wgl->cells);
  int cell_ref;
  gcell cell;
  printf("%2d: ", cnt);
  for( i = 0; i < cnt; i++ )
  {
    cell_ref = b_il_GetVal(wgl->cells, i);
    cell = gnc_GetGCELL(nc, cell_ref);
    printf("%s ", cell->name);
  }  
  printf("\n");
  return 1;
}


/*---------------------------------------------------------------------------*/

wiga_type wiga_Open()
{
  wiga_type wiga;
  wiga = (wiga_type)malloc(sizeof(struct _wiga_struct));
  if ( wiga != NULL )
  {
    wiga->type = WIGA_TYPE_AND;
    wiga->is_invert = 0;
    wiga->area = 0.0;
    wiga->levels = b_pl_Open();
    if ( wiga->levels != NULL )
    {
      return wiga;
    }
    free(wiga);
  }
  return NULL;
}

void wiga_Close(wiga_type wiga)
{
  int i, cnt = b_pl_GetCnt(wiga->levels);
  for( i = 0; i < cnt; i++ )
    wgl_Close(wiga_GetWGL(wiga, i));
  b_pl_Close(wiga->levels);
  wiga->levels = NULL;
  free(wiga);
}

/* returns position or -1 */
int wiga_AddWGL(wiga_type wiga, wgl_type wgl)
{
  int pos = b_pl_Add(wiga->levels, (void *)wgl);
  if ( pos >= 0 )
  {
    wiga->area += wgl->area;
    return pos;
  }
  return -1;
}

void wiga_Show(wiga_type wiga, gnc nc)
{
  int i, cnt = b_pl_GetCnt(wiga->levels);
  for( i = 0; i < cnt; i++ )
  {
    printf("%02d: ", i);
    wgl_Show(wiga_GetWGL(wiga, i), nc);
  }
  printf("invert: %d   area: %f\n", wiga->is_invert, wiga->area);
}

int wiga_cmp_fn(void *el, void *key)
{
  wiga_type wg = (wiga_type)el;
  int type = (*(int *)key)&3;
  int in_cnt = (*(int *)key)>>2;
  
  if ( wg->type < type )
    return -1;
  if ( wg->type > type )
    return 1;
  if ( wg->in_cnt < in_cnt )
    return -1;
  if ( wg->in_cnt > in_cnt )
    return 1;
  return 0;
}

/*---------------------------------------------------------------------------*/

wge_type wge_Open(gnc nc)
{
  wge_type wge;
  
  if ( gnc_AddEmptyCell(nc) < 0 )
    return NULL;
  
  if ( gnc_GetCellById(nc, GCELL_ID_INV) < 0 )
  {
    gnc_Error(nc, "Technology database: Inverter cell required.");
    return NULL;
  }
  
  wge = (wge_type)malloc(sizeof(struct _wge_struct));
  if ( wge != NULL )
  {
    wge->nc = nc;
    
    wge->and_cells  = NULL;
    wge->or_cells   = NULL;
    wge->nand_cells = NULL;
    wge->nor_cells  = NULL;

    wge->variation_list      = NULL;
    wge->variation_best_list = NULL;

    wge->wiga_cache = NULL;

    wge->out_net_refs = NULL;
    wge->in_net_refs  = NULL;
    wge->in2_net_refs = NULL;
    
    wge->is_and_or_logic = 0;
    wge->is_nand_nor_logic = 0;
    
    if ( gnc_GetCellById(nc, GCELL_ID_AND2) >= 0 &&
         gnc_GetCellById(nc, GCELL_ID_OR2) >= 0 &&
         gnc_GetCellById(nc, GCELL_ID_INV) >= 0 &&
         gnc_GetCellById(nc, GCELL_ID_EMPTY) >= 0 )
      wge->is_and_or_logic = 1;
    else
    {
      gnc_Log(nc, 4, "Technology database: AND-OR logic disabled.");
      if ( gnc_GetCellById(nc, GCELL_ID_AND2) < 0 )
        gnc_Log(nc, 4, "Technology database: Missing AND2.");
      if ( gnc_GetCellById(nc, GCELL_ID_OR2) < 0 )
        gnc_Log(nc, 4, "Technology database: Missing OR2.");
      
    }
      
    
    if ( gnc_GetCellById(nc, GCELL_ID_NAND2) >= 0 &&
         gnc_GetCellById(nc, GCELL_ID_NOR2) >= 0 &&
         gnc_GetCellById(nc, GCELL_ID_INV) >= 0 &&
         gnc_GetCellById(nc, GCELL_ID_EMPTY) >= 0 )
      wge->is_nand_nor_logic = 1;
    else
    {
      gnc_Log(nc, 4, "Technology database: NAND-NOR logic disabled.");
      if ( gnc_GetCellById(nc, GCELL_ID_NAND2) < 0 )
        gnc_Log(nc, 4, "Technology database: Missing NAND2.");
      if ( gnc_GetCellById(nc, GCELL_ID_NOR2) < 0 )
        gnc_Log(nc, 4, "Technology database: Missing NOR2.");
    }
    

    if ( wge->is_and_or_logic != 0 || wge->is_nand_nor_logic != 0 )
    {

      wge->and_cells  = gnc_GetCellListById(nc, GCELL_IDL_AND,  GCELL_ID_UNKNOWN);
      wge->or_cells   = gnc_GetCellListById(nc, GCELL_IDL_OR,   GCELL_ID_UNKNOWN);
      wge->nand_cells = gnc_GetCellListById(nc, GCELL_IDL_NAND, GCELL_ID_UNKNOWN);
      wge->nor_cells  = gnc_GetCellListById(nc, GCELL_IDL_NOR,  GCELL_ID_UNKNOWN);

      wge->and_empty_cells  = gnc_GetCellListById(nc, GCELL_IDL_AND,  GCELL_ID_EMPTY, GCELL_ID_UNKNOWN);
      wge->or_empty_cells   = gnc_GetCellListById(nc, GCELL_IDL_OR,   GCELL_ID_EMPTY, GCELL_ID_UNKNOWN);
      wge->nand_inv_cells   = gnc_GetCellListById(nc, GCELL_IDL_NAND, GCELL_ID_INV,   GCELL_ID_UNKNOWN);
      wge->nor_inv_cells    = gnc_GetCellListById(nc, GCELL_IDL_NOR,  GCELL_ID_INV,   GCELL_ID_UNKNOWN);

      wge->variation_list      = b_il_Open();
      wge->variation_best_list = b_il_Open();
      
      wge->wiga_cache = b_dic_Open();
      
      wge->out_net_refs = b_il_Open();
      wge->in_net_refs  = b_il_Open();
      wge->in2_net_refs = b_il_Open();

      if ( wge->and_cells != NULL && wge->or_cells != NULL &&
           wge->nand_cells != NULL && wge->nor_cells != NULL &&
           wge->and_empty_cells != NULL && wge->or_empty_cells != NULL &&
           wge->nand_inv_cells != NULL && wge->nor_inv_cells != NULL && 
           wge->variation_list != NULL && wge->variation_best_list != NULL &&
           wge->wiga_cache != NULL && wge->out_net_refs != NULL &&
           wge->in_net_refs != NULL && wge->in2_net_refs != NULL )
      {
        b_dic_SetCmpFn(wge->wiga_cache, wiga_cmp_fn);
        return wge;
      }
      if ( wge->and_cells != NULL ) b_il_Close(wge->and_cells);
      if ( wge->or_cells != NULL ) b_il_Close(wge->or_cells);
      if ( wge->nand_cells != NULL ) b_il_Close(wge->nand_cells);
      if ( wge->nor_cells != NULL ) b_il_Close(wge->nor_cells);

      if ( wge->and_empty_cells != NULL ) b_il_Close(wge->and_empty_cells);
      if ( wge->or_empty_cells != NULL ) b_il_Close(wge->or_empty_cells);
      if ( wge->nand_inv_cells != NULL ) b_il_Close(wge->nand_inv_cells);
      if ( wge->nor_inv_cells != NULL ) b_il_Close(wge->nor_inv_cells);

      if ( wge->variation_list != NULL ) b_il_Close(wge->variation_list);
      if ( wge->variation_best_list != NULL ) b_il_Close(wge->variation_best_list);

      if ( wge->wiga_cache != NULL ) b_dic_Close(wge->wiga_cache);

      if ( wge->out_net_refs != NULL ) b_il_Close(wge->out_net_refs);
      if ( wge->in_net_refs != NULL ) b_il_Close(wge->in_net_refs);
      if ( wge->in2_net_refs != NULL ) b_il_Close(wge->in2_net_refs);
      
      gnc_Error(nc, "Technology database: Out of Memory.");
    }
    else
    {
      gnc_Error(nc, "Technology database: No suitable logic synthesis possible.");
    }
    free(wge);
  }
  else
  {
    gnc_Error(nc, "Technology database: Out of Memory.");
  }
  return NULL;
}

void wge_Close(wge_type wge)
{
  int i;
  
  for( i = 0; i < b_dic_GetCnt(wge->wiga_cache); i++ )
    wiga_Close((wiga_type)b_dic_GetVal(wge->wiga_cache, i));
  
  if ( wge->and_cells  != NULL )      b_il_Close(wge->and_cells);
  if ( wge->or_cells   != NULL )      b_il_Close(wge->or_cells);
  if ( wge->nand_cells != NULL )      b_il_Close(wge->nand_cells);
  if ( wge->nor_cells  != NULL )      b_il_Close(wge->nor_cells);

  if ( wge->and_empty_cells != NULL ) b_il_Close(wge->and_empty_cells);
  if ( wge->or_empty_cells  != NULL ) b_il_Close(wge->or_empty_cells);
  if ( wge->nand_inv_cells  != NULL ) b_il_Close(wge->nand_inv_cells);
  if ( wge->nor_inv_cells   != NULL ) b_il_Close(wge->nor_inv_cells);

  if ( wge->variation_list != NULL ) b_il_Close(wge->variation_list);
  if ( wge->variation_best_list != NULL ) b_il_Close(wge->variation_best_list);

  if ( wge->wiga_cache != NULL ) b_dic_Close(wge->wiga_cache);

  if ( wge->out_net_refs != NULL ) b_il_Close(wge->out_net_refs);
  if ( wge->in_net_refs  != NULL ) b_il_Close(wge->in_net_refs);
  if ( wge->in2_net_refs != NULL ) b_il_Close(wge->in2_net_refs);

  wge->and_cells  = NULL;
  wge->or_cells   = NULL;
  wge->nand_cells = NULL;
  wge->nor_cells  = NULL;

  wge->and_empty_cells = NULL;
  wge->or_empty_cells  = NULL;
  wge->nand_inv_cells  = NULL;
  wge->nor_inv_cells   = NULL;
  
  wge->variation_list = NULL;
  wge->variation_best_list = NULL;
  
  wge->wiga_cache = NULL;
  
  wge->out_net_refs = NULL;
  wge->in_net_refs  = NULL;
  wge->in2_net_refs = NULL;
  
  free(wge);
}

/*-- resource (cell) allocation algorithm -----------------------------------*/

int wge_GetVariationInCnt(wge_type wge, b_il_type cells)
{
  int i, cnt = b_il_GetCnt(wge->variation_list);
  int cell_ref;
  int in_cnt = 0;
  for( i = 0; i < cnt; i++ )
  {
    cell_ref = b_il_GetVal(cells, b_il_GetVal(wge->variation_list, i));
    in_cnt += gnc_GetCellInCnt(wge->nc, cell_ref);
  }
  return in_cnt;
}


int wge_AddToVariation(wge_type wge)
{
  int pos;
  pos = b_il_Add(wge->variation_list, 0);
  if ( pos < 0 )
  {
    gnc_Error(wge->nc, "wge_AppendToVariation: Out of Memory (b_il_Add)");
    return 0;
  }
  return 1;  
}

int wge_IncVariation(wge_type wge, b_il_type cells)
{
  int last_cell_idx;
  int last_pos = b_il_GetCnt(wge->variation_list)-1;
  last_cell_idx = b_il_GetVal(wge->variation_list, last_pos);
  if ( last_cell_idx+1 >= b_il_GetCnt(cells) )
    return 0;
  b_il_SetVal(wge->variation_list, last_pos, last_cell_idx+1);
  return 1;
}

int wge_DelFromVariation(wge_type wge)
{
  int last_pos = b_il_GetCnt(wge->variation_list)-1;
  if ( last_pos < 0 )
    return 0;
  b_il_DelByPos(wge->variation_list, last_pos);
  return 1;
}

double wge_RateVariation(wge_type wge, b_il_type cells)
{
  int i, cnt = b_il_GetCnt(wge->variation_list);
  int cell_ref;
  gcell cell;

  wge->rate_min_in_cnt = 30000;
  wge->rate_max_in_cnt = 0;
  wge->rate_area = 0.0;

  for( i = 0; i < cnt; i++ )
  {
    cell_ref = b_il_GetVal(cells, b_il_GetVal(wge->variation_list, i));
    cell = gnc_GetGCELL(wge->nc, cell_ref);
    wge->rate_area += cell->area;
    
    if ( wge->rate_min_in_cnt > cell->in_cnt )
      wge->rate_min_in_cnt = cell->in_cnt;
    if ( wge->rate_max_in_cnt < cell->in_cnt )
      wge->rate_max_in_cnt = cell->in_cnt;
  }
  
  if ( wge->nc->syparam.gate_expansion == SYPARAM_GATE_EXPANSION_BALANCED )
    return (double)(wge->rate_max_in_cnt-wge->rate_min_in_cnt);
  return wge->rate_area;
}

int wge_ShowVariation(wge_type wge, b_il_type cells)
{
  int i, cnt = b_il_GetCnt(wge->variation_list);
  int cell_ref;
  gcell cell;
  printf("%2d: ", cnt);
  for( i = 0; i < cnt; i++ )
  {
    cell_ref = b_il_GetVal(cells, b_il_GetVal(wge->variation_list, i));
    cell = gnc_GetGCELL(wge->nc, cell_ref);
    printf("%s ", cell->name);
  }  
  printf("\n");
  return 1;
}


int wge_CopyVariation(wge_type wge)
{
  return b_il_Copy(wge->variation_best_list, wge->variation_list);
}

int wge_DoVariationSub(wge_type wge, b_il_type cells)
{
  int in_cnt;
  in_cnt = wge_GetVariationInCnt(wge, cells);
  if ( in_cnt > wge->variation_in_cnt )
    return 1;
    
  if ( in_cnt == wge->variation_in_cnt )
  {
    if ( wge->is_best == 0 || b_il_GetCnt(wge->variation_best_list) >= b_il_GetCnt(wge->variation_list) )
    {
      double rate;
      if ( b_il_GetCnt(wge->variation_best_list) > b_il_GetCnt(wge->variation_list) )
        wge->is_best = 0;
        
      rate = wge_RateVariation(wge, cells);
      if ( wge->is_best == 0 )
      {
        wge->is_best = 1;
        wge->rate_best = rate;
        if ( wge_CopyVariation(wge) == 0 )
          return 0;
      }
      else
      {
        if ( wge->rate_best > rate )
        {
          wge->rate_best = rate;
          if ( wge_CopyVariation(wge) == 0 )
            return 0;
        }
      }
    }
  }
  else 
  {
    if ( wge->is_best == 0 || b_il_GetCnt(wge->variation_best_list) > b_il_GetCnt(wge->variation_list) )
    {
      if ( wge_AddToVariation(wge) == 0 )
        return 0;

      for(;;)
      {
        if ( wge_DoVariationSub(wge, cells) == 0 )
          return 0;
        if ( wge_IncVariation(wge, cells) == 0 )
          break;
      }
      wge_DelFromVariation(wge);
    }
  }
  return 1;
}

int wge_FindMaxInCntPos(wge_type wge, b_il_type cells, int exclude)
{
  int i, cnt = b_il_GetCnt(cells);
  int pos = -1;
  int max_in_cnt = 0;
  int cell_ref;
  gcell cell;
  for( i = 0; i < cnt; i++ )
  {
    if ( i != exclude )
    {
      cell_ref = b_il_GetVal(cells, i);
      cell = gnc_GetGCELL(wge->nc, cell_ref);
      if ( max_in_cnt < cell->in_cnt )
      {
        max_in_cnt = cell->in_cnt;
        pos = i;
      }
    }
  }
  return pos;
}

int wge_PrepareVariation(wge_type wge, int in_cnt, b_il_type cells)
{
  int p1, p2;
  int i1, i2;
  int c, m;
  int i;
  p1 = wge_FindMaxInCntPos(wge, cells, -1);
  p2 = wge_FindMaxInCntPos(wge, cells, p1);
  if ( p1 < 0 && p2 < 0 )
    return 0; 
  i1 = gnc_GetGCELL(wge->nc,  b_il_GetVal(cells, p1))->in_cnt;
  i2 = gnc_GetGCELL(wge->nc,  b_il_GetVal(cells, p2))->in_cnt;
  c = (in_cnt+i1-1)/i1;
  m = (in_cnt - i2*c)/(i1-i2);

  for( i = 0; i < m; i++ )
  {
    if ( b_il_Add(wge->variation_list, p1) < 0 )
    {
      gnc_Error(wge->nc, "wge_PrepareVariation: Out of Memory (b_il_Add).");
      return 0;
    }
  }

  return 1;
}


/* the central resource allocation algorithm */

int wge_DoVariation(wge_type wge, int in_cnt, b_il_type cells)
{
  wge->variation_in_cnt = in_cnt;
  wge->is_best = 0;
  b_il_Clear(wge->variation_best_list);
  b_il_Clear(wge->variation_list);
  
  /* not really required, but speeds up the search process a lot */
  if ( wge_PrepareVariation(wge, in_cnt, cells) == 0 )
  {
    gnc_Error(wge->nc, "wge_DoVariation: wge_PrepareVariation.");
    return 0;
  }
    
  /* find the optimal solution */
  return wge_DoVariationSub(wge, cells);
}

/*-- specific gate creation -------------------------------------------------*/

int wge_CopyBestVariationToLevel(wge_type wge, b_il_type cells, wgl_type wgl)
{
  int i, cnt = b_il_GetCnt(wge->variation_best_list);
  int cell_ref;
  assert(wge->is_best != 0);
  b_il_Clear(wgl->cells);
  for( i = 0; i < cnt; i++ )
  {
    cell_ref = b_il_GetVal(cells, b_il_GetVal(wge->variation_best_list, i));
    if ( b_il_Add(wgl->cells, cell_ref) < 0 )
    {
      gnc_Error(wge->nc, "wge_CopyBestVariationToLevel: Out of Memory (b_il_Add).");
      return 0;
    }
  }

  if ( b_il_Copy(wge->variation_list, wge->variation_best_list) == 0 )
  {
    gnc_Error(wge->nc, "wge_CopyBestVariationToLevel: Out of Memory (b_il_Copy).");
    return 0;
  }
  
  wge_RateVariation(wge, cells);
  
  wgl->in_cnt = wge->variation_in_cnt;
  wgl->area = wge->rate_area;
  
  return 1;
}

int wge_CopyLevel(wge_type wge, int in_cnt, b_il_type cells, wgl_type wgl)
{
  if ( wge_DoVariation(wge, in_cnt, cells) == 0 )
    return 0;
  assert(wge->is_best != 0);
  if ( wge_CopyBestVariationToLevel( wge, cells, wgl ) == 0 )
    return 0;
  return 1;
}

wgl_type wge_CreateLevel(wge_type wge, int in_cnt, b_il_type cells)
{
  wgl_type wgl;
  wgl = wgl_Open();
  if ( wgl != NULL )
  {
    if ( wge_CopyLevel(wge, in_cnt, cells, wgl) != 0 )
    {
      return wgl;
    }
    wgl_Close(wgl);
  }
  else
  {
    gnc_Error(wge->nc, "wge_CreateLevel: Out of Memory (wgl_Open).");
    return NULL;
  }
  return NULL;
}

/* implements AND and OR gates */
int wge_AddPureGate(wge_type wge, int in_cnt, b_il_type cells, wiga_type wiga)
{
  wgl_type wgl;
  for(;;)
  {
    if ( in_cnt <= 1 )
      break;
    wgl = wge_CreateLevel(wge, in_cnt, cells);
    if ( wgl == NULL )
      return 0;
    if ( wiga_AddWGL(wiga, wgl) < 0 )
    {
      gnc_Error(wge->nc, "wge_AddPureGate: Out of Memory (wiga_AddWGL).");
      return 0;
    }
    in_cnt = wgl_GetCellCnt(wgl);    
  }
  return 1;
}

/* implements NAND and NOR gates */
int wge_AddNegGate(wge_type wge, int in_cnt, b_il_type cells1, b_il_type cells2, wiga_type wiga)
{
  wgl_type wgl;
  int level_cnt;
  b_il_type cells;
  level_cnt = 0;
  for(;;)
  {
    if ( in_cnt <= 1 )
      break;
    if ( (level_cnt&1) == 0 )
      cells = cells1;
    else
      cells = cells2;
    
    wgl = wge_CreateLevel(wge, in_cnt, cells);
    if ( wgl == NULL )
      return 0;
      
    if ( wiga_AddWGL(wiga, wgl) < 0 )
    {
      gnc_Error(wge->nc, "wge_AddNotGate: Out of Memory (wiga_AddWGL).");
      return 0;
    }
    in_cnt = wgl_GetCellCnt(wgl);
    level_cnt++;
  }
  return 1;
}


wiga_type wge_CreatePureGate(wge_type wge, int in_cnt, b_il_type cells)
{
  wiga_type wiga;
  wiga = wiga_Open();
  if ( wiga != NULL )
  {
    if ( wge_AddPureGate(wge, in_cnt, cells, wiga) != 0 )
    {
      wiga->is_invert = 0;
      return wiga;
    }
    wiga_Close(wiga);
  }
  else
  {
    gnc_Error(wge->nc, "wge_CreatePureGate: Out of Memory (wiga_Open).");
    return 0;
  }
  return NULL;
}

wiga_type wge_CreateNegGate(wge_type wge, int in_cnt, b_il_type cells1, b_il_type cells2)
{
  wiga_type wiga;
  wiga = wiga_Open();
  if ( wiga != NULL )
  {
    if ( wge_AddNegGate(wge, in_cnt, cells1, cells2, wiga) != 0 )
    {
      if ( (b_pl_GetCnt(wiga->levels)&1) == 0 )
        wiga->is_invert = 0;
      else
        wiga->is_invert = 1;
      return wiga;
    }
    wiga_Close(wiga);
  }
  else
  {
    gnc_Error(wge->nc, "wge_CreateNGate: Out of Memory (wiga_Open).");
    return 0;
  }
  return NULL;
}


static wiga_type wge_create_gate(wge_type wge, int in_cnt, b_il_type c1, b_il_type c2a, b_il_type c2b)
{
  wiga_type wg, wg1, wg2;
  wg1 = NULL;
  wg2 = NULL;
  if ( wge->is_and_or_logic != 0 )
    wg1 = wge_CreatePureGate(wge, in_cnt, c1);
  if ( wge->is_nand_nor_logic != 0 )
    wg2 = wge_CreateNegGate(wge, in_cnt, c2a, c2b);
  if ( wg1 == NULL && wg2 == NULL )
    return NULL;
  if ( wg1 != NULL && wg2 != NULL )
  {
    if ( wg1->area > wg2->area )
      wg = wg2;
    else
      wg = wg1;
  }
  else if ( wg1 == NULL )
    wg = wg2;
  else if ( wg2 == NULL )
    wg = wg1;
  
  assert( wg != NULL );
    
  if ( wg == wg1 && wg2 != NULL )
    wiga_Close(wg2);
  if ( wg == wg2 && wg1 != NULL )
    wiga_Close(wg1);
    
  wg->in_cnt = in_cnt;
  
  return wg;
}

wiga_type wge_CreateAndGate(wge_type wge, int in_cnt)
{
  wiga_type wg;
  wg = wge_create_gate(wge, in_cnt, wge->and_empty_cells, wge->nand_inv_cells, wge->nor_inv_cells);
  if ( wg != NULL )
  {
    wg->type = WIGA_TYPE_AND;
    return wg;
  }
  return NULL;
}

wiga_type wge_CreateOrGate(wge_type wge, int in_cnt)
{
  wiga_type wg;
  wg = wge_create_gate(wge, in_cnt, wge->or_empty_cells, wge->nor_inv_cells, wge->nand_inv_cells);
  if ( wg != NULL )
  {
    wg->type = WIGA_TYPE_OR;
    return wg;
  }
  return NULL;
}

wiga_type wge_CreateGate(wge_type wge, int in_cnt, int type)
{
  switch(type)
  {
    case WIGA_TYPE_AND:
      return wge_CreateAndGate(wge, in_cnt);
    case WIGA_TYPE_OR:
      return wge_CreateOrGate(wge, in_cnt);
  }
  return NULL;
}

/*-- cache ------------------------------------------------------------------*/

static int wge_get_cache_key(wge_type wge, int in_cnt, int type)
{
  return (in_cnt<<2) + type;
}


wiga_type wge_FindGate(wge_type wge, int in_cnt, int type)
{
  int key = wge_get_cache_key(wge, in_cnt, type);
  return (wiga_type)b_dic_Find(wge->wiga_cache, &key);
}


int wge_AddGate(wge_type wge, wiga_type wiga)
{
  int key = wge_get_cache_key(wge, wiga->in_cnt, wiga->type);
  if ( b_dic_Ins(wge->wiga_cache, (void *)wiga, &key) == 0 )
  {
    gnc_Error(wge->nc, "wge_AddGate: Out of Memory (b_dic_Ins).");
    return 0;
  }
  return 1;
}

wiga_type wge_GetGate(wge_type wge, int in_cnt, int type)
{
  wiga_type wiga;
  wiga = wge_FindGate(wge, in_cnt, type);
  if ( wiga != NULL )
    return wiga;
  wiga = wge_CreateGate(wge, in_cnt, type);
  if ( wiga != NULL )
  {
    assert(wiga->in_cnt == in_cnt);
    assert(wiga->type == type);
    if ( wge_AddGate(wge, wiga) != 0 )
    {
      return wiga;
    }
    wiga_Close(wiga);
  }
  return NULL;
}

/*-- synthesis --------------------------------------------------------------*/

int wge_SynthWGL(wge_type wge, int cell_ref, wgl_type wgl, b_il_type in_net_refs, b_il_type out_net_refs)
{
  b_il_type net_il;
  int processed_ports;
  int current_ports;
  int digital_cell_ref;
  int out_net_ref;
  int i;

  net_il = b_il_Open();
  if ( net_il == NULL )
  {
    gnc_Error(wge->nc, "wge_SynthWGL: Out of Memory (b_il_Open).");
    return 0;
  }
  
  b_il_Clear(out_net_refs);
  processed_ports = 0;
  for( i = 0; i < wgl_GetCellCnt(wgl); i++ )
  {
    digital_cell_ref = b_il_GetVal(wgl->cells, i);
    current_ports = gnc_GetGCELL(wge->nc, digital_cell_ref)->in_cnt;
    
    if ( b_il_CopyRange(net_il, in_net_refs, processed_ports, current_ports) == 0 )
    {
      gnc_Error(wge->nc, "wge_SynthWGL: Out of Memory (b_il_CopyRange).");
      b_il_Close(net_il);
      return 0;
    }
    
    out_net_ref = gnc_SynthDigitalCell(wge->nc, cell_ref, digital_cell_ref, net_il, -1);
    if ( out_net_ref < 0 )
      return b_il_Close(net_il), 0;
      
    if ( b_il_Add(out_net_refs, out_net_ref) < 0 ) 
    {
      gnc_Error(wge->nc, "wge_SynthWGL: Out of Memory (b_il_Add).");
      b_il_Close(net_il);
      return 0;
    }
    
    processed_ports += current_ports;
    assert(processed_ports <= b_il_GetCnt(in_net_refs));
  }
  
  /* this loop should never be executed */
  
  for( i = processed_ports; i < b_il_GetCnt(in_net_refs); i++ )
  {
    if ( b_il_Add(out_net_refs, b_il_GetVal(in_net_refs, i)) < 0 ) 
    {
      gnc_Error(wge->nc, "wge_SynthWGL: Out of Memory (b_il_Add).");
      b_il_Close(net_il);
      return 0;
    }
  }
  b_il_Close(net_il);
  return 1;
}


int wge_SynthWIGA(wge_type wge, int cell_ref, wiga_type wiga, b_il_type in_net_refs, int *p_out, int *n_out)
{
  int i, cnt = b_pl_GetCnt(wiga->levels);
  wgl_type wgl;
  int out_net_ref;
  int *out_ptr = NULL;
  
  b_il_Clear(wge->out_net_refs);
  if ( b_il_Copy(wge->in_net_refs, in_net_refs) == 0 )
  {
    gnc_Error(wge->nc, "wge_SynthWIGA: Out of Memory (b_il_Copy).");
    return 0;
  }
  
  for( i = 0; i < cnt; i++ )
  {
    wgl = wiga_GetWGL(wiga, i);
    if ( wge_SynthWGL(wge, cell_ref, wgl, wge->in_net_refs, wge->out_net_refs) == 0 )
      return 0;
    if ( b_il_Copy(wge->in_net_refs, wge->out_net_refs) == 0 )
    {
      gnc_Error(wge->nc, "wge_SynthWIGA: Out of Memory (b_il_Copy).");
      return 0;
    }
  }
  
  assert(b_il_GetCnt(wge->out_net_refs) == 1);
  out_net_ref = b_il_GetVal(wge->out_net_refs, 0);
  
  if ( wiga->is_invert == 0 )
  {
    if ( p_out != NULL )
      *p_out = out_net_ref;
    out_ptr = n_out;
  }
  else
  {
    if ( n_out != NULL )
      *n_out = out_net_ref;
    out_ptr = p_out;
  }
  
  if ( out_ptr != NULL )
  {
    int inv_cell_ref = gnc_GetCellById(wge->nc, GCELL_ID_INV);
    assert(inv_cell_ref >= 0);
    b_il_Clear(wge->in_net_refs);
    if ( b_il_Add(wge->in_net_refs, out_net_ref) < 0 )
    {
      gnc_Error(wge->nc, "wge_SynthWIGA: Out of Memory (b_il_Add).");
      return 0;
    }

    out_net_ref = gnc_SynthDigitalCell(wge->nc, cell_ref, 
                                      inv_cell_ref, wge->in_net_refs, -1);
    if ( out_net_ref < 0 )
      return 0;
    *out_ptr = out_net_ref;
  }
  
  return 1;
}


int wge_SynthGate(wge_type wge, int cell_ref, int type, b_il_type in_net_refs, int *p_out, int *n_out)
{
  wiga_type wiga;

  if ( b_il_GetCnt(in_net_refs) == 0 )
    return 0;

  if ( b_il_GetCnt(in_net_refs) > 1 )
  {
    wiga = wge_GetGate(wge, b_il_GetCnt(in_net_refs), type);
    if ( wiga == NULL )
      return 0;
    return wge_SynthWIGA(wge, cell_ref, wiga, in_net_refs, p_out, n_out);
  }
  
  if ( p_out != NULL )
    *p_out = b_il_GetVal(in_net_refs, 0);
    
  if ( n_out != NULL )
  {
    *n_out = gnc_SynthDigitalCell(wge->nc, cell_ref, 
        gnc_GetCellById(wge->nc, GCELL_ID_INV), in_net_refs, -1);
    if ( *n_out < 0 )
      return 0;
  }
  
  return 1;
  
}

/*---------------------------------------------------------------------------*/

int gnc_SynthGate(gnc nc, int cell_ref, int gate, b_il_type in_net_refs, int *p_out, int *n_out)
{
  if ( nc->wge == NULL )
    return 0;
  if ( gnc_GetCellById(nc, GCELL_ID_EMPTY) < 0 )
    return 0;
  return wge_SynthGate(nc->wge, cell_ref, gate, in_net_refs, p_out, n_out);
}


/*---------------------------------------------------------------------------*/

int wge_IsAND(wge_type wge, int id)
{
  if ( b_il_Find(wge->and_cells, id) >= 0 )
    return 1;
  return 0;
}

int wge_IsNAND(wge_type wge, int id)
{
  if ( b_il_Find(wge->nand_cells, id) >= 0 )
    return 1;
  return 0;
}

int wge_IsOR(wge_type wge, int id)
{
  if ( b_il_Find(wge->or_cells, id) >= 0 )
    return 1;
  return 0;
}

int wge_IsNOR(wge_type wge, int id)
{
  if ( b_il_Find(wge->nor_cells, id) >= 0 )
    return 1;
  return 0;
}

