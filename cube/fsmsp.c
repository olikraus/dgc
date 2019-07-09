/*

  fsmsp.c
  
  FSM shortest path algorithms

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
  
  
  spss = shortest path single source
  
  cormen, leiserson, rivest, p. 520
  
*/

#include <limits.h>
#include <assert.h>
#include "fsm.h"
#include "b_iq.h"
#include "b_rdic.h"
#include "mwc.h"

#define SP_INT_MAX INT_MAX
#define SP_WHITE 0
#define SP_GREY 1
#define SP_BLACK 2

/* set node_id as a source for the shortest path problem */
void fsm_spss_init(fsm_type fsm, int node_id)
{
  int n = -1;
  while( fsm_LoopNodes(fsm, &n) != 0 )
  {
    fsm_GetNode(fsm, n)->g_color = SP_WHITE;
    fsm_GetNode(fsm, n)->g_d = SP_INT_MAX;
    fsm_GetNode(fsm, n)->g_predecessor = -1;
  }
  fsm_GetNode(fsm, node_id)->g_d = 0;
  fsm_GetNode(fsm, node_id)->g_color = SP_GREY;
  fsm_GetNode(fsm, node_id)->g_predecessor = -1;
}

void fsm_spss_relax_edge(fsm_type fsm, int edge_id)
{
  int u = fsm_GetEdgeSrcNode(fsm, edge_id);
  int v = fsm_GetEdgeDestNode(fsm, edge_id);
  int w = 1;
  if ( fsm_GetNode(fsm, u)->g_d > fsm_GetNode(fsm, v)->g_d + w )
  {
    fsm_GetNode(fsm, v)->g_d = fsm_GetNode(fsm, u)->g_d + w;
    fsm_GetNode(fsm, v)->g_predecessor = edge_id;
  }
}


#ifdef _SPSS_FOR_WIGHTED_GRAPHS_NOT_USED

/* reverse order for estimate value: minimum is the last entry */
int fsm_spss_dijkstra_cb(void *data, int el, void *key)
{
  fsm_type fsm = (fsm_type)data;
  if ( fsm_GetNode(fsm, el)->g_d > *(int *)key )
    return -1;
  if ( fsm_GetNode(fsm, el)->g_d < *(int *)key )
    return 1;
  return 0;
}

int fsm_spss_dijkstra(fsm_type fsm, int node_id)
{
  b_rdic_type pq;
  int n;
  int edge_id;
  int loop;
  
  pq = b_rdic_Open();
  if ( pq == NULL )
    return 0;
  b_rdic_SetCmpFn(b_rdic, fsm_spss_dijkstra_cb, fsm);

  /* init the graph */  
  fsm_spss_init(fsm, node_id);
  
  /* init the priority queue */
  n = -1;
  while( fsm_LoopNodes(fsm, &n) != 0 )
  {
    if ( b_rdic_Ins(pq, n, &(fsm_GetNode(fsm, n)->g_d)) == 0 )
      return b_rdic_Close(pq), 0;
  }
  
  /* dijkstra's algorithm */
  while( b_rdic_GetCnt(pq) > 0 )
  {
    n = b_rdic_GetLast(pq);
    assert( n >= 0 )
    edge_id = -1;
    loop = -1;
    while( fsm_LoopNodeInEdges(fsm, n, &loop, &edge_id)
    {
      if ( fsm_GetEdgeSrcNode(fsm, edge_id) != fsm_GetEdgeDEstNode(fsm, edge_id) )
      {
        fsm_spss_relax_edge(fsm, edge_id);
      }
    }
  }

  return b_rdic_Close(pq), 1;
}

#endif /* _SPSS_FOR_WIGHTED_GRAPHS_NOT_USED */


/* cormen, leiserson, rivest, p. 470 */
/* breadth-first-search */
int fsm_spss_bfs(fsm_type fsm, int node_id)
{
  b_iq_type q;
  int n;
  int loop;
  int edge_id;
  
  q = b_iq_Open();
  if ( q == NULL )
    return 0;

  /* init the graph */  
  fsm_spss_init(fsm, node_id);

  if ( b_iq_Enqueue(q, node_id) == 0 )
    return b_iq_Close(q), 0;
  
  while(b_iq_IsEmpty(q) == 0)
  {
    node_id = b_iq_Dequeue(q);
    edge_id = -1;
    loop = -1;
    while( fsm_LoopNodeInNodes(fsm, node_id, &loop, &n) != 0 )
    {
      if ( node_id != n )
      {
        if ( fsm_GetNode(fsm, n)->g_color == SP_WHITE )
        {
          fsm_GetNode(fsm, n)->g_color = SP_GREY;
          fsm_GetNode(fsm, n)->g_d = fsm_GetNode(fsm, node_id)->g_d + 1;
          fsm_GetNode(fsm, n)->g_predecessor = node_id;
          if ( b_iq_Enqueue(q, n) == 0 )
            return b_iq_Close(q), 0;
        }
      }
    }
    fsm_GetNode(fsm, n)->g_color = SP_BLACK;
  }
  return b_iq_Close(q), 1;
}

/* returns the distance*/
/* -1 for error */
int fsm_DoShortestPath(fsm_type fsm, int src_id, int dest_id, int fn(void *data, fsm_type fsm, int edge_id, int i, int cnt), void *data)
{
  int edge_id;
  int i, cnt;
  
  if ( src_id == dest_id )
    return 0;  /* no path exists */
  if ( fsm_spss_bfs(fsm, dest_id) == 0 )
    return -1; /* memory error */
  if ( fsm_GetNode(fsm, src_id)->g_predecessor < 0 )
    return -1; /* no path found */
    
  cnt = fsm_GetNode(fsm, src_id)->g_d; 
  i = 0;
  for(;;)
  {
    dest_id = fsm_GetNode(fsm, src_id)->g_predecessor;
    if ( dest_id < 0 )
      break;
    edge_id = fsm_FindEdge(fsm, src_id, dest_id);
    assert(edge_id >= 0);
    assert(i < cnt);
    if ( fn(data, fsm, edge_id, i, cnt) == 0 )
      return -1;
    src_id = dest_id;
    i++;
  }
  return cnt;
}

int fsm_ShowAllShortestPath_cn(void *data, fsm_type fsm, int edge_id, int i, int cnt)
{
  printf(" -> %s", fsm_GetNodeName(fsm, fsm_GetEdgeDestNode(fsm, edge_id)));
  return 1;
}

void fsm_ShowAllShortestPath(fsm_type fsm)
{
  int src_id, dest_id;
  
  src_id = -1;
  while( fsm_LoopNodes(fsm, &src_id) != 0 )
  {
    dest_id = -1;
    while( fsm_LoopNodes(fsm, &dest_id) != 0 )
    {
      printf("%s>", fsm_GetNodeName(fsm, src_id));
      printf("%s: ", fsm_GetNodeName(fsm, dest_id));
      
      printf("%s", fsm_GetNodeName(fsm, src_id));
      if ( fsm_DoShortestPath(fsm, src_id, dest_id, fsm_ShowAllShortestPath_cn, NULL) < 0 )
        return;
      printf("\n");
    }
  }
}

