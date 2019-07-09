/*

  srn.h
    
  synthesis rectangles nodes

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

#ifndef _SRN_H
#define _SRN_H

#include "rect.h"

typedef struct _srnode_struct *srnode;
struct _srnode_struct
{
  srnode down;
  srnode next;
  int ref;
  rect r;
};

srnode srnOpen(rmatrix rm);
void srnClose(srnode srn);
void srnClear(srnode srn);
srnode srnLast(srnode srn);
void srnAddChild(srnode srn, srnode child);
srnode srnAddChildRect(rmatrix rm, srnode srn, rect *r);
void srnShow(rmatrix rm, srnode srn);
int srnGetDepth(srnode srn);

#endif /* _SRN_H */
