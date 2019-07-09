/*

  srect.h
  
  rectangles for synthesis

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

#ifndef _SRECT_H
#define _SRECT_H

#include "rect.h"


struct _srect_struct
{
  rect r;
  int p_net_ref; /* -1 if the output has not been assigned */
  int n_net_ref; /* -1 if the output has not been assigned */
};
typedef struct _srect_struct srect;

int srInit(rmatrix rm, srect *r);
void srDestroy(srect *r);
void srCopy(rmatrix rm, srect *dest, srect *src);

struct _srlist_struct
{
  srect *list;
  int max;
  int cnt;
};
typedef struct _srlist_struct *srlist;

#define srlCnt(srl) ((srl)->cnt)
/* #define srlGet(srl,pos) ((srl)->list+(pos)) */

srect *srlGet(srlist srl, int pos);

srlist srlOpen(void);
void srlClear(srlist srl);
void srlClose(srlist srl);
int srlExpandTo(rmatrix rm, srlist srl, int max);
int srlAddEmpty(rmatrix rm, srlist srl);          /* returns pos or -1 */
int srlAddRect(rmatrix rm, srlist srl, rect *r);  /* returns pos or -1 */

#endif /* _SRECT_H */
