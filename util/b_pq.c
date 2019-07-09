/* 

  b_heap.c
  
  heap implementation
  
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

#include "b_pq.h"
#include <stdlib.h>
#include <assert.h>

#define PARENT(i) ((i)/2)
#define LEFT(i) (2*(i))
#define RIGHT(i) (2*(i)+1)

static int compare_lower(b_pq_type h, int a, int b)
{
  assert( a>=1&&a<=b_pl_GetCnt(h->pl) );
  assert( b>=1&&b<=b_pl_GetCnt(h->pl) );
  return h->comp(h->data, b_pl_GetVal(h->pl, a-1), b_pl_GetVal(h->pl, b-1));
}

static void heap_element_swap(b_pq_type h, int a, int b)
{
  void *tmp;
  assert( a>=1&&a<=b_pl_GetCnt(h->pl) );
  assert( b>=1&&b<=b_pl_GetCnt(h->pl) );
  a--;
  b--;
  tmp = h->pl->list[a];
  h->pl->list[a] = h->pl->list[b];
  h->pl->list[b] = tmp;
}

static int heap_check(b_pq_type h)
{
  int i, cnt = b_pl_GetCnt(h->pl);
  for( i = 2; i <= cnt; i++ )
    if ( compare_lower(h, 1, i) != 0 )
      return 0;
  return 1;
}

static void heapify(b_pq_type h, int i)
{
  int l, r;
  int largest = i;

  l = LEFT(i);
  r = RIGHT(i);
  if ( l <= b_pl_GetCnt(h->pl) )
    if ( compare_lower(h, largest, l) != 0 )
      largest = l;
  if ( r <= b_pl_GetCnt(h->pl) )
    if ( compare_lower(h, largest, r) != 0 )
      largest = r;
      
  if ( largest != i )
  {
    heap_element_swap(h, largest, i);
    heapify(h, largest);
  }
}

b_pq_type b_pq_Open(int (*comp_lower)(void *data, const void *, const void *), void *data)
{
  b_pq_type h;
  h = (b_pq_type)malloc(sizeof(struct _b_pq));
  if ( h != NULL )
  {
    h->pl = b_pl_Open();
    if ( h->pl != NULL )
    {
      h->comp = comp_lower;
      h->data = data;
      return h;
    }
    free(h);
  }
  return NULL;
}

void b_pq_Close(b_pq_type h)
{
  b_pl_Close(h->pl);
  free(h);
}

int b_pq_Add(b_pq_type h, void *ptr)
{
  int i;
  i = b_pl_Add(h->pl, ptr);
  if ( i < 0 )
    return -1;
    
  i++;
  
  while( i > 1 && compare_lower(h, PARENT(i), i) != 0 )
  {
    heap_element_swap(h, PARENT(i), i);
    i = PARENT(i);
  }

  assert( heap_check(h) != 0 );
  
  return i-1;
}

void *b_pq_Max(b_pq_type h)
{
  if ( b_pl_GetCnt(h->pl) == 0 )
    return NULL;
  return b_pl_GetVal(h->pl, 0);
}

void b_pq_DelMax(b_pq_type h)
{
  int last;
  if ( b_pl_GetCnt(h->pl) == 0 )
    return;
    
  if ( b_pl_GetCnt(h->pl) == 1 )
  {
    b_pl_Clear(h->pl);
    return;
  }
  
  last = b_pl_GetCnt(h->pl)-1;
  
  b_pl_SetVal(h->pl, 0, b_pl_GetVal(h->pl, last));
  b_pl_DelByPos(h->pl, last);
  heapify(h, 1);

  assert( heap_check(h) != 0 );

}

#ifdef _B_PQ_MAIN

int comp_lower(void *data, const void *a, const void *b)
{
  if ( a < b )
    return 1;
  return 0;
}

void fillto(b_pq_type h, int cnt)
{
  while( h->pl->cnt < cnt )
    b_pq_Add(h, rand()&255);
}

void deleteto(b_pq_type h, int cnt)
{
  while( h->pl->cnt > cnt )
    b_pq_DelMax(h);
}

int main()
{
  b_pq_type h;
  int i;
  
  h = b_pq_Open(comp_lower, NULL);

  b_pq_Add(h, 0x01111);
  b_pq_Add(h, 0x02222);
  b_pq_Add(h, 0x03333);
  b_pq_Add(h, 0x04444);

  for(;;)
  {
    if ( b_pq_Max(h) == NULL )
      break;
    printf("%p\n", b_pq_Max(h));
    b_pq_DelMax(h);
    
  }
  
  puts("---");

  b_pq_Add(h, 0x05555);
  b_pq_Add(h, 0x02222);
  b_pq_Add(h, 0x07777);
  b_pq_Add(h, 0x04444);
  b_pq_Add(h, 0x03333);
  b_pq_Add(h, 0x01111);
  b_pq_Add(h, 0x09999);
  b_pq_Add(h, 0x08888);

  for(;;)
  {
    if ( b_pq_Max(h) == NULL )
      break;
    printf("%p\n", b_pq_Max(h));
    b_pq_DelMax(h);
    
    /*
    for( i = 0; i < h->pl->cnt; i++ )
      printf("%d: %p\n", i, h->pl->list[i]);
    */
  }
  
  
  for( i = 0; i < 10000; i++ )
  {
    if ( (i % 100) == 0 )
    {
      printf("%d\r", i); fflush(stdout);
    }
    fillto(h, (rand()&31) + 10);
    deleteto(h, (rand()&7));
  }
  printf("\n");

  b_pq_Close(h);
  return 0;
}

#endif
