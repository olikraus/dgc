/*

  recu.h
    
  combines dcube and rect structures into one class (recu)

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


#ifndef _RECU_H
#define _RECU_H

#include "dcube.h"
#include "rect.h"
#include "srn.h"

struct _recu_struct
{
  pinfo *pi;
  dclist cl;
  rmatrix rm;
  lrlist lrl;
  lrlist lrl_primes;
  lrlist lrl_tmp;
  lrlist lrl_output;
  
  rect *input_mask;
  rect *full_input_cover;
  rect *area_mask;
  
  srnode top;
};
typedef struct _recu_struct recu_struct;
typedef struct _recu_struct *recu;


/* private */
int rcExtendXY(recu rc, lrlist lrl, rect *area);

srnode rcAddCol(recu rc, srnode srn, int col);
void rcOutputToFunction(recu rc, srnode srn);

/* public */
recu rcOpen();
void rcClose(recu rc);
int rcSetDCList(recu rc, pinfo *pi, dclist cl);
recu rcOpenByDCList(pinfo *pi, dclist cl);
int rcLoadAndMinimizeDCList(recu rc, char *filename);
int rcLoadDCList(recu rc, char *filename);
int rcBuildTree(recu rc, int is_lev_optimize, int is_out_optimize);

#endif /* _RECU_H */

