/*

  dcubevhd.c

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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "dcube.h"
#include "mwc.h"

#define DCL_VHDL_TB_USE_VECTOR

/*-- dclWriteVHDLFP ---------------------------------------------------------*/

static int dclWriteVHDLInputPortFP(pinfo *pi, FILE *fp, int in)
{
  fprintf(fp, "i%d", in);
  return 1;
}

static int dclWriteVHDLPosNegInputPortFP(pinfo *pi, FILE *fp, int in, int neg)
{
  if ( neg != 0 )
    fprintf(fp, "(NOT ");
  dclWriteVHDLInputPortFP(pi, fp, in);
  if ( neg != 0 )
    fprintf(fp, ")");
  return 1;
}

static int dclWriteVHDLOutputPortFP(pinfo *pi, FILE *fp, int out)
{
  fprintf(fp, "o%d", out);
  return 1;
}

static int dclWriteVHDLNewLine(pinfo *pi, FILE *fp, int indent)
{
  fprintf(fp, "\n");
  while( indent-- > 0 )
    fprintf(fp, "  ");
  return 1;
}

static int dclWriteVHDLCubeFP(pinfo *pi, FILE *fp, dcube *c)
{
  int i, cnt = pi->in_cnt;
  int s;
  int is_first = 1;
  fprintf(fp, "(");
  for( i = 0; i < cnt; i++ )
  {
    s = dcGetIn(c, i);
    if ( s == 1 || s == 2 )
    {
      if ( is_first == 0 )
        fprintf(fp, " AND ");
      is_first = 0;
      if ( dclWriteVHDLPosNegInputPortFP(pi, fp, i, s==1?1:0) == 0 )
        return 0;
    }
  }
  fprintf(fp, ")");
  return 1;
}

static int dclWriteVHDLPortFP(pinfo *pi, FILE *fp)
{
  int i;
  int is_first;
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "PORT(");
  is_first = 1;
  for( i = 0; i < pi->in_cnt; i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    dclWriteVHDLInputPortFP(pi, fp, i);
  }
  fprintf(fp, " : IN STD_LOGIC;");
  dclWriteVHDLNewLine(pi, fp, 2);
  is_first = 1;
  for( i = 0; i < pi->out_cnt; i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    dclWriteVHDLOutputPortFP(pi, fp, i);
  }
  fprintf(fp, " : OUT STD_LOGIC);");
  return 1;
}

int dclWriteVHDLEntityFP(pinfo *pi, const char *entity, FILE *fp)
{

  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "LIBRARY ieee;");
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "USE ieee.std_logic_1164.all;");
  dclWriteVHDLNewLine(pi, fp, 0);

  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "ENTITY %s IS", entity);
  if ( dclWriteVHDLPortFP(pi, fp) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "END ENTITY;");
  dclWriteVHDLNewLine(pi, fp, 0);
  return 1;
}

int dclWriteVHDLArchitectureFP(pinfo *pi, dclist cl, const char *entity, FILE *fp)
{ 
  int o_idx, o_cnt = pi->out_cnt;
  int c_idx, c_cnt = dclCnt(cl);
  int is_first;
  
  
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "ARCHITECTURE logic OF %s IS", entity);
  
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "BEGIN");
  
  for( o_idx = 0; o_idx < o_cnt; o_idx++ )
  {
    dclWriteVHDLNewLine(pi, fp, 1);
    dclWriteVHDLOutputPortFP(pi, fp, o_idx);
    fprintf(fp, " <= ");
    dclWriteVHDLNewLine(pi, fp, 2);
    is_first = 1;
    for( c_idx = 0; c_idx < c_cnt; c_idx++ )
    {
      if ( dcGetOut(dclGet(cl, c_idx), o_idx) != 0 )
      {
        if ( is_first == 0 )
        {
          fprintf(fp, " OR ");  
          dclWriteVHDLNewLine(pi, fp, 2);
        }
        is_first = 0;
        dclWriteVHDLCubeFP(pi, fp, dclGet(cl, c_idx));
      }
    }
    fprintf(fp, ";");
  }

  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "END ARCHITECTURE;");
  dclWriteVHDLNewLine(pi, fp, 0);

  return 1;
}

/*-- dclWriteVHDLTBFP -------------------------------------------------------*/

static int dclWriteVHDLTBAssignmentFP(pinfo *pi, FILE *fp, dcube *c, int wait_ns)
{
  int i;
  
  dclWriteVHDLNewLine(pi, fp, 2);
#ifdef DCL_VHDL_TB_USE_VECTOR
  fprintf(fp, "x <= \"");
#else
  fprintf(fp, "(");
  for( i = 0; i < pi->in_cnt; i++ )
  {
    if ( i != 0 )
      fprintf(fp, ",");
    if ( dclWriteVHDLInputPortFP(pi, fp, i) == 0 )
      return 0;
  }
  fprintf(fp, ") <= STD_LOGIC_VECTOR'(\"");
#endif
  for( i = 0; i < pi->in_cnt; i++ )
  {
    if ( dcGetIn(c, i) == 1 )
      fprintf(fp, "0");
    else
      fprintf(fp, "1");
  }
#ifdef DCL_VHDL_TB_USE_VECTOR
  fprintf(fp, "\";");
#else
  fprintf(fp, "\");");
#endif

  dclWriteVHDLNewLine(pi, fp, 2);
  fprintf(fp, "WAIT FOR %d ns;", wait_ns);
  return 1;
}

static int dclWriteVHDLTBAssertFP(pinfo *pi, FILE *fp, dclist cl, dcube *c)
{
  int i, o;
  
  dcOutSetAll(pi, c, 0);

  dclWriteVHDLNewLine(pi, fp, 2);
#ifdef DCL_VHDL_TB_USE_VECTOR
  fprintf(fp, "ASSERT (y = \"");
#else
  fprintf(fp, "ASSERT ((");

  for( o = 0; o < pi->out_cnt; o++ )
  {
    if ( o != 0 )
      fprintf(fp, ",");
    if ( dclWriteVHDLOutputPortFP(pi, fp, o) == 0 )
      return 0;
  }
    
  fprintf(fp, ") = STD_LOGIC_VECTOR'(\"");
#endif
  
  for( o = 0; o < pi->out_cnt; o++ )
  {
    dcSetOut(c, o, 1);
    if ( dclIsSingleSubSet(pi, cl, c) != 0 )
      fprintf(fp, "1");
    else
      fprintf(fp, "0");
    dcSetOut(c, o, 0);
  }

#ifdef DCL_VHDL_TB_USE_VECTOR
  fprintf(fp, "\") REPORT \"input vector ");
#else
  fprintf(fp, "\")) REPORT \"input vector ");
#endif

  for( i = 0; i < pi->in_cnt; i++ )
  {
    if ( dcGetIn(c, i) == 1 )
      fprintf(fp, "0");
    else
      fprintf(fp, "1");
  }
  
  fprintf(fp, " failed\" SEVERITY ERROR;");
  
  return 1;
}


static int dclWriteVHDLTBProcessFP(pinfo *pi, FILE *fp, dclist cl, int wait_ns)
{
  int i, j, j_max = 1<<(pi->in_cnt-12);
  dcube c;
  
  if ( dcInit(pi, &c) == 0 )
    return 0;

  dcInSetAll(pi, &c, CUBE_IN_MASK_ZERO);

  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "PROCESS");
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "BEGIN");

  i = 0;
  j = 0;
  do
  {
    if ( ((i & ((1<<12)-1))) == 0 )
    {
      fprintf(stderr, "%3d%%\r", (j*100)/j_max);
      i = 0;
      j++;
    }
    if ( dclWriteVHDLTBAssignmentFP(pi, fp, &c, wait_ns) == 0 )
      return dcDestroy(&c), 0;
    if ( dclWriteVHDLTBAssertFP(pi, fp, cl, &c) == 0 )
      return dcDestroy(&c), 0;
    i++;
  } while ( dcInc(pi, &c) != 0 );
  fprintf(stderr, "100%% done\n");
  dclWriteVHDLNewLine(pi, fp, 2);
  fprintf(fp, "WAIT;");
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "END PROCESS;");

  return dcDestroy(&c), 1;
}

/* ununsed, why?
static int dclWriteVHDLTBSignalListFP(pinfo *pi, FILE *fp)
{
  int i;
  int is_first;

  is_first = 1;
  for( i = 0; i < pi->in_cnt; i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    dclWriteVHDLInputPortFP(pi, fp, i);
  }
  is_first = 0;
  for( i = 0; i < pi->out_cnt; i++ )
  {
    if ( is_first == 0 )
      fprintf(fp, ", ");
    is_first = 0;
    dclWriteVHDLOutputPortFP(pi, fp, i);
  }
  return 1;
}
*/

static int dclWriteVHDLTBEntityFP(pinfo *pi, FILE *fp, const char *entity)
{
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "ENTITY tb_%s IS", entity);
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "END tb_%s;", entity);
  return 1;
}

static int dclWriteVHDLTBArchitectureFP(pinfo *pi, FILE *fp, const char *entity, dclist cl, int wait_ns)
{
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "ARCHITECTURE test OF tb_%s IS", entity);
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "COMPONENT %s", entity);
  if ( dclWriteVHDLPortFP(pi, fp) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 1);
  /* fprintf(fp, "END COMPONENT %s;", entity); */
  fprintf(fp, "END COMPONENT;");
#ifdef DCL_VHDL_TB_USE_VECTOR  
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "SIGNAL x : STD_LOGIC_VECTOR(0 to %d);", pi->in_cnt-1);
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "SIGNAL y : STD_LOGIC_VECTOR(0 to %d);", pi->out_cnt-1);
#else
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "SIGNAL ");
  if ( dclWriteVHDLTBSignalListFP(pi, fp) == 0 )
    return 0;
  fprintf(fp, " : STD_LOGIC;");
#endif
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "BEGIN");
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "DUT: %s PORT MAP(", entity);
#ifdef DCL_VHDL_TB_USE_VECTOR
  {
    int i;
    for( i = 0; i < pi->in_cnt; i++ )
    {
      if ( i != 0 )
        fprintf(fp, ", ");
      fprintf(fp, "x(%d)", i);
    }
    for( i = 0; i < pi->out_cnt; i++ )
    {
      fprintf(fp, ", ");
      fprintf(fp, "y(%d)", i);
    }
  }
#else
  if ( dclWriteVHDLTBSignalListFP(pi, fp) == 0 )
    return 0;
#endif
  fprintf(fp, ");");
  dclWriteVHDLNewLine(pi, fp, 0);
  if ( dclWriteVHDLTBProcessFP(pi, fp, cl, wait_ns) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "END test;");
  return 1;
}

static int dclWriteVHDLTBConfigurationFP(pinfo *pi, FILE *fp, const char *entity)
{
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "CONFIGURATION cfg_tb_%s OF tb_%s IS", entity, entity);
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "FOR test");
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "END FOR;");
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "END cfg_tb_%s;", entity);
  return 1;
}

int dclWriteVHDLVecTBFP(pinfo *pi, dclist cl, const char *entity, int wait_ns, FILE *fp)
{
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "LIBRARY ieee;");
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "USE ieee.std_logic_1164.all;");
  dclWriteVHDLNewLine(pi, fp, 0);

  if ( dclWriteVHDLTBEntityFP(pi, fp, entity) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 0);
  if ( dclWriteVHDLTBArchitectureFP(pi, fp, entity, cl, wait_ns) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 0);
  if ( dclWriteVHDLTBConfigurationFP(pi, fp, entity) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 0);
  return 1;
}

static int dclWriteVHDLSOPTBArchitectureFP(pinfo *pi, FILE *fp, const char *entity, dclist cl, int wait_ns)
{
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "ARCHITECTURE test OF tb_%s IS", entity);
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "COMPONENT %s", entity);
  if ( dclWriteVHDLPortFP(pi, fp) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "END COMPONENT;");

  fprintf(fp, "COMPONENT %s", "reference_design");
  if ( dclWriteVHDLPortFP(pi, fp) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "END COMPONENT;");
  
  
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "SIGNAL x : STD_LOGIC_VECTOR(0 to %d);", pi->in_cnt-1);
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "SIGNAL y_dut : STD_LOGIC_VECTOR(0 to %d);", pi->out_cnt-1);
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "SIGNAL y_ref : STD_LOGIC_VECTOR(0 to %d);", pi->out_cnt-1);

  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "BEGIN");

  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "DUT: %s PORT MAP(", entity);
  {
    int i;
    for( i = 0; i < pi->in_cnt; i++ )
    {
      if ( i != 0 )
        fprintf(fp, ", ");
      fprintf(fp, "x(%d)", i);
    }
    for( i = 0; i < pi->out_cnt; i++ )
    {
      fprintf(fp, ", ");
      fprintf(fp, "y_dut(%d)", i);
    }
  }
  fprintf(fp, ");");
  
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "REF: %s PORT MAP(", "reference_design");
  {
    int i;
    for( i = 0; i < pi->in_cnt; i++ )
    {
      if ( i != 0 )
        fprintf(fp, ", ");
      fprintf(fp, "x(%d)", i);
    }
    for( i = 0; i < pi->out_cnt; i++ )
    {
      fprintf(fp, ", ");
      fprintf(fp, "y_ref(%d)", i);
    }
  }
  fprintf(fp, ");");
  
  dclWriteVHDLNewLine(pi, fp, 0);
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "PROCESS");
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "BEGIN");
  /*
  dclWriteVHDLNewLine(pi, fp, 2);
  fprintf(fp, "x <= (others => '0')");
  */
  dclWriteVHDLNewLine(pi, fp, 2);
  fprintf(fp, "FOR i IN 0 TO %d LOOP", (1<<pi->in_cnt)-1);
  dclWriteVHDLNewLine(pi, fp, 3);
  fprintf(fp, "x <= STD_LOGIC_VECTOR(TO_UNSIGNED(i, %d));", pi->in_cnt);
  dclWriteVHDLNewLine(pi, fp, 3);
  fprintf(fp, "WAIT FOR %d ns;", wait_ns);
  dclWriteVHDLNewLine(pi, fp, 3);
  fprintf(fp, "ASSERT (y_dut = y_ref) REPORT \"design mismatch\" SEVERITY ERROR;");
  dclWriteVHDLNewLine(pi, fp, 2);
  fprintf(fp, "END LOOP;");
  dclWriteVHDLNewLine(pi, fp, 2);
  fprintf(fp, "WAIT;");
  dclWriteVHDLNewLine(pi, fp, 1);
  fprintf(fp, "END PROCESS;");
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "END test;");
  return 1;
}

int dclWriteVHDLSOPTBFP(pinfo *pi, dclist cl, const char *entity, int wait_ns, FILE *fp)
{
  dclWriteVHDLEntityFP(pi, "reference_design", fp);
  dclWriteVHDLArchitectureFP(pi, cl, "reference_design", fp);
  
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "LIBRARY ieee;");
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "USE ieee.std_logic_1164.all;");
  dclWriteVHDLNewLine(pi, fp, 0);
  fprintf(fp, "USE ieee.numeric_std.all;");
  dclWriteVHDLNewLine(pi, fp, 0);

  if ( dclWriteVHDLTBEntityFP(pi, fp, entity) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 0);
  if ( dclWriteVHDLSOPTBArchitectureFP(pi, fp, entity, cl, wait_ns) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 0);
  if ( dclWriteVHDLTBConfigurationFP(pi, fp, entity) == 0 )
    return 0;
  dclWriteVHDLNewLine(pi, fp, 0);
  return 1;  
}

/*-- dclWriteVHDL -----------------------------------------------------------*/

int dclWriteVHDL(pinfo *pi, dclist cl, const char *entity, int wait_ns, char *name)
{
  FILE *fp;
  fp = fopen(name, "w");
  if ( fp == NULL )
    return 0;
  dclWriteVHDLEntityFP(pi, entity, fp);
  dclWriteVHDLArchitectureFP(pi, cl, entity, fp);
  if ( wait_ns >= 0 )
    dclWriteVHDLVecTBFP(pi, cl, entity, wait_ns, fp);
  fclose(fp);
  return 1;
}


