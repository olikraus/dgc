/*

  Gate Net (gnet.h)
  
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
  
  terms:
  cell    A logical unit, that has a certain number of input and output ports.
  port    A cell has several input and output ports to comunicate with other
          cells.
  net     A net connects the ports of several nodes. A net is a collection
          of "joins".
  join    A join is a pair if a reference to a node and a reference to a port.
  node    A node is a reference to a cell.
  
  There is some confusion with the names. Some functions have the
  name Port where they should better have Join. One could say the
  following:
  A join is also the port of a net. So, joins are sometimes also called port.
  
  Grrr.. I should rename the functions some day...
    ... partly done ...
  
*/


#ifndef _GNET_H
#define _GNET_H

#include <stdarg.h>
#include "b_set.h"
#include "b_rdic.h"
#include "dcube.h"
#include "fsm.h"
#include "xbm.h"
#include "recu.h"
#include "wiga.h"
#include "syparam.h"

#define GNET_GENERIC_LIB_NAME "__GENERIC_LIB__"
/* change this also in edif.c */
#define GNET_HIDDEN_LIB_NAME "__GNET_HIDDEN__"

#define GPORT_IN_PREFIX "x"
#define GPORT_OUT_PREFIX "y"

#define GPORT_NAME_CLK "clk"
#define GPORT_NAME_SET "set"
#define GPORT_NAME_CLR "clr"

#define GPORT_TYPE_BI 0
#define GPORT_TYPE_IN 1
#define GPORT_TYPE_OUT 2

#define GPORT_FN_LOGIC 0
#define GPORT_FN_D     1
#define GPORT_FN_CLK   2
#define GPORT_FN_Q     3
#define GPORT_FN_J     4
#define GPORT_FN_K     5
#define GPORT_FN_CLR   6
#define GPORT_FN_SET   7

/* only for internal and temprorary use */
#define GPORT_FN_NONE  8


/* one dimensional values */
struct _g1dv_struct
{
  int idx_cnt;
  double *idx_vals;
  double *values;
};
typedef struct _g1dv_struct *g1dv;

g1dv g1dvOpen(int cnt);
void g1dvClose(g1dv g1);
g1dv g1dvOpenByStr(const char *x, char *z);
double g1dvCalc(g1dv g1, double x);

/* two dimensional values */
struct _g2dv_struct
{
  int idx1_cnt;
  double *idx1_vals;
  int idx2_cnt;
  double *idx2_vals;
  double *values;
};
typedef struct _g2dv_struct *g2dv;

g2dv g2dvOpen(int xcnt, int ycnt);
void g2dvClose(g2dv g2);
double g2dvGetVal(g2dv g2, int xpos, int ypos);
void g2dvSetVal(g2dv g2, int xpos, int ypos, double val);
g2dv g2dvOpenByStr(const char *x, const char *y, char *z);
double g2dvCalc(g2dv g2, double x, double y);


/* rise delay is:                                                           */
/* D_rise = rise_block_delay + rise_fanout_delay * sum( all input_load's )  */
struct _gpoti_struct
{
  int related_port_ref;
  
  /* double input_load; */
  /* double max_load; */
  double rise_block_delay;     /* synopsys: intrinsic_rise */
  double rise_fanout_delay;    /* synopsys: rise_resistance */
  double fall_block_delay;     /* synopsys: intrinsic_fall */
  double fall_fanout_delay;    /* synopsys: fall_resistance */
  
  g2dv rise_cell;
  g1dv rise_propagation;
  g1dv rise_transition;
  g2dv fall_cell;
  g1dv fall_propagation;
  g1dv fall_transition;
  
};
typedef struct _gpoti_struct *gpoti;

gpoti gpotiOpen(void);
void gpotiClose(gpoti pt);

int gpotiWrite(gpoti pt, FILE *fp);
int gpotiRead(gpoti pt, FILE *fp);
size_t gpotiGetMemUsage(gpoti pt);

/* gate logic value for gnetlv.c and gnetsim.c */
#define GLV_UNKNOWN 0
#define GLV_ZERO    1
#define GLV_ONE     2

struct _glv_struct
{ 
  unsigned int logic_value;
  unsigned int logic_change_cnt;
};
typedef struct _glv_struct glv_struct;

struct _gport_struct
{
  int type;
  int fn;
  int is_inverted;
  char *name;
  double input_load;   /* the capacitance of a pin, defaults to 1.0 */
  b_set_type poti_set;
  glv_struct lv;      /* only for simulation */
};
typedef struct _gport_struct *gport;


struct _gjoin_struct
{
  int node; /* can contain -1: belongs to parent cell */
  int port;
};
typedef struct _gjoin_struct *gjoin;

struct _gnet_struct
{
  b_set_type join_set;
  char *name;
  int user_val;
  glv_struct lv;
};
typedef struct _gnet_struct *gnet;

#define gnetGetGJOIN(net,pos)  ((gjoin)b_set_Get((net)->join_set,(pos)))
#define gnetLoopJoinRef(net,pos) b_set_WhileLoop((net)->join_set, (pos))

struct _gnode_struct
{
  int cell_ref;
  char *name;
  unsigned flag:15;
  unsigned is_do_not_touch:1;   /* ignore for optimization */
  unsigned recalculate:1;       /* for simulation (gnetlv.c) */
  unsigned delta_cycle:1;       /* for simulation (gnetlv.c) */

  /* part of a heuristic search mechanism for net_refs */
  /* this list stores potential net's that might contain a port of the node */
  /* The idea is not to search nets for a matching node/port pair */
  b_il_type net_refs;
  
  /* an additional data field, used by the flatten machanism to store */
  /* a node_ref */
  /* also used by the simulation algorithm to store the state of a FF */
  /* --> gnetlv.c */
  int data;
};
typedef struct _gnode_struct *gnode;

/* note: is_do_not_touch is only considered for technology    */
/*       optimization (sylopt.c) at the moment.               */
/*       It is set in syfsm.c to avoid the change of the      */
/*       asynchronous doors. sylopt might introduce interters */
/*       so that the longest path calculation can not remove  */
/*       the cycles in the fsm. Indeed, setting the           */
/*       do_not_touch flag is more a work around than a good  */
/*       solution. It would be better to add an other cycle   */
/*       detection. Well, area is usually not increased.      */


/* netlist */
struct _gnl_struct
{
  b_set_type node_set;
  b_rdic_type node_by_name;
  b_set_type net_set;
  b_rdic_type net_by_name;
  b_il_type net_refs;   /* cache for cell/net connections */
};
typedef struct _gnl_struct *gnl;

#define gnlGetGNODE(nl,pos) ((gnode)b_set_Get((nl)->node_set,(pos)))
#define gnlGetGNET(nl,pos) ((gnet)b_set_Get((nl)->net_set,(pos)))

#define gnlLoopNodeRef(nl,pos) b_set_WhileLoop((nl)->node_set, (pos))
#define gnlLoopNetRef(nl,pos) b_set_WhileLoop((nl)->net_set, (pos))
#define gnlLoopJoinRef(nl,net_ref,pos) \
  gnetLoopJoinRef(gnlGetGNET((nl),(net_ref)),(pos))
#define gnlGetGJOIN(nl,net_ref,pos) \
  gnetGetGJOIN(gnlGetGNET((nl),(net_ref)),(pos))
int gnlAddNetPort(gnl nl, int net_ref, int node_ref, int port_ref, int is_unique);


/* cell io delay matrix */
struct _gciom_struct
{
  int in_cnt;     /* indeed this is the number of all ports of a cell */
                  /* because an output might be an input for another output */
  int out_cnt;
  double *min_delay;
  double *max_delay;
  double max;
  double min;
  char *is_used;
};
typedef struct _gciom_struct *gciom;


/* A GCELL_ID_xxxx value must be between 0 and GCELL_ID_MAX-1 */

#define GCELL_ID_UNKNOWN 0
#define GCELL_ID_EMPTY   1
#define GCELL_ID_INV     2
#define GCELL_ID_BUF     3

#define GCELL_ID_AND2    4
#define GCELL_ID_NAND2   5
#define GCELL_ID_OR2     6
#define GCELL_ID_NOR2    7

#define GCELL_ID_AND3    8
#define GCELL_ID_NAND3   9
#define GCELL_ID_OR3     10
#define GCELL_ID_NOR3    11

#define GCELL_ID_AND4    12
#define GCELL_ID_NAND4   13
#define GCELL_ID_OR4     14
#define GCELL_ID_NOR4    15

#define GCELL_ID_AND5    16
#define GCELL_ID_NAND5   17
#define GCELL_ID_OR5     18
#define GCELL_ID_NOR5    19

#define GCELL_ID_AND6    20
#define GCELL_ID_NAND6   21
#define GCELL_ID_OR6     22
#define GCELL_ID_NOR6    23

#define GCELL_ID_AND7    24
#define GCELL_ID_NAND7   25
#define GCELL_ID_OR7     26
#define GCELL_ID_NOR7    27

#define GCELL_ID_AND8    28
#define GCELL_ID_NAND8   29
#define GCELL_ID_OR8     30
#define GCELL_ID_NOR8    31

#define GCELL_ID_DFF_Q        40
#define GCELL_ID_DFF_Q_LC     41
#define GCELL_ID_DFF_Q_HC     42
#define GCELL_ID_DFF_Q_LS     43
#define GCELL_ID_DFF_Q_HS     44

#define GCELL_ID_DFF_Q_NQ     45
#define GCELL_ID_DFF_Q_NQ_LC  46
#define GCELL_ID_DFF_Q_NQ_HC  47
#define GCELL_ID_DFF_Q_NQ_LS  48
#define GCELL_ID_DFF_Q_NQ_HS  49


#define GCELL_ID_G_INV        55
#define GCELL_ID_G_AND        56
#define GCELL_ID_G_OR         57
#define GCELL_ID_G_REGISTER   58
#define GCELL_ID_G_DOOR       59

#define GCELL_ID_MAX     60

#define GCELL_IDL_AND   GCELL_ID_AND8, GCELL_ID_AND7, GCELL_ID_AND6, \
                        GCELL_ID_AND5, GCELL_ID_AND4, GCELL_ID_AND3, \
                        GCELL_ID_AND2

#define GCELL_IDL_OR    GCELL_ID_OR8, GCELL_ID_OR7, GCELL_ID_OR6, \
                        GCELL_ID_OR5, GCELL_ID_OR4, GCELL_ID_OR3, \
                        GCELL_ID_OR2

#define GCELL_IDL_NAND  GCELL_ID_NAND8, GCELL_ID_NAND7, GCELL_ID_NAND6, \
                        GCELL_ID_NAND5, GCELL_ID_NAND4, GCELL_ID_NAND3, \
                        GCELL_ID_NAND2

#define GCELL_IDL_NOR   GCELL_ID_NOR8, GCELL_ID_NOR7, GCELL_ID_NOR6, \
                        GCELL_ID_NOR5, GCELL_ID_NOR4, GCELL_ID_NOR3, \
                        GCELL_ID_NOR2

#define GCELL_IDL_DFF   GCELL_ID_DFF_Q,         \
                        GCELL_ID_DFF_Q_LC,      \
                        GCELL_ID_DFF_Q_HC,      \
                        GCELL_ID_DFF_Q_LS,      \
                        GCELL_ID_DFF_Q_HS,      \
                        GCELL_ID_DFF_Q_NQ,      \
                        GCELL_ID_DFF_Q_NQ_LC,   \
                        GCELL_ID_DFF_Q_NQ_HC,   \
                        GCELL_ID_DFF_Q_NQ_LS,   \
                        GCELL_ID_DFF_Q_NQ_HS
                    
struct _gcell_struct
{
  char *name;
  char *library;
  char *desc;
  b_set_type port_set;
  gnl nl;
  int id;
  int in_cnt;
  int out_cnt;
  double area;
  unsigned int flag;
  int register_width;   /* >0 if the cell contains a fsm */  
  /* int is_next_state_fn_inverted; */
  pinfo *pi;
  dclist cl_on;
  dclist cl_dc;
  dclist cl_off;
  gciom ciom;
};
typedef struct _gcell_struct *gcell;

int gcellSetName(gcell cell, const char *name);
int gcellSetLibrary(gcell cell, const char *library);
int gcellSetDescription(gcell cell, const char *desc);

#define gcellGetGPORT(cell,pos) ((gport)b_set_Get((cell)->port_set,(pos)))
#define gcellLoopPortRef(cell,pos) b_set_WhileLoop((cell)->port_set, (pos))
#define gcellLoopNodeRef(cell, pos) ((cell)->nl==NULL?0:gnlLoopNodeRef((cell)->nl,(pos)))
#define gcellLoopNetRef(cell, pos) ((cell)->nl==NULL?0:gnlLoopNetRef((cell)->nl,(pos)))
#define gcellLoopJoinRef(cell, net_ref, pos) \
  gnlLoopJoinRef((cell)->nl,(net_ref),(pos))

int gcellInitDelay(gcell cell);
int gcellSetDelay(gcell cell, int in_out_port_ref, int out_port_ref, double delay);
double gcellGetDelayMin(gcell cell, int in_out_port_ref, int out_port_ref);
double gcellGetDelayMax(gcell cell, int in_out_port_ref, int out_port_ref);
int gcellIsDelayUsed(gcell cell, int in_out_port_ref, int out_port_ref);
double gcellGetDelayTotalMax(gcell cell);
double gcellGetDelayTotalMin(gcell cell);


/* gate lib structure */
struct _glib_struct
{
  char *name;
  int cell_cnt;
};
typedef struct _glib_struct *glib;

/* gate net collection */
struct _gnc_struct
{
  char *name;
  
  b_set_type cell_set;
  b_set_type lib_set;
  
  /* quick access to the basic building blocks via DCELL_ID_xxxx macros */
  int bbb_cell_ref[GCELL_ID_MAX];
  int bbb_disable[GCELL_ID_MAX];
  
  /* error handling */
  void (*err_fn)(void *data, const char *fmt, va_list va);
  void *err_data;
  
  /* logging */
  int log_level;      /* 1-6, log_level = 1: all messages */
  void (*log_fn)(void *data, const char *fmt, va_list va);
  void *log_data;
  
  /* synthesis parameter */
  syparam_struct syparam;
  
  /* wide gate environment */
  wge_type wge;         /* wiga.c */
  
  /* probably this is a synthesis parameter */
  int max_asynchronous_delay_depth;
  
  /* a reference to the delay cell for async machines */
  /* can be -1 for the inverter cell */
  int async_delay_cell_ref;
  
  char *name_clk;
  char *name_set;
  char *name_clr;
  
  /* simulation loop emergency abort */
  int simulation_loop_abort;
  
};

#ifndef _GNC_TYPE
#define _GNC_TYPE
typedef struct _gnc_struct *gnc;
#endif

#define gnc_GetGCELL(nc,cell_ref) ((gcell)b_set_Get((nc)->cell_set, (cell_ref)))
#define gnc_GetGLIB(nc,lib_ref) ((glib)b_set_Get((nc)->lib_set, (lib_ref)))

gnode gnc_GetCellGNODE(gnc nc, int cell_ref, int node_ref);
gnet gnc_GetCellGNET(gnc nc, int cell_ref, int net_ref);
gnl gnc_GetGNL(gnc nc, int cell_ref, int is_create);


/* initial value for cell_ref: -1 */
#define gnc_LoopCell(nc, cell_ref) b_set_WhileLoop((nc)->cell_set, (cell_ref))

#define gnc_LoopCellNode(nc, cell_ref, node_ref) \
  gcellLoopNodeRef(gnc_GetGCELL(nc,cell_ref), node_ref)
  
#define gnc_LoopCellNet(nc, cell_ref, net_ref) \
  gcellLoopNetRef(gnc_GetGCELL(nc,cell_ref), (net_ref))
  
#define gnc_LoopCellPort(nc, cell_ref, port_ref) \
  gcellLoopPortRef(gnc_GetGCELL(nc,cell_ref), (port_ref))
  
#define gnc_LoopCellNetJoin(nc, cell_ref, net_ref, join_ref) \
  gcellLoopJoinRef(gnc_GetGCELL(nc,cell_ref), (net_ref), (join_ref))


/*
#define gnc_GetCellCnt(nc) b_dic_GetCnt((nc)->cell_set)
#define gnc_GetNLCnt(nc) b_pl_GetCnt((nc)->nl_set)
*/

gnc gnc_Open();
void gnc_Close(gnc nc);

void gnc_SetErrFn(gnc nc, void (*err_fn)(void *data, const char *fmt, va_list va), void *data);
void gnc_SetLogFn(gnc nc, void (*log_fn)(void *data, const char *fmt, va_list va), void *data);

int gnc_Write(gnc nc, FILE *fp);
int gnc_Read(gnc nc, FILE *fp);

int gnc_WriteBin(gnc nc, char *name);
int gnc_ReadBin(gnc nc, char *name);

void gnc_ErrorVA(gnc nc, const char *fmt, va_list va);
void gnc_Error(gnc nc, const char *fmt, ...);
void gnc_LogVA(gnc nc, int prio, const char *fmt, va_list va);
void gnc_Log(gnc nc, int prio, const char *fmt, ...);
/* contains triples char*, pinfo *, dclist */
void gnc_LogDCL(gnc nc, int prio, char *prefix, int cnt, ...);


int gnc_SetName(gnc nc, const char *name);
char *gnc_GetName(gnc nc);


int gnc_FindLib(gnc nc, const char *library);
#define gnc_LoopLib(nc, lib_ref) b_set_WhileLoop((nc)->lib_set, (lib_ref))
char *gnc_GetLibName(gnc nc, int lib_ref);
int gnc_LoopLibCell(gnc nc, int lib_ref, int *cell_ref);

/* returns cell_ref */
int gnc_AddCell(gnc nc, const char *cellname, const char *library);

void gnc_DelCell(gnc nc, int cell_ref);
void gnc_ClearCell(gnc nc, int cell_ref);

/* returns cell_ref */
int gnc_FindCell(gnc nc, const char *name, const char *library);

char *gnc_GetCellLibraryName(gnc nc, int cell_ref);
const char *gnc_GetCellName(gnc nc, int cell_ref);
const char *gnc_GetCellNameStr(gnc nc, int cell_ref);


/* returns position or -1 */
/* pintype is one of: GPORT_TYPE_BI, GPORT_TYPE_IN, GPORT_TYPE_OUT */
int gnc_AddCellPort(gnc nc, int cell_ref, int pin_type, const char *pin_name);

/* returns 0 or 1, does not update pinfo */
int gnc_SetCellPort(gnc nc, int cell_ref, int pos, int port_type, const char *port_name);

/* return the delay structure from port_ref to related_port_ref */
/* If the related port ref was not found, a default stucture might be returned */
/* the default structure has the relate_port_ref = -1 */
gpoti gnc_GetCellPortGPOTI(gnc nc, int cell_ref, int port_ref, int related_port_ref, int is_create);

int gnc_GetCellPortCnt(gnc nc, int cell_ref);
int gnc_GetCellPortMax(gnc nc, int cell_ref);   /* upper limit for ports */
int gnc_SetCellPortName(gnc nc, int cell_ref, int port_ref, const char *name);
const char *gnc_GetCellPortName(gnc nc, int cell_ref, int port_ref);
/* returned pintype is one of: GPORT_TYPE_BI, GPORT_TYPE_IN, GPORT_TYPE_OUT */
int gnc_GetCellPortType(gnc nc, int cell_ref, int port_ref);
void gnc_SetCellPortFn(gnc nc, int cell_ref, int port_ref, int fn, int is_inverted);
int gnc_GetCellPortFn(gnc nc, int cell_ref, int port_ref);
int gnc_IsCellPortInverted(gnc nc, int cell_ref, int port_ref);
void gnc_SetCellPortInputLoad(gnc nc, int cell_ref, int port_ref, double cap);
double gnc_GetCellPortInputLoad(gnc nc, int cell_ref, int port_ref);

void gnc_SetCellArea(gnc nc, int cell_ref, double area);
double gnc_GetCellArea(gnc nc, int cell_ref);
void gnc_SetCellId(gnc nc, int cell_ref, int id);
int gnc_GetCellId(gnc nc, int cell_ref);
int gnc_GetCellInCnt(gnc nc, int cell_ref);
void gnc_ClearAllCellsFlag(gnc nc);
int gnc_IsCellFlag(gnc nc, int cell_ref);
void gnc_SetCellFlag(gnc nc, int cell_ref);

/* returns port_ref */
int gnc_FindCellPort(gnc nc, int cell_ref, const char *port_name);

/* returns port_ref */
int gnc_FindCellPortByType(gnc nc, int cell_ref, int type);
int gnc_FindCellPortByFn(gnc nc, int cell_ref, int fn);
int gnc_GetNthPort(gnc nc, int cell_ref, int n, int type);

/* returns node_ref */
int gnc_AddCellNode(gnc nc, int cell_ref, const char *node_name, int node_cell_ref);

void gnc_DelCellNode(gnc nc, int cell_ref, int node_ref);
void gnc_DelCellNodeCell(gnc nc, int cell_ref, int node_cell_ref);

/* returns node_ref */
int gnc_FindCellNode(gnc nc, int cell_ref, const char *node_name);

int gnc_GetCellNodeCnt(gnc nc, int cell_ref);

/* node_ref: 0..gnc_GetCellNodeCnt() */
char *gnc_GetCellNodeName(gnc nc, int cell_ref, int node_ref);

/* returns cell_ref */
/* node_ref: 0..gnc_GetCellNodeCnt() */
int gnc_GetCellNodeCell(gnc nc, int cell_ref, int node_ref);

void gnc_ClearCellNodeFlags(gnc nc, int cell_ref);
unsigned gnc_GetCellNodeFlag(gnc nc, int cell_ref, int node_ref);
void gnc_SetCellNodeFlag(gnc nc, int cell_ref, int node_ref, unsigned val);

int gnc_IsCellNodeDoNotTouch(gnc nc, int cell_ref, int node_ref);
void gnc_SetCellNodeDoNotTouch(gnc nc, int cell_ref, int node_ref);

int gnc_LoopCellNodePortType(gnc nc, int cell_ref, int node_ref, int type, int *port_ref, int *net_ref, int *join_ref);
#define gnc_LoopCellNodeInputs(nc, cell_ref, node_ref, port_ref, net_ref, join_ref) \
  gnc_LoopCellNodePortType((nc), (cell_ref), (node_ref), GPORT_TYPE_IN, (port_ref), (net_ref), (join_ref))
#define gnc_LoopCellNodeOutputs(nc, cell_ref, node_ref, port_ref, net_ref, join_ref) \
  gnc_LoopCellNodePortType((nc), (cell_ref), (node_ref), GPORT_TYPE_OUT, (port_ref), (net_ref), (join_ref))

/* returns all nets of a node to ports of a specified type */
int gnc_AddCellNodeTypeNet(gnc nc, int cell_ref, int node_ref, int type, b_il_type il);
int gnc_GetCellNodeInputNet(gnc nc, int cell_ref, int node_ref, b_il_type il);
int gnc_GetCellNodeOutputNet(gnc nc, int cell_ref, int node_ref, b_il_type il);
/* returns all input nodes of a node */
int gnc_AddCellNodeInputNodes(gnc nc, int cell_ref, int node_ref, b_il_type il);

/* returns the net_ref of the first out port of the specified node */
/* this is similar to gnc_GetCellNodeOutputNet */
int gnc_GetCellNodeFirstOutputNet(gnc nc, int cell_ref, int node_ref);
int gnc_GetCellNodeFirstInputNet(gnc nc, int cell_ref, int node_ref);


/* node_ref: 0..gnc_GetCellNodeCnt() */
char *gnc_GetCellNodeCellName(gnc nc, int cell_ref, int node_ref);
char *gnc_GetCellNodeCellLibraryName(gnc nc, int cell_ref, int node_ref);


/* returns net_ref or -1 */
int gnc_AddCellNet(gnc nc, int cell_ref, const char *net_name);

/* delete a net */
void gnc_DelCellNet(gnc nc, int cell_ref, int net_ref);

/* delete all empty net entries */
void gnc_DelCellEmptyNet(gnc nc, int cell_ref);
void gnc_DelEmptyNet(gnc nc);

/* returns net_ref or -1 */
int gnc_FindCellNet(gnc nc, int cell_ref, const char *net_name);

/* returns net_ref or -1 */
int gnc_FindCellNetByPort(gnc nc, int cell_ref, int node_ref, int port_ref);

/* returns net_ref or -1 */
int gnc_FindCellNetByNode(gnc nc, int cell_ref, int node_ref);

/* returns join_ref or -1 */
/* should be renamed to gnc_AddCellNetJoin... 2001-12-5 done*/
int gnc_AddCellNetJoin(gnc nc, int cell_ref, int net_ref, int node_ref, int port_ref, int is_unique);
/*
#define gnc_AddCellNetJoin(nc, cell_ref, net_ref, node_ref, port_ref) \
  gnc_AddCellNetPort((nc), (cell_ref), (net_ref), (node_ref), (port_ref))
*/

/* should be renamed to gnc_DelCellNetJoin, ok done (25 okt 2001)*/
/* now rename the sources... */
void gnc_DelCellNetJoin(gnc nc, int cell_ref, int net_ref, int join_ref);
#define gnc_DelCellNetPort(nc, cell_ref, net_ref, join_ref) \
  gnc_DelCellNetJoin((nc), (cell_ref), (net_ref), (join_ref))

void gnc_DelCellNetJoinByPort(gnc nc, int cell_ref, int net_ref, int node_ref, int port_ref);

/* returns join_ref or -1 if port was not found */
/* find a join_ref that is connected to a 'port' of 'node' */
/* if 'node' is -1 a parent port is searched */
/* 'port' = -1 is a wildcard for any port of the node */
/* returns join_ref */
int gnc_FindCellNetJoin(gnc nc, int cell_ref, int net_ref, int node_ref, int port_ref);

/* returns cell_ref */
int gnc_GetCellByNode(gnc nc, int cell_ref, int node_ref);

/* number of different libraries, stored inside the gnet */
int gnc_GetLibraryCnt(gnc nc);

/* number of cells, that do have a corresponding netlist */
int gnc_GetLibraryNLCnt(gnc nc, int pos);

/* pos: 0..gnc_GetLibraryCnt() */
char *gnc_GetLibraryName(gnc nc, int pos);

int gnc_GetCellNetCnt(gnc nc, int cell_ref);

/* returns the area of all nodes of a net netlist of a cell */
/* returns 0.0 if the cell does not have a netlist */
double gnc_CalcCellNetNodeArea(gnc nc, int cell_ref);

/* net_ref: 0..gnc_GetCellNetCnt() */
/* returns the name of a net of a specified cell */
/* returns NULL of the cell does not have a net */
char *gnc_GetCellNetName(gnc nc, int cell_ref, int net_ref);

/* returns the number of ports, that a net connects together */
/* should be renamed to gnc_GetCellNetJoinCnt() ??? */
int gnc_GetCellNetPortCnt(gnc nc, int cell_ref, int net_ref);

/* returns a reference to the port of a node/port pair */
/* the fn gnc_GetCellNetNode() returns the corresponding node */
int gnc_GetCellNetPort(gnc nc, int cell_ref, int net_ref, int join_ref);

/* returns the node of the node/port pair of a net */
int gnc_GetCellNetNode(gnc nc, int cell_ref, int net_ref, int join_ref);

/* net_ref: 0..gnc_GetCellNetCnt() */
/* join_ref: 0..gnc_GetCellNetPortCnt() */
/* returns the name of a node of the node/port pair of a net */
char *gnc_GetCellNetNodeName(gnc nc, int cell_ref, int net_ref, int join_ref);

/* net_ref: 0..gnc_GetCellNetCnt() */
/* join_ref: 0..gnc_GetCellNetPortCnt() */
/* returns the cell of the node of the node/port pair of a net */
int gnc_GetCellNetNodeCell(gnc nc, int cell_ref, int net_ref, int join_ref);
const char *gnc_GetCellNetNodeCellName(gnc nc, int cell_ref, int net_ref, int join_ref);

/* net_ref: 0..gnc_GetCellNetCnt() */
/* join_ref: 0..gnc_GetCellNetPortCnt() */
/* returns the name of the port of the node/port pair of a net */
const char *gnc_GetCellNetPortName(gnc nc, int cell_ref, int net_ref, int join_ref);

/* returns the type of the port of the node/port pair of a net */
/* return value is one of GPORT_TYPE_BI, GPORT_TYPE_IN, GPORT_TYPE_OUT */
int gnc_GetCellNetPortType(gnc nc, int cell_ref, int net_ref, int join_ref);
int gnc_FindCellNetJoinByPortType(gnc nc, int cell_ref, int net_ref, int type);
int gnc_FindCellNetNodeByPortType(gnc nc, int cell_ref, int net_ref, int type);

/* returns join_ref/node_ref of the driving port */
int gnc_FindCellNetJoinByDriver(gnc nc, int cell_ref, int net_ref);
int gnc_FindCellNetNodeByDriver(gnc nc, int cell_ref, int net_ref);
int gnc_FindCellNetPortByDriver(gnc nc, int cell_ref, int net_ref);
int gnc_CheckCellNetDriver(gnc nc, int cell_ref);


/* net_ref: 0..gnc_GetCellNetCnt() */
/* join_ref: 0..gnc_GetCellNetPortCnt() */
/* returns the first join_ref, with a not to cell 'req_cell_ref' */
int gnc_FindCellNetCell(gnc nc, int cell_ref, int net_ref, int req_cell_ref);


/* returns the first node_ref with a output port on the specified net */
/* this function ignores parent ports */
int gnc_FindCellNetOutputNode(gnc nc, int cell_ref, int net_ref);
int gnc_FindFirstCellNetInputNode(gnc nc, int cell_ref, int net_ref);

void gnc_Clean(gnc nc);

int gnc_IsTopLevelCell(gnc nc, int cell_ref);
int gnc_GetTopLevelCellCnt(gnc nc);

/* total amount of memory */
size_t gnc_GetMemUsage(gnc nc);

/* read a gate library into the memory */
/* returns internal name of the library if library was NULL */
/* returns library_name if library_name != NULL */
char *gnc_ReadLibrary(gnc nc, const char *name, const char *libary_name, int is_msg);

/* write cells of a library to a file */
int gnc_WriteBinaryLibrary(gnc nc, const char *name, const char *libary_name);



/* bbb.c */
int bbb_check_desc_list(pinfo *pi_r, dclist cl_r);
int gnc_IdentifyAndMarkBBBs(gnc nc, int force);
int gnc_ApplyBBBs(gnc nc, int force);
void gnc_DisableBBB(gnc nc, int id);
void gnc_EnableBBB(gnc nc, int id);
void gnc_DisableStrBBB(gnc nc, const char *sid);
void gnc_EnableStrBBB(gnc nc, const char *sid);
void gnc_DisableStrListBBB(gnc nc, const char *sl);
int gnc_GetCellById(gnc nc, int id);

/* gnetutil.c */
int gnc_AddEmptyCell(gnc nc);
/* merge net2 into net1, deletes net2 */
int gnc_MergeNets(gnc nc, int cell_ref, int net1_ref, int net2_ref);
b_il_type gnc_GetCellListById(gnc nc, ...);
void gnc_RateCellRefs(gnc nc,  b_il_type cell_refs, int *in_cnt_p, int *out_cnt_p, double *area_p, int *max_in_p, int *min_in_p);
int gnc_GetGenericCell(gnc nc, int g_id, int in_cnt);
int gnc_ApplyRegisterResetCode(gnc nc, int cell_ref, pinfo *pi, dcube *code);
/* delete all nodes, with unconnected outputs */
void gnc_DeleteUnusedNodes(gnc nc, int cell_ref);

/* replace a node with the netlist of the corresponding cell's netlist */
int gnc_ReplaceCellNode(gnc nc, int cell_ref, int node_ref);
/* replace all references to node_cell_ref by the corresponding net */
int gnc_ReplaceCellCellRef(gnc nc, int cell_ref, int node_cell_ref);
/* replace all references to cell_ref by inserting the corresponding net */
int gnc_ReplaceCellRef(gnc nc, int cell_ref);

/* try to replace all references within 'cell_ref' by corresponding nets */
/* unused cells are not removed unless is_delete is set to none zero val */
int gnc_FlattenCell(gnc nc, int cell_ref, int is_delete);

/* flatten the hierarchy */
/* is_delete = 1: Delete cells, that are not required any more */
/* is_delete = 0: Do not delete unused cells, instead one might use */
/* gnc_Clean(nc), which would also delete those cells */
int gnc_Flatten(gnc nc, int is_delete);

/* sylib.c */
char *gnc_ReadSynopsysLibrary(gnc nc, const char *name, const char *libary_name);
int IsValidSynopsysLibraryFile(const char *name, int is_msg);

/* gl2gnet.c */
char *gnc_ReadGenlibLibrary(gnc nc, const char *name, const char *libary_name);
int IsValidGenlibFile(const char *name);

/* vhdlnet.c */
#define GNC_WRITE_VHDL_UPPER_KEY 0x01
#define GNC_WRITE_VHDL_UPPER_ID 0x02
int gnc_WriteVHDL(gnc nc, int cell_ref, const char *filename, const char *entity, const char *arch, int opt, int wait_ns, const char *lib_name);

/* sygate.c */
int gnc_GetInvertedNet(gnc nc, int cell_ref, int net_ref, int is_generic);
/* returns a pin of a parent net */
int gnc_GetDigitalCellNetByPort(gnc nc, int cell_ref, int port_ref, int is_neg, int is_generic);
/* erzeugt ein node aus einer vorhandenen zelle (wird aufgerufen von gnc_SynthGate) */
int gnc_SynthDigitalCell(gnc nc, int cell_ref, int digital_cell_ref, b_il_type in_net_refs, int out_net_ref);
/* erzeugt ggf eine generische zelle */
int gnc_SynthGenericGate(gnc nc, int cell_ref, int g_id, b_il_type in_net_refs, int out_net_ref);

/* convert __empty__ cells */
int gnc_SynthRemoveEmptyCells(gnc nc, int cell_ref);

/* converts all generic cells, except __empty__ */
int gnc_SynthGenericToLibExceptEmptyCells(gnc nc, int cell_ref);

/* converts all generic cells */
int gnc_SynthGenericToLib(gnc nc, int cell_ref);

/* wiga.c */
#define GNC_GATE_AND WIGA_TYPE_AND
#define GNC_GATE_OR  WIGA_TYPE_OR
/* erzeugt ein gate mit hilfe vorhandener kleinerer gates */
int gnc_SynthGate(gnc nc, int cell_ref, int gate, b_il_type in_net_refs, int *p_out, int *n_out);

/* syrecu.c */
/* returns 0 or 1 */

#define GNC_SYNTH_OPT_NECA        (0x0001)
#define GNC_SYNTH_OPT_GENERIC     (0x0002)
#define GNC_SYNTH_OPT_LIBARY      (0x0004)
#define GNC_SYNTH_OPT_LEVELS      (0x0008)
#define GNC_SYNTH_OPT_DLYPATH     (0x0010)
#define GNC_SYNTH_OPT_OUTPUTS     (0x0020)
#define GNC_SYNTH_OPT_DEFAULT    \
  (GNC_SYNTH_OPT_NECA|\
   GNC_SYNTH_OPT_GENERIC|\
   GNC_SYNTH_OPT_LIBARY|\
   GNC_SYNTH_OPT_LEVELS|\
   GNC_SYNTH_OPT_OUTPUTS)
int gnc_synth_on_set(gnc nc, int cell_ref, int option, double min_delay, int is_delete_empty_cells);
int gnc_SynthOnSet(gnc nc, int cell_ref, int option, double min_delay);

/* additional HL options: see below */
/* returns cell_ref or -1 */
/* min_delay is only usefull if GNC_SYNTH_OPT_DLYPATH is used */
int gnc_SynthDCL(gnc nc, pinfo *pi, dclist cl_on, dclist cl_dc, const char *cell_name, const char *lib_name, int hl_opt, int synth_option, double min_delay);
int gnc_SynthDCLByFile(gnc nc, const char *filename, const char *cell_name, const char *lib_name, int hl_opt, int synth_option, double min_delay);

int gnc_WriteCellBEX(gnc nc, int cell_ref, const char *filename);
int gnc_WriteCellPLA(gnc nc, int cell_ref, const char *filename);

/* sop.c */
int gnc_AddCellCube(gnc nc, int cell_ref, dcube *c);
pinfo *gnc_GetCellPinfo(gnc nc, int cell_ref);
/* int gnc_UpdateCellPortsWithPinfo(gnc nc, int cell_ref, int register_width); */
int gnc_InitDCUBE(gnc nc, int cell_ref);
int gnc_MinimizeCell(gnc nc, int cell_ref);
/*int gnc_SetPINFO(gnc nc, int cell_ref, int in_cnt, int out_cnt, int register_width); */
int gnc_SetDCL(gnc nc, int cell_ref, pinfo *pi, dclist cl_on, dclist cl_dc, int register_width);


/* edif.c */
int gnc_ReadEdif(gnc nc, const char *filename, int options);
int gnc_WriteEdif(gnc nc, const char *filename, const char *view_name);

/* xnf.c */
int gnc_WriteXNF(gnc nc, int cell_ref, const char *filename);

/* sygopt.c */
int gnc_ReplaceInputs(gnc nc, int cell_ref, int dest_node_ref, int sub_node_ref);
int gnc_OptGeneric(gnc nc, int cell_ref);

/* sylopt.c */
int gnc_OptLibrary(gnc nc, int cell_ref);

/* syfsm.c */
int gnc_SynthGenericRegisterToLib(gnc nc, int cell_ref, int register_node_ref);
int gnc_SynthGenericDoorToLib(gnc nc, int cell_ref, int register_node_ref);

int gnc_SetFSMDCLWithClock(gnc nc, int cell_ref, pinfo *pi, dclist cl_on, 
  int register_width, int reset_fn, int reset_is_neg);
int gnc_SetFSMDCLWithoutClock(gnc nc, int cell_ref, pinfo *pi, dclist cl_on, 
  int register_width, int reset_fn, int reset_is_neg);

int gnc_SynthGenericDoor(gnc nc, int cell_ref, int register_width);
int gnc_SynthGenericRegister(gnc nc, int cell_ref, int register_width);

/*
  synth_opt: same parameter as option in gnc_SynthOnSet:
      GNC_SYNTH_OPT_NECA
      GNC_SYNTH_OPT_GENERIC
      GNC_SYNTH_OPT_LIBARY
      GNC_SYNTH_OPT_LEVELS
      GNC_SYNTH_OPT_OUTPUTS
      GNC_SYNTH_OPT_DEFAULT
  fsm_opt:
      GNC_HL_OPT_CLR_HIGH
      GNC_HL_OPT_CLR_LOW
      GNC_HL_OPT_NO_RESET
      GNC_HL_OPT_NO_RESET_OUT_OPT
      GNC_HL_OPT_CAP_REPORT   generate capacitance report
      GNC_HL_OPT_BM_CHECK     pure burst mode check
  pla_opt:
      GNC_HL_OPT_MINIMIZE
  general:
      GNC_HL_OPT_FLATTEN      flatten design of dgd scripts
      GNC_HL_OPT_NO_DELAY     do not add delay path for asyncronous machines
      GNC_HL_OPT_FBO          Include machine output into the feedback lines      
                              (only for asynchronous machines at the moment)
      GNC_HL_OPT_MIN_STATE    Do some state minimization
          
*/

/* lowest 3 bits are used to specifiy reset behaviour */
#define GNC_HL_OPT_NO_RESET  5
#define GNC_HL_OPT_CLR_HIGH  1
#define GNC_HL_OPT_CLR_LOW   2

/* bits 4-11 are some general options */
#define GNC_HL_OPT_CLOCK        0x0010
#define GNC_HL_OPT_MINIMIZE     0x0020
#define GNC_HL_OPT_NO_DELAY     0x0040
#define GNC_HL_OPT_FLATTEN      0x0080
#define GNC_HL_OPT_FBO          0x0100
#define GNC_HL_OPT_CAP_REPORT   0x0200
#define GNC_HL_OPT_BM_CHECK     0x0400  /* do a pure burst mode hazard check */
#define GNC_HL_OPT_MIN_STATE    0x0800  /* do state minimization */

/* bits 12-15 are reserved for encoding strategies of clocked fsm's */
#define GNC_HL_OPT_ENC_MASK    0x0f000
#define GNC_HL_OPT_ENC_FAN_IN  0x01000
#define GNC_HL_OPT_ENC_IC_ALL  0x02000
#define GNC_HL_OPT_ENC_IC_PART 0x03000
#define GNC_HL_OPT_ENC_SIMPLE  0x04000

#define GNC_HL_OPT_USE_OLD_DLY    0x10000
#define GNC_HL_OPT_OLD_MIN_STATE  0x20000
#define GNC_HL_OPT_RSO            0x40000

#define GNC_HL_OPT_DEFAULT GNC_HL_OPT_CLR_HIGH|GNC_HL_OPT_CLOCK|GNC_HL_OPT_ENC_SIMPLE

int gnc_SynthFSM(gnc nc, fsm_type fsm, const char *cell_name, const char *lib_name, int hl_opt, int synth_opt);
int gnc_SynthFSMByFile(gnc nc, const char *import_file_name, const char *cell_name, const char *lib_name, int hl_opt, int synth_opt);

/* obsolete
int gnc_SynthClockedFSM(gnc nc, fsm_type fsm, const char *cell_name, const char *lib_name, int fsm_opt, int synth_opt);
int gnc_SynthClockedFSMByFile(gnc nc, const char *import_file_name, const char *cell_name, const char *lib_name, int fsm_opt, int synth_opt);
int gnc_SynthAsynchronousFSM(gnc nc, fsm_type fsm, const char *cell_name, const char *lib_name, int fsm_opt, int synth_opt);
int gnc_SynthAsynchronousFSMByFile(gnc nc, const char *import_file_name, const char *cell_name, const char *lib_name, int fsm_opt, int synth_opt);
*/

/* sydelay.c */
int gnc_InitCellDelay(gnc nc, int cell_ref);
int gnc_SetCellDelay(gnc nc, int cell_ref, int in_port_ref, int out_port_ref, double delay);
double gnc_GetCellMinDelay(gnc nc, int cell_ref, int in_port_ref, int out_port_ref);
double gnc_GetCellMaxDelay(gnc nc, int cell_ref, int in_port_ref, int out_port_ref);
double gnc_IsCellDelayUsed(gnc nc, int cell_ref, int in_port_ref, int out_port_ref);
int gnc_BuildDelayPath(gnc nc, int cell_ref, double dly, const char *in_delay_port_name, const char *out_delay_port_name);
int gnc_AddDelayPath(gnc nc, int cell_ref, double min_dly, const char *in_delay_port_name, const char *out_delay_port_name);

/* if the cell contains a fsm, register_width should be none 0 to */
/* break up the cycles */
double gnc_GetCellTotalMaxDelay(gnc nc, int cell_ref);
int gnc_CalcCellDelay(gnc nc, int cell_ref);
int gnc_ShowCellDelay(gnc nc, int cell_ref);


double gnc_CalcRequiredFeedbackDelay(gnc nc, int cell_ref, int z_port_ref);
int gnc_InsertFeedbackDelayElements(gnc nc, int cell_ref);
void gnc_ShowFSMFeedbackDelay(gnc nc, int cell_ref);

int gnc_InsertFeedbackDelay(gnc nc, int cell_ref, fsm_type fsm);
int gnc_InsertXBMFeedbackDelay(gnc nc, int cell_ref, xbm_type x);

double gnc_GetFundamentalModeXBM(gnc nc, int cell_ref, xbm_type x);


/* gnetpoti.c */
double gnc_GetCellNetInputLoad(gnc nc, int cell_ref, int net_ref);
double gnc_GetCellDefMaxDelay(gnc nc, int cell_ref, int i_port_ref, int o_port_ref, double cap);
double gnc_GetCellDefMinDelay(gnc nc, int cell_ref, int i_port_ref, int o_port_ref, double cap);
double gnc_GetCellNodeMaxDelay(gnc nc, int cell_ref, int node_ref, int i_port_ref, int o_port_ref);
double gnc_GetCellNodeMinDelay(gnc nc, int cell_ref, int node_ref, int i_port_ref, int o_port_ref);

#define GNC_CALC_CMD_NONE 0
#define GNC_CALC_CMD_RISE 1
#define GNC_CALC_CMD_FALL 2

double gnc_GetCellNodePropagationDelay(gnc nc, int cell_ref, int node_ref, 
  int calc_cmd, double transition_delay, 
  int i_port_ref, int o_port_ref);
double gnc_GetCellNodeTransitionDelay(gnc nc, int cell_ref, int node_ref, 
  int calc_cmd, double transition_delay, 
  int i_port_ref, int o_port_ref);



/* synth.c */
int gnc_SynthByFile(gnc nc, const char *import_file_name, const char *cell_name, const char *lib_name, int hl_opt, int synth_option);

/* dgd.c */
int gnc_SynthDGD(gnc nc, const char *import_file_name, int hl_opt, int synth_opt);

/* gnetdcex.c */
/* verify synthesis procedure */
int gnc_DoVerification(gnc nc, int cell_ref);

/* gnetlv.c */
/* calculate some switching capactances... requires, that */
/* the specified cell is an implementation of fsm */
/* results are stored in fsm */

void glvInit(glv_struct *glv);
int glvSetVal(glv_struct *glv, int val);

glv_struct *gnc_GetCellNetLogicVal(gnc nc, int cell_ref, int net_ref);
glv_struct *gnc_GetCellPortLogicVal(gnc nc, int cell_ref, int port_ref);
void gnc_SetAllCellNetLogicVal(gnc nc, int cell_ref, int val);
void gnc_SetAllCellPortLogicVal(gnc nc, int cell_ref, int val);

int gnc_CalculateCellNodeLogicVal(gnc nc, int cell_ref, int node_ref);

int gnc_UseInputPortValues(gnc nc, int cell_ref);
void gnc_FillOutputPortValues(gnc nc, int cell_ref);

void gnc_ApplyCellFSMInputForSimulation(gnc nc, int cell_ref, pinfo *pi, dcube *c);
void gnc_ApplyCellInputForSimulation(gnc nc, int cell_ref, dcube *c);
void gnc_ApplyCellFSMStateForSimulation(gnc nc, int cell_ref, pinfo *pi, dcube *c);
void gnc_ApplyCellStateForSimulation(gnc nc, int cell_ref, dcube *c);

/* the following three functions are required by gnetsim.c */
void gnc_ClearLogicSimulationChangeCnt(gnc nc, int cell_ref);
int gnc_GetLogicSimulationNetChangeCnt(gnc nc, int cell_ref);
double gnc_GetLogicSimulationNetLoadChange(gnc nc, int cell_ref);

/* obsolete? */
int gnc_CalculateTransitionCapacitance(gnc nc, int cell_ref, fsm_type fsm);

#endif /* _GNET_H */

