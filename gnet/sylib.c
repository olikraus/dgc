/*

  SYLIB.C

  read synopsys library file into the gate-lib

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
#include <stdio.h>
#include "gnet.h"
#include "sc_util.h"
#include "sc_io.h"
#include "sc_phr.h"
#include "sc_bexpr.h"
#include "dcube.h"
#include <assert.h>
#include "mwc.h"
#include "b_set.h"



struct _ltt_struct
{
  char *name;
  char *var1;
  char *idx1;
  char *var2;
  char *idx2;
};
typedef struct _ltt_struct *ltt_type;

#define SYLIB_PIN_MAX (32+8)


struct _sylib_struct
{
  gnc nc;
  const char *user_library_name;
  char *cell_name;
  char *library_name;
  int gnc_port_index[SYLIB_PIN_MAX];
  char *function[SYLIB_PIN_MAX];
  char *port_name[SYLIB_PIN_MAX];
  char *direction[SYLIB_PIN_MAX];
  double capacitance[SYLIB_PIN_MAX];
  int cell_ref;
  sc_be_type be;
  dcube c; /* local variable of synlib_generate_clist */
  pinfo pi;
  
  
  int pin_cnt;
  int timing_cnt;

  int ignore_cell;
  
  char *related_pin[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  double intrinsic_rise[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  double intrinsic_fall[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  double rise_resistance[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  double fall_resistance[SYLIB_PIN_MAX][SYLIB_PIN_MAX];

  char *rise_propagation_template[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  char *rise_propagation_values[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  char *rise_transition_template[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  char *rise_transition_values[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  char *cell_rise_template[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  char *cell_rise_values[SYLIB_PIN_MAX][SYLIB_PIN_MAX];

  char *fall_propagation_template[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  char *fall_propagation_values[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  char *fall_transition_template[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  char *fall_transition_values[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  char *cell_fall_template[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  char *cell_fall_values[SYLIB_PIN_MAX][SYLIB_PIN_MAX];
  
  char *ff_out_name;

  char *ff_inv_out_name;
  char *ff_next_state_expr;
  char *ff_clocked_on_expr;
  char *ff_clocked_on_also_expr;
  char *ff_clear_expr;
  char *ff_preset_expr;
  
  double cell_area;
  
  b_set_type ltt_set;
  int ltt_curr;
  
};
typedef struct _sylib_struct sylib_struct;
typedef struct _sylib_struct *sylib_type;


static int internal_sylib_set_name(char **s, char *name)
{
  if ( *s != NULL )
    free(*s);
  *s = NULL;
  if ( name != NULL )
  {
    *s = strdup(name);
    if ( *s == NULL )
      return 0;
  }
  return 1;
}

static ltt_type ltt_Open()
{
  ltt_type ltt;
  ltt = (ltt_type)malloc(sizeof(struct _ltt_struct));
  if ( ltt != NULL )
  {
    ltt->name = NULL;
    ltt->var1 = NULL;
    ltt->idx1 = NULL;
    ltt->var2 = NULL;
    ltt->idx2 = NULL;
    return ltt;
  }
  return NULL;
}

static void ltt_Close(ltt_type ltt)
{
  internal_sylib_set_name(&(ltt->var1), NULL);
  internal_sylib_set_name(&(ltt->idx1), NULL);
  internal_sylib_set_name(&(ltt->var2), NULL);
  internal_sylib_set_name(&(ltt->idx2), NULL);
  free(ltt);
}

#define sylib_SetCellName(sylib, name)\
  internal_sylib_set_name(&(sylib->cell_name), name)
#define sylib_SetLibraryName(sylib, name)\
  internal_sylib_set_name(&(sylib->library_name), name)
#define sylib_SetFunction(sylib, pos, name)\
  internal_sylib_set_name(&(sylib->function[pos]), name)
#define sylib_SetPortName(sylib, pos, name)\
  internal_sylib_set_name(&(sylib->port_name[pos]), name)
#define sylib_SetDirection(sylib, pos, name)\
  internal_sylib_set_name(&(sylib->direction[pos]), name)
#define sylib_SetRelatedPin(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->related_pin[pos1][pos2]), name)

#define sylib_SetRisePropagationTemplate(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->rise_propagation_template[pos1][pos2]), name)
#define sylib_SetRisePropagationValues(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->rise_propagation_values[pos1][pos2]), name)

#define sylib_SetRiseTransitionTemplate(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->rise_transition_template[pos1][pos2]), name)
#define sylib_SetRiseTransitionValues(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->rise_transition_values[pos1][pos2]), name)

#define sylib_SetCellRiseTemplate(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->cell_rise_template[pos1][pos2]), name)
#define sylib_SetCellRiseValues(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->cell_rise_values[pos1][pos2]), name)

#define sylib_SetFallPropagationTemplate(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->fall_propagation_template[pos1][pos2]), name)
#define sylib_SetFallPropagationValues(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->fall_propagation_values[pos1][pos2]), name)

#define sylib_SetFallTransitionTemplate(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->fall_transition_template[pos1][pos2]), name)
#define sylib_SetFallTransitionValues(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->fall_transition_values[pos1][pos2]), name)

#define sylib_SetCellFallTemplate(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->cell_fall_template[pos1][pos2]), name)
#define sylib_SetCellFallValues(sylib, pos1, pos2, name)\
  internal_sylib_set_name(&(sylib->cell_fall_values[pos1][pos2]), name)

#define sylib_SetFFOutName(sylib, name)\
  internal_sylib_set_name(&(sylib->ff_out_name), name)
#define sylib_SetFFInvOutName(sylib, name)\
  internal_sylib_set_name(&(sylib->ff_inv_out_name), name)
#define sylib_SetFFNextStateExpr(sylib, name)\
  internal_sylib_set_name(&(sylib->ff_next_state_expr), name)
#define sylib_SetFFClockedOnExpr(sylib, name)\
  internal_sylib_set_name(&(sylib->ff_clocked_on_expr), name)
#define sylib_SetFFClockedOnAlsoExpr(sylib, name)\
  internal_sylib_set_name(&(sylib->ff_clocked_on_also_expr), name)
#define sylib_SetFFClearExpr(sylib, name)\
  internal_sylib_set_name(&(sylib->ff_clear_expr), name)
#define sylib_SetFFPresetExpr(sylib, name)\
  internal_sylib_set_name(&(sylib->ff_preset_expr), name)

#define sylib_SetLTTName(sl, arg) \
  internal_sylib_set_name(\
  &(((ltt_type)b_set_Get((sl)->ltt_set, (sl)->ltt_curr))->name), (arg))
#define sylib_SetLTTIdx1(sl, arg) \
  internal_sylib_set_name(\
  &(((ltt_type)b_set_Get((sl)->ltt_set, (sl)->ltt_curr))->idx1), (arg))
#define sylib_SetLTTVar1(sl, arg) \
  internal_sylib_set_name(\
  &(((ltt_type)b_set_Get((sl)->ltt_set, (sl)->ltt_curr))->var1), (arg))
#define sylib_SetLTTIdx2(sl, arg) \
  internal_sylib_set_name(\
  &(((ltt_type)b_set_Get((sl)->ltt_set, (sl)->ltt_curr))->idx2), (arg))
#define sylib_SetLTTVar2(sl, arg) \
  internal_sylib_set_name(\
  &(((ltt_type)b_set_Get((sl)->ltt_set, (sl)->ltt_curr))->var2), (arg))
  

sylib_type sylib_Open(gnc nc, const char *user_library_name)
{
  sylib_type sl;
  int i, j;
  sl = malloc(sizeof(sylib_struct));
  if ( sl != NULL )
  {
    sl->nc = nc;
    sl->user_library_name = user_library_name;
    sl->cell_name = NULL;
    sl->library_name = NULL;
    sl->ignore_cell = 0;
    for( i = 0; i < SYLIB_PIN_MAX; i++ )
    {
      sl->gnc_port_index[i] = 0;
      sl->function[i] = NULL;
      sl->port_name[i] = NULL;
      sl->direction[i] = NULL;
      sl->capacitance[i] = 1.0;
      
      for( j = 0; j < SYLIB_PIN_MAX; j++ )
      {
        sl->related_pin[i][j] = NULL;
        
        sl->intrinsic_rise[i][j] = 0.0;
        sl->intrinsic_fall[i][j] = 0.0;
        sl->rise_resistance[i][j] = 0.0;
        sl->fall_resistance[i][j] = 0.0;

        sl->rise_propagation_template[i][j] = NULL;
        sl->rise_propagation_values[i][j] = NULL;
        sl->rise_transition_template[i][j] = NULL;
        sl->rise_transition_values[i][j] = NULL;
        sl->cell_rise_template[i][j] = NULL;
        sl->cell_rise_values[i][j] = NULL;

        sl->fall_propagation_template[i][j] = NULL;
        sl->fall_propagation_values[i][j] = NULL;
        sl->fall_transition_template[i][j] = NULL;
        sl->fall_transition_values[i][j] = NULL;
        sl->cell_fall_template[i][j] = NULL;
        sl->cell_fall_values[i][j] = NULL;
      }
    }
    sl->cell_ref = -1;
    sl->pin_cnt = 0;
    sl->timing_cnt = 0;
    
    sl->ff_out_name = NULL;
    sl->ff_inv_out_name = NULL;
    sl->ff_next_state_expr = NULL;
    sl->ff_clocked_on_expr = NULL;
    sl->ff_clocked_on_also_expr = NULL;
    sl->ff_clear_expr = NULL;
    sl->ff_preset_expr = NULL;
    
    sl->ltt_curr = -1;
    
    sl->be = sc_be_Open();
    if ( sl->be != NULL )
    {
      sl->ltt_set = b_set_Open();
      if ( sl->ltt_set != NULL )
      {
        if ( pinfoInit(&(sl->pi)) != 0 )
        {
          return sl;
        }
        b_set_Close(sl->ltt_set);
      }
      sc_be_Close(sl->be);
    }
    free(sl);
  }
  return NULL;
}

void sylib_Close(sylib_type sl)
{
  int i, j;
  
  i = -1;
  while(b_set_WhileLoop(sl->ltt_set, &i) != 0)
    ltt_Close((ltt_type)b_set_Get(sl->ltt_set, i));
    
  b_set_Close(sl->ltt_set);
  
  sylib_SetCellName(sl, NULL);
  sylib_SetLibraryName(sl, NULL);
  for( i = 0; i < SYLIB_PIN_MAX; i++ )
  {
    sylib_SetFunction(sl, i, NULL);
    sylib_SetPortName(sl, i, NULL);
    sylib_SetDirection(sl, i, NULL);
    
    for( j = 0; j < SYLIB_PIN_MAX; j++ )
    {
      sylib_SetRelatedPin(sl, i, j, NULL);
      sl->intrinsic_rise[i][j] = 0.0;
      sl->intrinsic_fall[i][j] = 0.0;
      sl->rise_resistance[i][j] = 0.0;
      sl->fall_resistance[i][j] = 0.0;
      
      sylib_SetRisePropagationTemplate(sl, i, j, NULL);
      sylib_SetRisePropagationValues(sl, i, j, NULL);

      sylib_SetRiseTransitionTemplate(sl, i, j, NULL);
      sylib_SetRiseTransitionValues(sl, i, j, NULL);

      sylib_SetCellRiseTemplate(sl, i, j, NULL);
      sylib_SetCellRiseValues(sl, i, j, NULL);

      sylib_SetFallPropagationTemplate(sl, i, j, NULL);
      sylib_SetFallPropagationValues(sl, i, j, NULL);

      sylib_SetFallTransitionTemplate(sl, i, j, NULL);
      sylib_SetFallTransitionValues(sl, i, j, NULL);

      sylib_SetCellFallTemplate(sl, i, j, NULL);
      sylib_SetCellFallValues(sl, i, j, NULL);
    }
  }

  sylib_SetFFOutName(sl, NULL);
  sylib_SetFFInvOutName(sl, NULL);
  sylib_SetFFNextStateExpr(sl, NULL);
  sylib_SetFFClockedOnExpr(sl, NULL);
  sylib_SetFFClockedOnAlsoExpr(sl, NULL);
  sylib_SetFFClearExpr(sl, NULL);
  sylib_SetFFPresetExpr(sl, NULL);

  pinfoDestroy(&(sl->pi));
  sc_be_Close(sl->be);
  free(sl);
} 

int sylib_AddLTT(sylib_type sl)
{
  ltt_type ltt = ltt_Open();
  if ( ltt != NULL )
  {
    sl->ltt_curr = b_set_Add(sl->ltt_set, ltt);
    if ( sl->ltt_curr >= 0 )
    {
      return 1;
    }
    ltt_Close(ltt);
  }
  return 0;
}

ltt_type sylib_FindLTT(sylib_type sl, char *name)
{
  ltt_type ltt;
  int i = -1;
  while(b_set_WhileLoop(sl->ltt_set, &i) != 0)
  {
    ltt = (ltt_type)b_set_Get(sl->ltt_set, i);
    if ( sc_strcmp(ltt->name, name) == 0 )
      return ltt;
  }

  gnc_Error(sl->nc, "Synopsys import: Template '%s' not found.", name);  
  return NULL;
}

int synlib_do_msg_aux_start(sc_phr_type phr, void *data)
{
  sc_phr_SetLocalData(phr, sc_phr_GetUserData(phr));
  return 1;
}

int synlib_do_msg_aux_end(sc_phr_type phr, void *data)
{
  return 1;
}

int synlib_do_msg_grp_start(sc_phr_type phr, void *data)
{
  sc_phr_grp_struct *grp = (sc_phr_grp_struct *)data;
  sylib_type sl = sc_phr_GetLocalData(phr);
  int i, j;
  
  if ( sc_strcmp(grp->name, "cell") == 0 )
  {
    
    sl->pin_cnt = 0;
    sl->ignore_cell = 0;
    
    sylib_SetCellName(sl, NULL);
    for( i = 0; i < SYLIB_PIN_MAX; i++ )
    {
      sl->gnc_port_index[i] = -1;
      sylib_SetFunction(sl, i, NULL);
      sylib_SetPortName(sl, i, NULL);
      sylib_SetDirection(sl, i, NULL);
      sl->capacitance[i] = 1.0;
      
      for( j = 0; j < SYLIB_PIN_MAX; j++ )
      {
        sylib_SetRelatedPin(sl, i, j, NULL);
        sl->intrinsic_rise[i][j] = 0.0;
        sl->intrinsic_fall[i][j] = 0.0;
        sl->rise_resistance[i][j] = 0.0;
        sl->fall_resistance[i][j] = 0.0;
      }
    
    }

    sylib_SetFFOutName(sl, NULL);
    sylib_SetFFInvOutName(sl, NULL);
    sylib_SetFFNextStateExpr(sl, NULL);
    sylib_SetFFClockedOnExpr(sl, NULL);
    sylib_SetFFClockedOnAlsoExpr(sl, NULL);
    sylib_SetFFClearExpr(sl, NULL);
    sylib_SetFFPresetExpr(sl, NULL);
    
    sl->pin_cnt = 0;
    sl->cell_area = 0.0;
    
    if ( sylib_SetCellName(sl, grp->s1) == 0 )
      return 0;
    if( sl->user_library_name != NULL )
      sl->cell_ref = gnc_AddCell(sl->nc, sl->cell_name, sl->user_library_name);
    else
      sl->cell_ref = gnc_AddCell(sl->nc, sl->cell_name, sl->library_name);
    if ( sl->cell_ref < 0 )
      return 0;
      
    /* printf("%d: %s\n", sl->cell_ref, sl->cell_name); */
  }
  else if ( sc_strcmp(grp->name, "pin") == 0 )
  {
    sl->capacitance[sl->pin_cnt] = 1.0;
    sl->timing_cnt = 0;
    if ( sylib_SetPortName(sl, sl->pin_cnt, grp->s1) == 0 )
      return 0;
  }
  else if ( sc_strcmp(grp->name, "library") == 0 )
  {
    if ( sylib_SetLibraryName(sl, grp->s1) == 0 )
      return 0;
  }
  else if ( sc_strcmp(grp->name, "ff") == 0 )
  {
    sylib_SetFFOutName(sl, grp->s1);
    sylib_SetFFInvOutName(sl, grp->s2);
  }
  else if ( sc_strcmp(grp->name, "rise_propagation") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sylib_SetRisePropagationTemplate(sl, sl->pin_cnt, sl->timing_cnt, grp->s1) == 0 )
          return 0;
      }
    }
  }
  else if ( sc_strcmp(grp->name, "rise_transition") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sylib_SetRiseTransitionTemplate(sl, sl->pin_cnt, sl->timing_cnt, grp->s1) == 0 )
          return 0;
      }
    }
  }
  else if ( sc_strcmp(grp->name, "cell_rise") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sylib_SetCellRiseTemplate(sl, sl->pin_cnt, sl->timing_cnt, grp->s1) == 0 )
          return 0;
      }
    }
  }
  else if ( sc_strcmp(grp->name, "fall_propagation") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sylib_SetFallPropagationTemplate(sl, sl->pin_cnt, sl->timing_cnt, grp->s1) == 0 )
          return 0;
      }
    }
  }
  else if ( sc_strcmp(grp->name, "fall_transition") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sylib_SetFallTransitionTemplate(sl, sl->pin_cnt, sl->timing_cnt, grp->s1) == 0 )
          return 0;
      }
    }
  }
  else if ( sc_strcmp(grp->name, "cell_fall") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sylib_SetCellFallTemplate(sl, sl->pin_cnt, sl->timing_cnt, grp->s1) == 0 )
          return 0;
      }
    }
  }
  else if ( sc_strcmp(grp->name, "lu_table_template") == 0 )
  {
    if ( sylib_AddLTT(sl) == 0 )
      return 0;
    if ( sylib_SetLTTName(sl, grp->s1) == 0 )
      return 0;
  }
  return 1;
}

int synlib_FindPin(sylib_type sl, char *name)
{
  int i;
  for( i = 0; i < sl->pin_cnt; i++ )
  {
    if ( sc_strcmp(sl->port_name[i], name) == 0 )
      return i;
  }
  return -1;
}

int synlib_FindPinByFn(sylib_type sl, char *fn)
{
  int i;
  for( i = 0; i < sl->pin_cnt; i++ )
  { 
    if ( sl->function[i] != NULL )
      if ( sc_strcmp(sl->function[i], fn) == 0 )
        return i;
  }
  return -1;
}

int sylib_AddPortByIndex(sylib_type sl, int sylib_pin_index, int gnet_fn, int is_inv)
{
  int gnet_type;
  int gnet_port_index;
  
  if ( sc_strcmp(sl->direction[sylib_pin_index], "input") == 0 )
    gnet_type = GPORT_TYPE_IN;
  else if ( sc_strcmp(sl->direction[sylib_pin_index], "output") == 0 )
    gnet_type = GPORT_TYPE_OUT;
  else
    return 0;
    
  gnet_port_index = gnc_AddCellPort(sl->nc, 
    sl->cell_ref, gnet_type, sl->port_name[sylib_pin_index]);
  if ( gnet_port_index < 0)
    return 0;
    
  gnc_SetCellPortFn(sl->nc, sl->cell_ref, gnet_port_index, gnet_fn, is_inv);

  gnc_SetCellPortInputLoad(sl->nc, sl->cell_ref, gnet_port_index, sl->capacitance[sylib_pin_index]);
  
  sl->gnc_port_index[sylib_pin_index] = gnet_port_index;

  return 1;  
}

int sylib_g2dv(sylib_type sl, g2dv *dest, char *template, char *values)
{
  ltt_type ltt;
  
  if ( template == NULL || values == NULL )
    return 1;
  
  ltt = sylib_FindLTT(sl, template);
  if ( ltt == NULL )
    return 0;
  
  if ( ltt->var1 == NULL || ltt->var2 == NULL )
  {
    gnc_Log(sl->nc, 3, "Synopsys import: Two dimensional template expected (template '%s', cell '%s').", template, sl->cell_name);
    return 1;
  }
  
  if ( *dest != NULL )
    g2dvClose(*dest);

  *dest = g2dvOpenByStr(ltt->idx1, ltt->idx2, values);
  if ( *dest == NULL )
    return gnc_Error(sl->nc, "Synopsys import: Out of Memory (g2dvOpenByStr)."), 0;

  return 1;  
}

int sylib_g1dv(sylib_type sl, g1dv *dest, char *template, char *values)
{
  ltt_type ltt;
  
  if ( template == NULL || values == NULL )
    return 1;
  
  ltt = sylib_FindLTT(sl, template);
  if ( ltt == NULL )
    return 0;

  if ( ltt->var1 == NULL || ltt->var2 != NULL )
  {
    gnc_Error(sl->nc, "Synopsys import: One dimensional template expected (template '%s', cell '%s').", template, sl->cell_name);
    return 1;
  }
    
  if ( *dest != NULL )
    g1dvClose(*dest);

  *dest = g1dvOpenByStr(ltt->idx1, values);
  if ( *dest == NULL )
    return gnc_Error(sl->nc, "Synopsys import: Out of Memory (g1dvOpenByStr)."), 0;

  return 1;  
}

int sylib_AddTimingByIndex(sylib_type sl, int i)  
{
  int j;
  int pin_index = -1;
  int related_index = -1;
  gpoti poti;
  for( j = 0; j < SYLIB_PIN_MAX; j++ )
  {
    if ( sl->related_pin[i][j] != NULL )
    {
      related_index = gnc_FindCellPort(sl->nc, sl->cell_ref, 
            sl->related_pin[i][j]);
      if ( related_index >= 0 )
      {
        pin_index = sl->gnc_port_index[i];
        
        /* related and pin index are swapped!!! */
        /* this is, beacuse a synopsys library provires several */
        /* timing groups for each OUTPUT pin, gnc does the inverted way */
        poti = gnc_GetCellPortGPOTI(sl->nc, sl->cell_ref, 
          related_index, pin_index, 1);
         
        if ( poti != NULL )
        {
          poti->rise_block_delay    = sl->intrinsic_rise[i][j];
          poti->rise_fanout_delay   = sl->rise_resistance[i][j];
          poti->fall_block_delay    = sl->intrinsic_fall[i][j];
          poti->fall_fanout_delay   = sl->fall_resistance[i][j];
          
          if ( sylib_g2dv(sl, &(poti->rise_cell), 
                    sl->cell_rise_template[i][j], 
                    sl->cell_rise_values[i][j]) == 0 )
            return 0;

          if ( sylib_g2dv(sl, &(poti->fall_cell), 
                    sl->cell_fall_template[i][j], 
                    sl->cell_fall_values[i][j]) == 0 )
            return 0;

          if ( sylib_g1dv(sl, &(poti->rise_propagation), 
                    sl->rise_propagation_template[i][j], 
                    sl->rise_propagation_values[i][j]) == 0 )
            return gnc_Error(sl->nc, "Synopsys import: rise_propagation failed"), 0;
            
          if ( sylib_g1dv(sl, &(poti->fall_propagation), 
                    sl->fall_propagation_template[i][j], 
                    sl->fall_propagation_values[i][j]) == 0 )
            return gnc_Error(sl->nc, "Synopsys import: fall_propagation failed"), 0;

          if ( sylib_g1dv(sl, &(poti->rise_transition), 
                    sl->rise_transition_template[i][j], 
                    sl->rise_transition_values[i][j]) == 0 )
            return gnc_Error(sl->nc, "Synopsys import: rise_transition failed"), 0;
            
          if ( sylib_g1dv(sl, &(poti->fall_transition), 
                    sl->fall_transition_template[i][j], 
                    sl->fall_transition_values[i][j]) == 0 )
            return gnc_Error(sl->nc, "Synopsys import: fall_transition failed"), 0;
        }
      }
    }
  }
  return 1;
}
  

int sylib_AddPortByName(sylib_type sl, char *name, int gnet_fn, int is_inv)
{
  int sylib_pin_index;
  
  sylib_pin_index = synlib_FindPin(sl, name);
  if ( sylib_pin_index < 0 )
    return 0;

  return sylib_AddPortByIndex(sl, sylib_pin_index, gnet_fn, is_inv);
}

int sylib_AddPortByExpr(sylib_type sl, int gnet_fn, char *expr)
{
  char *name;
  
  if ( expr == NULL )
    return 1;
  
  if ( sc_be_GenerateTree(sl->be, expr) == 0 )
    return 0;
  if ( sl->be->ben->cnt != 0 )
    return 0;
  name = sc_be_identifier(sl->be->ben->id);
  
  return sylib_AddPortByName(sl, name, gnet_fn, sl->be->ben->is_invert);
}


int sylib_AddPortByFunction(sylib_type sl, int gnet_fn, int is_inv, char *fn)
{
  int sylib_pin_index;
  
  if ( fn == NULL )
    return 1;
  
  sylib_pin_index = synlib_FindPinByFn(sl, fn);
  if ( sylib_pin_index < 0 )
    return 1;

  return sylib_AddPortByIndex(sl, sylib_pin_index, gnet_fn, is_inv);
}


int synlib_generate_ff(sylib_type sl)
{
  int is_ok = 1;

  if ( sl->ff_clocked_on_also_expr != NULL )
    return 1; /* not supported */

  is_ok &= sylib_AddPortByExpr(sl, GPORT_FN_D, sl->ff_next_state_expr);
  is_ok &= sylib_AddPortByExpr(sl, GPORT_FN_CLK, sl->ff_clocked_on_expr);
  is_ok &= sylib_AddPortByExpr(sl, GPORT_FN_CLR, sl->ff_clear_expr);
  is_ok &= sylib_AddPortByExpr(sl, GPORT_FN_SET, sl->ff_preset_expr);

  is_ok &= sylib_AddPortByFunction(sl, GPORT_FN_Q, 0, sl->ff_out_name);
  is_ok &= sylib_AddPortByFunction(sl, GPORT_FN_Q, 1, sl->ff_inv_out_name);

  return is_ok;
}

int synlib_bc_eval(void *data, int ref)
{
  sylib_type sl = (sylib_type)data;
  return dcGetIn(&(sl->c), ref)-1;
}

/* assumes, that the ports are already ok */
int synlib_generate_clist(sylib_type sl, char *expr)
{
  int i;
  int input_port_cnt;

  if ( sc_be_GenerateTree(sl->be, expr) == 0 )
  {
    gnc_Error(sl->nc, "synlib_generate_clist: Parse of '%s' failed.", expr);
    return 0;
  }

  sc_strl_Clear(sl->be->strl);
  
  input_port_cnt = 0;
  i = -1;
  while( gnc_LoopCellPort(sl->nc, sl->cell_ref, &i) )
  {
    if ( gnc_GetCellPortType(sl->nc, sl->cell_ref, i) == GPORT_TYPE_IN )
    {
      if ( sc_strl_AddUnique(sl->be->strl, gnc_GetCellPortName(sl->nc, sl->cell_ref, i)) == 0 )
        return 0;
      input_port_cnt++;
    }
  }

  if ( _sc_bc_RegisterIdentifier(sl->be, sl->be->ben) == 0 )
    return 0;

  if ( input_port_cnt != sc_strl_GetCnt(sl->be->strl) )
  {
    /* illegal matching between function and pins */
    return 0;
  }
  
  if ( gnc_GetCellPinfo(sl->nc, sl->cell_ref)->in_cnt != input_port_cnt )
  {
    /* illegal matching */
    gnc_Error(sl->nc, "synlib_generate_clist: Illegal input matching.");
    return 0;
  }

  if ( gnc_GetCellPinfo(sl->nc, sl->cell_ref)->out_cnt != 1 )
  {
    /* illegal matching */
    gnc_Error(sl->nc, "synlib_generate_clist: Illegal input matching.");
    return 0;
  }

  pinfoSetInCnt(&(sl->pi),input_port_cnt);
  pinfoSetOutCnt(&(sl->pi),1);
  
  sc_bc_SetEvalFn(sl->be, synlib_bc_eval, sl);

  if ( dcInit(&(sl->pi), &(sl->c)) == 0 )
    return 0;

  dcInSetAll(&(sl->pi), &(sl->c), CUBE_IN_MASK_ZERO);
  dcOutSetAll(&(sl->pi), &(sl->c), 0);
  dcSetOut(&(sl->c), 0, 1);

  if ( input_port_cnt > 0 )
  {
    do
    {
      if ( sc_bc_Eval(sl->be) != 0 )
      {
        if ( gnc_AddCellCube(sl->nc, sl->cell_ref, &(sl->c)) == 0 )
          return dcDestroy(&(sl->c)), 0;
      }
    } while ( dcInc( &(sl->pi), &(sl->c)) != 0 );
  }
  return dcDestroy(&(sl->c)), 1;
}

int synlib_generate_gate(sylib_type sl)
{
  int i;
  int fn_index = -1;
  int output_index = -1;


  /* create the ports */

  for( i = 0; i < sl->pin_cnt; i++ )
  {
    if ( sylib_AddPortByIndex(sl, i, GPORT_FN_LOGIC, 0) == 0 )
      return 0;
  }


  /* add timing information */
  
  for( i = 0; i < sl->pin_cnt; i++ )
  {
    if ( sylib_AddTimingByIndex(sl, i) == 0 )
      return 0;
  }
  
  
  /* check functions and outputs */
    
  for( i = 0; i < sl->pin_cnt; i++ )
    if ( sl->function[i] != NULL )
    {
      if ( fn_index < 0 )
        fn_index = i;
      else
        return 0;     /* only one function per gate */
    }
  
  for( i = 0; i < sl->pin_cnt; i++ )
    if ( sc_strcmp(sl->direction[i], "output") == 0 )
    {
      if ( output_index < 0 )
        output_index = i;
      else
        return 0;     /* only one output signal per gate */
    }

  if ( fn_index != output_index )
    return 0;   /* something is wrong */  

  if ( fn_index >= 0 )
  {
    if ( gnc_InitDCUBE(sl->nc, sl->cell_ref) == 0 )
      return 0;

    if ( synlib_generate_clist(sl, sl->function[fn_index]) == 0 )
      return 0;
  }

  return 1;
}


int synlib_do_msg_grp_end(sc_phr_type phr, void *data)
{
  sc_phr_grp_struct *grp = (sc_phr_grp_struct *)data;
  sylib_type sl = sc_phr_GetLocalData(phr);
  
  if ( sc_strcmp(grp->name, "pin") == 0 )
  {
    assert(sl->pin_cnt < SYLIB_PIN_MAX);
    sl->pin_cnt++;
  }
  else if ( sc_strcmp(grp->name, "lu_table_template") == 0 )
  { 
    /*
    printf("Name: %s\n", ((ltt_type)b_set_Get(sl->ltt_set, sl->ltt_curr))->name );
    printf("Var1: %s\n", ((ltt_type)b_set_Get(sl->ltt_set, sl->ltt_curr))->var1 );
    printf("Idx1: %s\n", ((ltt_type)b_set_Get(sl->ltt_set, sl->ltt_curr))->idx1 );
    if ( ((ltt_type)b_set_Get(sl->ltt_set, sl->ltt_curr))->var2 != NULL )
      printf("Var2: %s\n", ((ltt_type)b_set_Get(sl->ltt_set, sl->ltt_curr))->var2 );
    if ( ((ltt_type)b_set_Get(sl->ltt_set, sl->ltt_curr))->idx2 != NULL )
      printf("Idx2: %s\n", ((ltt_type)b_set_Get(sl->ltt_set, sl->ltt_curr))->idx2 );
    */
  }
  else if ( sc_strcmp(grp->name, "timing") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "pin") == 0 )
      {
        assert(sl->timing_cnt < SYLIB_PIN_MAX);
        sl->timing_cnt++;
      }
    }
  }
  else if ( sc_strcmp(grp->name, "cell") == 0 )
  {
    if ( sl->ignore_cell == 0 )
    {
      if ( sl->ff_out_name != NULL )
      {
        if ( synlib_generate_ff(sl) == 0 )
        {
          if ( sl->cell_ref >= 0 )
            gnc_DelCell(sl->nc, sl->cell_ref);
          gnc_Log(sl->nc, 2, "Synopsys import: FF '%s' failed.", sl->cell_name);
          return 1;
        }
        else
        {
          gnc_Log(sl->nc, 0, "Synopsys import: FF '%s' import done (%d ports).", 
            sl->cell_name, gnc_GetCellPortCnt(sl->nc, sl->cell_ref));
        }
      }
      else 
      {
        if ( synlib_generate_gate(sl) == 0 )
        {
          if ( sl->cell_ref >= 0 )
            gnc_DelCell(sl->nc, sl->cell_ref);
          gnc_Log(sl->nc, 2, "Synopsys import: Gate '%s' failed.", sl->cell_name);
          return 1;
        }
        else
        {
          gnc_Log(sl->nc, 0, "Synopsys import: Gate '%s' import done.", sl->cell_name);
        }
      }
      if ( sl->cell_ref >= 0 )
        gnc_SetCellArea(sl->nc, sl->cell_ref, sl->cell_area);
    }
    else
    {
      gnc_Log(sl->nc, 0, "Synopsys import: Cell '%s' ignored.", 
        sl->cell_name);
    }
  }
  return 1;
}

int synlib_do_msg_attrib(sc_phr_type phr, void *data)
{
  int result = 1;
  sc_phr_grp_struct *grp = sc_phr_GetCurrGrpStruct(phr);
  sylib_type sl = (sylib_type)sc_phr_GetLocalData(phr);

  /* pin attributes */
  
  if ( sc_strcmp(grp->name, "pin") == 0 )
  {
    if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_STR )
    {
      char *arg = sc_phr_GetArg(phr, 0)->u.s;
      result = 1;
      if ( sc_strcmp((char *)data, "direction") == 0 )
        result = sylib_SetDirection(sl, sl->pin_cnt, arg);
      else if ( sc_strcmp((char *)data, "function") == 0 )
        result = sylib_SetFunction(sl, sl->pin_cnt, arg);
      else if ( sc_strcmp((char *)data, "three_state") == 0 )
        sl->ignore_cell = 1;
      if ( result == 0 ) 
        return 0;
    }
    if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_DOUBLE )
    {
      double arg = sc_phr_GetArg(phr, 0)->u.d;
      if ( sc_strcmp((char *)data, "capacitance") == 0 )
        sl->capacitance[sl->pin_cnt] = arg;
    }
  }

  /* timing attributes */
  
  if ( sc_strcmp(grp->name, "timing") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "pin") == 0 )
      {
        if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_STR )
        {
          char *arg = sc_phr_GetArg(phr, 0)->u.s;
          result = 1;
          if ( sc_strcmp((char *)data, "related_pin") == 0 )
            result = sylib_SetRelatedPin(sl, sl->pin_cnt, sl->timing_cnt, arg);
          if ( result == 0 ) 
            return 0;
        }
        if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_DOUBLE )
        {
          double arg = sc_phr_GetArg(phr, 0)->u.d;
          if ( sc_strcmp((char *)data, "intrinsic_rise") == 0 )
            sl->intrinsic_rise[sl->pin_cnt][sl->timing_cnt] = arg;
          else if ( sc_strcmp((char *)data, "intrinsic_fall") == 0 )
            sl->intrinsic_fall[sl->pin_cnt][sl->timing_cnt] = arg;
          else if ( sc_strcmp((char *)data, "rise_resistance") == 0 )
            sl->rise_resistance[sl->pin_cnt][sl->timing_cnt] = arg;
          else if ( sc_strcmp((char *)data, "fall_resistance") == 0 )
            sl->fall_resistance[sl->pin_cnt][sl->timing_cnt] = arg;
        }
      }
    }
  }
  
  if ( sc_strcmp(grp->name, "cell_fall") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_STR )
        {
          char *arg = sc_phr_GetArg(phr, 0)->u.s;
          if ( sc_strcmp((char *)data, "values") == 0 )
            if ( sylib_SetCellFallValues(sl, sl->pin_cnt, sl->timing_cnt, arg) == 0 )
              return 0;
        }
      }
    }
  }

  if ( sc_strcmp(grp->name, "cell_rise") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_STR )
        {
          char *arg = sc_phr_GetArg(phr, 0)->u.s;
          if ( sc_strcmp((char *)data, "values") == 0 )
            if ( sylib_SetCellRiseValues(sl, sl->pin_cnt, sl->timing_cnt, arg) == 0 )
              return 0;
        }
      }
    }
  }
  
  if ( sc_strcmp(grp->name, "fall_transition") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_STR )
        {
          char *arg = sc_phr_GetArg(phr, 0)->u.s;
          if ( sc_strcmp((char *)data, "values") == 0 )
            if ( sylib_SetFallTransitionValues(sl, sl->pin_cnt, sl->timing_cnt, arg) == 0 )
              return 0;
        }
      }
    }
  }

  if ( sc_strcmp(grp->name, "rise_transition") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_STR )
        {
          char *arg = sc_phr_GetArg(phr, 0)->u.s;
          if ( sc_strcmp((char *)data, "values") == 0 )
            if ( sylib_SetRiseTransitionValues(sl, sl->pin_cnt, sl->timing_cnt, arg) == 0 )
              return 0;
        }
      }
    }
  }

  if ( sc_strcmp(grp->name, "fall_propagation") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_STR )
        {
          char *arg = sc_phr_GetArg(phr, 0)->u.s;
          if ( sc_strcmp((char *)data, "values") == 0 )
            if ( sylib_SetFallPropagationValues(sl, sl->pin_cnt, sl->timing_cnt, arg) == 0 )
              return 0;
        }
      }
    }
  }

  if ( sc_strcmp(grp->name, "rise_propagation") == 0 )
  {
    if ( sc_phr_GetGrpDepth(phr) > 1 )
    {
      if ( sc_strcmp(sc_phr_GetLastGrpStruct(phr)->name, "timing") == 0 )
      {
        if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_STR )
        {
          char *arg = sc_phr_GetArg(phr, 0)->u.s;
          if ( sc_strcmp((char *)data, "values") == 0 )
            if ( sylib_SetRisePropagationValues(sl, sl->pin_cnt, sl->timing_cnt, arg) == 0 )
              return 0;
        }
      }
    }
  }

  /* ff attributes */
  
  if ( sc_strcmp(grp->name, "ff") == 0 )
  {
    if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_STR )
    {
      char *arg = sc_phr_GetArg(phr, 0)->u.s;
      result = 1;
      if ( sc_strcmp((char *)data, "next_state") == 0 )
        result = sylib_SetFFNextStateExpr(sl, arg);
      else if ( sc_strcmp((char *)data, "clocked_on") == 0 )
        result = sylib_SetFFClockedOnExpr(sl, arg);
      else if ( sc_strcmp((char *)data, "clocked_on_also") == 0 )
        result = sylib_SetFFClockedOnAlsoExpr(sl, arg);
      else if ( sc_strcmp((char *)data, "clear") == 0 )
        result = sylib_SetFFClearExpr(sl, arg);
      else if ( sc_strcmp((char *)data, "preset") == 0 )
        result = sylib_SetFFPresetExpr(sl, arg);
      if ( result == 0 ) 
        return 0;
    }
  }

  /* cell attributes */

  if ( sc_strcmp(grp->name, "cell") == 0 )
  {
    double arg = 0.0;
    if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_LONG )
    {
      arg = (double)sc_phr_GetArg(phr, 0)->u.l;
    }
    if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_DOUBLE )
    {
      arg = sc_phr_GetArg(phr, 0)->u.d;
    }
    if ( sc_strcmp((char *)data, "area") == 0 )
      sl->cell_area = arg;
  }
  
  /* lu_table_template attributes */

  if ( sc_strcmp(grp->name, "lu_table_template") == 0 )
  {
    if ( sc_phr_GetArgCnt(phr) == 1 && sc_phr_GetArg(phr, 0)->typ == SC_PHR_ARG_STR )
    {
      char *arg = sc_phr_GetArg(phr, 0)->u.s;
      result = 1;
      if ( sc_strcmp((char *)data, "variable_1") == 0 )
        result = sylib_SetLTTVar1(sl, arg);
      else if ( sc_strcmp((char *)data, "variable_2") == 0 )
        result = sylib_SetLTTVar2(sl, arg);
      else if ( sc_strcmp((char *)data, "index_1") == 0 )
        result = sylib_SetLTTIdx1(sl, arg);
      else if ( sc_strcmp((char *)data, "index_2") == 0 )
        result = sylib_SetLTTIdx2(sl, arg);
      if ( result == 0 ) 
        return 0;
    }
    
  }
  
  return 1;
}

int synlib_gnc_aux(sc_phr_type phr, int msg, void *data)
{
  int result = 1;
  switch(msg)
  {
    case SC_MSG_AUX_START:
      result = synlib_do_msg_aux_start(phr, data);
      break;
    case SC_MSG_AUX_END:
      result = synlib_do_msg_aux_end(phr, data);
      break;
    case SC_MSG_GRP_START:
      result = synlib_do_msg_grp_start(phr, data);
      break;
    case SC_MSG_GRP_END:
      result = synlib_do_msg_grp_end(phr, data);
      break;
    case SC_MSG_ATTRIB:
      result = synlib_do_msg_attrib(phr, data);
      break;
  }
  return result;
}


char *gnc_ReadSynopsysLibrary(gnc nc, const char *name, const char *user_library_name)
{
  static char lib[512];
  char *libp = NULL;
  sc_phr_type phr;
  sylib_type sl;
  sl = sylib_Open(nc, user_library_name);
  if ( sl != NULL )
  {
    phr = sc_phr_Open(name);
    if ( phr != NULL )
    {
      sc_phr_SetLocalData(phr, (void *)sl);
      sc_phr_SetAux(phr, synlib_gnc_aux, (void *)sl);
      if ( sc_phr_Do(phr) != 0 )
      {
        if ( sl->user_library_name != NULL )
        {
          strncpy(lib, sl->user_library_name, 512);
          lib[512-1] = '\0';
          libp = lib;
        }
        else
        {
          strncpy(lib, sl->library_name, 512);
          lib[512-1] = '\0';
          libp = lib;
        }
      }
      else
      {
        gnc_Error(nc, "Synopsys import: Parser failed.");
      }
      sc_phr_Close(phr);
    }
    else
    {
      gnc_Error(nc, "Synopsys import: Out of memory (sc_phr_Open).");
    }
    sylib_Close(sl);
  }
  else
  {
    gnc_Error(nc, "Synopsys import: Out of memory (sylib_Open).");
  }
  return libp;
}

/*--------------------------------------------------------------------------------------------*/

static int is_valid_synlib;


static int valid_synlib_aux(sc_phr_type phr, int msg, void *data)
{
  int result = 1;
  static int state = 0;
  sc_phr_grp_struct *grp;
  
  switch(msg)
  {
    case SC_MSG_AUX_START:
      state = 0;
      is_valid_synlib = 0;
      break;
    case SC_MSG_AUX_END:
      break;
    case SC_MSG_GRP_START:
      grp = (sc_phr_grp_struct *)data;
      if ( state == 0 )
      {
        if ( sc_strcmp(grp->name, "library") == 0 )
          state++;
      }
      else if ( state == 1 )
      {
        if ( sc_strcmp(grp->name, "cell") == 0 )
          state++;
      }
      else if ( state == 2 )
      {
        if ( sc_strcmp(grp->name, "pin") == 0 )
        {
          is_valid_synlib = 1;
          return 0;
        }
      }
      break;
    case SC_MSG_GRP_END:
      break;
    case SC_MSG_ATTRIB:
      break;
  }
  return result;
}

int IsValidSynopsysLibraryFile(const char *name, int is_msg)
{
  sc_phr_type phr;
  is_valid_synlib = 0;
  phr = sc_phr_Open(name);
  if ( phr != NULL )
  {
    phr->is_error_msg = is_msg;
    sc_phr_SetAux(phr, valid_synlib_aux, NULL);
    sc_phr_Do(phr);
  }
  sc_phr_Close(phr);
  return is_valid_synlib;
}

