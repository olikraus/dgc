/*

  gl2gnet.c
  
  sis genlib import for gnet structure

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
#include <string.h>
#include "genlib.h"
#include "gnet.h"
#include "dcex.h"
#include "mwc.h"

struct _gl2gn_struct
{
  gnc nc;
  gl_type gl;
  int cell_ref;
  char library_name[GL_IDENTIFIER_MAX];
  char dcex_prefix[GL_IDENTIFIER_MAX+32];
  dcex_type dcex;
};
typedef struct _gl2gn_struct gl2gn_struct;
typedef struct _gl2gn_struct *gl2gn_type;

gl2gn_type gl2gn_Open(gnc nc, gl_type gl)
{
  gl2gn_type gg;
  gg = (gl2gn_type)malloc(sizeof(struct _gl2gn_struct));
  if ( gg != NULL )
  {
    gg->nc = nc;
    gg->gl = gl;
    strcpy(gg->library_name, "genlib");
    gg->cell_ref = -1;
    gg->dcex = dcexOpen();
    if ( gg->dcex != NULL )
    {
      return gg;
    }
    free(gg);
  }
  return NULL;
}

void gl2gn_Close(gl2gn_type gg)
{
  dcexClose(gg->dcex);
  free(gg);
}

int gl2gn_AddGateOutput(gl2gn_type gg, char *output_name)
{
  int port_ref;
  if ( gg->cell_ref < 0 )
    return 0;

  port_ref = gnc_AddCellPort(gg->nc, gg->cell_ref, GPORT_TYPE_OUT, output_name);
  if ( port_ref < 0 )
    return 0;
    
  return 1;
}

int gl2gn_AddGateCell(gl2gn_type gg)
{
  dcexn n;
  int i;
  int port_ref;
  
  /* add a new cell */
  
  gg->cell_ref = gnc_AddCell(gg->nc, gg->gl->gate_name, gg->library_name);
  if ( gg->cell_ref < 0 )
    return 0;
  
  /* apply the total cell area */
  
  gnc_SetCellArea(gg->nc, gg->cell_ref, gg->gl->gate_area);

  /* parse the genlib expression */
  
  sprintf(gg->dcex_prefix, "GENLIB (gate '%s'): ", gg->gl->gate_name);
  if ( dcexSetErrorPrefix(gg->dcex, gg->dcex_prefix) == 0 )
    return 0;
  dcexClearVariables(gg->dcex);
  n = dcexGenlibParser(gg->dcex, gg->gl->gate_fn);
  if ( n == NULL )
  {
    gnc_Error(gg->nc, "%sFunction parse error.", gg->dcex_prefix);
    return 0;
  }
  
  
  /* add all input pins */
  /* this is done before any pin is read to have the same sequence */
  /* of variables as they appear in the expression */
  
  for( i = 0; i < b_sl_GetCnt(gg->dcex->in_variables); i++ )
  {
    port_ref = gnc_AddCellPort(gg->nc, gg->cell_ref, GPORT_TYPE_IN, 
        b_sl_GetVal(gg->dcex->in_variables, i));
    if ( port_ref < 0 )
      return dcexnClose(n), 0;
  }
  

  return dcexnClose(n),1;
}

/* this is called after all pins have been added */
int gl2gn_AddGateFunction(gl2gn_type gg)
{
  int i;
  int port_ref;
  dcexn n;
  gcell cell;
  
  /* parse the genlib expression */
  
  sprintf(gg->dcex_prefix, "GENLIB (gate '%s'): ", gg->gl->gate_name);
  if ( dcexSetErrorPrefix(gg->dcex, gg->dcex_prefix) == 0 )
    return 0;
  dcexClearVariables(gg->dcex);
  n = dcexGenlibParser(gg->dcex, gg->gl->gate_fn);
  if ( n == NULL )
  {
    gnc_Error(gg->nc, "%sFunction parse error.", gg->dcex_prefix);
    return 0;
  }
  /*
  dcexShow(gg->dcex, n);
  puts("");
  */
  
  /* add all output pins */
  
  for( i = 0; i < b_sl_GetCnt(gg->dcex->out_variables); i++ )
  {
    port_ref = gnc_AddCellPort(gg->nc, gg->cell_ref, GPORT_TYPE_OUT, 
        b_sl_GetVal(gg->dcex->out_variables, i));
    if ( port_ref < 0 )
      return dcexnClose(n), 0;
  }
  
  /* prepare internal SOP memory */

  /* this must be called after all ports have been added */
  if ( gnc_InitDCUBE(gg->nc, gg->cell_ref) == 0 )
    return dcexnClose(n), 0;
  /* now, the port number must not change */

  cell = gnc_GetGCELL(gg->nc, gg->cell_ref);
  if ( dcexEvalInOutCubes(gg->dcex, cell->pi, cell->cl_on, n) == 0 )
    return dcexnClose(n), 0;

  dclMinimize(cell->pi, cell->cl_on);

  /*
  for( i = 0; i < b_sl_GetCnt(gg->dcex->in_variables); i++ )
    puts(b_sl_GetVal(gg->dcex->in_variables, i));
  dclShow(cell->pi, cell->cl_on);
  */
  
  dcexnClose(n);
  return 1;
}

int gl2gn_AddGatePin(gl2gn_type gg)
{
  int port_ref;
  gpoti p;
  
  if ( gg->cell_ref < 0 )
    return 0;

  /* input ports had been added, so just find the pin */
  
  port_ref = gnc_FindCellPort(gg->nc, gg->cell_ref, gg->gl->pin_name);
  if ( port_ref < 0 )
  {
    gnc_Error(gg->nc, "GENLIB (gate '%s'): Input pin '%s' not used inside gate expression.", gg->gl->gate_name, gg->gl->pin_name);
    return 0;
  }
  p = gnc_GetCellPortGPOTI(gg->nc, gg->cell_ref, port_ref, -1, 1);
  if ( p == NULL )
    return 0;
  
  /* obsolete
  p->input_load         = gg->gl->pin_input_load;
  p->max_load           = gg->gl->pin_max_load;
  */
  p->rise_block_delay   = gg->gl->pin_rise_block_delay;
  p->rise_fanout_delay  = gg->gl->pin_rise_fanout_delay;
  p->fall_block_delay   = gg->gl->pin_fall_block_delay;
  p->fall_fanout_delay  = gg->gl->pin_fall_fanout_delay;
  
  gnc_SetCellPortInputLoad(gg->nc, gg->cell_ref, port_ref, 
        gg->gl->pin_input_load);
  
  /* not yet implemented
  gnc_SetCellPortMaxLoad(gg->nc, gg->cell_ref, port_ref, 
        gg->gl->pin_max_load);
  */
  
  return 1;
  
}

int gl2gn_AddLatchCell(gl2gn_type gg)
{
  /* add a new cell */
  
  gg->cell_ref = gnc_AddCell(gg->nc, gg->gl->latch_name, gg->library_name);
  if ( gg->cell_ref < 0 )
    return 0;
  
  /* apply the total cell area */
  
  gnc_SetCellArea(gg->nc, gg->cell_ref, gg->gl->latch_area);

  return 1;
}

/*
  fn: GPORT_FN_D, GPORT_FN_CLR, GPORT_FN_SET
*/
int gl2gn_AddLatchInput(gl2gn_type gg, int fn, int is_inv)
{
  int port_ref;
  
  if ( gg->cell_ref < 0 )
    return 0;

  port_ref = gnc_AddCellPort(gg->nc, gg->cell_ref, GPORT_TYPE_IN, gg->gl->pin_name);
  if ( port_ref < 0 )
    return 0;
    
  gnc_SetCellPortFn(gg->nc, gg->cell_ref, port_ref, fn, is_inv);
  
  if ( fn == GPORT_FN_D )
  {
    gpoti p = gnc_GetCellPortGPOTI(gg->nc, gg->cell_ref, port_ref, -1, 1);
    if ( p == NULL )
      return 0;

    p->rise_block_delay   = gg->gl->pin_rise_block_delay;
    p->rise_fanout_delay  = gg->gl->pin_rise_fanout_delay;
    p->fall_block_delay   = gg->gl->pin_fall_block_delay;
    p->fall_fanout_delay  = gg->gl->pin_fall_fanout_delay;

    gnc_SetCellPortInputLoad(gg->nc, gg->cell_ref, port_ref, 
          gg->gl->pin_input_load);

    /* not yet implemented
    gnc_SetCellPortMaxLoad(gg->nc, gg->cell_ref, port_ref, 
          gg->gl->pin_max_load);
    */

  }
  
  return 1;
}

int gl2gn_AddLatchClock(gl2gn_type gg)
{
  int port_ref;
  gpoti p;
  
  if ( gg->cell_ref < 0 )
    return 0;

  port_ref = gnc_AddCellPort(gg->nc, gg->cell_ref, GPORT_TYPE_IN, gg->gl->latch_ctl_name);
  if ( port_ref < 0 )
    return 0;
    
  gnc_SetCellPortFn(gg->nc, gg->cell_ref, port_ref, GPORT_FN_CLK, 0);
  
  p = gnc_GetCellPortGPOTI(gg->nc, gg->cell_ref, port_ref, -1, 1);
  if ( p == NULL )
    return 0;

  p->rise_block_delay   = gg->gl->latch_ctl_rise_block_delay;
  p->rise_fanout_delay  = gg->gl->latch_ctl_rise_fanout_delay;
  p->fall_block_delay   = gg->gl->latch_ctl_fall_block_delay;
  p->fall_fanout_delay  = gg->gl->latch_ctl_fall_fanout_delay;

  gnc_SetCellPortInputLoad(gg->nc, gg->cell_ref, port_ref, 
        gg->gl->latch_ctl_input_load);
  
  /* not yet implemented
  gnc_SetCellPortMaxLoad(gg->nc, gg->cell_ref, port_ref, 
        gg->gl->latch_ctl_max_load);
  */
  
  return 1;
}

int gl2gn_AddLatchOutput(gl2gn_type gg)
{
  int port_ref;
  
  if ( gg->cell_ref < 0 )
    return 0;

  port_ref = gnc_AddCellPort(gg->nc, gg->cell_ref, GPORT_TYPE_OUT, gg->gl->latch_input);
  if ( port_ref < 0 )
    return 0;
    
  gnc_SetCellPortFn(gg->nc, gg->cell_ref, port_ref, GPORT_FN_Q, 0);
  
  return 1;
}

int gl2gn_AddLatchPin(gl2gn_type gg)
{
  /* we should better investigate the function instead of doing this heuristics  */
  if ( strcasecmp(gg->gl->pin_phase, "NONINV") == 0 )
  {
    if ( gg->gl->pin_name[0] == 'D' || gg->gl->pin_name[0] == 'd' )
    {
      return gl2gn_AddLatchInput(gg, GPORT_FN_D, 0);
    }
    if ( gg->gl->pin_name[0] == 'S' || gg->gl->pin_name[0] == 's' )
    {
      return gl2gn_AddLatchInput(gg, GPORT_FN_SET, 0);
    }
    if ( gg->gl->pin_name[0] == 'R' || gg->gl->pin_name[0] == 'r' )
    {
      return gl2gn_AddLatchInput(gg, GPORT_FN_CLR, 0);
    }
    return gl2gn_AddLatchInput(gg, GPORT_FN_D, 0);
  }
  if ( strcasecmp(gg->gl->pin_phase, "INV") == 0 )
  {
    if ( gg->gl->pin_name[0] == 'S' || gg->gl->pin_name[0] == 's' )
    {
      return gl2gn_AddLatchInput(gg, GPORT_FN_SET, 1);
    }
    if ( gg->gl->pin_name[0] == 'R' || gg->gl->pin_name[0] == 'r' )
    {
      return gl2gn_AddLatchInput(gg, GPORT_FN_CLR, 1);
    }
    return gl2gn_AddLatchInput(gg, GPORT_FN_CLR, 1);
  }
  return gl2gn_AddLatchInput(gg, GPORT_FN_D, 0);
}

int gl2gn_cb(gl_type gl, int msg, void *user_data)
{
  gl2gn_type gg = (gl2gn_type)user_data;
  
  switch(msg)
  {
    case GL_MSG_CB_START        :
      gnc_Log(gg->nc, 0, "GENLIB IMPORT: Start.");
      break;
    case GL_MSG_CB_END          :
      gnc_Log(gg->nc, 0, "GENLIB IMPORT: Done.");
      break;

    case GL_MSG_GATE_START      :
      gnc_Log(gg->nc, 0, "GENLIB IMPORT: Gate %s.", gl->gate_name);
      return gl2gn_AddGateCell(gg);
    case GL_MSG_GATE_PIN        :
      return gl2gn_AddGatePin(gg);
    case GL_MSG_GATE_END        :
      return gl2gn_AddGateFunction(gg);

    case GL_MSG_LATCH_START     :
      gnc_Log(gg->nc, 0, "GENLIB IMPORT: Latch/FF %s.", gl->latch_name);
      return gl2gn_AddLatchCell(gg);
    case GL_MSG_LATCH_END       :
      break;
    case GL_MSG_LATCH_SEQ       :
      return gl2gn_AddLatchOutput(gg);
    case GL_MSG_LATCH_PIN       :
      return gl2gn_AddLatchPin(gg);
    case GL_MSG_LATCH_CONTROL   :
      return gl2gn_AddLatchClock(gg);
      break;
    case GL_MSG_LATCH_CONSTRAINT:
      break;
  }
  return 1;
}

char *gnc_ReadGenlibLibrary(gnc nc, const char *name, const char *user_library_name)
{
  static char library_name[GL_IDENTIFIER_MAX];
  gl_type gl;
  gl2gn_type gg;
  gl = gl_Open();
  if ( gl != NULL )
  { 
    gg = gl2gn_Open(nc, gl);
    
    if ( gg != NULL )
    {
      if ( user_library_name != NULL )
      {
        strncpy(gg->library_name, user_library_name, GL_IDENTIFIER_MAX);
        gg->library_name[GL_IDENTIFIER_MAX-1] = '\0';
      }
      if ( gl_SetFile(gl, name) != 0 )
      {
        if ( gl_SetCB(gl, gl2gn_cb, gg) != 0 )
        {
          if ( gl_ReadAll(gl) != 0 )
          {
            strcpy(library_name, gg->library_name);
            gl_Close(gl);
            gl2gn_Close(gg);
            return library_name;
          }
        }
      }
      gl2gn_Close(gg);
    }
    gl_Close(gl);
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/

int IsValidGenlibFile(const char *name)
{
  gl_type gl;
  gl = gl_Open();
  if ( gl != NULL )
  {
    if ( gl_SetFile(gl, name) != 0 )
    {
      if ( gl_ReadAll(gl) != 0 )
      {
        gl_Close(gl);
        return 1;
      }
    }
    gl_Close(gl);
  }
  return 0;
}


