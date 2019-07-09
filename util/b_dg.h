/*

  b_dg.h

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


#ifndef _B_DG_H
#define _B_DG_H

#include "b_il.h"
#include "b_set.h"
#include "b_pl.h"

struct _b_dg_node_struct
{
  int color;
  int predecessor_node_ref;
  double finish[2];
  double discover;
  b_il_type out_edge_refs;
  int user_val;
  int layer;
  int is_internal_node;
  void *user_ptr;
};
typedef struct _b_dg_node_struct *b_dg_node_type;

struct _b_dg_edge_struct
{
  double w;
  int src_node_ref;
  int dest_node_ref;
  int user_val;
  void *user_ptr;
};
typedef struct _b_dg_edge_struct *b_dg_edge_type;

struct _b_dg_layer_struct
{
  b_il_type node_refs;
};
typedef struct _b_dg_layer_struct *b_dg_layer_type;

struct _b_dg_struct
{
  b_set_type nodes;
  b_set_type edges;
  b_pl_type layers;
  int is_acyclic;
  int layer_cnt;              /* calculated in b_dg_DAG_ASSIGN_LAYER */
  int max_nodes_per_layer;    /* assigned in b_dg_BuildLayer */
};
typedef struct _b_dg_struct *b_dg_type;

b_dg_type b_dg_Open();
void b_dg_Clear(b_dg_type dg);
void b_dg_Close(b_dg_type dg);
int b_dg_AddNode(b_dg_type dg, int user_val);
int b_dg_Connect(b_dg_type dg, int src_node_ref, int dest_node_ref, double w, int user_val);
void b_dg_Disconnect(b_dg_type dg, int src_node_ref, int dest_node_ref);

#define b_dg_GetNodePredecessor(dg, ref) \
  (((b_dg_node_type)b_set_Get((dg)->nodes, (ref)))->predecessor_node_ref)
#define b_dg_GetNodeUserVal(dg, ref) \
  (((b_dg_node_type)b_set_Get((dg)->nodes, (ref)))->user_val)
#define b_dg_GetNodeUserPtr(dg, ref) \
  (((b_dg_node_type)b_set_Get((dg)->nodes, (ref)))->user_ptr)

#define b_dg_GetEdgeWeight(dg, ref) \
  (((b_dg_edge_type)b_set_Get((dg)->edges, (ref)))->w)
#define b_dg_GetEdgeUserVal(dg, ref) \
  (((b_dg_edge_type)b_set_Get((dg)->edges, (ref)))->user_val)
#define b_dg_GetEdgeUserPtr(dg, ref) \
  (((b_dg_edge_type)b_set_Get((dg)->edges, (ref)))->user_ptr)


/*
  Calculate all shortest paths to a given source node.
  Returns 0 if the graph contains cycles or a memory error occured.
*/
int b_dg_CalcAcyclicShortestPaths(b_dg_type dg, int src_node_ref);

/*
  Loop through a path.
  Returns 0 at the end of a path.
  Requires a previous call to a path calculating algorithm.
*/
int b_dg_LoopPath(b_dg_type dg, int *node_ref, int *edge_ref);

/* returns 0 if no path exists */
int b_dg_GetPathWeight(b_dg_type dg, int src_node_ref, int dest_node_ref, double *weight);

void b_dg_Show(b_dg_type dg);

#endif /* _B_DG_H */

