/*

  edif.c

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
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include "edif.h"
#include "mwc.h"

int edif_ExtractCellNetlistContents(edif e, int n);

#define EDIF_UNIQUE_FLAG 1

/*-- Init ---------------------------------------------------------------------*/

#define EDIF_STR(e,s)\
  if ( (e->str_##s = tree_RegisterString(e->t, #s)) == NULL ) return 0

int edif_Init(edif e)
{
  e->root_node = -1;
  e->options = 0;

  EDIF_STR(e, edif);
  EDIF_STR(e, external);
  EDIF_STR(e, library);
  EDIF_STR(e, cell);
  EDIF_STR(e, cellType);
  EDIF_STR(e, view);
  EDIF_STR(e, viewType);
  EDIF_STR(e, NETLIST);
  EDIF_STR(e, interface);
  EDIF_STR(e, port);
  EDIF_STR(e, direction);
  EDIF_STR(e, INPUT);
  EDIF_STR(e, OUTPUT);
  EDIF_STR(e, INOUT);
  EDIF_STR(e, rename);
  EDIF_STR(e, contents);
  EDIF_STR(e, instance);
  EDIF_STR(e, viewRef);
  EDIF_STR(e, cellRef);
  EDIF_STR(e, libraryRef);
  EDIF_STR(e, net);
  EDIF_STR(e, joined);
  EDIF_STR(e, portRef);
  EDIF_STR(e, instanceRef);
  
  EDIF_STR(e, edifVersion);
  EDIF_STR(e, edifLevel);
  EDIF_STR(e, keywordMap);
  EDIF_STR(e, keywordLevel);
  EDIF_STR(e, status);
  EDIF_STR(e, written);
  EDIF_STR(e, timeStamp);
  EDIF_STR(e, program);
  EDIF_STR(e, technology);
  EDIF_STR(e, numberDefinition);
  EDIF_STR(e, GENERIC);
  EDIF_STR(e, design);
  
  return 1;
}

/*-- Error ---------------------------------------------------------------------*/

void edif_Error(edif e, char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  e->err_fn(e->err_data, fmt, va);
  va_end(va);
}

void edif_err_fn(void *data, char *fmt, va_list va)
{
  vprintf(fmt, va);
}

void edif_SetErrFn(edif e, void (*err_fn)(void *data, char *fmt, va_list va), void *data)
{
  e->err_fn = err_fn;
  e->err_data = data;
}

void edif_tree_error(void *data, char *fmt, va_list va)
{
  edif e = (edif)data;
  e->err_fn(e->err_data, fmt, va);
}


/*-- Open ---------------------------------------------------------------------*/

edif edif_OpenErrFn(gnc n, void (*err_fn)(void *data, char *fmt, va_list va), void *data)
{
  edif e;
  int is_gnc_allocated = 0;
  
  if ( n == NULL )
  {
    n = gnc_Open();
    if ( n == NULL )
      return NULL;
    is_gnc_allocated = 1;
  }
  
  e = (edif)malloc(sizeof(struct _edif_struct));
  if ( e != NULL )
  {
    edif_SetErrFn(e, err_fn, data);
    e->is_gnc_allocated = is_gnc_allocated;
    e->nc = n;
    e->t = tree_Open();
    if ( e->t != NULL )
    {
      tree_SetErrFn(e->t, edif_tree_error, e);
      if (edif_Init(e) != 0 )
      {
        return e;
      }
      tree_Close(e->t);
    }
    free(e);
  }
  
  if ( is_gnc_allocated != 0  )
    gnc_Close(n);
  
  return NULL;
}

edif edif_Open(gnc n)
{
  return edif_OpenErrFn(n, edif_err_fn, NULL);
}

void edif_Close(edif e)
{
  if ( e->is_gnc_allocated != 0  )
    gnc_Close(e->nc);
  tree_Close(e->t);
  free(e);
}

int edif_ReadFile(edif e, const char *filename)
{
  if ( e->root_node >= 0 )
  {
    tree_Free(e->t, e->root_node);
  }
  e->root_node = tree_ReadEdif(e->t, filename);
  if ( e->root_node < 0 )
    return 0;
  return 1;
}

/*-- little helper functions --------------------------------------------------*/

char *edif_TStr(edif e, int n)
{
  if ( n < 0 )
    return NULL;
  if ( tree_N(e->t, n)->type != TR_NODE_TYPE_CSTR )
    return NULL;
  return tree_N(e->t, n)->d.s;
}

int edif_TVal(edif e, int n)
{
  if ( n < 0 )
    return 0;
  return tree_N(e->t, n)->d.v;
}

/* Tree Search String */
char *edif_TSStr(edif e, int n, char *s)
{
  n = tree_Search(e->t, n, s);
  if ( n < 0 )
    return NULL;
  return edif_TStr(e, tree_Down(e->t, n));
}

char *edif_TSStrChild(edif e, int n, char *s)
{
  n = tree_SearchChild(e->t, n, s);
  if ( n < 0 )
    return NULL;
  return edif_TStr(e, tree_Down(e->t, n));
}

int edif_ErrorKeyword(edif e, int node, char *str)
{
  if ( tree_N(e->t, node)->d.s != str )
  {
    edif_Error(e, "keyword '%s' expected\n", str);
    return 0;
  }
  return 1;
}

/*-- tree netlist extraction ------------------------------------------------*/

/* expects an identifier, name or the rename construct */
/* returns the identifier part of the rename construct */
char *edif_NameDef(edif e, int n)
{
  char *s;
  if ( n < 0 )
    return NULL;
  if ( tree_N(e->t, n)->type != TR_NODE_TYPE_CSTR )
    return NULL;
  s = tree_N(e->t, n)->d.s;
  if ( s != e->str_rename )
    return s;
  n = tree_Down(e->t, n);
  if ( n < 0 )
  {
    edif_Error(e, "'rename' without argument");
    return NULL;
  }
  s = tree_N(e->t, n)->d.s;
  return s;
}

/* returns node of the head */
int edif_FindHeadAndName(edif e, int n, char *head, char *name)
{
  n = tree_Search(e->t, n, head);
  while( n >= 0 )
  {
    if ( edif_NameDef(e, tree_Down(e->t, n)) == name )
      return n;
    n = tree_SearchNext(e->t, n, head);
  }
  return -1;
}


/* expects port */
int edif_ExtractPort(edif e, int n)
{
  if ( edif_ErrorKeyword(e, n, e->str_port) == 0 )
    return 0;
  
  n = tree_Down(e->t, n);
  e->portName = edif_NameDef(e, n);
  if ( e->portName == NULL )
  {
    edif_Error(e, "missing port name "
     "in view %s of cell '%s' of library '%s'", 
      e->viewName, e->cellName, e->libraryName);
    return 0;
  }
  
  n = tree_Next(e->t, n);
  
  e->direction = edif_TSStr(e, n, e->str_direction);
  if ( e->direction == NULL )
  {
    edif_Error(e, "missing port direction in "
     "port %s of view %s of cell '%s' of library '%s'", 
      e->portName, e->viewName, e->cellName, e->libraryName);
    return 0;
  }

  /* hmm... e->cell_reference should already be valid */
  e->cell_reference = gnc_AddCell(e->nc, e->cellName, e->libraryName);
  
  e->port_reference = -1;
  
  if (e->direction == e->str_INPUT)
    e->port_reference = gnc_AddCellPort(e->nc, e->cell_reference, GPORT_TYPE_IN, e->portName);
  else if (e->direction == e->str_OUTPUT)
    e->port_reference = gnc_AddCellPort(e->nc, e->cell_reference, GPORT_TYPE_OUT, e->portName);
  else if (e->direction == e->str_INOUT)
    e->port_reference = gnc_AddCellPort(e->nc, e->cell_reference, GPORT_TYPE_BI, e->portName);
  
  if ( e->port_reference < 0 )
  {
    edif_Error(e, "illegal direction or memory error in "
     "port '%s' of view '%s' of cell '%s' of library '%s'", 
      e->portName, e->viewName, e->cellName, e->libraryName);
    return 0;
  }
  
  return 1;
}

/* expects an interface */
int edif_ExtractInterface(edif e, int n)
{
  int nn;
  
  if ( edif_ErrorKeyword(e, n, e->str_interface) == 0 )
    return 0;
  
  nn = tree_Down(e->t, n);
  
  /* port loop */
  nn = tree_Search(e->t, nn, e->str_port);
  while( nn >= 0 )
  {
    if ( edif_ExtractPort(e, nn) == 0 )
      return 0;
    nn = tree_SearchNext(e->t, nn, e->str_port);
  }
  
  return 1;
}

int edif_GetViewByRef(edif e, char *viewRef, char *cellRef, char *libraryRef)
{
  int n, nn;
  char *s;
  
  n = tree_Down(e->t, e->root_node);
  n = tree_Next(e->t, n);
  
  nn = -1;
  while( n >= 0 )
  {
    s = edif_TStr(e, n);
    if ( s != NULL )
    {
      if ( s == e->str_external || s == e->str_library )
      {
        nn = tree_Down(e->t, n);
        if ( edif_NameDef(e, nn) == libraryRef )
          break;
      }
    }
    n = tree_Next(e->t, n);
  }
  if ( n < 0 )
    return -1;
  
  n = tree_Down(e->t, n);
  if ( n < 0 )
    return -1;
    
  n = tree_Next(e->t, n);
  if ( n < 0 )
    return -1;
  
  n = edif_FindHeadAndName(e, n, e->str_cell, cellRef);
  if ( n < 0 )
    return -1;

  n = tree_Down(e->t, n);
  if ( n < 0 )
    return -1;
    
  n = tree_Next(e->t, n);
  if ( n < 0 )
    return -1;
  
  n = edif_FindHeadAndName(e, n, e->str_view, viewRef);
  if ( n < 0 )
    return -1;

  return n;
}

/* expects view with viewtype netlist */
int edif_ExtractCellViewNetlist(edif e, int n)
{
  int nn;
  if ( edif_ErrorKeyword(e, n, e->str_view ) == 0 )
    return 0;
  
  n = tree_Down(e->t, n);
  
  e->viewName = edif_NameDef(e, n);
  if ( e->viewName == NULL )
  {
    edif_Error(e, "illegal view in cell '%s' of library '%s'", e->cellName, e->libraryName);
    return 0;
  }

  n = tree_Next(e->t, n);

  /* already done... */
  e->viewType = edif_TSStr(e, n, e->str_viewType);

  /* extract the interface */
  nn = tree_SearchNext(e->t, n, e->str_interface);
  if ( nn < 0 )
  {
    edif_Error(e, "no interface found (cell '%s')", e->cellName);
    return 0;
  }
  if ( edif_ExtractInterface(e, nn) == 0 )
    return 0;

  /* extract the contents, if available */
  nn = tree_SearchNext(e->t, n, e->str_contents);
  if ( nn >= 0 )
    return edif_ExtractCellNetlistContents(e, nn);
  
  return 1;  
}

/* expects viewRef */
/* this will fill the variables viewRef, cellRef and libraryRef */
int edif_ViewRef(edif e, int n)
{
  if ( edif_ErrorKeyword(e, n, e->str_viewRef) == 0 )
    return 0;

  e->viewRef = e->viewName;
  e->cellRef = e->cellName;
  e->libraryRef = e->libraryName;
    
  n = tree_Down(e->t, n);
  e->viewRef = edif_NameDef(e, n);
  if ( e->viewRef == NULL )
  {
    edif_Error(e, "missing view reference name "
     "in view '%s' of cell '%s' of library '%s'", 
      e->viewName, e->cellName, e->libraryName);
    return 0;
  }

  n = tree_Next(e->t, n);
  if ( edif_TStr(e, n) == e->str_cellRef )
  {
    n = tree_Down(e->t, n);
    e->cellRef = edif_NameDef(e, n);
    if ( e->cellRef == NULL )
    {
      edif_Error(e, "missing cell reference name "
       "in viewRef '%s' of view '%s' of cell '%s' of library '%s'", 
        e->viewRef, e->viewName, e->cellName, e->libraryName);
      return 0;
    }
    
    n = tree_Next(e->t, n);
    if ( edif_TStr(e, n) == e->str_libraryRef )
    {
      n = tree_Down(e->t, n);
      e->libraryRef = edif_NameDef(e, n);
      if ( e->cellRef == NULL )
      {
        edif_Error(e, "missing library reference name "
         "in cellRef '%s' of viewRef '%s' of view '%s' of cell '%s' of library '%s'", 
          e->cellRef, e->viewRef, e->viewName, e->cellName, e->libraryName);
        return 0;
      }
    }
  }
  
  return 1;
}

/* expects instance */
int edif_ExtractInstance(edif e, int n)
{
  int nn;
  if ( edif_ErrorKeyword(e, n, e->str_instance) == 0 )
    return 0;
  
  n = tree_Down(e->t, n);
  e->instanceName = edif_NameDef(e, n);
  if ( e->instanceName == NULL )
  {
    edif_Error(e, "missing instance name "
     "in view %s of cell '%s' of library '%s'", 
      e->viewName, e->cellName, e->libraryName);
    return 0;
  }
  
  n = tree_Next(e->t, n);
  
  nn = tree_Search(e->t, n, e->str_viewRef);
  if ( nn >= 0 )
  {
    int to_cell_reference;
  
    if ( edif_ViewRef(e, nn) == 0 )
      return 0;
      
      
    if ( edif_GetViewByRef(e, e->viewRef, e->cellRef, e->libraryRef) < 0 )
    {
      edif_Error(e, "instance %s: reference view '%s' cell '%s' library '%s' invalid",
        e->instanceName, e->viewRef, e->cellRef, e->libraryRef);
      return 0;
    }

    to_cell_reference = gnc_FindCell(e->nc, e->cellRef, e->libraryRef);
    e->node_reference = gnc_AddCellNode(e->nc, 
      e->cell_reference, e->instanceName, to_cell_reference);

    if ( e->node_reference < 0 )
    {
      edif_Error(e, "out of memory (net in cell '%s')", e->cellName);
      return 0;
    }

  }
  else
  {
    edif_Error(e, "'viewRef' not found for instance '%s' (viewList not supported)",
      e->instanceName);
    return 0;
    /* viewList not supported */
  }
  
  return 1;
}

/* expects, that nl_reference and net_reference are valid */
int edif_ExtractNetJoinedPortRef(edif e, int n)
{
  if ( edif_ErrorKeyword(e, n, e->str_portRef ) == 0 )
    return 0;
  
  n = tree_Down(e->t, n);
  e->portRef = edif_NameDef(e, n);
  if ( n < 0 || e->portRef == NULL )
  {
    edif_Error(e, "illegal port reference in cell '%s' of library '%s'", 
      e->cellName, e->libraryName);
    return 0;
  }
  
  e->instance_reference = -1;
  e->instanceRef = NULL;
  
  n = tree_Next(e->t, n);
  if ( n >= 0 )
  {
    if ( edif_TStr(e, n) == e->str_instanceRef )
    { 
      n = tree_Down(e->t, n);
      e->instanceRef = edif_NameDef(e, n);
      if ( e->instanceRef == NULL )
      {
        edif_Error(e, "illegal instance reference in cell '%s' of library '%s'", 
          e->cellName, e->libraryName);
        return 0;
      }
    }
  }
  
  if ( e->instanceRef != NULL )
  {
    int node_ref;
    int cell_ref;
    int port_ref;
    
    node_ref = gnc_FindCellNode(e->nc, e->cell_reference, e->instanceRef);
    if ( node_ref < 0 )
    {
      edif_Error(e, "instance '%s' in cell '%s' of library '%s' not found", 
          e->instanceRef, e->cellName, e->libraryName);
      return 0;
    }
    cell_ref = gnc_GetCellByNode(e->nc, e->cell_reference, node_ref);
    port_ref = gnc_FindCellPort(e->nc, cell_ref, e->portRef);
    
    if ( gnc_AddCellNetJoin(e->nc, e->cell_reference, e->net_reference, node_ref, port_ref, EDIF_UNIQUE_FLAG) < 0 )
    {
      edif_Error(e, "out of memory (create instance port '%s' in net '%s' of cell '%s' of library '%s')", 
          e->portRef, e->netName, e->cellName, e->libraryName);
      return 0;
    }

  }
  else
  {
    int cell_ref;
    int port_ref;
    
    cell_ref = gnc_FindCell(e->nc, e->cellName, e->libraryName);
    port_ref = gnc_FindCellPort(e->nc, cell_ref, e->portRef);

    if ( gnc_AddCellNetJoin(e->nc, e->cell_reference, e->net_reference, -1, port_ref, EDIF_UNIQUE_FLAG) < 0 )
    {
      edif_Error(e, "out of memory (create global port '%s' in net '%s' of cell '%s' of library '%s')", 
          e->portRef, e->netName, e->cellName, e->libraryName);
      return 0;
    }
    
  }
  
  return 1;
}


/* expects, that nl_reference and net_reference are valid */
int edif_ExtractNetJoined(edif e, int n)
{
  int nn;
  
  if ( edif_ErrorKeyword(e, n, e->str_joined ) == 0 )
    return 0;

  
  /* portRef loop */
  nn = tree_SearchChild(e->t, n, e->str_portRef);
  while( nn >= 0 )
  {
    if ( edif_ExtractNetJoinedPortRef(e, nn) == 0 )
      return 0;
    nn = tree_SearchNext(e->t, nn, e->str_portRef);
  }
  
  return 1;
}


/* expects a net */
int edif_ExtractNet(edif e, int n)
{
  int nn;
  if ( edif_ErrorKeyword(e, n, e->str_net ) == 0 )
    return 0;
    
  n = tree_Down(e->t, n);
  
  e->netName = edif_NameDef(e, n);
  if ( e->netName == NULL )
  {
    edif_Error(e, "illegal net name in cell '%s' of library '%s'", 
      e->cellName, e->libraryName);
    return 0;
  }

  e->net_reference = gnc_AddCellNet(e->nc, e->cell_reference, e->netName);
  if ( e->net_reference < 0 )
  {
    edif_Error(e, "out of memory (allocation of a single net in cell '%s' of library '%s')", 
      e->cellName, e->libraryName);
    return 0;
  }
  
  nn = tree_SearchNext(e->t, n, e->str_joined);
  if ( nn >= 0 )
  {
    if ( edif_ExtractNetJoined(e, nn) == 0 )
      return 0;
  }

  return 1;  
}


/* expects the contents of a cell with netlist view */
int edif_ExtractCellNetlistContents(edif e, int n)
{
  int nn;
  
  if ( edif_ErrorKeyword(e, n, e->str_contents ) == 0 )
    return 0;
  
  nn = tree_Down(e->t, n);
  
  /* instance loop */
  nn = tree_Search(e->t, nn, e->str_instance);
  while( nn >= 0 )
  {
    if ( edif_ExtractInstance(e, nn) == 0 )
      return 0;
    nn = tree_SearchNext(e->t, nn, e->str_instance);
  }

  nn = tree_Down(e->t, n);
  
  /* net loop */
  nn = tree_Search(e->t, nn, e->str_net);
  while( nn >= 0 )
  {
    if ( edif_ExtractNet(e, nn) == 0 )
      return 0;
    nn = tree_SearchNext(e->t, nn, e->str_net);
  }
  
  return 1;
}

/* expects cell */
int edif_ExtractCell(edif e, int n)
{
  if ( edif_ErrorKeyword(e, n, e->str_cell ) == 0 )
    return 0;
    
  n = tree_Down(e->t, n);
  
  e->cellName = edif_NameDef(e, n);
  if ( e->cellName == NULL )
  {
    edif_Error(e, "illegal cell in library '%s'", e->libraryName);
    return 0;
  }
  
  n = tree_Next(e->t, n);
  
  e->cellType = edif_TSStr(e, n, e->str_cellType);
  e->cell_reference = gnc_AddCell(e->nc, e->cellName, e->libraryName);
  if ( e->cell_reference < 0 )
  {
    edif_Error(e, "out of memory (cell '%s')", e->cellName);
    return 0;
  }
  
  n = tree_SearchNext(e->t, n, e->str_view);
  while( n >= 0 )
  {
    e->viewType = edif_TSStrChild(e, n, e->str_viewType);
    if ( e->viewType == e->str_NETLIST )
    {
      if ( edif_ExtractCellViewNetlist(e, n) == 0 )
        return 0;
      break;
    }
    n = tree_SearchNext(e->t, n, e->str_view);
  }
  
  return 1;
}

int edif_ExtractExternalLib(edif e, int n)
{
  if ( edif_ErrorKeyword(e, n, e->str_external ) == 0 )
    return 0;
    
  n = tree_Down(e->t, n);
  
  e->libraryName = edif_NameDef(e, n);
  if ( e->libraryName == NULL )
  {
    edif_Error(e, "illegal library");
    return 0;
  }
  
  n = tree_SearchNext(e->t, n, e->str_cell);
  while( n >= 0 )
  {
    if ( edif_ExtractCell(e, n) == 0 )
      return 0;
    n = tree_SearchNext(e->t, n, e->str_cell);
  }
  return 1;
}

int edif_ExtractLibrary(edif e, int n)
{
  if ( edif_ErrorKeyword(e, n, e->str_library ) == 0 )
    return 0;
    
  n = tree_Down(e->t, n);
  
  e->libraryName = edif_NameDef(e, n);
  if ( e->libraryName == NULL )
  {
    edif_Error(e, "illegal library");
    return 0;
  }
  
  n = tree_SearchNext(e->t, n, e->str_cell);
  while( n >= 0 )
  {
    if ( edif_ExtractCell(e, n) == 0 )
      return 0;
    n = tree_SearchNext(e->t, n, e->str_cell);
  }
  return 1;
}


int edif_ExtractTree(edif e)
{
  int n;
  if ( edif_ErrorKeyword(e, e->root_node, e->str_edif ) == 0 )
    return 0;

  n = tree_Down(e->t, e->root_node);
  
  e->edifName = edif_NameDef(e, n);
  if ( e->edifName == NULL )
  {
    edif_Error(e, "illegal edif file");
    return 0;
  }
  
  /* external loop */
  
  n = tree_SearchChild(e->t, e->root_node, e->str_external);
  while( n >= 0 )
  {
    if ( edif_ExtractExternalLib(e, n) == 0 )
      return 0;
    n = tree_SearchNext(e->t, n, e->str_external);
  }
  
  /* library loop */
  
  n = tree_SearchChild(e->t, e->root_node, e->str_library);
  while( n >= 0 )
  {
    if ( edif_ExtractLibrary(e, n) == 0 )
      return 0;
    n = tree_SearchNext(e->t, n, e->str_library);
  }
  
  
  return 1;
}

/*-- write netlist ----------------------------------------------------------*/

int edif_WriteLine(edif e, const char *fmt, ...)
{
  int i;
  va_list va;
  int c;
  for( i = 0; i < e->indent_pos*e->indent_chars; i++ )
  {
    c = putc((int)' ', e->fp);
    if ( c != ' ' )
    {
      edif_Error(e, "write error (%s)", strerror(errno));
      return 0;
    }
  }
  
  va_start(va, fmt);
  errno = 0;
  vfprintf(e->fp, fmt, va);
  if ( errno != 0 )
  {
    edif_Error(e, "write error (%s)", strerror(errno));
    return 0;
  }
  va_end(va);
  return 1;
}

static const char *edif_ws(edif e, char *s, const char *default_str)
{
  if ( s != NULL )
    return s;
  return default_str;
}

int edif_WriteCell(edif e, int pos, const char *view_name)
{
  gcell c;
  int i;
  int j;
  char *direction;
  c = gnc_GetGCELL(e->nc, pos);
  if ( edif_WriteLine(e, "(%s %s\n", e->str_cell, edif_ws(e, c->name, "?")) == 0 )
    return 0;
  e->indent_pos++;
  if ( edif_WriteLine(e, "(%s %s)\n", e->str_cellType, e->str_GENERIC) == 0 )
    return 0;
  if ( edif_WriteLine(e, "(%s %s\n", e->str_view, view_name) == 0 )
    return 0;
  e->indent_pos++;
  if ( edif_WriteLine(e, "(%s %s)\n", e->str_viewType, e->str_NETLIST) == 0 )
    return 0;

  if ( edif_WriteLine(e, "(%s\n", e->str_interface) == 0 )
    return 0;
  e->indent_pos++;

  i = -1;
  while( gnc_LoopCellPort(e->nc, pos, &i) )
  {
    switch(gnc_GetCellPortType(e->nc, pos, i))
    {
      case GPORT_TYPE_BI: direction = e->str_INOUT; break;
      case GPORT_TYPE_IN: direction = e->str_INPUT; break;
      case GPORT_TYPE_OUT: direction = e->str_OUTPUT; break;
      default: direction = e->str_OUTPUT; break;
    }
    
    if ( edif_WriteLine(e, "(%s %s (%s %s))\n", 
          e->str_port, 
          edif_ws(e, gnc_GetCellPortName(e->nc, pos, i), "?"),
          e->str_direction,
          direction) == 0 )
      return 0;
    
  }

  e->indent_pos--;
  if ( edif_WriteLine(e, ")\n") == 0 )  /* interface */
    return 0;
  
  if ( gnc_GetCellNetCnt(e->nc, pos) > 0 )
  {
    if ( edif_WriteLine(e, "(%s\n", e->str_contents) == 0 )
      return 0;
    e->indent_pos++;
      
    i = -1;
    while( gnc_LoopCellNode(e->nc, pos, &i) )
    {
      if ( gnc_GetCellNodeCellLibraryName(e->nc, pos, i) == NULL )
      {
        if ( edif_WriteLine(e, "(%s %s (%s %s (%s %s)))\n", 
          e->str_instance, 
          gnc_GetCellNodeName(e->nc, pos, i),
          e->str_viewRef,
          view_name,
          e->str_cellRef,
          gnc_GetCellNodeCellName(e->nc, pos, i)
          ) == 0 )
          return 0;
      }
      else
      {
        if ( edif_WriteLine(e, "(%s %s (%s %s (%s %s (%s %s))))\n", 
          e->str_instance, 
          gnc_GetCellNodeName(e->nc, pos, i),
          e->str_viewRef,
          view_name,
          e->str_cellRef,
          gnc_GetCellNodeCellName(e->nc, pos, i),
          e->str_libraryRef,
          gnc_GetCellNodeCellLibraryName(e->nc, pos, i)
          ) == 0 )
          return 0;
      }
    }
    
      
    i = -1;
    while( gnc_LoopCellNet(e->nc, pos, &i) )
    {
      if ( edif_WriteLine(e, "(%s %s\n", e->str_net,
              gnc_GetCellNetName(e->nc, pos, i) ) == 0 )
        return 0;
      e->indent_pos++;
      if ( edif_WriteLine(e, "(%s\n", e->str_joined,
              gnc_GetCellNetName(e->nc, pos, i) ) == 0 )
        return 0;
      e->indent_pos++;
      
      j = -1;
      while( gnc_LoopCellNetJoin(e->nc, pos, i, &j) )
      {
        if ( gnc_GetCellNetNode(e->nc, pos, i, j) < 0 )
        {
          if ( edif_WriteLine(e, "(%s %s)\n", e->str_portRef,
                  gnc_GetCellNetPortName(e->nc, pos, i, j) ) == 0 )
            return 0;
        }
        else
        {
          if ( edif_WriteLine(e, "(%s %s (%s %s))\n", 
                  e->str_portRef,
                  gnc_GetCellNetPortName(e->nc, pos, i, j),
                  e->str_instanceRef,
                  gnc_GetCellNetNodeName(e->nc, pos, i, j) ) == 0 )
            return 0;
        }
      }
      
      
      e->indent_pos--;
      if ( edif_WriteLine(e, ")\n", e->str_net) == 0 )
        return 0;
      e->indent_pos--;
      if ( edif_WriteLine(e, ")\n", e->str_net) == 0 )
        return 0;
    }

    e->indent_pos--;
    if ( edif_WriteLine(e, ")\n") == 0 ) /* contents */
      return 0;
  }
  e->indent_pos--;
  if ( edif_WriteLine(e, ")\n") == 0 )  /* view */
    return 0;
  e->indent_pos--;
  if ( edif_WriteLine(e, ")\n") == 0 )  /* cell */
    return 0;
  return 1;  
}

int edif_WriteLibrary(edif e, int lib_ref, const char *keyword, const char *view_name)
{
  char *lib_name;
  int i;
  
  lib_name = gnc_GetLibName(e->nc, lib_ref);
  if ( lib_name != NULL )
    if ( strcmp(lib_name, "__GNET_HIDDEN__") == 0 )
      return 1;

  if ( edif_WriteLine(e, "(%s %s\n", keyword, edif_ws(e, lib_name, EDIF_DEFAULT_LIB_NAME)) == 0 )
    return 0;
  e->indent_pos++;
  if ( edif_WriteLine(e, "(%s 0)\n", e->str_edifLevel) == 0 )
    return 0;
  if ( edif_WriteLine(e, "(%s (%s))\n", e->str_technology, e->str_numberDefinition) == 0 )
    return 0;
  
  i = -1;
  while( gnc_LoopLibCell(e->nc, lib_ref, &i) )
    if ( edif_WriteCell(e, i, view_name) == 0 )
      return 0;
  
  e->indent_pos--;
  if ( edif_WriteLine(e, ")\n") == 0 )
    return 0;
  return 1;
}

int edif_WriteLibraries(edif e, const char *view_name)
{
  int i;
  int nl_cnt_per_lib;  

  i = -1;
  while( gnc_LoopLib(e->nc, &i) )
  {
    nl_cnt_per_lib = gnc_GetLibraryNLCnt(e->nc, i);
    if ( nl_cnt_per_lib == 0 )
      if ( edif_WriteLibrary(e, i, e->str_external, view_name) == 0 )
        return 0;
  }

  i = -1;
  while( gnc_LoopLib(e->nc, &i) )
  {
    nl_cnt_per_lib = gnc_GetLibraryNLCnt(e->nc, i);
    if ( nl_cnt_per_lib > 0 )
      if ( edif_WriteLibrary(e, i, e->str_library, view_name) == 0 )
        return 0;
  }

  return 1;
}

int edif_WriteDesigns(edif e)
{
  int i = -1;
  
  while(gnc_LoopCell(e->nc, &i))
  {
    if ( gnc_IsTopLevelCell(e->nc, i) != 0 )
    {
      
      if ( edif_WriteLine(e, "(%s %s\n", e->str_design, gnc_GetCellName(e->nc, i)) == 0 )
        return 0;
      e->indent_pos++;

      if ( edif_WriteLine(e, "(%s %s (%s %s))\n", 
        e->str_cellRef,
        gnc_GetCellName(e->nc, i),
        e->str_libraryRef,
        gnc_GetCellLibraryName(e->nc, i)==NULL?EDIF_DEFAULT_LIB_NAME:gnc_GetCellLibraryName(e->nc, i)) == 0 )
        return 0;
      
      
      e->indent_pos--;
      if ( edif_WriteLine(e, ")\n") == 0 )
        return 0;
    }
  }
  return 1;
}


int edif_WriteEdif(edif e, const char *view_name)
{
  struct tm *ts;
  time_t tloc;
  
  if ( view_name == NULL )
    view_name = EDIF_VIEW_NAME;

  e->indent_chars = 2;
  e->indent_pos = 0;

  time(&tloc);
  ts = gmtime(&tloc);

  if ( edif_WriteLine(e, "(%s %s\n", e->str_edif, edif_ws(e, e->nc->name, "myedif")) == 0 )
    return 0;
  e->indent_pos++;
  if ( edif_WriteLine(e, "(%s 2 0 0)\n", e->str_edifVersion) == 0 )
    return 0;
  if ( edif_WriteLine(e, "(%s 0)\n", e->str_edifLevel) == 0 )
    return 0;
  if ( edif_WriteLine(e, "(%s (%s 0))\n", 
    e->str_keywordMap, e->str_keywordLevel) == 0 )
    return 0;
    
  if ( edif_WriteLine(e, "(%s\n", e->str_status) == 0 )
    return 0;
  e->indent_pos++;
  if ( edif_WriteLine(e, "(%s\n", e->str_written) == 0 )
    return 0;
  e->indent_pos++;
  if ( edif_WriteLine(e, "(%s %d %d %d %d %d %d)\n", e->str_timeStamp,
    ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday, ts->tm_hour, ts->tm_min, ts->tm_sec) == 0 )
    return 0;
  if ( edif_WriteLine(e, "(%s \"edif-gnet library\")\n", e->str_program) == 0 )
    return 0;
  e->indent_pos--;
  if ( edif_WriteLine(e, ")\n") == 0 )
    return 0;
  e->indent_pos--;
  if ( edif_WriteLine(e, ")\n") == 0 )
    return 0;
    
  if ( edif_WriteLibraries(e, view_name) == 0 )
    return 0;

  if ( edif_WriteDesigns(e) == 0 )
    return 0;

  e->indent_pos--;
  if ( edif_WriteLine(e, ")\n") == 0 )
    return 0;

  return 1;
}

int edif_WriteFile(edif e, const char *name, const char *view_name)
{
  e->fp = fopen(name, "w");
  if ( e->fp == NULL )
  {
    edif_Error(e, "file open error (%s)", strerror(errno));
    return 0;
  }
  if ( edif_WriteEdif(e, view_name) == 0 )
  {
    fclose(e->fp);
    return 0;
  }
  fclose(e->fp);
  return 1;
}

void gnc_edif_error(void *data, char *fmt, va_list va)
{
  gnc nc = (gnc)data;
  gnc_ErrorVA(nc, fmt, va);
}

int gnc_ReadEdif(gnc nc, const char *filename, int options)
{
  edif e;
  e = edif_OpenErrFn(nc, gnc_edif_error, nc);
  if ( e != NULL )
  {
    e->options = options;
    
    if ( edif_ReadFile(e, filename) != 0 )
    {
      if ( edif_ExtractTree(e) != 0 )
      {
        if ( (e->options & EDIF_OPT_ASSIGN_NAME) != 0 )
        {
          if ( gnc_SetName(nc, e->edifName) == 0 )
          {
            gnc_Error(nc, "edif import from file '%s' failed.", filename);
            edif_Close(e);
            return 0;
          }
        }
        edif_Close(e);
        return 1;
      }
    }
    edif_Close(e);
  }
  gnc_Error(nc, "edif import from file '%s' failed.", filename);
  return 0;
}

/*!
  \ingroup gncexport
  Export all netlists of all cells to a file. Use a EDIF description of the netlist.
  This file format can store the whole contents of a gnc object.
  
  \param nc A pointer to a gnc structure.
  \param filename The name of a EDIF-file.
  \param view_name A view name for the EDIF format. For further processing
    a special keyword is required (e.g. "symbol").
  
  \return 0 if an error occured.
  
  \see gnc_Open()
  \see gnc_SynthByFile()
*/
int gnc_WriteEdif(gnc nc, const char *filename, const char *view_name)
{
  edif e;
  
  gnc_Clean(nc);
  
  e = edif_OpenErrFn(nc, gnc_edif_error, nc);
  if ( e != NULL )
  {
    edif_SetErrFn(e, gnc_edif_error, nc);
    if ( edif_WriteFile(e, filename, view_name) != 0 )
    {
      edif_Close(e);
      return 1;
    }
    gnc_Error(nc, "edif export to file '%s' failed.", filename);
    edif_Close(e);
  }
  return 0;
}

/* #define EDIF_MAIN */
#ifdef EDIF_MAIN
int main()
{
  gnc nc;
  nc = gnc_Open();
  if ( nc != NULL )
  {
    if ( gnc_ReadEdif(nc, "s38584.edf", EDIF_OPT_DEFAULT) != 0 )
    {
      /*
      printf("gnet imported, memory usage: %u\n", gnc_GetMemUsage(nc));
      {
        int i = -1;
        while( gnc_LoopCell(nc, &i) )
        {
          if ( gnc_IsTopLevelCell(nc, i) != 0 )
          {
            printf("%d: %s\n", i, gnc_GetCellName(nc, i));
          }
        }
      }
      */
      puts("edif import ok");
    }
    
    if ( gnc_WriteEdif(nc, "tmp1.edif") != 0 )
    {
      printf("edif export ok\n");
    }
    gnc_Close(nc);
  }
  return 0;
}

#endif
