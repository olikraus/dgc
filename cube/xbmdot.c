/*

  xbmdot.c

  finite state machine for eXtended burst mode machines
  dot export

  Copyright (C) 2002 Hua Bao (baohua_2001@yahoo.com)

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
 
#include "xbm.h"
#include <stdio.h>

int xbm_write_dot_line(xbm_type x, FILE *fp, dcube *level_cube,dcube *edge_cube, int st1, int st2)
{
  int i;
  int pos_ref;
  char var_name[32];
      
  fprintf(fp, " %s", xbm_GetStNameStr(x, st1));
  fprintf(fp, "  -> %s ", xbm_GetStNameStr(x, st2));


  fprintf(fp," [ label =\"");
  
  for( i = 0; i < x->inputs; i++ )
    {
     pos_ref = dcGetIn(edge_cube,i);
     strcpy(var_name,xbm_GetVarNameStrByIndex(x,XBM_DIRECTION_IN,i));
     
     if (pos_ref == 1)
     fprintf(fp,"  %s-",var_name);
     
     if (pos_ref == 2)
     fprintf(fp,"  %s+",var_name);
    }    
 
  for( i = 0; i < x->inputs; i++ )
    {
     pos_ref = dcGetIn(level_cube,i);
     strcpy(var_name,xbm_GetVarNameStrByIndex(x,XBM_DIRECTION_IN,i));
     
     if (pos_ref == 1)
     fprintf(fp,"  [%s-]",var_name);
     
     if (pos_ref == 2)
     fprintf(fp,"  [%s+]",var_name);
     
     if (pos_ref == 3)
     fprintf(fp,"  %s*",var_name);
    } 
  
   
  fprintf(fp,"  |  ");     
  for( i = 0; i < x->outputs; i++ )
  {
   pos_ref = dcGetIn(level_cube, i+ x->inputs );
   strcpy(var_name,xbm_GetVarNameStrByIndex(x,XBM_DIRECTION_OUT,i));
        
     if (pos_ref == 1)
     fprintf(fp,"  %s-",var_name);
     
     if (pos_ref == 2)
     fprintf(fp,"  %s+",var_name);
    } 
  
     
  fprintf(fp, " \" ]\n");
  return 1;
}

int xbm_write_dot_tr(xbm_type x, FILE *fp, int tr_pos)
{
  int i;
  dcube *level_cube,*edge_cube;
  int st1, st2;
  
  level_cube = &(xbm_GetTr(x, tr_pos)->level_cond);
  edge_cube  = &(xbm_GetTr(x, tr_pos)->edge_cond);
  st1 = xbm_GetTrSrcStPos(x, tr_pos);
  st2 = xbm_GetTrDestStPos(x, tr_pos);
  
  return xbm_write_dot_line(x, fp, level_cube, edge_cube, st1, st2);
}



int xbm_WriteDOTFP(xbm_type x, FILE *fp)
{
  int i;
  fprintf(fp,"digraph G {\n");
  
  i = -1;
  while( xbm_LoopTr(x, &i) != 0 )
    if ( xbm_write_dot_tr(x, fp, i) == 0 )
      return 0;
 
  fprintf(fp,"}\n");
  return 1;
}

int xbm_WriteDOT(xbm_type x, const char *name)
{
  FILE *fp;
  int ret;
  fp = fopen(name, "w");
  if ( fp == NULL )
    return 0;
  xbm_Log(x, 5, "XBM: Write dot file '%s'.", name);
  ret = xbm_WriteDOTFP(x, fp);
  fclose(fp);
  return ret;
}
