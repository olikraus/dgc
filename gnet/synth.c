/*

  SYNTH.C
  
  The toplevel synthesis function for gnc 
  
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

#include "gnet.h"
#include "mwc.h"

/* from xbm.h */
int IsValidXBMFile(const char *name);


/*

  Args:
    nc                Netlist collection, allocated with gnc_Open
    import_file_name  The name of a file with some kind of digital circuit
                      description
    cell_name         The name of the new cell or NULL.
    lib_name          The library name of the cell
    hl_opt            ORed list of high level flags
    synth_option      ORed list of synthesis flags
                    

  Description:
    Does a synthesis of the specified file. The results are stored in a
    new cell. The cell will get the specified cell and lib name. Both 
    can be NULL.
    
    The function determines the type of the file and does either a
    synthesis of a combinational circuit or a synthesis of a 
    sequential circuit.
    
  Returns:
    The reference to the new cell or -1.
    
  Preconditions:
    A technology (gate) library must have been imported with
      gnc_ReadLibrary
    and the cells inside the library must have been selected for the
    synthesis process with
      gnc_ApplyBBBs


*/

/*!
  \ingroup gncsynth

  \em Synthesis is the process of optaining a cell description that is
  based on some simple cells (basic building blocks) from a 
  functional description.
  
  This functions assumes a functional description in the file 
  \a import_file_name. A synthesis is performend on this file and
  the result is stored in a new cell with the cell-name \a cell_name
  and the library name \a lib_name. The result is called a circuit
  and is described by a netlist.
  
  Several different file formats are allowed for \a import_file_name.
  This is a (possible incomplete) list of supported file formats:  
  - KISS
  - BMS
  - PLA
  - NEX
  - BEX
  - DGD
  
  Synthesis might be changed and optimized by several options. Options
  are divided into 
    - high level options and
    - synthesis options.
  
  High level options can be ORed together. A useful default is
  \c GNC_HL_OPT_DEFAULT. Options are:
      -# Reset behaviour of the circuit
         - \c GNC_HL_OPT_NO_RESET  No reset at all.
         - \c GNC_HL_OPT_CLR_HIGH  Aktive high reset line.
         - \c GNC_HL_OPT_CLR_LOW  Aktive low reset line.
      -# For state machines only: Synchronous or asynchronous:
          - \c GNC_HL_OPT_CLOCK  Generate a clocked circuit and a clock input line.
          - The absence of \c GNC_HL_OPT_CLOCK generates an asynchronous circuit if possible.
      -# Minimization
          - \c GNC_HL_OPT_MINIMIZE  Do a boolean minimzation of the description.
          - The absence of \c GNC_HL_OPT_MINIMIZE makes immediate use of the result of the import filter for the description.
      -# For asynchronous state machines : Perform delay correction
          - \c GNC_HL_OPT_NO_DELAY  Do delay correction
          - The absence of \c GNC_HL_OPT_NO_DELAY avoids the delay correction
      -# For hierachical import formats:
          - \c GNC_HL_OPT_FLATTEN  Flatten hierarchy
          - The absence of \c GNC_HL_OPT_FLATTEN preserves the hierarchy.
            Note that not some export filter might not support hierarchical
            dependencies.
      

  Synthesis options can also be ORed together. A useful default is
  \c GNC_SYNTH_OPT_DEFAULT. This will enable all optimization, but does
  not create a delay path. Options are:
      -# \c GNC_SYNTH_OPT_NECA  Collects common inputs together
      -# \c GNC_SYNTH_OPT_GENERIC  Does an optimization of the generic intermediate format.
      -# \c GNC_SYNTH_OPT_LIBARY  Performs an optimization step on the result
      -# \c GNC_SYNTH_OPT_LEVELS  Does a multilevel mimization of the imported boolean function.
      -# \c GNC_SYNTH_OPT_OUTPUTS  Merge common subexpressions of two or more functions.
      -# \c GNC_SYNTH_OPT_DLYPATH  Creates an additional delay path with a delay which
      is larger than the largest input-output delay time of the circuit. This will
      also add two additional ports to the cell.
  
  \pre Basic Building Blocks must be available. Usually this requires
  a call to gnc_ReadLibrary() and gnc_ApplyBBBs().
  
  \param nc A pointer to a gnc structure.
  \param import_file_name The filename of the description of the new cell
  \param cell_name The cell-name of the new cell.
  \param lib_name The library-name of the new cell or \c NULL.
  \param hl_opt High level options
  \param synth_opt  Synthesis options


  \return A reference (\a cell_ref) to the new cell or -1 if an error occured.
    
  \see gnc_Open()
  \see gnc_ReadLibrary()
  \see gnc_ApplyBBBs()
*/

int gnc_SynthByFile(gnc nc, const char *import_file_name, const char *cell_name, const char *lib_name, int hl_opt, int synth_opt)
{
  if ( IsValidDCLFile(import_file_name) != 0 )
    return gnc_SynthDCLByFile(nc, import_file_name, cell_name, lib_name, hl_opt, synth_opt, 0.0);
  if ( IsValidXBMFile(import_file_name) != 0 )
    return gnc_SynthXBMByFile(nc, import_file_name, cell_name, lib_name, hl_opt, synth_opt);
  if ( IsValidFSMFile(import_file_name) != 0 )
    return gnc_SynthFSMByFile(nc, import_file_name, cell_name, lib_name, hl_opt, synth_opt);
  return gnc_SynthDGD(nc, import_file_name, hl_opt, synth_opt);
}

