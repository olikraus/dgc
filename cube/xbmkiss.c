/*

  xbmkiss.c

  finite state machine for eXtended burst mode machines
  kiss export

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

#include "xbm.h"
#include <stdio.h>

int xbm_write_kiss_line(xbm_type x, FILE *fp, dcube *c, int st1, int st2)
{
  int i;

  for( i = 0; i < x->inputs; i++ )
    fprintf(fp, "%c", "x01-"[dcGetIn(c, i)]);
    
  fprintf(fp, " %s", xbm_GetStNameStr(x, st1));
  fprintf(fp, " %s ", xbm_GetStNameStr(x, st2));

  for( i = 0; i < x->outputs; i++ )
    fprintf(fp, "%c", "01"[dcGetOut(&(xbm_GetSt(x, st2)->out), i)]);

  fprintf(fp, "\n");
  return 1;
}

int xbm_write_kiss_tr(xbm_type x, FILE *fp, int tr_pos)
{
  int i;
  dcube *c;
  int st1, st2;
  c = &(xbm_GetTr(x, tr_pos)->end_cond);
  st1 = xbm_GetTrSrcStPos(x, tr_pos);
  st2 = xbm_GetTrDestStPos(x, tr_pos);
  
  return xbm_write_kiss_line(x, fp, c, st1, st2);
}

int xbm_write_kiss_st(xbm_type x, FILE *fp, int st_pos)
{
  int i, cnt = dclCnt(xbm_GetSt(x, st_pos)->in_self_cl);
  dcube *c;
  for( i = 0; i < cnt; i++ )
  {
    c = dclGet(xbm_GetSt(x, st_pos)->in_self_cl, i);
    if ( xbm_write_kiss_line(x, fp, c, st_pos, st_pos) == 0 )
      return 0;
  }
  return 1;
}

int xbm_WriteKISSFP(xbm_type x, FILE *fp)
{
  int i;
  
  fprintf(fp, ".i %d\n", x->inputs);
  fprintf(fp, ".o %d\n", x->outputs);
  
  i = -1;
  while( xbm_LoopTr(x, &i) != 0 )
    if ( xbm_write_kiss_tr(x, fp, i) == 0 )
      return 0;

  i = -1;
  while( xbm_LoopSt(x, &i) != 0 )
    if ( xbm_write_kiss_st(x, fp, i) == 0 )
      return 0;
  
  return 1;
}

int xbm_WriteKISS(xbm_type x, const char *name)
{
  FILE *fp;
  int ret;
  fp = fopen(name, "w");
  if ( fp == NULL )
    return 0;
  xbm_Log(x, 5, "XBM: Write KISS file '%s'.", name);
  ret = xbm_WriteKISSFP(x, fp);
  fclose(fp);
  return ret;
}
