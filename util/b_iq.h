/*

  b_iq.h

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


#ifndef _B_IQ_H
#define _B_IQ_H

#include "b_il.h"

struct _b_iq_struct
{
  b_il_type il;
  int first;
};
typedef struct _b_iq_struct *b_iq_type;


#define b_iq_IsEmpty(iq) (b_il_GetCnt((iq)->il) <= (iq)->first )

b_iq_type b_iq_Open();
void b_iq_Close(b_iq_type iq);

int b_iq_Enqueue(b_iq_type iq, int val);
int b_iq_Dequeue(b_iq_type iq);

#endif /* _B_IQ_H */
