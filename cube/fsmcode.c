/*

  fsmcode.c
  
  create a burst mode (extended fundamental) asynchronous state machine
  
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

#include "fsm.h"
#include "mwc.h"


#ifdef OBSOLETE


static int fsm_CodeToDCL(fsm_type fsm, dclist cl)
{
  int node_id, pos;
  dclRealClear(cl);
  node_id = -1;
  while( fsm_LoopNodes(fsm, &node_id) != 0 )
  {
    pos = dclAdd(fsm_GetCodePINFO(fsm), cl, fsm_GetNodeCode(fsm, node_id));
    if ( pos < 0 )
    {
      fsm_Log(fsm, "FSM: Out of memory (fsm_CodeToDCL, dclAdd).");
      return 0;
    }
    dcOutSetAll(fsm_GetCodePINFO(fsm), dclGet(cl, pos), CUBE_OUT_MASK);
    dclGet(cl, pos)->n = node_id;
  }
  return 1;
}

static void fsm_ReplaceCodes(fsm_type fsm, dclist cl)
{
  int i, cnt = dclCnt(cl);
  for( i = 0; i < cnt; i++ )
    dcCopyIn(fsm_GetCodePINFO(fsm), 
      fsm_GetNodeCode(fsm, dclGet(cl, i)->n), dclGet(cl, i));
}

/* checks, if dclGet(cl,pos) has a non-null intersection with any other cubes */
static int dclIsIntersectionCube(pinfo *pi, dclist cl, int pos)
{
  int i, cnt = dclCnt(cl);
  for( i = 0; i < cnt; i++ )
    if ( i != pos )
      if ( dcIsDeltaInNoneZero(pi, dclGet(cl, i), dclGet(cl, pos)) == 0 )
        return 1;
  return 0;
}

static int dclExpandNonintersectionCubePosition(pinfo *pi, dclist cl, int pos, int in)
{
  int s;
  s = dcGetIn(dclGet(cl, pos), in);
  if ( s == 3 )
    return 0; /* can not increase don't care */
  dcSetIn(dclGet(cl, pos), in, 3);
  if ( dclIsIntersectionCube(pi, cl, pos) == 0 )
    return 1; /* increased! */
  dcSetIn(dclGet(cl, pos), in, s);
  return 0; /* not increased */
}

void dclExpandNonintersectionCubes(pinfo *pi, dclist cl)
{
  int i, cnt = dclCnt(cl);
  int in;
  for( in = 0; in < pi->in_cnt; in++ )
  {
    for( i = 0; i < cnt; i++ )
    {
      dclExpandNonintersectionCubePosition(pi, cl, i, in);
    }
  }
}

/*
  precondition: assumes valid codes in the input part of the codes
  action: tries to increase the number of don't cares in these codes
  used by: burst mode transfer function
*/
int fsm_ExpandCodes(fsm_type fsm)
{
  dclist cl;
  if ( dclInit(&cl) == 0 )
  {
    fsm_Log(fsm, "FSM: Out of memory (fsm_ExpandCodes, dclInit).");
    return 0;
  }

  if ( fsm_CodeToDCL(fsm, cl) == 0 )
  {
    dclDestroy(cl);
    return 0;
  }

  fsm_Log(fsm, "FSM: Expanding state codes.");

  dclExpandNonintersectionCubes(fsm_GetCodePINFO(fsm), cl);

  fsm_LogDCL(fsm, "FSM: ", 1, "Expanded state codes", fsm_GetCodePINFO(fsm), cl);

  fsm_ReplaceCodes(fsm, cl);

  return 1;  
}

#endif
