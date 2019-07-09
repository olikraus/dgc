/*

  sop.c
  
  assign a sum of product representation to a cell
  
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


#include "gnet.h"
#include <assert.h>
#include "mwc.h"


int gnc_AddCellCube(gnc nc, int cell_ref, dcube *c)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  if ( cell->pi == NULL )
  {
    cell->pi = pinfoOpen();
    if ( cell->pi == NULL )
    {
      gnc_Error(nc, "gnc_AddCellCube: Out of Memory (pinfoOpen).");
      return 0;
    }
    /* 25. sep 2001: this can not work... pinfoSetInCnt missing for cell->pi */
    /* assert added */
    assert(0);
  }

  if ( cell->cl_on == NULL )
  {
    if ( dclInit(&(cell->cl_on)) == 0 )
    {
      gnc_Error(nc, "gnc_AddCellCube: Out of Memory (dclInit).");
      return 0;
    }
  }
  if ( dclAdd(cell->pi, cell->cl_on, c) < 0 )
  {
    gnc_Error(nc, "gnc_AddCellCube: Out of Memory (dclAdd).");
    return 0;
  }
  return 1;
}

pinfo *gnc_GetCellPinfo(gnc nc, int cell_ref)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  return cell->pi;
}

static int gnc_UpdateCellPortsWithPinfo(gnc nc, int cell_ref, int register_width)
{
  gcell cell;
  int port_ref;
  char s[32];
  int i, j;
  
  /* FIXME: register_width is now part of the cell structure and should */
  /* have the same value... TODO: remove parameter, check the value of */
  /* the gcell structure for correctness */

  /* ok, check this for a moment... feb 2003 ok */
  assert( register_width == gnc_GetGCELL(nc, cell_ref)->register_width );
  
  cell = gnc_GetGCELL(nc, cell_ref);
  if ( cell->pi == NULL )
    return 0;
  if ( register_width < 0 )
    register_width = 0;
  
  j = 0;
  for( i = 0; i < cell->pi->in_cnt-register_width; i++ )
  {
    sprintf(s, "%s%d", "i", i);
    port_ref = gnc_SetCellPort(nc, cell_ref, j++, GPORT_TYPE_IN, s);
    if ( port_ref < 0 )
      return 0;
  }
  
  for( i = 0; i < register_width; i++ )
  {
    sprintf(s, "%s%d", "z", i);
    port_ref = gnc_SetCellPort(nc, cell_ref, j++, GPORT_TYPE_OUT, s);
    if ( port_ref < 0 )
      return 0;
  }

  for( i = 0; i < register_width; i++ )
  {
    sprintf(s, "%s%d", "zz", i);
    port_ref = gnc_SetCellPort(nc, cell_ref, j++, GPORT_TYPE_OUT, s);
    if ( port_ref < 0 )
      return 0;
  }
  
  for( i = 0; i < cell->pi->out_cnt-register_width; i++ )
  {
    sprintf(s, "%s%d", "o", i);
    port_ref = gnc_SetCellPort(nc, cell_ref, j++, GPORT_TYPE_OUT, s);
    if ( port_ref < 0 )
      return 0;
  }
  
  return 1;
}

static int gnc_UpdateCellPortNamesWithPinfo(gnc nc, int cell_ref)
{
  gcell cell;
  int i;

  cell = gnc_GetGCELL(nc, cell_ref);
  if ( cell->pi == NULL )
    return 0;

  for( i = 0; i < cell->pi->in_cnt; i++ )
    if ( pinfoGetInLabel(cell->pi, i) != NULL )
      if ( gnc_SetCellPortName(nc, cell_ref, i, 
        pinfoGetInLabel(cell->pi, i)) == 0 )
        return 0;
  
  for( i = 0; i < cell->pi->out_cnt; i++ )
    if ( pinfoGetOutLabel(cell->pi, i) != NULL )
      if ( gnc_SetCellPortName(nc, cell_ref, i+cell->pi->in_cnt, 
        pinfoGetOutLabel(cell->pi, i)) == 0 )
        return 0;

  return 1;
}

int gnc_InitDCUBE(gnc nc, int cell_ref)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);

  if ( cell->pi == NULL )
  {
    cell->pi = pinfoOpen();
    if ( cell->pi == NULL )
    {
      gnc_Error(nc, "gnc_InitDCUBE: Out of Memory (pinfoOpen).");
      return 0;
    }
    if ( pinfoSetInCnt(cell->pi, cell->in_cnt) == 0 )
    {
      gnc_Error(nc, "gnc_InitDCUBE: Out of Memory (pinfoSetInCnt).");
      return 0;
    }
    if ( pinfoSetOutCnt(cell->pi, cell->out_cnt) == 0 )
    {
      gnc_Error(nc, "gnc_InitDCUBE: Out of Memory (pinfoSetOutCnt).");
      return 0;
    }
      
  }
  if ( cell->cl_on == NULL )
    if ( dclInit(&(cell->cl_on)) == 0 )
    {
      gnc_Error(nc, "gnc_InitDCUBE: Out of Memory (dclInit).");
      return 0;
    }
  if ( cell->cl_dc == NULL )
    if ( dclInit(&(cell->cl_dc)) == 0 )
    {
      gnc_Error(nc, "gnc_InitDCUBE: Out of Memory (dclInit).");
      return 0;
    }

  dclRealClear(cell->cl_on);
  dclRealClear(cell->cl_dc);

  return 1;
}

int gnc_MinimizeCell(gnc nc, int cell_ref)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  
  /* force minimize condition */
  if ( dclSubtract(cell->pi, cell->cl_on, cell->cl_dc) == 0 )
    return 0;
  
/* attention: additional parameter in dclMinimizeDC for switching to exact/heuristic solving
            greedy = 0, 1:   0 exact minimization, 1 to minimize a greedy heuristic way */
  if ( dclMinimizeDC(cell->pi, cell->cl_on, cell->cl_dc, 0, 0) == 0 )
    return 0;
  return 1;
}


/* this function does only assign default names to the ports */
static int gnc_SetPINFO(gnc nc, int cell_ref, int in_cnt, int out_cnt, int register_width)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  /* int register_width = gnc_GetGCELL(nc, cell_ref)->register_width; */
  
  assert( register_width == gnc_GetGCELL(nc, cell_ref)->register_width );
  
  if ( gnc_InitDCUBE(nc, cell_ref) == 0 )
    return 0;
    
  if ( pinfoSetInCnt(cell->pi, in_cnt) == 0 )
  {
    gnc_Error(nc, "gnc_SetPINFO: Out of Memory (pinfoSetInCnt).");
    return 0;
  }
  if ( pinfoSetOutCnt(cell->pi, out_cnt) == 0 )
  {
    gnc_Error(nc, "gnc_SetPINFO: Out of Memory (pinfoSetOutCnt).");
    return 0;
  }

  if ( gnc_UpdateCellPortsWithPinfo(nc, cell_ref, register_width) == 0 )
    return 0;
  
  /* we can not assign the port names here, because there is no pinfo */
  /* structure */  
  
  return 1;
}

/*
  Pure combinational logic and finite state machines can be described
  by a pi/cl_on/cl_dc triple. The following formats can be handeld by
  this function:

    boolean expressions:
    
        input    |   output
      -----------+------------
        in vars  | out vars
        
        
    finite state machines
    
      DCL  | input                   |         output
      -----+---------+---------------+-------------+---------
      COLs | in vars | D-FF outputs  | D-FF inputs | out vars 
      -----+---------+---------------------------------------
      CELL | input   |            output
      
  Precondition:
    - The cell must not contain any ports
    
  This function will
    - copy pi, cl_on, cl_dc to the internal structures
    - create input, output and state ports for the cell
    - apply names to the ports, according to the information of 'pi'


  This function is intended to be a general entry point for all
  dclist based designs.
        
*/

int gnc_SetDCL(gnc nc, int cell_ref, pinfo *pi, dclist cl_on, dclist cl_dc, int register_width)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);

  /* ->register_width will be used for many fsm operations */
  gnc_GetGCELL(nc, cell_ref)->register_width = register_width;
  
  if ( gnc_GetCellPortCnt(nc, cell_ref) != 0 )
  {
    gnc_Error(nc, "gnc_SetDCL: Internal error (cell already contains ports).");
    return 0;
  }
  
  
  if ( gnc_SetPINFO(nc, cell_ref, pi->in_cnt, pi->out_cnt, register_width) == 0 )
    return 0;
  if ( cl_on != NULL )
  {
    if ( dclCopy(pi, cell->cl_on, cl_on) == 0 )
    {
      gnc_Error(nc, "gnc_SetDCL: Out of Memory (dclCopy).");
      return 0;
    }
  }
  if ( cl_dc != NULL )
  {
    if ( dclCopy(pi, cell->cl_dc, cl_dc) == 0 )
    {
      gnc_Error(nc, "gnc_SetDCL: Out of Memory (dclCopy).");
      return 0;
    }
  }
    
  /* copy any existing label names */

  if ( pinfoCopyLabels(cell->pi, pi) == 0 )
  {
    gnc_Error(nc, "gnc_SetDCL: Out of Memory (pinfoCopyLabels).");
    return 0;
  }

  /* assign the labels for the port names... */
  
  if ( gnc_UpdateCellPortNamesWithPinfo(nc, cell_ref) == 0 )  
    return 0;
  
  return 1;
}

