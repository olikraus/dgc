/*

  gnetsim.c
  
  gate level simulator
  
  purpose: exact calculation of the delay for an asynchronous state machine
  
  
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

#ifndef _GNETSIM_H
#define _GNETSIM_H

#include "gnet.h"
#include "b_pq.h"

/* gnet simulator priority queue */
#define GSPQ_T_POOL_CNT 8

#define GSPQ_INDEXED_TIME_CNT 4
struct _gspq_struct
{
  b_pq_type h;
  gnc nc;
  int cell_ref;
  double time;
  double indexed_time[GSPQ_INDEXED_TIME_CNT];
  /* idx 0: events before output */
  /* idx 1: events after passed output port*/
  
  int is_cap_analysis;        /* checked in gspq_ProcessEvent */
  double capacitance_sum;     /* changed in gspq_ProcessEvent */
  double change_cnt;          /* changed in gspq_ProcessEvent */
  
  
  int is_stop_at_fsm_input;
  int is_calc_max_in_out_dly;
  double *t_max;      /* GSPQ_T_POOL_CNT*gnc_GetCellPortMax */
  double *m_max;      /* gnc_GetCellPortMax*gnc_GetCellPortMax */
  
  int cap_analysis_transition_cnt; /* changed in gspq_fsm_simulation */
  double ff_capacitance_sum;       /* changed in gspq_fsm_simulation */
  
};
typedef struct _gspq_struct *gspq_type;

/*
  pool 0:  Cleared by gspq_Open(), automaticly filled
  pool 1:  Cleared by gspq_DoFSMLogicSimulation(), automaticly filled
  pool 2:  free usage
  
*/


gspq_type gspq_Open(gnc nc, int cell_ref);
void gspq_Close(gspq_type pq);

void gspq_ClearPool(gspq_type pq, int pool);
void gspq_ApplyMaxTime(gspq_type pq, int pool, int pos, double time);
double gspq_GetTime(gspq_type pq, int pool, int pos);

void gspq_ApplyMaxMTime(gspq_type pq, int x, int y, double time);
double gspq_GetMTime(gspq_type pq, int x, int y);

void gspq_CalculateUnknownValues(gspq_type pq);
int gspq_ApplyCellFSMSimulationTransitionState(gspq_type pq, pinfo *pi_in, dcube *c_in, pinfo *pi_z, dcube *c_z);
int gspq_ApplyCellFSMSimulationTransition(gspq_type pq, pinfo *pi_in, dcube *c_in);

/* these three functions expect a pl_machine cube */
/* a fsm simlation is performed */
int gspq_ApplyCellXBMSimulationTransitionState(gspq_type pq, dcube *c);
int gspq_ApplyCellXBMSimulationTransitionResetState(gspq_type pq, dcube *c);
int gspq_ApplyCellXBMSimulationTransition(gspq_type pq, dcube *c);
int gspq_CompareCellXBMStateForSimulation(gspq_type pq, dcube *c);

/* capacitance analysis */
void gspq_EnableCapacitanceAnalysis(gspq_type pq);
int gspq_GetNetChangeCnt(gspq_type pq);
double gspq_GetNetChangeCapacitance(gspq_type pq);
double gspq_GetAveragetNetChangeCapacitance(gspq_type pq);


#endif

