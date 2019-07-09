/*

  fsmenc.h

  finite state machine, based on b_set, b_il

  Partly Copyright (C) 2001 Oliver Kraus (olikraus@yahoo.com)

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

#ifndef _FSMENC_H
#define _FSMENC_H

#include "fsm.h"

int fsm_MinimizeClockedMachine(fsm_type fsm);
int fsm_BuildClockedMachine(fsm_type fsm, int coding_type);

#define FSM_ENCODE_FAN_IN 0
#define FSM_ENCODE_IC_ALL 1
#define FSM_ENCODE_IC_PART 2
#define FSM_ENCODE_SIMPLE 3
 
#endif /* _FSMENC_H */

