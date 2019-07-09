/*

  neca.c
  
  Net Cache 

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

#include <stdlib.h>
#include <assert.h>
#include "neca.h"
#include "gnet.h"
#include "mwc.h"


/* net cache element */
struct _nce_struct
{
  b_il_type net_list;
  int out_net_ref;
  unsigned int is_subset_cnt;
  unsigned int has_subset_cnt;
  int node_ref;
};
typedef struct _nce_struct nce_struct;


/*---------------------------------------------------------------------------*/

static nce_struct *nce_Open()
{
  nce_struct *nce;
  nce = (nce_struct *)malloc(sizeof(nce_struct));
  if ( nce != NULL )
  {
    nce->out_net_ref = -1;
    nce->is_subset_cnt = 0;
    nce->has_subset_cnt = 0;
    nce->node_ref = -1;
    nce->net_list = b_il_Open();
    if ( nce->net_list != NULL )
    {
      return nce;
    }
    free(nce);
  }
  return NULL;
}

static void nce_Close(nce_struct *nce)
{
  b_il_Close(nce->net_list);
  free(nce);
}

/*---------------------------------------------------------------------------*/

static int neca_compare_fn(void *_el, void *_key)
{
  nce_struct *el = (nce_struct *)_el;
  b_il_type key = (b_il_type)_key;
  return b_il_CmpEl(el->net_list, key);
}

neca_type neca_Open()
{
  neca_type neca;
  neca = (neca_type)malloc(sizeof(struct _neca_struct));
  if ( neca != NULL )
  {
    neca->net_cache = b_dic_Open();
    if ( neca->net_cache != NULL )
    {
      b_dic_SetCmpFn(neca->net_cache, neca_compare_fn);
      return neca;
    }
    free(neca);
  }
  return NULL;
}

static nce_struct *neca_GetNCE(neca_type neca, int ref)
{
  return (nce_struct *)b_dic_GetVal(neca->net_cache, ref);
}

void neca_Clear(neca_type neca)
{
  int i, cnt = b_dic_GetCnt(neca->net_cache);
  for( i = 0; i < cnt; i++ )
    nce_Close(neca_GetNCE(neca, i));
  b_dic_Clear(neca->net_cache);
}

void neca_Close(neca_type neca)
{
  neca_Clear(neca);
  b_dic_Close(neca->net_cache);
  free(neca);
}

static nce_struct *neca_FindNCE(neca_type neca, b_il_type net_list)
{
  return b_dic_Find(neca->net_cache, net_list);
}

/* returns 0 for error */
static int neca_AddNCE(neca_type neca, nce_struct *nce)
{
  return b_dic_Ins(neca->net_cache, nce, nce->net_list);
}

/* returns 0 for error */
int neca_Ins(neca_type neca, b_il_type net_list, int out_net_ref)
{
  nce_struct *nce;
  b_il_Sort(net_list);
  nce = nce_Open();
  if ( nce != NULL )
  {
    if ( b_il_Copy(nce->net_list, net_list) != 0 )
    {
      nce->out_net_ref = out_net_ref;
      if ( neca_AddNCE(neca, nce) != 0 )
      {
        return 1;
      }
    }
    nce_Close(nce);
  }
  return 0;
}

/* returns out_net_ref or -1 */
int neca_Get(neca_type neca, b_il_type net_list)
{
  nce_struct *nce;
  b_il_Sort(net_list);
  nce = neca_FindNCE(neca, net_list);
  if ( nce == NULL )
    return -1;
  return nce->out_net_ref;
}

void neca_Show(neca_type neca)
{
  int i, cnt = b_dic_GetCnt(neca->net_cache);
  int j;
  nce_struct *nce;
  for( i = 0; i < cnt; i++ )
  {
    nce = neca_GetNCE(neca, i);
    printf("%04d/%04d %c %c%d ", i, cnt, nce->is_subset_cnt==0?' ':'s', nce->has_subset_cnt==0?' ':'h', nce->has_subset_cnt);
    for( j = 0; j < b_il_GetCnt(nce->net_list); j++ )
      printf("%d ", b_il_GetVal(nce->net_list, j));
    printf("\n");
  }
}

void neca_CheckSubSet(neca_type neca)
{
  int i, j, cnt = b_dic_GetCnt(neca->net_cache);
  nce_struct *nce_i;
  nce_struct *nce_j;
  for( i = 0; i < cnt; i++ )
  {
    nce_i = neca_GetNCE(neca, i);
    for( j = i+1; j < cnt; j++ )
    { 
      nce_j = neca_GetNCE(neca, j);
      if ( b_il_IsSubSetEl(nce_i->net_list, nce_j->net_list) != 0 )
      {
        nce_i->has_subset_cnt++;
        nce_j->is_subset_cnt++;
      }
      else if ( b_il_IsSubSetEl(nce_j->net_list, nce_i->net_list) != 0 )
      {
        nce_j->has_subset_cnt++;
        nce_i->is_subset_cnt++;
      }
    }
  }
}


int neca_FindFirstSubSet(neca_type neca, int pos)
{
  int i, cnt = b_dic_GetCnt(neca->net_cache);
  nce_struct *nce_pos;
  nce_struct *nce_i;
  nce_pos = neca_GetNCE(neca, pos);
  for( i = 0; i < cnt; i++ )
  {
    if ( i != pos )
    {
      nce_i = neca_GetNCE(neca, i);
      if ( b_il_IsSubSetEl(nce_pos->net_list, nce_i->net_list) != 0 )
        return i;
    }
  }
  return -1;
}

int neca_Optimize(neca_type neca, gnc nc, int cell_ref)
{
  int i, cnt = b_dic_GetCnt(neca->net_cache);
  int subsetpos;
  int dest_node_ref;
  int sub_node_ref;
  nce_struct *nce_i;
  neca_CheckSubSet(neca);
  for( i = 0; i < cnt; i++ )
  {
    nce_i = neca_GetNCE(neca, i);
    if ( nce_i->has_subset_cnt >= 1 )
    {
      subsetpos = neca_FindFirstSubSet(neca, i);
      assert(subsetpos >= 0);
      if ( neca_GetNCE(neca, subsetpos)->has_subset_cnt == 0 )
      {
        dest_node_ref = gnc_FindCellNetNodeByPortType(nc, cell_ref, nce_i->out_net_ref, GPORT_TYPE_OUT);
        sub_node_ref = gnc_FindCellNetNodeByPortType(nc, cell_ref, neca_GetNCE(neca, subsetpos)->out_net_ref, GPORT_TYPE_OUT);
        if ( gnc_ReplaceInputs(nc, cell_ref, dest_node_ref, sub_node_ref) == 0 )
          return 0;
      }
    }
  }
  return 1;
}

