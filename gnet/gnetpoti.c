/*

  gnetpoti.c
  
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
#include <string.h>
#include <assert.h>
#include "gnet.h"
#include "b_io.h"
#include "mwc.h"

static int gxdv_get_pos(int cnt, double *vals, double v)
{
  int i;
  if ( v < vals[0] )
    return 0;
  assert(cnt >= 2);
  for( i = 1; i < cnt-1; i++ )
    if ( v < vals[i] )
      return i-1;
  return i-1;
}

static int gdxv_get_string_float_cnt(const char *s)
{
  char *delim = ", \n\t";
  int cnt = 0;
  
  s += strspn(s, delim);
  while( *s != 0 )
  {
    cnt++;
    s += strcspn(s, delim);
    s += strspn(s, delim);
  }
  return cnt;
}

static void gdxv_str_to_values(const char *s, double *vals)
{
  char *delim = ", \n\t";
  int cnt = 0;
  
  s += strspn(s, delim);
  while( *s != 0 )
  {
    vals[cnt++] = atof(s);
    s += strcspn(s, delim);
    s += strspn(s, delim);
  }
}

static int gdxv_write_doubles(FILE *fp, int cnt, double *vals)
{
  int i;
  for( i = 0; i < cnt; i++ )
    if ( b_io_WriteDouble(fp, vals[i]) == 0 )
      return 0;
  return 1;
}

static int gdxv_read_doubles(FILE *fp, int cnt, double *vals)
{
  int i;
  for( i = 0; i < cnt; i++ )
    if ( b_io_ReadDouble(fp, vals+i) == 0 )
      return 0;
  return 1;
}


/*----------------------------------------------------------------------------*/

g1dv g1dvOpen(int cnt)
{
  g1dv g1;
  g1 = (g1dv)malloc(sizeof(struct _g1dv_struct));
  if ( g1 != NULL )
  {
    memset(g1, 0, sizeof(struct _g1dv_struct));
    g1->idx_vals = (double *)malloc(sizeof(double)*cnt);
    if ( g1->idx_vals != NULL )
    {
      g1->values = (double *)malloc(sizeof(double)*cnt);
      if ( g1->values != NULL )
      {
        g1->idx_cnt = cnt;
        return g1;
      }
      free(g1->idx_vals);
    }
    free(g1);
  }
  return NULL;
}

void g1dvClose(g1dv g1)
{
  if ( g1->idx_vals != NULL )
    free(g1->idx_vals);
  if ( g1->values != NULL )
    free(g1->values);
  free(g1);
}


g1dv g1dvOpenByStr(const char *x, char *z)
{
  int xcnt;
  g1dv g1;
  xcnt = gdxv_get_string_float_cnt(x);
  if ( gdxv_get_string_float_cnt(z) == xcnt )
  {
    g1 = g1dvOpen(xcnt);
    if ( g1 != NULL )
    {
      gdxv_str_to_values(x, g1->idx_vals);
      gdxv_str_to_values(z, g1->values);
      return g1;
    }
  }  
  return NULL;
}

double g1dvCalc(g1dv g1, double x)
{
  int xpos;
  double x1, x2, z1, z2;
  double z;

  if ( g1->idx_cnt == 0 )
    return 0.0;
  
  if ( g1->idx_cnt == 1 )
    return g1->values[0];

  xpos = gxdv_get_pos(g1->idx_cnt, g1->idx_vals, x);
  
  z1 = g1->values[xpos  ];
  z2 = g1->values[xpos+1];
  
  x1 = g1->idx_vals[xpos  ];
  x2 = g1->idx_vals[xpos+1];
  
  z = (x*z1 - x2*z1 - x*z2 + x1*z2)/(x1 - x2);
  
  return z;
}

int g1dvWrite(g1dv g1, FILE *fp)
{
  if ( g1 == NULL )
  {
    if ( b_io_WriteInt(fp, 0) == 0 )                               return 0;
    return 1;
  }


  if ( b_io_WriteInt(fp, 1) == 0 )                               return 0;

  if ( b_io_WriteInt(fp, g1->idx_cnt) == 0 )                     return 0;
  if ( gdxv_write_doubles(fp, g1->idx_cnt, g1->idx_vals) == 0 )  return 0;
  if ( gdxv_write_doubles(fp, g1->idx_cnt, g1->values) == 0 )    return 0;
  return 1;
}

int g1dvRead(g1dv *g1, FILE *fp)
{
  int idx;
  if ( *g1 != NULL )
    g1dvClose(*g1);
  *g1 = NULL;

  if ( b_io_ReadInt(fp, &(idx)) == 0 )                      return 0;
  if ( idx == 0 )
    return 1;
    
  if ( b_io_ReadInt(fp, &(idx)) == 0 )                      return 0;
  
  *g1 = g1dvOpen(idx);
  if ( *g1 == NULL )                                        return 0;
  if ( gdxv_read_doubles(fp, idx, (*g1)->idx_vals) == 0 )   return 0;
  if ( gdxv_read_doubles(fp, idx, (*g1)->values) == 0 )     return 0;
  return 1;
}

size_t g1dvGetMemUsage(g1dv g1)
{
  return sizeof(struct _g1dv_struct)+2*g1->idx_cnt*sizeof(double);
}

/*----------------------------------------------------------------------------*/


g2dv g2dvOpen(int xcnt, int ycnt)
{
  g2dv g2;
  g2 = (g2dv)malloc(sizeof(struct _g2dv_struct));
  if ( g2 != NULL )
  {
    memset(g2, 0, sizeof(struct _g2dv_struct));
    g2->idx1_vals = (double *)malloc(sizeof(double)*xcnt);
    if ( g2->idx1_vals != NULL )
    {
      g2->idx2_vals = (double *)malloc(sizeof(double)*ycnt);
      if ( g2->idx2_vals != NULL )
      {
        g2->values = (double *)malloc(sizeof(double)*xcnt*ycnt);
        if ( g2->values != NULL )
        {
          g2->idx1_cnt = xcnt;
          g2->idx2_cnt = ycnt;
          return g2;
        }
        free(g2->idx2_vals);
      }
      free(g2->idx1_vals);
    }
    free(g2);
  }
  return NULL;
}

void g2dvClose(g2dv g2)
{
  if ( g2->idx1_vals != NULL )
    free(g2->idx1_vals);
  if ( g2->idx2_vals != NULL )
    free(g2->idx2_vals);
  if ( g2->values != NULL )
    free(g2->values);
  free(g2);
}

double g2dvGetVal(g2dv g2, int xpos, int ypos)
{
  return g2->values[ypos + xpos * g2->idx2_cnt];
}

void g2dvSetVal(g2dv g2, int xpos, int ypos, double val)
{
  g2->values[ypos + xpos * g2->idx2_cnt] = val;
}

g2dv g2dvOpenByStr(const char *x, const char *y, char *z)
{
  int xcnt, ycnt;
  g2dv g2;
  xcnt = gdxv_get_string_float_cnt(x);
  ycnt = gdxv_get_string_float_cnt(y);
  if ( gdxv_get_string_float_cnt(z) == xcnt*ycnt )
  {
    g2 = g2dvOpen(xcnt, ycnt);
    if ( g2 != NULL )
    {
      gdxv_str_to_values(x, g2->idx1_vals);
      gdxv_str_to_values(y, g2->idx2_vals);
      gdxv_str_to_values(z, g2->values);
      return g2;
    }
  }  
  return NULL;
}

double g2dvCalc(g2dv g2, double x, double y)
{
  int xpos = gxdv_get_pos(g2->idx1_cnt, g2->idx1_vals, x);
  int ypos = gxdv_get_pos(g2->idx2_cnt, g2->idx2_vals, y);

  double z;
  double z11, z12, z21, z22;
  double x1, x2, y1, y2;
  double a,b,c,d;
  
  z11 = g2dvGetVal(g2, xpos,   ypos  );
  z12 = g2dvGetVal(g2, xpos,   ypos+1);
  z21 = g2dvGetVal(g2, xpos+1, ypos  );
  z22 = g2dvGetVal(g2, xpos+1, ypos+1);
  
  x1 = g2->idx1_vals[xpos  ];
  x2 = g2->idx1_vals[xpos+1];

  y1 = g2->idx2_vals[ypos  ];
  y2 = g2->idx2_vals[ypos+1];
  
  /* calculated with mathematica, based on the expressons */
  /* of the synopsys library manual 'library compiler */
  /* user guide' volume 2 p2-27 */
  
  a = -((-(x2*y2*z11) + x2*y1*z12 + x1*y2*z21 - x1*y1*z22)/
      ((x1 - x2)*(y1 - y2)));
      
  b = -((y2*z11 - y1*z12 - y2*z21 + y1*z22)/
      ((x1 - x2)*(y1 - y2)));
      
  c = -((x2*z11 - x2*z12 - x1*z21 + x1*z22)/
      ((x1 - x2)*(y1 - y2)));
      
  d = -((-z11 + z12 + z21 - z22)/
      (x1*y1 - x2*y1 - x1*y2 + x2*y2));
  
  z = a + b*x + c*y + d*x*y;
  
  return z;
}

int g2dvWrite(g2dv g2, FILE *fp)
{
  if ( g2 == NULL )
  {
    if ( b_io_WriteInt(fp, 0) == 0 )                               return 0;
    return 1;
  }
  
  if ( b_io_WriteInt(fp, 1) == 0 )                                 return 0;
    
  if ( b_io_WriteInt(fp, g2->idx1_cnt) == 0 )                      return 0;
  if ( b_io_WriteInt(fp, g2->idx2_cnt) == 0 )                      return 0;
  if ( gdxv_write_doubles(fp, g2->idx1_cnt, g2->idx1_vals) == 0 )  return 0;
  if ( gdxv_write_doubles(fp, g2->idx2_cnt, g2->idx2_vals) == 0 )  return 0;
  if ( gdxv_write_doubles(fp, g2->idx1_cnt*g2->idx2_cnt, g2->values) == 0 )    return 0;
  return 1;
}

int g2dvRead(g2dv *g2, FILE *fp)
{
  int idx1, idx2;
  if ( *g2 != NULL )
    g2dvClose(*g2);
  *g2 = NULL;

  if ( b_io_ReadInt(fp, &(idx1)) == 0 )                        return 0;
  if ( idx1 == 0 )
    return 1;
    
  if ( b_io_ReadInt(fp, &(idx1)) == 0 )                        return 0;
  if ( b_io_ReadInt(fp, &(idx2)) == 0 )                        return 0;
  
  *g2 = g2dvOpen(idx1, idx2);
  if ( *g2 == NULL )                                           return 0;
  if ( gdxv_read_doubles(fp, idx1, (*g2)->idx1_vals) == 0 )    return 0;
  if ( gdxv_read_doubles(fp, idx2, (*g2)->idx2_vals) == 0 )    return 0;
  if ( gdxv_read_doubles(fp, idx1*idx2, (*g2)->values) == 0 )  return 0;
  return 1;
}

size_t g2dvGetMemUsage(g2dv g2)
{
  return sizeof(struct _g2dv_struct)+
    g2->idx1_cnt*sizeof(double)+
    g2->idx2_cnt*sizeof(double)+
    g2->idx2_cnt*g2->idx1_cnt*sizeof(double);
}


/*----------------------------------------------------------------------------*/

gpoti gpotiOpen(void)
{
  gpoti pt;
  pt = (gpoti)malloc(sizeof(struct _gpoti_struct));
  if ( pt != NULL )
  {
    pt->related_port_ref  = -1;
    pt->rise_block_delay  = 0.0;  /* synopsys: intrinsic_rise */
    pt->rise_fanout_delay = 0.0;  /* synopsys: rise_resistance */
    pt->fall_block_delay  = 0.0;  /* synopsys: intrinsic_fall */
    pt->fall_fanout_delay = 0.0;  /* synopsys: fall_resistance */
    
    pt->rise_cell = NULL;
    pt->rise_propagation = NULL;
    pt->rise_transition = NULL;
    pt->fall_cell = NULL;
    pt->fall_propagation = NULL;
    pt->fall_transition = NULL;
    
    return pt;
  }
  return NULL;
}

void gpotiClose(gpoti pt)
{

  if ( pt->rise_cell != NULL )
    g2dvClose(pt->rise_cell);
  if ( pt->rise_propagation != NULL )
    g1dvClose(pt->rise_propagation);
  if ( pt->rise_transition != NULL )
    g1dvClose(pt->rise_transition);

  if ( pt->fall_cell != NULL )
    g2dvClose(pt->fall_cell);
  if ( pt->fall_propagation != NULL )
    g1dvClose(pt->fall_propagation);
  if ( pt->fall_transition != NULL )
    g1dvClose(pt->fall_transition);

  free(pt);
}

int gpotiWrite(gpoti pt, FILE *fp)
{
  if ( b_io_WriteInt(fp, pt->related_port_ref) == 0 )               return 0;
  if ( b_io_WriteDouble(fp, pt->rise_block_delay) == 0 )            return 0;
  if ( b_io_WriteDouble(fp, pt->rise_fanout_delay) == 0 )           return 0;
  if ( b_io_WriteDouble(fp, pt->fall_block_delay) == 0 )            return 0;
  if ( b_io_WriteDouble(fp, pt->fall_fanout_delay) == 0 )           return 0;
  if ( g2dvWrite(pt->rise_cell, fp) == 0 )                          return 0;
  if ( g1dvWrite(pt->rise_propagation, fp) == 0 )                   return 0;
  if ( g1dvWrite(pt->rise_transition, fp) == 0 )                    return 0;
  if ( g2dvWrite(pt->fall_cell, fp) == 0 )                          return 0;
  if ( g1dvWrite(pt->fall_propagation, fp) == 0 )                   return 0;
  if ( g1dvWrite(pt->fall_transition, fp) == 0 )                    return 0;
  return 1;
}

int gpotiRead(gpoti pt, FILE *fp)
{
  if ( b_io_ReadInt(fp, &(pt->related_port_ref)) == 0 )            return 0;
  if ( b_io_ReadDouble(fp, &(pt->rise_block_delay)) == 0 )         return 0;
  if ( b_io_ReadDouble(fp, &(pt->rise_fanout_delay)) == 0 )        return 0;
  if ( b_io_ReadDouble(fp, &(pt->fall_block_delay)) == 0 )         return 0;
  if ( b_io_ReadDouble(fp, &(pt->fall_fanout_delay)) == 0 )        return 0;
  if ( g2dvRead(&(pt->rise_cell), fp) == 0 )                       return 0;
  if ( g1dvRead(&(pt->rise_propagation), fp) == 0 )                return 0;
  if ( g1dvRead(&(pt->rise_transition), fp) == 0 )                 return 0;
  if ( g2dvRead(&(pt->fall_cell), fp) == 0 )                       return 0;
  if ( g1dvRead(&(pt->fall_propagation), fp) == 0 )                return 0;
  if ( g1dvRead(&(pt->fall_transition), fp) == 0 )                 return 0;
  return 1;
}

size_t gpotiGetMemUsage(gpoti pt)
{
  return sizeof(struct _gpoti_struct)+
    g2dvGetMemUsage(pt->rise_cell)+
    g1dvGetMemUsage(pt->rise_propagation)+
    g1dvGetMemUsage(pt->rise_transition)+
    g2dvGetMemUsage(pt->fall_cell)+
    g1dvGetMemUsage(pt->fall_propagation)+
    g1dvGetMemUsage(pt->fall_transition);
}


/*----------------------------------------------------------------------------*/

double gnc_GetCellNetInputLoad(gnc nc, int cell_ref, int net_ref)
{
  int join_ref = -1;
  int node_cell_ref;
  int port_ref;
  double cap = 0.0;
  while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
  {
    node_cell_ref = gnc_GetCellNetNodeCell(nc, cell_ref, net_ref, join_ref);
    if ( node_cell_ref >= 0 )
    {
      port_ref = gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref);
      if ( gnc_GetCellPortType(nc, node_cell_ref, port_ref) != GPORT_TYPE_OUT )
      {
        cap += gnc_GetCellPortInputLoad(nc, node_cell_ref, port_ref);
      }
    }
  }
  return cap;
}

static int gnc_calc_delay_parameter(gnc nc, int cell_ref, int node_ref, int i_port_ref, int o_port_ref, gpoti *poti, int *o_net_ref, double *cap)
{
  int node_cell_ref;
  
  /* find the GPOTI structure */
  
  node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
  if ( node_cell_ref < 0 )
    return 0;
  
  *poti  = gnc_GetCellPortGPOTI(nc, node_cell_ref, i_port_ref, o_port_ref, 0);
  if ( *poti == NULL )
    return 0;

  /* find the net, that should be driven by the node ... */
  
  *o_net_ref = gnc_FindCellNetByPort(nc, cell_ref, node_ref, o_port_ref);
  if ( *o_net_ref < 0 )
    return 0;
  
  /* ... and calculate the capacitance of the net */
  
  *cap = gnc_GetCellNetInputLoad(nc, cell_ref, *o_net_ref);
  
  return 1;
}

/*
  returns the maximum transistion delay that can occur on net 'net_ref'.
  cap value is optional. If cap is < 0.0, cap is recalulated.
  if cap >= 0.0 it is assumed that cap has been calculated with
  gnc_GetCellNetInputLoad(): cap is the load of the net.
*/
double gnc_GetMaxTransitionDelay(gnc nc, int cell_ref, int net_ref, double cap)
{
  double v1, v2;
  double delay, max_delay;
  gpoti poti;
  int join_ref;
  int node_ref;
  int node_cell_ref;

  int i_port_ref;
  int o_port_ref;
  
  if ( cap < 0.0 )
    cap = gnc_GetCellNetInputLoad(nc, cell_ref, net_ref);
  max_delay = -1.0;
  
  join_ref = -1;
  while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
  {
    node_ref = gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
    if ( node_ref >= 0 )
    {
      if ( gnc_GetCellNetPortType(nc, cell_ref, net_ref, join_ref) == GPORT_TYPE_OUT )
      {
        node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);
        o_port_ref = gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref);
        
        i_port_ref = -1;
        while( gnc_LoopCellPort(nc, node_cell_ref, &i_port_ref) != 0 )
        {
          if ( gnc_GetCellPortType(nc, node_cell_ref, i_port_ref) == GPORT_TYPE_IN )
          {
            poti = gnc_GetCellPortGPOTI(nc, node_cell_ref, i_port_ref, o_port_ref, 0);
            if ( poti != NULL )
            {
              if ( poti->fall_transition != NULL && poti->rise_transition != NULL )
              {
                v1 = g1dvCalc(poti->fall_transition, cap);
                v2 = g1dvCalc(poti->rise_transition, cap);
                delay = v1 > v2 ? v1 : v2;
                if ( max_delay < delay )
                  max_delay = delay;
              }
            }
          }
        } /* while */
      }
    }
  } /* while */

  return max_delay;    
}


double gnc_GetCellDefMaxDelay(gnc nc, int cell_ref, int i_port_ref, int o_port_ref, double cap)
{
  double v1, v2;
  gpoti poti  = gnc_GetCellPortGPOTI(nc, cell_ref, i_port_ref, o_port_ref, 0);
  if ( poti == NULL )
    return 0;
    
  if ( poti->rise_cell != NULL && poti->rise_transition != NULL && poti->fall_cell != NULL && poti->fall_transition != NULL)
  {
    double transition_delay;
    
    transition_delay = 0.0;
  
    v1 = g2dvCalc(poti->rise_cell, transition_delay, cap);
    v2 = g2dvCalc(poti->fall_cell, transition_delay, cap);
  }
  else
  {
    v1 = poti->rise_block_delay + cap*poti->rise_fanout_delay;
    v2 = poti->fall_block_delay + cap*poti->fall_fanout_delay;
  }
  
  return v1 > v2 ? v1 : v2;
}

double gnc_GetCellDefMinDelay(gnc nc, int cell_ref, int i_port_ref, int o_port_ref, double cap)
{
  double v1, v2;
  gpoti poti  = gnc_GetCellPortGPOTI(nc, cell_ref, i_port_ref, o_port_ref, 0);
  if ( poti == NULL )
    return 0;
    
  if ( poti->rise_cell != NULL && poti->rise_transition != NULL && poti->fall_cell != NULL && poti->fall_transition != NULL)
  {
    double transition_delay;
    
    transition_delay = 0.0;
  
    v1 = g2dvCalc(poti->rise_cell, transition_delay, cap);
    v2 = g2dvCalc(poti->fall_cell, transition_delay, cap);
  }
  else
  {
    v1 = poti->rise_block_delay + cap*poti->rise_fanout_delay;
    v2 = poti->fall_block_delay + cap*poti->fall_fanout_delay;
  }
  
  return v1 < v2 ? v1 : v2;
}

/*
  calculate the delay of a cell (a node) from the specified input pin
  to the specified output pin.
*/
double gnc_GetCellNodeMaxDelay(gnc nc, int cell_ref, int node_ref, int i_port_ref, int o_port_ref)
{
  gpoti poti;
  int net_ref;
  double cap;
  double v1, v2;

  if ( gnc_calc_delay_parameter(nc, cell_ref, node_ref, i_port_ref, o_port_ref, &poti, &net_ref, &cap) == 0 )
    return 0.0;
  
  if ( poti->rise_cell != NULL && poti->rise_transition != NULL && poti->fall_cell != NULL && poti->fall_transition != NULL)
  {
    double transition_delay;
    
    /* net_ref is the output net: is this correct????? */
    transition_delay = gnc_GetMaxTransitionDelay(nc, cell_ref, net_ref, cap);
  
    v1 = g2dvCalc(poti->rise_cell, transition_delay, cap);
    v2 = g2dvCalc(poti->fall_cell, transition_delay, cap);
  }
  else
  {
    v1 = poti->rise_block_delay + cap*poti->rise_fanout_delay;
    v2 = poti->fall_block_delay + cap*poti->fall_fanout_delay;
  }
  
  return v1 > v2 ? v1 : v2;
}


/*
  calculate the delay of a cell (a node) from the specified input pin
  to the specified output pin.
*/
double gnc_GetCellNodeMinDelay(gnc nc, int cell_ref, int node_ref, int i_port_ref, int o_port_ref)
{
  gpoti poti;
  int net_ref;
  double cap;
  double v1, v2;

  if ( gnc_calc_delay_parameter(nc, cell_ref, node_ref, i_port_ref, o_port_ref, &poti, &net_ref, &cap) == 0 )
    return 0.0;
  
  if ( poti->rise_cell != NULL && poti->rise_transition != NULL && poti->fall_cell != NULL && poti->fall_transition != NULL)
  {
    double transition_delay;
    
    /* net_ref is the output net: is this correct????? */
    transition_delay = gnc_GetMaxTransitionDelay(nc, cell_ref, net_ref, cap);
  
    v1 = g2dvCalc(poti->rise_cell, transition_delay, cap);
    v2 = g2dvCalc(poti->fall_cell, transition_delay, cap);
  }
  else
  {
    v1 = poti->rise_block_delay + cap*poti->rise_fanout_delay;
    v2 = poti->fall_block_delay + cap*poti->fall_fanout_delay;
  }
  
  return v1 < v2 ? v1 : v2;
}

/*---- rise/fall calculation ------------------------------------------------*/

double gnc_GetCellNodePropagationDelay(gnc nc, int cell_ref, int node_ref, 
  int calc_cmd, double transition_delay, 
  int i_port_ref, int o_port_ref)
{
  gpoti poti;
  int net_ref;
  double cap;

  if ( gnc_calc_delay_parameter(nc, cell_ref, node_ref, i_port_ref, o_port_ref, &poti, &net_ref, &cap) == 0 )
    return 0.0;
  
  if ( poti->rise_cell != NULL && poti->rise_transition != NULL && poti->fall_cell != NULL && poti->fall_transition != NULL)
  {
    switch(calc_cmd)
    {
      case GNC_CALC_CMD_RISE:
        return g2dvCalc(poti->rise_cell, transition_delay, cap);
      case GNC_CALC_CMD_FALL:
        return g2dvCalc(poti->fall_cell, transition_delay, cap);
    }
  }
  else
  {
    switch(calc_cmd)
    {
      case GNC_CALC_CMD_RISE:
        return poti->rise_block_delay + cap*poti->rise_fanout_delay;
      case GNC_CALC_CMD_FALL:
        return poti->fall_block_delay + cap*poti->fall_fanout_delay;
    }
  }

  return 0.0;
}

double gnc_GetCellNodeTransitionDelay(gnc nc, int cell_ref, int node_ref, 
  int calc_cmd, double transition_delay, 
  int i_port_ref, int o_port_ref)
{
  gpoti poti;
  int net_ref;
  double cap;

  if ( gnc_calc_delay_parameter(nc, cell_ref, node_ref, i_port_ref, o_port_ref, &poti, &net_ref, &cap) == 0 )
    return 0.0;
  
  if ( poti->rise_cell != NULL && poti->rise_transition != NULL && poti->fall_cell != NULL && poti->fall_transition != NULL)
  {
    switch(calc_cmd)
    {
      case GNC_CALC_CMD_RISE:
        return g1dvCalc(poti->rise_transition, cap);
      case GNC_CALC_CMD_FALL:
        return g1dvCalc(poti->fall_transition, cap);
    }
  }
  else
  { 
    return 0.0;
  }

  return 0.0;
}

