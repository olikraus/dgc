/*

  index_table.c
  
  Description of the index table for fsm nodes
  
*/
#include <stdlib.h>

#include "index_table.h"
#include "fsm.h"

INDEX_TABLE index_Open(int nr)
{
  INDEX_TABLE tab;
  
  if((tab = (INDEX_TABLE) malloc(sizeof(struct _index_table))) == NULL)
    return NULL;
    
  if((tab->table = (int *) malloc(nr * sizeof(int))) == NULL)
    return NULL;
    
  tab->nr_nodes = nr;

  return (INDEX_TABLE) tab;
}

void index_Close(INDEX_TABLE tab)
{
  if(!tab)
    return;
    
  free(tab->table);
  free(tab);
}

int index_GetNodeId(INDEX_TABLE tab, int pos)
{
  if(!tab)
    return -1;
    
  return tab->table[pos];
}

int index_SetNodeId(INDEX_TABLE tab, int pos, int node_id)
{
  if(!tab)
    return 0;
    
  tab->table[pos] = node_id;
  
  return 1;
}

int index_FindNodeIndex(INDEX_TABLE tab, int node_id)
{
  int i;
  
  if(!tab)
    return -1;
    
  for(i = 0; i < tab->nr_nodes; i++)
    if(tab->table[i] == node_id)
      return i;
      
  return -1;
}

INDEX_TABLE index_CreateTable( fsm_type fsm)
{
  int node_id;
  int crt = 0;
  int nr_nodes = b_set_Cnt(fsm->nodes);
  INDEX_TABLE tab;
  
  tab = index_Open(nr_nodes);
  
  
  node_id = -1;
  while(fsm_LoopNodes(fsm, &node_id) != 0)
    tab->table[crt++] = node_id;
    
  return tab;
}

void index_WriteTable(INDEX_TABLE tab, fsm_type fsm)
{
  int i;
  
  if(!tab)
    return;
    
  for(i = 0; i < tab->nr_nodes; i++)
  {
    printf("Index = %d; node_id = %d; node_name = %s\n", i, tab->table[i], fsm_GetNodeName(fsm, tab->table[i]));
  }
}
