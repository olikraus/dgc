/*

  b_dg.c

  directed graph
  
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
#include <limits.h>
#include "b_dg.h"
#include "mwc.h"



#define NODE(dg,ref) ((b_dg_node_type)b_set_Get((dg)->nodes, (ref)))
#define EDGE(dg,ref) ((b_dg_edge_type)b_set_Get((dg)->edges, (ref)))
#define LAYER(dg,ref) ((b_dg_layer_type)b_pl_GetVal((dg)->layers, (ref)))

#define b_dg_LoopNodes(dg, pos) b_set_WhileLoop((dg)->nodes, (pos))
#define b_dg_InvLoopNodes(dg, pos) b_set_InvWhileLoop((dg)->nodes, (pos))
#define b_dg_LoopEdges(dg, pos) b_set_WhileLoop((dg)->edges, (pos))

b_dg_node_type b_dg_node_Open()
{
  b_dg_node_type n;
  n = (b_dg_node_type)malloc(sizeof(struct _b_dg_node_struct));
  if ( n != NULL )
  {
    n->out_edge_refs = b_il_Open();
    if ( n->out_edge_refs != NULL )
    {
      n->color = 0;
      n->predecessor_node_ref = -1;
      n->finish[0] = 0.0;
      n->finish[1] = 0.0;
      n->discover = 0.0;
      n->user_val = -1;
      n->layer = -1;
      n->is_internal_node = 0;
      return n;
    }
    free(n);
  }
  return NULL;
}

void b_dg_node_Close(b_dg_node_type n)
{
  b_il_Close(n->out_edge_refs);
  free(n);
}

b_dg_edge_type b_dg_edge_Open()
{
  b_dg_edge_type e;
  e = (b_dg_edge_type)malloc(sizeof(struct _b_dg_edge_struct));
  if ( e != NULL )
  {
    e->src_node_ref = -1;
    e->dest_node_ref = -1;
    e->user_val = -1;
    return e;
  }
  return NULL;
}

void b_dg_edge_Close(b_dg_edge_type e)
{
  free(e);
}


b_dg_layer_type b_dg_layer_Open()
{
  b_dg_layer_type l;
  l = (b_dg_layer_type)malloc(sizeof(struct _b_dg_layer_struct));
  if ( l != NULL )
  {
    l->node_refs = b_il_Open();
    if ( l->node_refs != NULL )
    {
      return l;
    }
    free(l);
  }
  return NULL;
}

void b_dg_layer_Close(b_dg_layer_type l)
{
  b_il_Close(l->node_refs);
  free(l);
}


b_dg_type b_dg_Open()
{
  b_dg_type dg;
  dg = (b_dg_type)malloc(sizeof(struct _b_dg_struct));
  if ( dg != NULL )
  {
    dg->nodes = b_set_Open();
    if ( dg->nodes != NULL )
    {
      dg->edges = b_set_Open();
      if ( dg->edges != NULL )
      {
        dg->layers = b_pl_Open();
        if ( dg->layers != NULL )
        {
          dg->is_acyclic = 0;
          dg->layer_cnt = 0;
          return dg;
        }
        b_set_Close(dg->edges);
      }
      b_set_Close(dg->nodes);
    }
    free(dg);
  }
  return NULL;
}

void b_dg_Clear(b_dg_type dg)
{
  int ref;
  
  dg->layer_cnt = 0;
  
  ref = -1;
  while( b_dg_LoopNodes(dg, &ref) != 0 )
    b_dg_node_Close(NODE(dg, ref));

  ref = -1;
  while( b_dg_LoopEdges(dg, &ref) != 0 )
    b_dg_edge_Close(EDGE(dg, ref));

  b_set_Clear(dg->nodes);
  b_set_Clear(dg->edges);

  for( ref = 0; ref < b_pl_GetCnt(dg->layers); ref++ )
    b_dg_layer_Close(LAYER(dg,ref));
  b_pl_Clear(dg->layers);
}

void b_dg_Close(b_dg_type dg)
{
  b_dg_Clear(dg);

  b_set_Close(dg->nodes);
  b_set_Close(dg->edges);
  b_pl_Close(dg->layers);
  
  free(dg);
}

/* returns node_ref or -1 */
int b_dg_AddNode(b_dg_type dg, int user_val)
{
  b_dg_node_type n;
  n = b_dg_node_Open();
  if ( n != NULL )
  {
    int ref = b_set_Add(dg->nodes, n);
    n->user_val = user_val;
    if ( ref >= 0 )
      return ref;
    b_dg_node_Close(n);
  }
  return -1;
}

/* returns edge_ref or -1 */
int b_dg_AddEmptyEdge(b_dg_type dg)
{
  b_dg_edge_type e;
  e = b_dg_edge_Open();
  if ( e != NULL )
  {
    int ref = b_set_Add(dg->edges, e);
    if ( ref >= 0 )
      return ref;
    b_dg_edge_Close(e);
  }
  return -1;
}

/* returns edge_ref or -1 */
static int b_dg_find_edge(b_dg_type dg, int src_node_ref, int dest_node_ref)
{
  b_il_type il;
  int pos, edge_ref;
  assert(src_node_ref >= 0);
  assert(dest_node_ref >= 0);
    
  il = NODE(dg, src_node_ref)->out_edge_refs; 
  pos = -1; edge_ref = -1;
  while( b_il_Loop(il , &pos, &edge_ref) != 0 )
    if ( EDGE(dg, edge_ref)->dest_node_ref == dest_node_ref )
      return edge_ref;
      
  return -1;
}

/* assumes an unconnected and valid edge, returns 0 or 1 */
static int b_dg_connect_edge(b_dg_type dg, int edge_ref, int src_node_ref, int dest_node_ref, double w, int user_val)
{
  assert(src_node_ref >= 0);
  assert(dest_node_ref >= 0);
  assert(edge_ref >= 0);

  if ( b_il_Add(NODE(dg, src_node_ref)->out_edge_refs, edge_ref) >= 0 )
  {
    EDGE(dg, edge_ref)->src_node_ref = src_node_ref;
    EDGE(dg, edge_ref)->dest_node_ref = dest_node_ref;
    EDGE(dg, edge_ref)->user_val = user_val;
    EDGE(dg, edge_ref)->w = w;
    return 1;
  }
  
  return 0;
}

static void b_dg_disconnect_edge(b_dg_type dg, int edge_ref)
{
  b_dg_edge_type e = EDGE(dg, edge_ref);
  
  if ( e->src_node_ref >= 0 )
  {
    b_il_DelByVal(NODE(dg, e->src_node_ref)->out_edge_refs, edge_ref);
    e->src_node_ref = -1;
  }

  e->dest_node_ref = -1;
}

int b_dg_ConnectEdge(b_dg_type dg, int edge_ref, int src_node_ref, int dest_node_ref, double w, int user_val)
{
  b_dg_disconnect_edge(dg, edge_ref);
  return b_dg_connect_edge(dg, edge_ref, src_node_ref, dest_node_ref, w, user_val);
}

void b_dg_DeleteEdge(b_dg_type dg, int edge_ref)
{
  b_dg_disconnect_edge(dg, edge_ref);
  b_dg_edge_Close(EDGE(dg, edge_ref));
  b_set_Del(dg->edges, edge_ref);
}

int b_dg_Connect(b_dg_type dg, int src_node_ref, int dest_node_ref, double w, int user_val)
{
  int edge_ref;
  
  edge_ref = b_dg_find_edge(dg, src_node_ref, dest_node_ref);
  if ( edge_ref >= 0 )
    return edge_ref;
  edge_ref = b_dg_AddEmptyEdge(dg);
  if ( edge_ref >= 0 )
  {
    if ( b_dg_connect_edge(dg, edge_ref, src_node_ref, dest_node_ref, w, user_val) != 0 )
    {
      return edge_ref;
    }
    b_dg_DeleteEdge(dg, edge_ref);
  }
  return -1;
}

/* add additional node_cnt nodes between src_node and dest_nodes */
/* mark those additional nodes as internal nodes */
int b_dg_ConnectWithInternalNodes(b_dg_type dg, int src_node_ref, int dest_node_ref, int node_cnt)
{
  int node_ref;
  while( node_cnt > 0 )
  {
    node_ref = b_dg_AddNode(dg, -1);
    if ( node_ref < 0 )
      return 0;
    NODE(dg, node_ref)->is_internal_node = 1;
    if ( b_dg_Connect(dg, src_node_ref, node_ref, 0.0, -1) < 0 )
      return 0;
    src_node_ref = node_ref;
    node_cnt--;
  }
  if ( b_dg_Connect(dg, src_node_ref, dest_node_ref, 0.0, -1) < 0 )
    return 0;
  return 1;
}

void b_dg_Disconnect(b_dg_type dg, int src_node_ref, int dest_node_ref)
{
  int edge_ref;
  
  edge_ref = b_dg_find_edge(dg, src_node_ref, dest_node_ref);
  if ( edge_ref < 0 )
    return;
    
  b_dg_DeleteEdge(dg, edge_ref);
}

#define COLOR(dg, node_ref) (NODE((dg), (node_ref))->color)
#define PI(dg, node_ref) (NODE((dg), (node_ref))->predecessor_node_ref)
#define D(dg, node_ref) (NODE((dg), (node_ref))->discover)
#define F(dg, node_ref) (NODE((dg), (node_ref))->finish)


/* DFS algorithms, based Corman, Leiserson, Rivest p478  */

int b_dg_DFS_VISIT(b_dg_type dg, int u, double *time, b_il_type il, int is_inv)
{
  b_il_type adj =  NODE(dg, u)->out_edge_refs;
  int pos, edge_ref;
  int v;

  COLOR(dg, u) = 1;
  *time = *time + 1.0;
  D(dg, u) = *time;

  pos = -1; edge_ref = -1;
  while( b_il_IsInvLoop(adj, &pos, &edge_ref, is_inv) != 0 )
  {
    v = EDGE(dg, edge_ref)->dest_node_ref;
    if ( COLOR(dg, v) == 0 )
    {
      PI(dg, v) = u;
      if ( b_dg_DFS_VISIT(dg, v, time, il, is_inv) == 0 )
        return 0;
    }
    else if ( COLOR(dg, v) == 1 )
      dg->is_acyclic = 0; /* tree is cyclic */
  }
  
  COLOR(dg, u) = 2;
  *time = *time + 1.0;
  F(dg, u)[is_inv] = *time;
  
  if ( il != NULL )
    if ( b_il_Add(il, u) < 0 )
      return 0;
  
  return 1;
}

void b_dg_DFS_START(b_dg_type dg)
{
  int u = -1;
  dg->is_acyclic = 1;
  while( b_dg_LoopNodes(dg, &u) != 0 )
  {
    COLOR(dg, u) = 0;
    PI(dg, u) = -1;
  }
}

void b_dg_DFS(b_dg_type dg, int is_inv)
{
  int u;
  double time = 0.0;

  b_dg_DFS_START(dg);
  
  u = -1;
  if ( is_inv == 0 )  
  {
    while( b_dg_LoopNodes(dg, &u) != 0 )
      if ( COLOR(dg, u) == 0 )
        b_dg_DFS_VISIT(dg, u, &time, NULL, 0);
  }
  else
  {  
    while( b_dg_InvLoopNodes(dg, &u) != 0 )
      if ( COLOR(dg, u) == 0 )
        b_dg_DFS_VISIT(dg, u, &time, NULL, 1);
  }
}

/* returns node_res in 'il' in REVERSE order! */
int b_dg_TOPOLOGICAL_SORT(b_dg_type dg, b_il_type il)
{
  int u;
  double time = 0.0;

  b_il_Clear(il);
  
  b_dg_DFS_START(dg);
  
  u = -1;
  while( b_dg_LoopNodes(dg, &u) != 0 )
    if ( COLOR(dg, u) == 0 )
      if ( b_dg_DFS_VISIT(dg, u, &time, il, 0) == 0 )
        return 0;
  return 1;
}

/* Corman, Leiserson, Rivest p520  */
void b_dg_INIT_SINGLE_SOURCE(b_dg_type dg, int src_node_ref)
{
  int u = -1;
  while( b_dg_LoopNodes(dg, &u) != 0 )
  {
    D(dg, u) = INT_MAX;
    PI(dg, u) = -1;
  }
  D(dg, src_node_ref) = 0.0;
}

/* Corman, Leiserson, Rivest p520 */
void b_dg_RELAX(b_dg_type dg, int edge_ref)
{
  double w = EDGE(dg, edge_ref)->w;
  int u = EDGE(dg, edge_ref)->src_node_ref;
  int v = EDGE(dg, edge_ref)->dest_node_ref;
  if ( D(dg, v) > D(dg, u) + w )
  {
    D(dg, v) = D(dg, u) + w;
    PI(dg, v) = u;
  }
}

/* Corman, Leiserson, Rivest p536 */
/* DAG = directed acyclic graph */
int b_dg_DAG_SHORTEST_PATHS(b_dg_type dg, int src_node_ref)
{
  b_il_type il;
  int i;
  int u;
  int pos, edge_ref;
  b_il_type adj;
  il = b_il_Open();
  if ( il == NULL )
    return 0;         /* memory error */
    
  if ( b_dg_TOPOLOGICAL_SORT(dg, il) == 0 )
    return b_il_Close(il), 0;         /* memory error */
  
  if ( dg->is_acyclic == 0 )
    return b_il_Close(il), 0;         /* not a DAG */

  b_dg_INIT_SINGLE_SOURCE(dg, src_node_ref);
  
  i = b_il_GetCnt(il);
  while( i > 0)
  {
    i--;
    u = b_il_GetVal(il, i);
    
    adj =  NODE(dg, u)->out_edge_refs;

    pos = -1; edge_ref = -1;
    while( b_il_Loop(adj , &pos, &edge_ref) != 0 )
      b_dg_RELAX(dg, edge_ref);
  }
  
  return b_il_Close(il), 1;
}

void b_dg_Show(b_dg_type dg)
{
  int ref;
  
  ref = -1;
  while( b_dg_LoopNodes(dg, &ref) != 0 )
  {
    printf("Node %d: user=%d layer=%d  f0=%.1f f1=%.1f\n", 
      ref, NODE(dg, ref)->user_val, NODE(dg, ref)->layer,
      NODE(dg, ref)->finish[0], NODE(dg, ref)->finish[1]
      );
  }

  ref = -1;
  while( b_dg_LoopEdges(dg, &ref) != 0 )
  {
    printf("Edge %d: src=%d dest=%d w=%f user=%d\n", ref, EDGE(dg, ref)->src_node_ref, EDGE(dg, ref)->dest_node_ref, 
      EDGE(dg, ref)->w, EDGE(dg, ref)->user_val);
  }
}

int b_dg_CalcAcyclicShortestPaths(b_dg_type dg, int src_node_ref)
{
  return b_dg_DAG_SHORTEST_PATHS(dg, src_node_ref);
}


int b_dg_LoopPath(b_dg_type dg, int *node_ref, int *edge_ref)
{
  if ( PI(dg, *node_ref) < 0 )
    return 0;
  *edge_ref = b_dg_find_edge(dg, PI(dg, *node_ref), *node_ref);
  *node_ref = PI(dg, *node_ref);
  return 1;
}

int b_dg_GetPathWeight(b_dg_type dg, int src_node_ref, int dest_node_ref, double *weight)
{
  int edge_ref;

  if ( PI(dg, dest_node_ref) < 0 )
    return 0;

  *weight = 0.0;
  while( b_dg_LoopPath(dg, &dest_node_ref, &edge_ref) != 0 ) 
    *weight += b_dg_GetEdgeWeight(dg, edge_ref);
    
  if ( src_node_ref != dest_node_ref )
    return 0;
    
  return 1;
}

/*---------------------------------------------------------------------------*/
/* assign layer position to each node                                        */
/*---------------------------------------------------------------------------*/

void b_dg_INIT_ASSIGN_LAYER(b_dg_type dg)
{
  int u = -1;
  dg->is_acyclic = 1;
  dg->layer_cnt = 0;
  while( b_dg_LoopNodes(dg, &u) != 0 )
    NODE(dg, u)->layer = 0;
}

int b_dg_DAG_ASSIGN_LAYER(b_dg_type dg)
{
  b_il_type il;
  int i;
  int u;
  int pos, edge_ref;
  b_il_type adj;
  int max_layer;
  int cur_layer;
  
  il = b_il_Open();
  if ( il == NULL )
    return 0;         /* memory error */
    
  b_dg_INIT_ASSIGN_LAYER(dg);
    
  if ( b_dg_TOPOLOGICAL_SORT(dg, il) == 0 )
    return b_il_Close(il), 0;         /* memory error */
  
  if ( dg->is_acyclic == 0 )
    return b_il_Close(il), 0;         /* not a DAG */

  i = 0;
  while( i < b_il_GetCnt(il))
  {
    u = b_il_GetVal(il, i);
    
    adj =  NODE(dg, u)->out_edge_refs;

    pos = -1; edge_ref = -1;
    max_layer = -1;
    while( b_il_Loop(adj , &pos, &edge_ref) != 0 )
    {
      assert(u == EDGE(dg, edge_ref)->src_node_ref);
      cur_layer = NODE(dg, EDGE(dg, edge_ref)->dest_node_ref)->layer;
      if ( max_layer < cur_layer )
        max_layer = cur_layer;
    } 
    if ( max_layer >= 0 )
    {
      NODE(dg, u)->layer = max_layer + 1;
    }
    if ( dg->layer_cnt < NODE(dg, u)->layer+1 )
      dg->layer_cnt = NODE(dg, u)->layer+1;
    i++;
  }
  return b_il_Close(il), 1;
}

/*---------------------------------------------------------------------------*/
/* create additional internal nodes                                          */
/*---------------------------------------------------------------------------*/

int b_dg_DAG_ASSIGN_LAYER_AND_NODES(b_dg_type dg)
{ 
  int u = -1;
  int v;
  int pos, edge_ref, node_ref;
  b_il_type adj;
  int u_layer, v_layer;
  
  if ( b_dg_DAG_ASSIGN_LAYER(dg) == 0 )
    return 0;


  while( b_dg_LoopNodes(dg, &u) != 0 )
  {
    adj =  NODE(dg, u)->out_edge_refs;
    u_layer = NODE(dg, u)->layer;

    while( b_il_Loop(adj , &pos, &edge_ref) != 0 )
    {
      v = EDGE(dg, edge_ref)->dest_node_ref;
      assert(u == EDGE(dg, edge_ref)->src_node_ref);
      
      v_layer = NODE(dg, v)->layer;
      if ( v_layer+1 < u_layer )
      {
        node_ref = b_dg_AddNode(dg, -1);
        if ( node_ref < 0 )
          return 0;
        NODE(dg, node_ref)->is_internal_node = 1;
        EDGE(dg, edge_ref)->dest_node_ref = node_ref;
        if ( b_dg_ConnectWithInternalNodes(dg, node_ref, v, u_layer-v_layer-2) == 0 )
          return 0;
      }
    } 
  }

  if ( b_dg_DAG_ASSIGN_LAYER(dg) == 0 )
    return 0;
    
  return 1;
}

/*---------------------------------------------------------------------------*/
/* build layer and sorted index tables                                       */
/*---------------------------------------------------------------------------*/

/* cormen, leiserson, rivest, S. 154 */
int b_dg_layer_PARTITION(b_dg_type dg, int l, int p, int r, 
  int (*cmp)(b_dg_type dg, int n1, int n2))
{
  int x = p;
  int i = p-1;
  int j = r+1;
  for(;;)
  {
    do
    {
      j--;
    } while( cmp( dg, b_il_GetVal(LAYER(dg,l)->node_refs, j), 
                      b_il_GetVal(LAYER(dg,l)->node_refs, x) ) > 0 );
    
    do
    {
      i++;
    } while( cmp( dg, b_il_GetVal(LAYER(dg,l)->node_refs, i), 
                      b_il_GetVal(LAYER(dg,l)->node_refs, x) ) < 0 );
    
    if ( i >= j )
      break;
      
    b_il_Swap(LAYER(dg,l)->node_refs, i, j);
  }
  return j;
}

void b_dg_layer_QUICKSORT(b_dg_type dg, int l, int p, int r, 
  int (*cmp)(b_dg_type dg, int n1, int n2))
{
  int q;
  if ( p < r )
  {
    q = b_dg_layer_PARTITION(dg, l, p, r, cmp);
    b_dg_layer_QUICKSORT(dg, l, p, q, cmp);
    b_dg_layer_QUICKSORT(dg, l, q+1, r, cmp);
  }
}

int b_dg_cmp_layer_delta_finish(b_dg_type dg, int n1, int n2)
{
  double d1, d2;
  d1 = NODE(dg, n1)->finish[0] - NODE(dg, n1)->finish[1];
  d2 = NODE(dg, n2)->finish[0] - NODE(dg, n2)->finish[1];
  if ( d1 < d2 )
    return -1;
  if ( d1 > d2 )
    return 1;
  return 0;
}

void b_dg_sort_layer_nodes_by_delta_finish(b_dg_type dg, int l)
{
  b_dg_layer_QUICKSORT(dg, l, 0, b_il_GetCnt(LAYER(dg,l)->node_refs)-1,
    b_dg_cmp_layer_delta_finish);
}

int b_dg_BuildLayer(b_dg_type dg)
{
  int i;
  int l_ref;
  b_dg_layer_type l;

  if ( b_dg_DAG_ASSIGN_LAYER_AND_NODES(dg) == 0 )
    return 0;

  for( i = 0; i < b_pl_GetCnt(dg->layers); i++ )
    b_dg_layer_Close(LAYER(dg,i));
  b_pl_Clear(dg->layers);

  for( i = 0; i < dg->layer_cnt; i++ )
  {
    l = b_dg_layer_Open();
    if ( l == NULL )
      return 0;
    if ( b_pl_Add(dg->layers, l) < 0 )
    { 
      b_dg_layer_Close(l);
      return 0;
    }
  }
  assert(b_pl_GetCnt(dg->layers) == dg->layer_cnt);

  i = -1;  
  while( b_dg_LoopNodes(dg, &i) != 0 )
  {
    l_ref = NODE(dg, i)->layer;
    assert(l_ref >= 0 && l_ref < dg->layer_cnt);
    if ( b_il_Add(LAYER(dg, l_ref)->node_refs, i) < 0 )
      return 0;
  }

  b_dg_DFS(dg, 0); /* calculate the left (y) coordinates */
  b_dg_DFS(dg, 1); /* calculate the right (x) coordinates */
  
  /* sort elements of a layer according to finish[0]-finish[1] */
  /* finish[0]-finish[1] is proportional to the distance of a line going */
  /* through the origin with 45 degrees. */
  
  for( i = 0; i < b_pl_GetCnt(dg->layers); i++ )
    b_dg_sort_layer_nodes_by_delta_finish(dg, i);
  
  /* calculate the max width of all layers */
  dg->max_nodes_per_layer = 0;
  for( i = 0; i < b_pl_GetCnt(dg->layers); i++ )
    if ( dg->max_nodes_per_layer < b_il_GetCnt(LAYER(dg, i)->node_refs) )
      dg->max_nodes_per_layer = b_il_GetCnt(LAYER(dg, i)->node_refs);
      
  return 1;
}


#ifdef B_DG_TEST_MAIN

void show_layer(b_dg_type dg)
{
  int i, j;
  int n;
  for( i = 0; i < b_pl_GetCnt(dg->layers); i++ )
  {
    printf("layer %3d: ", i);
    for( j = 0; j < b_il_GetCnt(LAYER(dg, i)->node_refs); j++ )
    {
      n = b_il_GetVal(LAYER(dg, i)->node_refs, j);
      printf(" %3d", n);
    }
    printf("\n");
  }
}

void show(b_dg_type dg, int node_ref)
{
  int edge_ref;
  printf("%d", node_ref);
  while( b_dg_LoopPath(dg, &node_ref, &edge_ref) != 0 )
    printf(" <-[%.2f]-- %d", b_dg_GetEdgeWeight(dg, edge_ref), node_ref);
  printf("\n");
}

int main()
{
  b_dg_type dg = b_dg_Open();
  int a,b,c,d;
  int r;

  a = b_dg_AddNode(dg, -1);
  r = b_dg_DAG_SHORTEST_PATHS(dg, a);
  assert( r != 0 );
  assert( PI(dg, a) < 0 );
  /* show(dg, a); */
  
  b = b_dg_AddNode(dg, -1);
  r = b_dg_DAG_SHORTEST_PATHS(dg, a);
  assert( r != 0 );
  assert( PI(dg, a) < 0 );
  assert( PI(dg, b) < 0 );
  /* show(dg, a); */
  /* show(dg, b); */
  
  b_dg_Connect(dg, a, b, 15, -1);
  r = b_dg_DAG_SHORTEST_PATHS(dg, a);
  assert( r != 0 );
  assert( PI(dg, a) < 0 );
  assert( PI(dg, b) == a );
  /* show(dg, a); */
  /* show(dg, b); */
  
  r = b_dg_DAG_SHORTEST_PATHS(dg, b);
  assert( r != 0 );
  assert( PI(dg, a) < 0 );
  assert( PI(dg, a) < 0 );
  assert( PI(dg, b) < 0 );
  /* show(dg, a); */
  /* show(dg, b); */

  c = b_dg_AddNode(dg, -1);
  b_dg_Connect(dg, a, c, 20, -1);
  r = b_dg_DAG_SHORTEST_PATHS(dg, a);
  assert( r != 0 );
  assert( PI(dg, a) < 0 );
  assert( PI(dg, b) == a );
  assert( PI(dg, c) == a );
  /* show(dg, b); */
  /* show(dg, c); */

  d = b_dg_AddNode(dg, -1);
  b_dg_Connect(dg, b, d, 17, -1);
  r = b_dg_DAG_SHORTEST_PATHS(dg, a);
  assert( r != 0 );
  assert( PI(dg, a) < 0 );
  assert( PI(dg, b) == a );
  assert( PI(dg, c) == a );
  assert( PI(dg, d) == b );
  /* show(dg, d); */

  b_dg_Connect(dg, c, d, 2, -1);
  r = b_dg_DAG_SHORTEST_PATHS(dg, a);
  assert( r != 0 );
  assert( PI(dg, a) < 0 );
  assert( PI(dg, b) == a );
  assert( PI(dg, c) == a );
  assert( PI(dg, d) == c );
  /* show(dg, d); */

  b_dg_Connect(dg, a, d, 111, -1);
  r = b_dg_DAG_SHORTEST_PATHS(dg, a);
  assert( r != 0 );
  assert( PI(dg, a) < 0 );
  assert( PI(dg, b) == a );
  assert( PI(dg, c) == a );
  assert( PI(dg, d) == c );
  /* show(dg, d); */
  
  b_dg_Disconnect(dg, a, d);
  b_dg_Connect(dg, d, a, 111, -1);    /* this makes an acyclic graph */
  r = b_dg_DAG_SHORTEST_PATHS(dg, a);
  assert( r == 0 );
  
  b_dg_Clear(dg);
  
  a = b_dg_AddNode(dg, -1);
  b = b_dg_AddNode(dg, -1);
  c = b_dg_AddNode(dg, -1);
  b_dg_Connect(dg,b,a,0.0,-1);
  b_dg_Connect(dg,c,b,0.0,-1);
  b_dg_Connect(dg,c,a,0.0,-1);

  b_dg_DAG_ASSIGN_LAYER(dg);
  assert( NODE(dg, a)->layer == 0 );
  assert( NODE(dg, b)->layer == 1 );
  assert( NODE(dg, c)->layer == 2 );

  b_dg_DAG_ASSIGN_LAYER_AND_NODES(dg);
  assert( NODE(dg, a)->layer == 0 );
  assert( NODE(dg, b)->layer == 1 );
  assert( NODE(dg, c)->layer == 2 );
  assert( b_set_Cnt(dg->nodes) == 4 );
  d = b_dg_AddNode(dg, -1);
  b_dg_Connect(dg,d,c,0.0,-1);
  b_dg_Connect(dg,d,a,0.0,-1);
  b_dg_DAG_ASSIGN_LAYER(dg);
  assert( NODE(dg, a)->layer == 0 );
  assert( NODE(dg, b)->layer == 1 );
  assert( NODE(dg, c)->layer == 2 );
  assert( NODE(dg, d)->layer == 3 );
  assert( b_set_Cnt(dg->nodes) == 5 );

  b_dg_DAG_ASSIGN_LAYER_AND_NODES(dg);
  assert( NODE(dg, a)->layer == 0 );
  assert( NODE(dg, b)->layer == 1 );
  assert( NODE(dg, c)->layer == 2 );
  assert( NODE(dg, d)->layer == 3 );
  assert( b_set_Cnt(dg->nodes) == 7 );
  
  
  b_dg_Clear(dg);
  
  a = b_dg_AddNode(dg, -1);
  b = b_dg_AddNode(dg, -1);
  c = b_dg_AddNode(dg, -1);
  b_dg_Connect(dg, b, a, 15, -1);
  b_dg_Connect(dg, c, b, 15, -1);
  b_dg_Connect(dg, c, a, 15, -1);
  
  b_dg_BuildLayer(dg);
  show_layer(dg);
  b_dg_Show(dg);

  

  b_dg_Close(dg);

  return 0;
}

#endif
