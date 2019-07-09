/*

  syrecu.c

  synthesis of recu structure.

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

  int gnc_SynthGate(gnc nc, 
    int cell_ref,             specifies the target netlist
    int gate,                 GNC_GATE_AND or GNC_GATE_OR
    b_il_type in_net_refs,    input lines to the gate
    int *p_out,               positive output
    int *n_out)               inverted output
 
*/

#include "recu.h"
#include "b_il.h"
#include "b_pl.h"
#include "gnet.h"
#include "neca.h"
#include <assert.h>
#include "mwc.h"

int neca_Optimize(neca_type neca, gnc nc, int cell_ref);

struct _syrecu_struct
{
  gnc nc;
  int cell_ref;
  recu rc;
  b_il_type net_refs;
  neca_type and_gates;
  neca_type or_gates;
  int is_generic;
};
typedef struct _syrecu_struct *syrecu_type;

syrecu_type syrecu_Open(gnc nc)
{
  syrecu_type sr;
  sr = (syrecu_type)malloc(sizeof(struct _syrecu_struct));
  if ( sr != NULL )
  {
    sr->nc = nc;
    sr->cell_ref = -1;
    sr->rc = NULL;
    sr->is_generic = 1;
    sr->and_gates = neca_Open();
    if ( sr->and_gates != NULL )
    {
      sr->or_gates = neca_Open();
      if ( sr->or_gates != NULL )
      {
        sr->net_refs = b_il_Open();
        if ( sr->net_refs != NULL )
        {
          return sr;
        }
        neca_Close(sr->or_gates);
      }
      neca_Close(sr->and_gates);
    }
    free(sr);
  }
  return NULL;
}

void syrecu_Close(syrecu_type sr)
{
  neca_Close(sr->or_gates);
  neca_Close(sr->and_gates);
  b_il_Close(sr->net_refs);
  free(sr);
}

/* input: a column of the recu matrix (well an index for the mapping table */
/* returns net_ref or -1 */
int syrecu_GetPortNet(syrecu_type sr, int col, int is_neg)
{
  /* assumes ports are created with gnc_UpdateCellPortsWithPinfo */
  int port_ref = col; 
  return gnc_GetDigitalCellNetByPort(sr->nc, sr->cell_ref, port_ref, is_neg, sr->is_generic);
}

/*
  net's are added to 'net_refs'.
*/
int syrecu_SynthNetRectX(syrecu_type sr, b_il_type net_refs, rect *r)
{
  int i;
  int symbol;
  int net_ref;
  
  for( i = 0; i < sr->rc->pi->in_cnt; i++ )
  {
    symbol = rGet(r, 0, i);
    if ( symbol != 0 )
    {
      net_ref = syrecu_GetPortNet(sr, i, symbol==1?1:0);
      if ( net_ref < 0 )
        return 0;
      assert( gnc_FindCellNetJoinByDriver(sr->nc, sr->cell_ref, net_ref) >= 0 );
      if ( b_il_Add(net_refs, net_ref) < 0 )
      {
        gnc_Error(sr->nc, "syrecu_SynthNetRectX: Out of Memory (b_il_Add).");
        return 0;
      }
    }
  }
  return 1;
}

int syrecu_SynthAND(syrecu_type sr, b_il_type net_refs, int *p_out)
{
  if ( b_il_GetCnt(net_refs) > 1 )
  {
    *p_out = neca_Get(sr->and_gates, net_refs);
    if ( *p_out >= 0 )
    {
      return 1;
    }
  }
  
  if ( sr->is_generic == 0 )
  {
    if ( gnc_SynthGate(sr->nc, sr->cell_ref, WIGA_TYPE_AND, net_refs, p_out, NULL) == 0 )
      return 0;
  }
  else
  {
    *p_out = gnc_SynthGenericGate(sr->nc, sr->cell_ref, GCELL_ID_G_AND, net_refs, -1);
    if ( *p_out < 0 )
      return 0;
  }
    
  if ( b_il_GetCnt(net_refs) > 1 )
    if ( neca_Ins(sr->and_gates, net_refs, *p_out) == 0 )
      return 0;
  
  return 1;
}

int syrecu_SynthOR(syrecu_type sr, b_il_type net_refs, int *p_out)
{
  if ( b_il_GetCnt(net_refs) > 1 )
  {
    *p_out = neca_Get(sr->or_gates, net_refs);
    if ( *p_out >= 0 )
    {
      return 1;
    }
  }
  
  if ( sr->is_generic == 0 )
  {
    if ( gnc_SynthGate(sr->nc, sr->cell_ref, WIGA_TYPE_OR, net_refs, p_out, NULL) == 0 )
      return 0;
  }
  else
  {
    *p_out = gnc_SynthGenericGate(sr->nc, sr->cell_ref, GCELL_ID_G_OR, net_refs, -1);
    if ( *p_out < 0 )
      return 0;
  }

  if ( b_il_GetCnt(net_refs) > 1 )
    if ( neca_Ins(sr->or_gates, net_refs, *p_out) == 0 )
      return 0;
  return 1;
}


int syrecu_SynthOutputCoverTree(syrecu_type sr, srnode srn)
{
  b_il_type net_refs_and;
  b_il_type net_refs_or;
  int net_ref;
  rect *r;

  srnode child;
  
  if ( b_il_OpenMultiple(2, &net_refs_and, &net_refs_or) == 0 )
  {
    gnc_Error(sr->nc, "syrect_SynthRect: Out of Memory (b_il_OpenMiltiple).");
    return -1;
  }

  /* synthesis of a product of input values */

  r = &(srn->r);
  if ( syrecu_SynthNetRectX(sr, net_refs_and, r) == 0 )
    return b_il_CloseMultiple(2, net_refs_and, net_refs_or), -1;

  /* synthesis of a disjunction */
  
  child = srn->down;
  while( child != NULL )
  {
    net_ref = syrecu_SynthOutputCoverTree(sr, child);
    if ( net_ref < 0 )
      return b_il_CloseMultiple(2, net_refs_and, net_refs_or), -1;
    if ( b_il_Add(net_refs_or, net_ref) < 0 )
      return b_il_CloseMultiple(2, net_refs_and, net_refs_or), -1;
    
    child = child->next;
  }

  /* no product */

  if ( b_il_GetCnt(net_refs_and) == 0 )
  {
    if ( syrecu_SynthOR(sr, net_refs_or, &net_ref) == 0 )
      return b_il_CloseMultiple(2, net_refs_and, net_refs_or), -1;
    assert( gnc_FindCellNetJoinByDriver(sr->nc, sr->cell_ref, net_ref) >= 0 );
    return b_il_CloseMultiple(2, net_refs_and, net_refs_or), net_ref;
  }

  /* no disjunction */
  
  if ( b_il_GetCnt(net_refs_or) == 0 )
  {
    if ( syrecu_SynthAND(sr, net_refs_and, &net_ref) == 0 )
      return b_il_CloseMultiple(2, net_refs_and, net_refs_or), -1;
    assert( gnc_FindCellNetJoinByDriver(sr->nc, sr->cell_ref, net_ref) >= 0 );
    return b_il_CloseMultiple(2, net_refs_and, net_refs_or), net_ref;
  }

  /* we have both cases */

  if ( syrecu_SynthOR(sr, net_refs_or, &net_ref) == 0 )
    return b_il_CloseMultiple(2, net_refs_and, net_refs_or), -1;

  assert( gnc_FindCellNetJoinByDriver(sr->nc, sr->cell_ref, net_ref) >= 0 );

  if ( b_il_Add(net_refs_and, net_ref) < 0 )
    return b_il_CloseMultiple(2, net_refs_and, net_refs_or), -1;

  if ( syrecu_SynthAND(sr, net_refs_and, &net_ref) == 0 )
    return b_il_CloseMultiple(2, net_refs_and, net_refs_or), -1;

  assert( gnc_FindCellNetJoinByDriver(sr->nc, sr->cell_ref, net_ref) >= 0 );

  b_il_CloseMultiple(2, net_refs_and, net_refs_or);
  return net_ref;
}

int syrecu_SynthTree(syrecu_type sr, srnode srn)
{
  int i;
  srnode child;
  rect *r;
  b_il_type net_refs;
  int net_ref;

  net_refs = b_il_Open();
  if ( net_refs == NULL )
    return 0;

  child = srn->down;
  while(child != NULL)
  {
    child->ref = syrecu_SynthOutputCoverTree(sr, child);
    if ( child->ref < 0 )
    {
      b_il_Close(net_refs);
      return 0;
    }
    child = child->next;
  }


  for( i = 0; i < sr->rc->pi->out_cnt; i++ )
  {
    b_il_Clear(net_refs);
    child = srn->down;
    while(child != NULL)
    {
      r = &(child->r);
      if ( rGet(r, 0, i+sr->rc->pi->in_cnt) != 0 )
      { 
        if ( b_il_Add(net_refs, child->ref) < 0 )
        {
          gnc_Error(sr->nc, "syrecu_SynthTree: Out of Memory (b_il_Add).");
          b_il_Close(net_refs);
          return 0;
        }
      }
      child = child->next;
    }
    if ( b_il_GetCnt(net_refs) >= 1 )
    {
      if ( b_il_GetCnt(net_refs) > 1 )
      {
        if ( syrecu_SynthOR(sr, net_refs, &net_ref) == 0 )
        {
          b_il_Close(net_refs);
          return 0;
        }
        assert( gnc_FindCellNetJoinByDriver(sr->nc, sr->cell_ref, net_ref) >= 0 );
      }
      else
      {
        net_ref = b_il_GetVal(net_refs, 0);
      }
      if ( gnc_MergeNets(sr->nc, sr->cell_ref, net_ref, syrecu_GetPortNet(sr, i+sr->rc->pi->in_cnt, 0)) == 0 )
      {
        b_il_Close(net_refs);
        return 0;
      }
      assert( gnc_FindCellNetJoinByDriver(sr->nc, sr->cell_ref, net_ref) >= 0 );
    }
  } /* for */
  b_il_Close(net_refs);

  return 1;
}



int gnc_SynthRECU(gnc nc, int cell_ref, recu rc, int is_optimize)
{
  syrecu_type sr;
  sr = syrecu_Open(nc);
  if ( sr != NULL )
  {
    sr->cell_ref = cell_ref;
    sr->rc = rc;
   
    /* removed, because there might be a register, that */
    /* would let the check function fail */
    /* assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0); */
    if ( syrecu_SynthTree(sr, rc->top) == 0 )
      return gnc_Error(nc, "gnc_SynthRECU: Synthesis of intermediate tree failed."), 0;
    if ( gnc_CheckCellNetDriver(nc, cell_ref) == 0 )
      return 0;
    assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);
    
    if ( is_optimize != 0 )
    {
      neca_Optimize(sr->and_gates, nc, cell_ref);
      assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);
      neca_Optimize(sr->or_gates, nc, cell_ref);
      assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);
    }
    
    syrecu_Close(sr);
    return 1;
  }
  return 0;
}

/*

  well, one could say that this is the central synthesis function...

  The 'min_delay' parameter is only used if the option GNC_SYNTH_OPT_DLYPATH
  is enabled.
*/
int gnc_synth_on_set(gnc nc, int cell_ref, int option, double min_delay, int is_delete_empty_cells)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  recu rc;
  
  if ( nc->wge == NULL )
  {
    gnc_Error(nc, "Synthesis: No technology data available.");
    return 0;
  }
  
  if ( cell->pi == NULL || cell->cl_on == NULL )
  {
    gnc_Error(nc, "Synthesis: Cell '%s' has no cube-cover.", 
      gnc_GetCellName(nc, cell_ref));
    return 0;
  }

  {
    int lit_cnt = dclGetLiteralCnt(cell->pi, cell->cl_on);
    gnc_Log(nc, 4, "Synthesis: Reading ON-set (%d product term%s, %d literal%s).", 
      dclCnt(cell->cl_on), dclCnt(cell->cl_on)==1?"":"s",
      lit_cnt, lit_cnt==1?"":"s");
  }
  
  gnc_LogDCL(nc, 0, "Synthesis: ", 2, "ON-set", cell->pi, cell->cl_on, "DC-set", cell->pi, cell->cl_dc);
  

  rc = rcOpenByDCList(cell->pi, cell->cl_on);
  if ( rc == NULL )
  {
    gnc_Error(nc, "Synthesis: Out of Memory (rcOpenByDCList).");
    return 0;
  }
  
  gnc_Log(nc, 4, "Synthesis: Building intermediate tree.");
  
  if ( rcBuildTree(rc, (option&GNC_SYNTH_OPT_LEVELS)==GNC_SYNTH_OPT_LEVELS, 
                       (option&GNC_SYNTH_OPT_OUTPUTS)==GNC_SYNTH_OPT_OUTPUTS) == 0 )
  {
    gnc_Error(nc, "Synthesis: Intermediate synthesis failed (rcBuildTree).");
    rcClose(rc);
    return 0;
  }

  gnc_Log(nc, 0, "Synthesis: Depth of intermediate tree is %d (level optimization is %s).",
    srnGetDepth(rc->top), (option&GNC_SYNTH_OPT_LEVELS)==GNC_SYNTH_OPT_LEVELS?"enabled":"disabled");

  if ( gnc_SynthRECU(nc, cell_ref, rc, (option&GNC_SYNTH_OPT_NECA)==GNC_SYNTH_OPT_NECA) == 0 )
  {
    rcClose(rc);
    return 0;
  }
  assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);

  if ( (option&GNC_SYNTH_OPT_GENERIC)==GNC_SYNTH_OPT_GENERIC )
  {
    gnc_Log(nc, 4, "Synthesis: Generic cell optimization.");
    gnc_OptGeneric(nc, cell_ref);
    assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);
  }

  gnc_Log(nc, 4, "Synthesis: Technology mapping.");
  
  if ( is_delete_empty_cells != 0 )
  {
    if ( gnc_SynthGenericToLib(nc, cell_ref) == 0 )
    {
      gnc_Error(nc, "Synthesis: Generic --> Lib mapping failed.");
      rcClose(rc);
      return 0;
    }
  }
  else
  {
    if ( gnc_SynthGenericToLibExceptEmptyCells(nc, cell_ref) == 0 )
    {
      gnc_Error(nc, "Synthesis: Generic --> Lib mapping failed.");
      rcClose(rc);
      return 0;
    }
  }
  assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);


  if ( (option&GNC_SYNTH_OPT_LIBARY)==GNC_SYNTH_OPT_LIBARY )
  {
    gnc_Log(nc, 4, "Synthesis: Technology optimization.");
    if ( gnc_OptLibrary(nc, cell_ref) == 0 )
      return 0;
    assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);
  }

  gnc_Log(nc, 4, "Synthesis: Verification.");

  if ( gnc_DoVerification(nc, cell_ref) == 0 )
    return 0;


  if ( (option&GNC_SYNTH_OPT_DLYPATH)==GNC_SYNTH_OPT_DLYPATH )
  {
    gnc_Log(nc, 4, "Synthesis: Delay path calculation.");
    if ( gnc_AddDelayPath(nc, cell_ref, min_delay, NULL, NULL) == 0 )
      return 0;
    assert(gnc_CheckCellNetDriver(nc, cell_ref)!=0);
  }

  gnc_Log(nc, 4, "Synthesis: Done (Cell area %lf).", gnc_CalcCellNetNodeArea(nc, cell_ref));

  gnc_DelCellEmptyNet(nc, cell_ref);

  rcClose(rc);
  return 1;
}

int gnc_SynthOnSet(gnc nc, int cell_ref, int option, double min_delay)
{
  return gnc_synth_on_set(nc, cell_ref, option, min_delay, 1);
}

/*!
  \ingroup gncsynth

  This function creates a netlist from a sum of product (\c dclist) representation of
  boolean functions. Boolean function are usually represented by a \c dclist
  structure. There are many other functions that create, read, store and 
  transform a \c dclist. 
  
  If the option \c GNC_HL_OPT_MINIMIZE is used, this function
  calls dclMinimizeDC().
  
  \pre Basic Building Blocks must be available. Usually this requires
  a call to gnc_ReadLibrary() and gnc_ApplyBBBs().

  \param nc A pointer to a gnc structure.
  \param pi The problem info structure for the boolean functions.
  \param cl_on The ON-set of the boolean function.
  \param cl_dc The DC-set of the boolean function.
  \param cell_name The cell-name of the new cell.
  \param lib_name The library-name of the new cell or \c NULL.
  \param hl_opt High level options (see gnc_SynthByFile()).
  \param synth_opt  Synthesis options  (see gnc_SynthByFile()).
  \param min_delay  Set the minimum delay for the delay path. Ifnored unless
    the option GNC_SYNTH_OPT_DLYPATH is used. The default value should be 0.0.

  \return A reference (\a cell_ref) to the new cell or -1 if an error occured.
  
  \see gnc_Open()
  \see gnc_ReadLibrary()
  \see gnc_ApplyBBBs()
  \see gnc_SynthByFile()
  \see dclImport()
  \see dclMinimizeDC()
  
*/
int gnc_SynthDCL(gnc nc, pinfo *pi, dclist cl_on, dclist cl_dc, const char *cell_name, const char *lib_name, int hl_opt, int synth_option, double min_delay)
{
  int cell_ref;

  gnc_Log(nc, 5, "Boolean function (%s): Synthesis started.", cell_name);
  
  cell_ref = gnc_AddCell(nc, cell_name, lib_name);
  if ( cell_ref < 0 )
    return -1;
    
  if ( gnc_SetDCL(nc, cell_ref, pi, cl_on, cl_dc, 0) == 0 )
    return gnc_DelCell(nc, cell_ref), -1;  

  if ( (hl_opt & GNC_HL_OPT_MINIMIZE) == GNC_HL_OPT_MINIMIZE )
  {
    gcell cell;
    cell = gnc_GetGCELL(nc, cell_ref);

/* attention: additional parameter in dclMinimizeDC for switching to exact/heuristic solving
            greedy = 0, 1:   0 exact minimization, 1 to minimize a greedy heuristic way */
    if ( dclMinimizeDC(cell->pi, cell->cl_on, cell->cl_dc, 0, 0) == 0 )
      return gnc_DelCell(nc, cell_ref), -1;  
    gnc_Log(nc, 5, "Boolean function (%s): 2-level minimization done (%d implicants).", cell_name, dclCnt(cell->cl_on));
  }
  
  if ( gnc_SynthOnSet(nc, cell_ref, synth_option, min_delay) == 0 )
    return gnc_DelCell(nc, cell_ref), -1;  

  return cell_ref;
}

/*!
  \ingroup gncsynth

  This function creates a netlist from a sum of product representation of
  boolean functions. The description is read from the file \a filename.
  The following file formats are supported:
  - NEX
  - BEX
  - PLA

  If the option \c GNC_HL_OPT_MINIMIZE is used, this function
  calls dclMinimizeDC().  

  \pre Basic Building Blocks must be available. Usually this requires
  a call to gnc_ReadLibrary() and gnc_ApplyBBBs().

  \param nc A pointer to a gnc structure.
  \param filename File with the description of a boolean functions.
  \param cell_name The cell-name of the new cell.
  \param lib_name The library-name of the new cell or \c NULL.
  \param hl_opt High level options (see gnc_SynthByFile()).
  \param synth_opt  Synthesis options  (see gnc_SynthByFile()).
  \param min_delay  Set the minimum delay for the delay path. Ifnored unless
    the option GNC_SYNTH_OPT_DLYPATH is used. The default value should be 0.0.

  \return A reference (\a cell_ref) to the new cell or -1 if an error occured.
  
  \see gnc_Open()
  \see gnc_ReadLibrary()
  \see gnc_ApplyBBBs()
  \see gnc_SynthDCL()
  \see gnc_SynthByFile()
  \see dclMinimizeDC()
  
  
*/
int gnc_SynthDCLByFile(gnc nc, const char *filename, const char *cell_name, const char *lib_name, int hl_opt, int synth_option, double min_delay)
{
  pinfo pi;
  int cell_ref;
  dclist cl_on, cl_dc;
  
  if ( pinfoInit(&pi) == 0 )
    return gnc_Error(nc, "gnc_SynthDCLByFile: pinfoInit failed"), -1;
  
  if ( dclInitVA(2, &cl_on, &cl_dc) == 0 )
    return gnc_Error(nc, "gnc_SynthDCLByFile: dclInitVA failed"), -1;

  if ( dclImport(&pi, cl_on, cl_dc, filename) == 0 )
    return gnc_Error(nc, "gnc_SynthDCLByFile: dclImport failed"), dclDestroyVA(2, cl_on, cl_dc), -1;

  cell_ref = gnc_SynthDCL(nc, &pi, cl_on, cl_dc, cell_name, lib_name, hl_opt, synth_option, min_delay);
  if ( cell_ref < 0 )
    return gnc_Error(nc, "gnc_SynthDCLByFile: gnc_SynthDCL failed"), dclDestroyVA(2, cl_on, cl_dc), -1;
    
  dclDestroyVA(2, cl_on, cl_dc);
  
  pinfoDestroy(&pi);
  
  return cell_ref;
}


int gnc_WriteCellBEX(gnc nc, int cell_ref, const char *filename)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  if ( cell->pi == NULL || cell->cl_on == NULL )
    return gnc_Error(nc, "gnc_WriteCellBEX: Cell does not contain SOP specification."), -1;
  
  return dclWriteBEX(cell->pi, cell->cl_on, filename);
}

int gnc_WriteCellPLA(gnc nc, int cell_ref, const char *filename)
{
  gcell cell = gnc_GetGCELL(nc, cell_ref);
  if ( cell->pi == NULL || cell->cl_on == NULL )
    return gnc_Error(nc, "gnc_WriteCellPLA: Cell does not contain SOP specification."), -1;
  
  return dclWritePLA(cell->pi, cell->cl_on, filename);
}

