/* 

  b_pq.h
  
  priority queue implementation
  
  based on: CLR, "Introduction to Algorithms"

  J. W. J. Williams, Algorithm 232 (heapsort). CACM 7:347-348, 1964
  Robert W. Floyd. Algorithm 245 (treesort). CACM, 7:701, 1964

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


#ifndef _B_PQ_H
#define _B_PQ_H

#include "b_pl.h"

struct _b_pq
{
  b_pl_type pl;
  int (*comp)(void *data, const void *, const void *);
  void *data;
};
typedef struct _b_pq *b_pq_type;

b_pq_type b_pq_Open(int (*comp_lower)(void *data, const void *, const void *), void *data);
void b_pq_Close(b_pq_type h);
int b_pq_Add(b_pq_type h, void *ptr);
void *b_pq_Max(b_pq_type h);
void b_pq_DelMax(b_pq_type h);

#endif
