/*

  fsmtest.h
  
  test functions for fsm

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


#ifndef _FSMTEST_H
#define _FSMTEST_H

#include "fsm.h"


/* test transition */
struct _fsmtt_struct
{
  int edge_id;
  dcube c;
};
typedef struct _fsmtt_struct *fsmtt_type;

/* test list */
struct _fsmtl_struct
{
  fsm_type fsm;
  b_pl_type l;
  int (*outfn)(void *data, fsm_type fsm, int msg, int arg, dcube *c);
  void *data;
  dcube curr_input;
};
typedef struct _fsmtl_struct *fsmtl_type;

#define FSM_TEST_MSG_START 1
#define FSM_TEST_MSG_END 2
/* arg: src node, dcube: current (stable) input or NULL (unstable reset) */
#define FSM_TEST_MSG_RESET 10
/* arg: edge id, dcube: current input */
#define FSM_TEST_MSG_GO_EDGE 11
/* arg: edge id, dcube: current input */
#define FSM_TEST_MSG_DO_EDGE 12

/* cube selection */
#define FSM_TEST_CS_RAND 0
#define FSM_TEST_CS_MIN_DELTA 1
#define FSM_TEST_CS_MAX_DELTA 2

fsmtl_type fsmtl_Open(fsm_type fsm);
void fsmtl_Clear(fsmtl_type tl);
void fsmtl_Close(fsmtl_type tl);
void fsmtl_SetDevice(fsmtl_type tl, int (*outfn)(void *data, fsm_type fsm, int msg, int arg, dcube *c), void *data);
int fsmtl_Do(fsmtl_type tl);
int fsmtl_Show(fsmtl_type tl);
int fsmtl_AddAllEdgeSequence(fsmtl_type tl);

/* options for fsm_WriteVHDLTB are defined in fsm.h! */

int fsm_WriteVHDLTB(fsm_type fsm, char *filename, fsmtl_type tl, char *tb_entity, 
  char *entity, char *clock, char *reset, int opt, int ns);


#endif

