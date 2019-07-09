/*

  b_iq.c

  integer queue
  
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
#include "b_iq.h"
#include "mwc.h"

#define B_IQ_NORMAL_CNT 2

b_iq_type b_iq_Open()
{
  b_iq_type iq;
  iq = (b_iq_type)malloc(sizeof(struct _b_iq_struct));
  if ( iq != NULL )
  {
    iq->il = b_il_Open();
    if ( iq->il != NULL )
    { 
      iq->first = 0;
      return iq;
    }
    free(iq);
  }
  return NULL;
}

void b_iq_Close(b_iq_type iq)
{
  b_il_Close(iq->il);
  free(iq);
}

void b_iq_MakeNormal(b_iq_type iq)
{
  b_il_DelRange(iq->il, 0, iq->first);
  iq->first = 0;
}

int b_iq_Enqueue(b_iq_type iq, int val)
{
  if ( b_il_Add(iq->il, val) < 0 )
    return 0;
  return 1;
}

int b_iq_Dequeue(b_iq_type iq)
{
  int val;
  val = b_il_GetVal(iq->il, iq->first);
  iq->first++;
  if ( iq->first >= B_IQ_NORMAL_CNT )
    b_iq_MakeNormal(iq);
  return val;
}
