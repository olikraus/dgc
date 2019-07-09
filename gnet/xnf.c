/*

  xnf.c
  
  Write Xilinx Netlist Format
  
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
#include <stdio.h>
#include "mwc.h"

#define XNF_KEY_LCANET "LCANET"
#define XNF_KEY_SYM "SYM"
#define XNF_KEY_END "END"
#define XNF_KEY_PIN "PIN"
#define XNF_KEY_PIN_IN "I"
#define XNF_KEY_PIN_OUT "O"
#define XNF_KEY_DIR_IN "I"
#define XNF_KEY_DIR_OUT "O"
#define XNF_KEY_IBUF "IBUF"
#define XNF_KEY_OBUF "OBUF"
#define XNF_KEY_EXT "EXT"
#define XNF_KEY_EOF "EOF"

#define XNF_PREFIX_NODE "I"
#define XNF_PREFIX_NET "N"
#define XNF_PREFIX_BUF "BI"
#define XNF_PREFIX_BNET "BN"

#define XNF_SYM_INV "INV"
#define XNF_SYM_AND "AND"
#define XNF_SYM_NAND "NAND"
#define XNF_SYM_OR "OR"
#define XNF_SYM_NOR "NOR"
#define XNF_SYM_DFF "DFF"


char *gnc_xnf_GetSymName(gnc nc, int cell_ref, int node_ref)
{
  int node_cell_ref;
  int id;
  node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
  id = gnc_GetCellId(nc, node_cell_ref);
  if ( id == GCELL_ID_INV )
    return XNF_SYM_INV;
  if ( id == GCELL_ID_DFF_Q_HC )
    return XNF_SYM_DFF;
  if ( id >= 4 && id < 32 )
  {
    switch(id&3)
    {
      case 0: return XNF_SYM_AND;
      case 1: return XNF_SYM_NAND;
      case 2: return XNF_SYM_OR;
      case 3: return XNF_SYM_NOR;
    }
  }
  gnc_Error(nc, "Xilinx Export: Cell '%s' is not allowed.", 
    gnc_GetCellName(nc, node_cell_ref));
  return NULL;
}

int gnc_xnf_WriteRealNodeFP(gnc nc, FILE *fp, int cell_ref, int node_ref)
{
  char *type;
  char *pinname;
  int net_ref;
  int join_ref;
  int port_ref;
  
  type = gnc_xnf_GetSymName(nc, cell_ref, node_ref);
  if ( type == NULL )
    return 0;
  fprintf(fp, "%s,%s%06d,%s\n", XNF_KEY_SYM, XNF_PREFIX_NODE, node_ref, type);

  net_ref = -1;
  join_ref = -1;
  port_ref = -1;

  while( gnc_LoopCellNodeInputs(nc, cell_ref, node_ref, &port_ref, &net_ref, &join_ref) != 0 )
  {
    pinname = gnc_GetCellNetPortName(nc, cell_ref, net_ref, join_ref);
    fprintf(fp, "%s,%s,%s,%s%06d\n", 
      XNF_KEY_PIN, 
      pinname,
      XNF_KEY_DIR_IN, 
      XNF_PREFIX_NET, 
      net_ref);
  }
  
  net_ref = -1;
  join_ref = -1;
  port_ref = -1;

  while( gnc_LoopCellNodeOutputs(nc, cell_ref, node_ref, &port_ref, &net_ref, &join_ref) != 0 )
  {
    pinname = gnc_GetCellNetPortName(nc, cell_ref, net_ref, join_ref);
    fprintf(fp, "%s,%s,%s,%s%06d\n", 
      XNF_KEY_PIN, 
      pinname,
      XNF_KEY_DIR_OUT, 
      XNF_PREFIX_NET, 
      net_ref);
  }
  
  fprintf(fp, "%s\n", XNF_KEY_END);
  return 1;
}

int gnc_xnf_WriteRealNodesFP(gnc nc, FILE *fp, int cell_ref)
{
  int node_ref = -1;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
    if ( gnc_xnf_WriteRealNodeFP(nc, fp, cell_ref, node_ref) == 0 )
      return 0;
      
  return 1;
}

int gnc_xnf_WriteBufConnectionsFP(gnc nc, FILE *fp, int cell_ref)
{
  int port_ref = -1;
  int net_ref = -1;
  while( gnc_LoopCellPort(nc, cell_ref, &port_ref) != 0 )
  {
    net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, port_ref);
    if ( net_ref >= 0 )
    {
      switch(gnc_GetCellPortType(nc, cell_ref, port_ref))
      {
        case GPORT_TYPE_IN:
          fprintf(fp, "%s,%s%03d,%s\n", 
            XNF_KEY_SYM, 
            XNF_PREFIX_BUF, 
            port_ref, 
            XNF_KEY_IBUF);
          fprintf(fp, "%s,%s,%s,%s%03d,\n", 
            XNF_KEY_PIN, 
            XNF_KEY_PIN_IN,
            XNF_KEY_DIR_IN, 
            XNF_PREFIX_BNET,
            port_ref);
          fprintf(fp, "%s,%s,%s,%s%06d,\n", 
            XNF_KEY_PIN, 
            XNF_KEY_PIN_OUT,
            XNF_KEY_DIR_OUT, 
            XNF_PREFIX_NET,
            net_ref);
          fprintf(fp, "%s\n", XNF_KEY_END);
          break;
        case GPORT_TYPE_OUT:
          fprintf(fp, "%s,%s%03d,%s\n", 
            XNF_KEY_SYM, 
            XNF_PREFIX_BUF, 
            port_ref, 
            XNF_KEY_OBUF);
          fprintf(fp, "%s,%s,%s,%s%06d,\n", 
            XNF_KEY_PIN, 
            XNF_KEY_PIN_IN,
            XNF_KEY_DIR_IN, 
            XNF_PREFIX_NET,
            net_ref);
          fprintf(fp, "%s,%s,%s,%s%03d,\n", 
            XNF_KEY_PIN, 
            XNF_KEY_PIN_OUT,
            XNF_KEY_DIR_OUT, 
            XNF_PREFIX_BNET,
            port_ref);
          fprintf(fp, "%s\n", XNF_KEY_END);
          break;
      }
    }
  }
  return 1;
}

int gnc_xnf_WriteExternalConnectionFP(gnc nc, FILE *fp, int cell_ref)
{
  int port_ref = -1;
  int net_ref = -1;
  while( gnc_LoopCellPort(nc, cell_ref, &port_ref) != 0 )
  {
    net_ref = gnc_FindCellNetByPort(nc, cell_ref, -1, port_ref);
    if ( net_ref >= 0 )
    {
      switch(gnc_GetCellPortType(nc, cell_ref, port_ref))
      {
        case GPORT_TYPE_IN:
          fprintf(fp, "%s,%s%03d,%s\n", 
            XNF_KEY_EXT, 
            XNF_PREFIX_BNET,
            port_ref,
            XNF_KEY_DIR_IN);
          break;
        case GPORT_TYPE_OUT:
          fprintf(fp, "%s,%s%03d,%s\n", 
            XNF_KEY_EXT, 
            XNF_PREFIX_BNET,
            port_ref,
            XNF_KEY_DIR_OUT);
          break;
      }
    }
  }
  return 1;
}

int gnc_WriteXNFFP(gnc nc, int cell_ref, FILE *fp)
{
  fprintf(fp, "%s,6\n", XNF_KEY_LCANET);
  if ( gnc_xnf_WriteRealNodesFP(nc, fp, cell_ref) == 0 )
    return 0;
  if ( gnc_xnf_WriteBufConnectionsFP(nc, fp, cell_ref) == 0 )
    return 0;
  if ( gnc_xnf_WriteExternalConnectionFP(nc, fp, cell_ref) == 0 )
    return 0;
  fprintf(fp, "%s\n", XNF_KEY_EOF);
  return 1;
}

/*!
  \ingroup gncexport
  Export the netlist of a cell to a file. Use the Xilinx netlist format.
  
  \pre 
    - The cell \a cell_ref must have a valid netlist.
    - The netlist must not depend on any other cells except basic building
      blocks. This is ensured by applying flag \c GNC_HL_OPT_FLATTEN
      to a synthesis procedure or by calling gnc_FlattenCell().
  
  
  \param nc A pointer to a gnc structure.
  \param cell_ref The handle of a cell (for example the value returned by 
    gnc_SynthByFile()).
  \param filename The name of a XNF-file.
  
  \return 0 if an error occured.
  
  \see gnc_Open()
  \see gnc_SynthByFile()
  \see gnc_FlattenCell()
*/
int gnc_WriteXNF(gnc nc, int cell_ref, const char *filename)
{
  int ret = 0;
  FILE *fp;
  fp = fopen(filename, "w");
  if ( fp != NULL )
  {
    ret = gnc_WriteXNFFP(nc, cell_ref, fp);
    fclose(fp);
  }
  return ret;
}

