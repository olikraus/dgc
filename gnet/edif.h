/*

  edif.h

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

#ifndef _EDIF_H
#define _EDIF_H

#include <stddef.h>
#include <stdio.h>
#include "tree.h"
#include "gnet.h"

#define EDIF_OPT_READ_EXTERNAL       0x01
#define EDIF_OPT_READ_LIBRARY_CELLS  0x02
#define EDIF_OPT_READ_LIBRARY_NETS   0x04
#define EDIF_OPT_READ_LIBRARY        (EDIF_OPT_READ_LIBRARY_CELLS|EDIF_OPT_READ_LIBRARY_NETS)
#define EDIF_OPT_ASSIGN_NAME         0x08

#define EDIF_OPT_DEFAULT 0x0ffff
#define EDIF_OPT_ALL     0x0ffff

#define EDIF_DEFAULT_LIB_NAME "nonamelib"
#define EDIF_VIEW_NAME "netlistview"

struct _edif_struct
{
  tree t;
  gnc nc;
  int is_gnc_allocated;
  
  int root_node;
  
  int options;
  
  /* constant strings */
  
  char *str_edif;
  char *str_external;
  char *str_library;
  char *str_cell;
  char *str_cellType;
  char *str_view;
  char *str_viewType;
  char *str_NETLIST;

  char *str_interface;
  char *str_port;
  char *str_direction;
  char *str_INPUT;
  char *str_OUTPUT;
  char *str_INOUT;
  char *str_rename;
  
  char *str_contents;
  char *str_instance;
  char *str_viewRef;
  char *str_cellRef;
  char *str_libraryRef;
  char *str_net;
  char *str_joined;
  char *str_portRef;
  char *str_instanceRef;
  
  /* additional keywords for the writer */
  char *str_edifVersion;
  char *str_edifLevel;
  char *str_keywordMap;
  char *str_keywordLevel;
  char *str_status;
  char *str_written;
  char *str_timeStamp;
  char *str_program;
  char *str_technology;
  char *str_numberDefinition;
  char *str_GENERIC;
  char *str_design;
  
  /* strings during extraction */
  
  char *edifName;
  char *libraryName;
  char *cellName;
  char *cellType;
  int cell_reference;   /* cell reference, created by gnc_AddCell() */
  char *viewName;
  char *viewType;
  char *portName;
  char *direction;
  char *instanceName;
  
  char *viewRef;
  char *cellRef;
  char *libraryRef;
  int node_reference;
  char *portRef;
  char *instanceRef;
  int port_reference;
  int instance_reference;
  char *netName;
  int net_reference;
  
  /* write: filepointer */
  FILE *fp;
  int indent_pos;
  int indent_chars;
  
  /* error handling */
  void (*err_fn)(void *data, char *fmt, va_list va);
  void *err_data;
};
typedef struct _edif_struct *edif;

/*
  import an edif file into a gnc structure.
*/
int gnc_ReadEdif(gnc nc, const char *filename, int options);

/* write the net as an edif file */
int gnc_WriteEdif(gnc nc, const char *filename, const char *view_name);


#endif


