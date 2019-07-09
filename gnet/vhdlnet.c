
/*

  vhdlnet.c

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
  

  LIBRARY ieee;
  USE ieee.std_logic_1164.all;
  
  ENTITY e IS
      PORT( x1, x2, x3, x4: IN STD_LOGIC; y0: OUT STD_LOGIC );
  END e

  ARCHITECTURE a OF e IS
    SIGNAL internal_1: STD_LOGIC;
    SIGNAL internal_2: STD_LOGIC;
    ...
  
    COMPONENT and2
      PORT( a, b: IN STD_LOGIC; y: OUT STD_LOGIC );
    END COMPONENT;

    COMPONENT and3
      PORT( a, b, c: IN STD_LOGIC; y: OUT STD_LOGIC );
    END COMPONENT;
    ...
    
  BEGIN
    C0: and3 PORT MAP(a => x1, b => x2, c => x3, y => internal_1);
    C1: and2 PORT MAP(a => internal_1, b => x4, y => y0 );
  
  END a;
    
    
*/

#include "gnet.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mwc.h"

#define GNC_VHDL_INDENT_SIZE 2

struct _vhdlnet_struct
{
  char *key_begin;
  char *key_end;
  char *key_is;
  char *key_of;
  char *key_signal;
  char *key_in;
  char *key_out;
  char *key_component;
  char *key_architecture;
  char *key_entity;
  char *key_port;
  char *key_map;
  char *key_library;
  char *key_use;
  char *key_open;
  char *key_signal_prefix;
  char *key_signal_type;
  char *key_component_prefix;
  
  FILE *fp;
  gnc nc;
};
typedef struct _vhdlnet_struct *vhdlnet;

#define VN_ASSIGN_STR(vn, s)\
  (((vn)->key_##s = strdup(#s))==NULL?0:1)

#define VN_ASSIGN_STR2(vn, s, n)\
  (((vn)->key_##s = strdup(n))==NULL?0:1)

static int vnInit(vhdlnet vn)
{
  int is_ok = 1;
  is_ok &= VN_ASSIGN_STR(vn, begin);
  is_ok &= VN_ASSIGN_STR(vn, end);
  is_ok &= VN_ASSIGN_STR(vn, is);
  is_ok &= VN_ASSIGN_STR(vn, of);
  is_ok &= VN_ASSIGN_STR(vn, signal);
  is_ok &= VN_ASSIGN_STR(vn, in);
  is_ok &= VN_ASSIGN_STR(vn, out);
  is_ok &= VN_ASSIGN_STR(vn, component);
  is_ok &= VN_ASSIGN_STR(vn, architecture);
  is_ok &= VN_ASSIGN_STR(vn, entity);
  is_ok &= VN_ASSIGN_STR(vn, port);
  is_ok &= VN_ASSIGN_STR(vn, map);
  is_ok &= VN_ASSIGN_STR(vn, library);
  is_ok &= VN_ASSIGN_STR(vn, use);
  is_ok &= VN_ASSIGN_STR(vn, open);

  is_ok &= VN_ASSIGN_STR2(vn, signal_prefix, "l");
  is_ok &= VN_ASSIGN_STR2(vn, signal_type, "std_logic");
  is_ok &= VN_ASSIGN_STR2(vn, component_prefix, "c");
  return is_ok;
}

#define VN_FREE_STR(vn, s)\
  if ( ((vn)->key_##s) != NULL ) free((vn)->key_##s)
  
static void vnClose(vhdlnet vn)
{
  VN_FREE_STR(vn, begin);
  VN_FREE_STR(vn, end);
  VN_FREE_STR(vn, is);
  VN_FREE_STR(vn, of);
  VN_FREE_STR(vn, signal);
  VN_FREE_STR(vn, in);
  VN_FREE_STR(vn, out);
  VN_FREE_STR(vn, component);
  VN_FREE_STR(vn, architecture);
  VN_FREE_STR(vn, entity);
  VN_FREE_STR(vn, port);
  VN_FREE_STR(vn, map);
  VN_FREE_STR(vn, library);
  VN_FREE_STR(vn, use);
  VN_FREE_STR(vn, open);

  VN_FREE_STR(vn, signal_prefix);
  VN_FREE_STR(vn, signal_type);
  VN_FREE_STR(vn, component_prefix);
  free(vn);
}

static void vhdlnet_strupr(char *s)
{
  while( *s != '\0' )
  {
    *s = toupper((int)(unsigned char)*s);
    s++;
  }
}

void vnUpperCaseKeywords(vhdlnet vn)
{
  vhdlnet_strupr(vn->key_begin);
  vhdlnet_strupr(vn->key_end);
  vhdlnet_strupr(vn->key_is);
  vhdlnet_strupr(vn->key_of);
  vhdlnet_strupr(vn->key_signal);
  vhdlnet_strupr(vn->key_in);
  vhdlnet_strupr(vn->key_out);
  vhdlnet_strupr(vn->key_component);
  vhdlnet_strupr(vn->key_architecture);
  vhdlnet_strupr(vn->key_entity);
  vhdlnet_strupr(vn->key_port);
  vhdlnet_strupr(vn->key_map);
  vhdlnet_strupr(vn->key_library);
  vhdlnet_strupr(vn->key_use);
  vhdlnet_strupr(vn->key_open);
  vhdlnet_strupr(vn->key_signal_type);
}


void vnUpperCaseIdentifier(vhdlnet vn)
{
  vhdlnet_strupr(vn->key_signal_prefix);
  vhdlnet_strupr(vn->key_component_prefix);
}

static vhdlnet vnOpen()
{
  vhdlnet vn;
  vn = (vhdlnet)malloc(sizeof(struct _vhdlnet_struct));
  if ( vn != NULL )
  {
    vn->fp = NULL;
    if ( vnInit(vn) != 0 )
    {
      return vn;
    }
    vnClose(vn);
  }
  return NULL;
}


int vn_WriteVHDLSpace(vhdlnet vn, int spaces)
{
  while(spaces-- > 0)
    if ( fprintf(vn->fp, " ") < 0 )
      return 0;
  return 1;
}

int vn_WriteVHDLPortName(vhdlnet vn, int cell_ref, int port_ref)
{
  if ( fprintf(vn->fp, "%s", gnc_GetCellPortName(vn->nc, cell_ref, port_ref)) < 0 )
    return 0;
  return 1;
}


int vn_WriteVHDLInternalSignalName(vhdlnet vn, int cell_ref, int net_ref)
{
  if ( fprintf(vn->fp, "%s%06d", vn->key_signal_prefix, net_ref) < 0 )
    return 0;
  return 1;
}

int vn_WriteVHDLInternalSignalDefinition(vhdlnet vn, int cell_ref, int net_ref)
{
  if ( vn_WriteVHDLSpace(vn, 1*GNC_VHDL_INDENT_SIZE) == 0 )
    return 0;
  if ( fprintf(vn->fp, "%s ", vn->key_signal) < 0 )
    return 0;
  if ( vn_WriteVHDLInternalSignalName(vn, cell_ref, net_ref) == 0 )
    return 0;
  if ( fprintf(vn->fp, ": %s;", vn->key_signal_type) < 0 )
    return 0;
  if ( fprintf(vn->fp, "\n") < 0 )
    return 0;
  return 1;
}

int vn_WriteVHDLComponent(vhdlnet vn, int cell_ref, int node_ref)
{
  int nc_ref;
  int port_ref;
  int is_first;
  nc_ref = gnc_GetCellNodeCell(vn->nc, cell_ref, node_ref);
  vn_WriteVHDLSpace(vn, 1*GNC_VHDL_INDENT_SIZE);
  fprintf(vn->fp, "%s %s", vn->key_component, gnc_GetCellName(vn->nc, nc_ref));
  fprintf(vn->fp, "\n");
  vn_WriteVHDLSpace(vn, 1*GNC_VHDL_INDENT_SIZE);
  
  fprintf(vn->fp, "%s(", vn->key_port);
  fprintf(vn->fp, "\n");
  is_first = 1;
  port_ref = -1;
  while( gnc_LoopCellPort(vn->nc, nc_ref, &port_ref) != 0 )
  {
    switch(gnc_GetCellPortType(vn->nc, nc_ref, port_ref))
    {
      case GPORT_TYPE_IN:
        if ( is_first == 0 )
        {
          fprintf(vn->fp, ";");
          fprintf(vn->fp, "\n");
        }
        is_first = 0;
        vn_WriteVHDLSpace(vn, 2*GNC_VHDL_INDENT_SIZE);
        fprintf(vn->fp, "%s: %s %s", gnc_GetCellPortName(vn->nc, nc_ref, port_ref), vn->key_in, vn->key_signal_type);
        break;
      case GPORT_TYPE_OUT:
        if ( is_first == 0 )
        {
          fprintf(vn->fp, ";");
          fprintf(vn->fp, "\n");
        }
        is_first = 0;
        vn_WriteVHDLSpace(vn, 2*GNC_VHDL_INDENT_SIZE);
        fprintf(vn->fp, "%s: %s %s", gnc_GetCellPortName(vn->nc, nc_ref, port_ref), vn->key_out, vn->key_signal_type);
        break;
    }
  }
  fprintf(vn->fp, ");");
  fprintf(vn->fp, "\n");
  vn_WriteVHDLSpace(vn, 1*GNC_VHDL_INDENT_SIZE);
  fprintf(vn->fp, "%s %s;", vn->key_end, vn->key_component);
  fprintf(vn->fp, "\n");
  return 1;
}

int vn_WriteVHDLInstance(vhdlnet vn, int cell_ref, int node_ref)
{
  int nc_ref;
  int net_ref;
  int port_ref;
  int is_first;
  nc_ref = gnc_GetCellNodeCell(vn->nc, cell_ref, node_ref);
  if ( nc_ref < 0 )
    return 0;
  
  if ( vn_WriteVHDLSpace(vn, 1*GNC_VHDL_INDENT_SIZE) == 0 )
    return 0;
  if ( fprintf(vn->fp, "%s%06d: %s %s %s(", 
      vn->key_component_prefix, 
      node_ref, 
      gnc_GetCellName(vn->nc, nc_ref), 
      vn->key_port, 
      vn->key_map) < 0 )
    return 0;
  
  port_ref = -1;
  is_first = 1;
  while( gnc_LoopCellPort(vn->nc, nc_ref, &port_ref) != 0 )
  {
    if ( is_first == 0 )
      if ( fprintf(vn->fp, ", ") < 0 )
        return 0;
    is_first = 0;
    net_ref = gnc_FindCellNetByPort(vn->nc, cell_ref, node_ref, port_ref);
    if ( net_ref >= 0 )
    {
      if ( fprintf(vn->fp, "%s => ", gnc_GetCellPortName(vn->nc, nc_ref, port_ref)) < 0 )
        return 0;
#ifdef OLD_WRONG_CODE        
      join_ref = gnc_FindCellNetJoin(vn->nc, cell_ref, net_ref, -1, -1);
      if ( join_ref < 0 )
      {
        /* connected to an internal net */
        if ( vn_WriteVHDLInternalSignalName(vn, cell_ref, net_ref) == 0 )
          return 0;
      }
      else
      {
        parent_port_ref = gnc_GetCellNetPort(vn->nc, cell_ref, net_ref, join_ref);
        vn_WriteVHDLPortName(vn, cell_ref, parent_port_ref);
      }
#else
      if ( vn_WriteVHDLInternalSignalName(vn, cell_ref, net_ref) == 0 )
        return 0;
#endif 
    }
    else
    {
      if ( fprintf(vn->fp, "%s => %s", gnc_GetCellPortName(vn->nc, nc_ref, port_ref), vn->key_open) < 0 )
        return 0;
    }
  }  
  if ( fprintf(vn->fp, ");") < 0 )
    return 0;
  if ( fprintf(vn->fp, "\n") < 0 )
    return 0;
  return 1;
}

int vn_WriteVHDLArchitecture(vhdlnet vn, int cell_ref, const char *entity, const char *arch)
{
  int net_ref;
  int node_ref;
  int nc_ref;
  int join_ref;

  fprintf(vn->fp, "%s %s %s %s %s", vn->key_architecture, arch, vn->key_of, entity, vn->key_is);
  fprintf(vn->fp, "\n");


  /* 23. aug 2001: every net has its own signal; VHDL-LRM should be happy now, */
  /* my code seems also to be a little bit more simple, but the output is */
  /* harder to read... */
  net_ref = -1;
  while( gnc_LoopCellNet(vn->nc, cell_ref, &net_ref) != 0 )
  {
#ifdef OLD_WRONG_CODE        
    if ( gnc_FindCellNetJoin(vn->nc, cell_ref, net_ref, -1, -1) < 0 )
#endif
      vn_WriteVHDLInternalSignalDefinition(vn, cell_ref, net_ref);
  }
    
  gnc_ClearAllCellsFlag(vn->nc);
  node_ref = -1;
  while( gnc_LoopCellNode(vn->nc, cell_ref, &node_ref) != 0 )
  {
    nc_ref = gnc_GetCellNodeCell(vn->nc, cell_ref, node_ref);
    if ( gnc_IsCellFlag(vn->nc, nc_ref) == 0 )
    {
      vn_WriteVHDLComponent(vn, cell_ref, node_ref);
      gnc_SetCellFlag(vn->nc, nc_ref);
    }
  }
  
  fprintf(vn->fp, "%s", vn->key_begin);
  fprintf(vn->fp, "\n");
  
  node_ref = -1;
  while( gnc_LoopCellNode(vn->nc, cell_ref, &node_ref) != 0 )
    if ( vn_WriteVHDLInstance(vn, cell_ref, node_ref) == 0 )
      return 0;
  
  
#ifdef OLD_WRONG_CODE        
  /* 23. aug 2001: this code is now obsolete and replaced by the loop below */

  /* do not forget to write net's that do not contain any nodes */

  net_ref = -1;
  while( gnc_LoopCellNet(vn->nc, cell_ref, &net_ref) != 0 )
  {
    join_ref = -1;
    is_pure_in_out_connection = 1;
    input_join_ref = -1;
    while( gnc_LoopCellNetJoin(vn->nc, cell_ref, net_ref, &join_ref) != 0 )
    {
      if ( gnc_GetCellNetNode(vn->nc, cell_ref, net_ref, join_ref) >= 0 )
      {
        is_pure_in_out_connection = 0;
      }
      if ( gnc_GetCellNetPortType(vn->nc, cell_ref, net_ref, join_ref) == GPORT_TYPE_IN )
      {
        input_join_ref = join_ref;
      }
    }
    
    if ( is_pure_in_out_connection != 0 && input_join_ref >= 0 )
    {
      while( gnc_LoopCellNetJoin(vn->nc, cell_ref, net_ref, &join_ref) != 0 )
      {
        if ( input_join_ref != join_ref )
        {
          if ( vn_WriteVHDLSpace(vn, 1*GNC_VHDL_INDENT_SIZE) == 0 )
            return 0;
          fprintf(vn->fp, "%s <= %s;\n",
            gnc_GetCellNetPortName(vn->nc, cell_ref, net_ref, join_ref),
            gnc_GetCellNetPortName(vn->nc, cell_ref, net_ref, input_join_ref));
        }
      }
    }
  }
#else
  /* 23. aug 2001: assign all inputs and outputs to the internal names */

  /* with all nets */
  net_ref = -1;
  while( gnc_LoopCellNet(vn->nc, cell_ref, &net_ref) != 0 )
  {
    /* with all join_refs of each net */
    join_ref = -1;
    while( gnc_LoopCellNetJoin(vn->nc, cell_ref, net_ref, &join_ref) != 0 )
    {
      if ( gnc_GetCellNetNode(vn->nc, cell_ref, net_ref, join_ref) < 0 )
      {
        switch(gnc_GetCellNetPortType(vn->nc, cell_ref, net_ref, join_ref))
        {
          case GPORT_TYPE_IN:
            if ( vn_WriteVHDLSpace(vn, 1*GNC_VHDL_INDENT_SIZE) == 0 )
              return 0;
            if ( vn_WriteVHDLInternalSignalName(vn, cell_ref, net_ref) == 0 )
              return 0;
            fprintf(vn->fp, " <= %s;\n",
              gnc_GetCellNetPortName(vn->nc, cell_ref, net_ref, join_ref));
            break;
          case GPORT_TYPE_OUT:
            if ( vn_WriteVHDLSpace(vn, 1*GNC_VHDL_INDENT_SIZE) == 0 )
              return 0;
            fprintf(vn->fp, "%s <= ",
              gnc_GetCellNetPortName(vn->nc, cell_ref, net_ref, join_ref));
            if ( vn_WriteVHDLInternalSignalName(vn, cell_ref, net_ref) == 0 )
              return 0;
            fprintf(vn->fp, ";\n");
            break;
          default:
            /* can there ever be any other type??? */
            /* ... not yet ... */
            break;
        }
      }
    }
  }
#endif
  

  if ( fprintf(vn->fp, "%s %s;", vn->key_end, arch) < 0 )
    return 0;
  if ( fprintf(vn->fp, "\n") < 0 )
    return 0;
  
  return 1;
}

int vn_WriteVHDLUsePackage(vhdlnet vn)
{
  fprintf(vn->fp, "%s ieee;\n", vn->key_library);
  fprintf(vn->fp, "%s ieee.std_logic_1164.all;\n", vn->key_use);
  fprintf(vn->fp, "\n");
  return 1;
}

int vn_WriteVHDLUseLibrary(vhdlnet vn, const char *lib_name)
{
  if ( lib_name != NULL )
  {
    if ( lib_name[0] != '\0' )
    {
      fprintf(vn->fp, "%s %s;\n", vn->key_library, lib_name);
      fprintf(vn->fp, "%s %s.components.all;\n", vn->key_use, lib_name);
      fprintf(vn->fp, "\n");
    }
  }
  return 1;
}

int vn_WriteVHDLEntity(vhdlnet vn, int cell_ref, const char *entity)
{
  int port_ref = -1;
  int is_first = 1;
  fprintf(vn->fp, "%s %s %s", vn->key_entity, entity, vn->key_is);
  fprintf(vn->fp, "\n");

  fprintf(vn->fp, "  %s(", vn->key_port);
  
  while( gnc_LoopCellPort(vn->nc, cell_ref, &port_ref) != 0 )
  {
    if ( is_first == 0 )
      fprintf(vn->fp, ";\n    ");
    is_first = 0;
      
    vn_WriteVHDLPortName(vn, cell_ref, port_ref);
    fprintf(vn->fp, " : ");
    switch(gnc_GetCellPortType(vn->nc, cell_ref, port_ref))
    {
      case GPORT_TYPE_IN:
        fprintf(vn->fp, "IN");
        break;
      case GPORT_TYPE_OUT:
        fprintf(vn->fp, "OUT");
        break;
      case GPORT_TYPE_BI:
        fprintf(vn->fp, "INOUT");
        break;
    }
    fprintf(vn->fp, " %s", vn->key_signal_type);
  }
  
  fprintf(vn->fp, ");");
  fprintf(vn->fp, "\n");
  
  fprintf(vn->fp, "%s %s;", vn->key_end, entity);
  fprintf(vn->fp, "\n");
  fprintf(vn->fp, "\n");
  return 1;
}

/*!
  \ingroup gncexport
  Export the netlist of a cell to a file. Use a VHDL description of the netlist.
  
  
  \pre 
    - The cell \a cell_ref must have a valid netlist.
    - The netlist must not depend on any other cells except basic building
      blocks. This is ensured by applying flag \c GNC_HL_OPT_FLATTEN
      to a synthesis procedure or by calling gnc_FlattenCell().
  
  
  \param nc A pointer to a gnc structure.
  \param cell_ref The handle of a cell (for example the value returned by 
    gnc_SynthByFile()).
  \param filename The name of a VHDL-file.
  \param entity The name of the entity for the VHDL description
  \param arch The name of the architecture for the VHDL description.
  \param opt A logical OR of the following flags: 
    \c GNC_WRITE_VHDL_UPPER_KEY, \c GNC_WRITE_VHDL_UPPER_ID 
  \param wait_ns should be 0.
  \param lib_name The name of an library for a use statement that is placed
    before the entity of the VHDL file.
  
  \return 0 if an error occured.
  
  \see gnc_Open()
  \see gnc_SynthByFile()
  \see gnc_FlattenCell()
*/
int gnc_WriteVHDL(gnc nc, int cell_ref, const char *filename, 
  const char *entity, const char *arch, int opt, int wait_ns, const char *lib_name)
{
  FILE *fp;
  vhdlnet vn;
  int ret;
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  
  if ( cell_ref < 0 || filename == NULL )
    return 0;

  gnc_DelCellEmptyNet(nc, cell_ref);
  
  fp = fopen(filename, "w");
  if ( fp != NULL )
  {
    vn = vnOpen();
    if ( vn != NULL )
    {
      vn->fp = fp;
      vn->nc = nc;
      if ( (opt & GNC_WRITE_VHDL_UPPER_KEY) == GNC_WRITE_VHDL_UPPER_KEY )
        vnUpperCaseKeywords(vn);
      if ( (opt & GNC_WRITE_VHDL_UPPER_ID) == GNC_WRITE_VHDL_UPPER_ID )
        vnUpperCaseIdentifier(vn);
      vn_WriteVHDLUsePackage(vn);
      vn_WriteVHDLUseLibrary(vn, lib_name);
      vn_WriteVHDLEntity(vn, cell_ref, entity);
      ret = vn_WriteVHDLArchitecture(vn, cell_ref, entity, arch);
      /*
      if ( wait_ns >= 0 && cell->pi != NULL && cell->cl_on != NULL )
        dclWriteVHDLVecTBFP(cell->pi, cell->cl_on, entity, wait_ns, fp);
      */
      if ( wait_ns >= 0 && cell->pi != NULL && cell->cl_on != NULL )
        dclWriteVHDLSOPTBFP(cell->pi, cell->cl_on, entity, wait_ns, fp);

      vnClose(vn);
    }
    fclose(fp);
  }
  return ret;
}

