/*

  syparam.h
  
  synthesis parameter
  
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

#ifndef _SYPARAM_H
#define _SYPARAM_H

#define SYPARAM_GATE_EXPANSION_BALANCED         0
#define SYPARAM_GATE_EXPANSION_AREA_OPTIMIZED   1

#define GNC_DEFAULT_DELAY_SAFETY 1.10



struct _syparam_struct
{
  /*
    function: gnc_RateCellListSolution
    values: SYPARAM_GATE_EXPANSION_BALANCED, 
            SYPARAM_GATE_EXPANSION_AREA_OPTIMIZED
  */
  int gate_expansion;
  /*
    file: sydelay.c
  */
  double delay_safety;
};
typedef struct _syparam_struct syparam_struct;

int syparam_Init(syparam_struct *syparam);

#endif /* _SYPARAM_H */
