/*

   SC_DATA.C
   
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

   29.06.96  Oliver Kraus
   
*/

#include <stddef.h>
#include "sc_sld.h"
#include "mwc.h"

char sc_str_bundle[]                = "bundle";
char sc_str_bus[]                   = "bus";
char sc_str_cell[]                  = "cell";
char sc_str_cell_enum[]             = "cell_enum";
char sc_str_cell_fall[]             = "cell_fall";
char sc_str_cell_rise[]             = "cell_rise";
char sc_str_fall_constraint[]       = "fall_constraint";
char sc_str_fall_power[]            = "fall_power";               /* new */
char sc_str_fall_propagation[]      = "fall_propagation";
char sc_str_fall_transition[]       = "fall_transition";
char sc_str_ff[]                    = "ff";
char sc_str_ff_bank[]               = "ff_bank";
char sc_str_input_voltage[]         = "input_voltage";
char sc_str_internal_power[]        = "internal_power";
char sc_str_latch[]                 = "latch";
char sc_str_latch_bank[]            = "latch_bank";
char sc_str_layer[]                 = "layer";
char sc_str_library[]               = "library";
char sc_str_library_symbol[]        = "library_symbol";
char sc_str_lu_table_template[]     = "lu_table_template";        
char sc_str_operating_conditions[]  = "operating_conditions";
char sc_str_output_voltage[]        = "output_voltage";
char sc_str_memory[]                = "memory";
char sc_str_parameterized_cell[]    = "parameterized_cell";
char sc_str_parameterized_pin[]     = "parameterized_pin";
char sc_str_pin[]                   = "pin";
char sc_str_power_lut_template[]    = "power_lut_template";       
char sc_str_rise_constraint[]       = "rise_constraint";
char sc_str_rise_power[]            = "rise_power";                /* new */
char sc_str_rise_propagation[]      = "rise_propagation";
char sc_str_rise_transition[]       = "rise_transition";
char sc_str_routing_track[]         = "routing_track";
char sc_str_scaled_cell[]           = "scaled_cell";
char sc_str_scaling_factors[]       = "scaling_factors";
char sc_str_state[]                 = "state";
char sc_str_statetable[]            = "statetable";
char sc_str_symbol[]                = "symbol";
char sc_str_test_cell[]             = "test_cell";
char sc_str_timing[]                = "timing";
char sc_str_timing_range[]          = "timing_range";
char sc_str_type[]                  = "type";
char sc_str_wire_load[]             = "wire_load";
char sc_str_wire_load_selection[]   = "wire_load_selection";
char sc_str_wire_load_table[]       = "wire_load_table";            /* new */

/*---------------------------------------------------------------------------*/

sc_sdef_init_struct sc_sdef_bundle[] =
{
   { "direction",                           SC_SDEF_TYP_IDENTIFIER },
   { "capacitance",                         SC_SDEF_TYP_FLOAT      },
   { "function",                            SC_SDEF_TYP_STRING     },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_bundle[] =
{
   { "members",      { SC_CDEF_TYP_STRING, SC_CDEF_TYP_STRING, SC_CDEF_TYP_STRING, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END }  },
   { "pin_equal",    { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { "pin_opposite", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_bus[] =
{
   { "bus_type",                       SC_SDEF_TYP_STRING  },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_bus[] =
{
   { "pin_equal",    { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { "pin_opposite", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_cell[] =
{
   { "version",                        SC_SDEF_TYP_FLOAT   },
   { "area",                           SC_SDEF_TYP_FLOAT   },
   { "auxiliary_pad_cell",             SC_SDEF_TYP_BOOLEAN },
   { "cell_footprint",                 SC_SDEF_TYP_STRING  },
   { "cell_leakage_power",             SC_SDEF_TYP_FLOAT   },
   { "cell_power",                     SC_SDEF_TYP_FLOAT   },
   { "dont_touch",                     SC_SDEF_TYP_BOOLEAN },
   { "dont_use",                       SC_SDEF_TYP_BOOLEAN },
   { "pad_cell",                       SC_SDEF_TYP_BOOLEAN },
   { "pad_type",                       SC_SDEF_TYP_IDENTIFIER },
   { "pin_limit",                      SC_SDEF_TYP_INTEGER },
   { "preferred",                      SC_SDEF_TYP_BOOLEAN },
   { "scaling_factors",                SC_SDEF_TYP_STRING  },
   { "scan_group",                     SC_SDEF_TYP_STRING  },
   { "vhdl_name",                      SC_SDEF_TYP_STRING  },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_cell[] =
{
   { "pin_equal",    { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { "pin_opposite", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_cell_enum[] =
{
   { "cell_property",                  SC_SDEF_TYP_STRING  },
   { "default_enum",                   SC_SDEF_TYP_BOOLEAN },
   { "vhdl_name",                      SC_SDEF_TYP_STRING  },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_cell_enum[] =
{
   { "pin_equal",    { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { "pin_opposite", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_cell_fall[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_cell_fall[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_cell_rise[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_cell_rise[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_fall_constraint[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_fall_constraint[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};


sc_sdef_init_struct sc_sdef_fall_power[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_fall_power[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_fall_propagation[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_fall_propagation[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_fall_transition[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_fall_transition[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_ff[] =
{
   { "clocked_on",                     SC_SDEF_TYP_STRING },
   { "next_state",                     SC_SDEF_TYP_STRING },
   { "clear",                          SC_SDEF_TYP_STRING },
   { "preset",                         SC_SDEF_TYP_STRING },
   { "clear_preset_var1",              SC_SDEF_TYP_IDENTIFIER },
   { "clear_preset_var2",              SC_SDEF_TYP_IDENTIFIER },
   { "clocked_on_also",                SC_SDEF_TYP_STRING },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_ff[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_ff_bank[] =
{
   { "clocked_on",                     SC_SDEF_TYP_STRING },
   { "next_state",                     SC_SDEF_TYP_STRING },
   { "clear",                          SC_SDEF_TYP_STRING },
   { "preset",                         SC_SDEF_TYP_STRING },
   { "clear_preset_var1",              SC_SDEF_TYP_IDENTIFIER },
   { "clear_preset_var2",              SC_SDEF_TYP_IDENTIFIER },
   { "clocked_on_also",                SC_SDEF_TYP_STRING },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_ff_bank[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_input_voltage[] =
{
   { "vil",                       SC_SDEF_TYP_FLOAT },
   { "vih",                       SC_SDEF_TYP_FLOAT },
   { "vimin",                     SC_SDEF_TYP_FLOAT },
   { "vimax",                     SC_SDEF_TYP_FLOAT },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_input_voltage[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_internal_power[] =
{
   { "when",                      SC_SDEF_TYP_STRING },
   { "related_input",             SC_SDEF_TYP_STRING },
   { "related_output",            SC_SDEF_TYP_STRING },
   { "related_inputs",            SC_SDEF_TYP_STRING },
   { "related_outputs",           SC_SDEF_TYP_STRING },
   { "related_pin",               SC_SDEF_TYP_STRING },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_internal_power[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};


sc_sdef_init_struct sc_sdef_latch[] =
{
   { "enable",                         SC_SDEF_TYP_STRING },
   { "data_in",                        SC_SDEF_TYP_STRING },
   { "clear",                          SC_SDEF_TYP_STRING },
   { "preset",                         SC_SDEF_TYP_STRING },
   { "clear_preset_var1",              SC_SDEF_TYP_IDENTIFIER },
   { "clear_preset_var2",              SC_SDEF_TYP_IDENTIFIER },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_latch[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_latch_bank[] =
{
   { "enable",                         SC_SDEF_TYP_STRING },
   { "data_in",                        SC_SDEF_TYP_STRING },
   { "clear",                          SC_SDEF_TYP_STRING },
   { "preset",                         SC_SDEF_TYP_STRING },
   { "clear_preset_var1",              SC_SDEF_TYP_IDENTIFIER },
   { "clear_preset_var2",              SC_SDEF_TYP_IDENTIFIER },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_latch_bank[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_layer[] =
{
   { "blue",                           SC_SDEF_TYP_INTEGER },
   { "green",                          SC_SDEF_TYP_INTEGER },
   { "line_width",                     SC_SDEF_TYP_INTEGER },
   { "red",                            SC_SDEF_TYP_INTEGER },
   { "scalable_lines",                 SC_SDEF_TYP_BOOLEAN },
   { "visible",                        SC_SDEF_TYP_BOOLEAN },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_layer[] =
{
   { "set_font",    { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_library[] =
{
   { "aux_no_pulldown_pin_property",   SC_SDEF_TYP_STRING  },
   { "bus_naming_style",               SC_SDEF_TYP_STRING  },
   { "comment",                        SC_SDEF_TYP_STRING  },
   { "current_unit",                   SC_SDEF_TYP_STRING  },
   { "date",                           SC_SDEF_TYP_STRING  },
   { "delay_model",                    SC_SDEF_TYP_STRING },
   { "in_place_swap_mode",             SC_SDEF_TYP_IDENTIFIER },
   { "max_wired_emitters",             SC_SDEF_TYP_INTEGER },
   { "multiple_driver_legal",          SC_SDEF_TYP_BOOLEAN },
   { "nom_process",                    SC_SDEF_TYP_FLOAT   },
   { "nom_temperature",                SC_SDEF_TYP_FLOAT   },
   { "nom_voltage",                    SC_SDEF_TYP_FLOAT   },
   { "nonpaired_twin_inc_delay_func",  SC_SDEF_TYP_STRING  },
   { "no_pulldown_pin_property",       SC_SDEF_TYP_STRING  },
   { "power_unit",                     SC_SDEF_TYP_FLOAT   },
   { "pulling_resistance_unit",        SC_SDEF_TYP_STRING  },
   { "revision",                       SC_SDEF_TYP_FLOAT   },  /* also string */
   { "simulation",                     SC_SDEF_TYP_BOOLEAN },
   { "time_unit",                      SC_SDEF_TYP_STRING  },
   { "unconnected_pin_property",       SC_SDEF_TYP_STRING  },
   { "voltage_unit",                   SC_SDEF_TYP_STRING  },
   { "wired_logic_function",           SC_SDEF_TYP_IDENTIFIER},
   { "input_threshold_pct_rise",       SC_SDEF_TYP_FLOAT   },
   { "input_threshold_pct_fall",       SC_SDEF_TYP_FLOAT   },
   { "output_threshold_pct_rise",      SC_SDEF_TYP_FLOAT   },
   { "output_threshold_pct_fall",      SC_SDEF_TYP_FLOAT   },
   { "slew_lower_threshold_pct_rise",  SC_SDEF_TYP_FLOAT   },
   { "slew_upper_threshold_pct_rise",  SC_SDEF_TYP_FLOAT   },
   { "slew_lower_threshold_pct_fall",  SC_SDEF_TYP_FLOAT   },
   { "slew_upper_threshold_pct_fall",  SC_SDEF_TYP_FLOAT   },
   { "slew_derate_from_library",       SC_SDEF_TYP_FLOAT   },
   { "default_cell_power",             SC_SDEF_TYP_FLOAT  },
   { "default_cell_leakage_power",     SC_SDEF_TYP_FLOAT  },
   { "default_connection_class",       SC_SDEF_TYP_STRING },
   { "default_conection_class",        SC_SDEF_TYP_STRING },  /* wrong... !!!??? */
   { "default_emitter_count",          SC_SDEF_TYP_INTEGER},
   { "default_fall_delay_intercept",   SC_SDEF_TYP_FLOAT  },
   { "default_fall_nonpaired_twin",    SC_SDEF_TYP_FLOAT  },
   { "default_fall_pin_resistance",    SC_SDEF_TYP_FLOAT  },
   { "default_fall_wire_resistance",   SC_SDEF_TYP_FLOAT  },
   { "default_fall_wor_emitter",       SC_SDEF_TYP_FLOAT  },
   { "default_fall_wor_intercept",     SC_SDEF_TYP_FLOAT  },
   { "default_fanout_load",            SC_SDEF_TYP_FLOAT  },
   { "default_inout_pin_cap",          SC_SDEF_TYP_FLOAT  },
   { "default_inout_pin_fall_res",     SC_SDEF_TYP_FLOAT  },
   { "default_inout_pin_rise_res",     SC_SDEF_TYP_FLOAT  },
   { "default_input_pin_cap",          SC_SDEF_TYP_FLOAT  },
   { "default_intrinsic_fall",         SC_SDEF_TYP_FLOAT  },
   { "default_intrinsic_rise",         SC_SDEF_TYP_FLOAT  },
   { "default_leakage_power_density",  SC_SDEF_TYP_FLOAT  },
   { "default_max_fanout",             SC_SDEF_TYP_FLOAT  },
   { "default_max_transition",         SC_SDEF_TYP_FLOAT  },
   { "default_operating_conditions",   SC_SDEF_TYP_STRING },
   { "default_output_pin_cap",         SC_SDEF_TYP_FLOAT  },
   { "default_output_pin_fall_res",    SC_SDEF_TYP_FLOAT  },
   { "default_output_pin_rise_res",    SC_SDEF_TYP_FLOAT  },
   { "default_pin_limit",              SC_SDEF_TYP_INTEGER},
   { "default_pin_power",              SC_SDEF_TYP_INTEGER},
   { "default_rise_delay_intercept",   SC_SDEF_TYP_FLOAT  },
   { "default_rise_nonpaired_twin",    SC_SDEF_TYP_FLOAT  },
   { "default_rise_pin_resistance",    SC_SDEF_TYP_FLOAT  },
   { "default_rise_wire_resistance",   SC_SDEF_TYP_FLOAT  },
   { "default_rise_wor_emitter",       SC_SDEF_TYP_FLOAT  },
   { "default_rise_wor_intercept",     SC_SDEF_TYP_FLOAT  },
   { "default_slope_fall",             SC_SDEF_TYP_FLOAT  },
   { "default_slope_rise",             SC_SDEF_TYP_FLOAT  },
   { "default_wire_load",              SC_SDEF_TYP_STRING },
   { "default_wire_load_mode",         SC_SDEF_TYP_STRING },
   { "default_wire_load_capacitance",  SC_SDEF_TYP_FLOAT  },
   { "default_wire_load_resistance",   SC_SDEF_TYP_FLOAT  },
   { "default_wire_load_area",         SC_SDEF_TYP_FLOAT  },
   { "default_wire_load_selection",    SC_SDEF_TYP_STRING },
   { "k_process_cell_leakage_power",   SC_SDEF_TYP_FLOAT  },
   { "k_process_cell_power",           SC_SDEF_TYP_FLOAT  },
   { "k_process_cell_rise",            SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_cell_fall",            SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_fall_delay_intercept", SC_SDEF_TYP_FLOAT  },
   { "k_process_fall_pin_resistance",  SC_SDEF_TYP_FLOAT  },
   { "k_process_fall_propagation",     SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_fall_transition",      SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_fall_wire_resistance", SC_SDEF_TYP_FLOAT  },
   { "k_process_fall_wor_emitter",     SC_SDEF_TYP_FLOAT  },
   { "k_process_fall_wor_intercept",   SC_SDEF_TYP_FLOAT  },
   { "k_process_hold_fall",            SC_SDEF_TYP_FLOAT  },
   { "k_process_hold_rise",            SC_SDEF_TYP_FLOAT  },
   { "k_process_drive_fall",           SC_SDEF_TYP_FLOAT  },
   { "k_process_drive_rise",           SC_SDEF_TYP_FLOAT  },
   { "k_process_internal_power",       SC_SDEF_TYP_FLOAT  },
   { "k_process_intrinsic_fall",       SC_SDEF_TYP_FLOAT  },
   { "k_process_intrinsic_rise",       SC_SDEF_TYP_FLOAT  },
   { "k_process_slope_fall",           SC_SDEF_TYP_FLOAT  },
   { "k_process_slope_rise",           SC_SDEF_TYP_FLOAT  },
   { "k_process_min_period",           SC_SDEF_TYP_FLOAT  },
   { "k_process_min_pulse_width_high", SC_SDEF_TYP_FLOAT  },
   { "k_process_min_pulse_width_low",  SC_SDEF_TYP_FLOAT  },
   { "k_process_pin_cap",              SC_SDEF_TYP_FLOAT  },
   { "k_process_pin_power",            SC_SDEF_TYP_FLOAT  },
   { "k_process_recovery_fall",        SC_SDEF_TYP_FLOAT  },
   { "k_process_recovery_rise",        SC_SDEF_TYP_FLOAT  },
   { "k_process_rise_delay_intercept", SC_SDEF_TYP_FLOAT  },
   { "k_process_rise_pin_resistance",  SC_SDEF_TYP_FLOAT  },
   { "k_process_rise_propagation",     SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_rise_transition",      SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_rise_wire_resistance", SC_SDEF_TYP_FLOAT  },
   { "k_process_rise_wor_emitter",     SC_SDEF_TYP_FLOAT  },
   { "k_process_rise_wor_intercept",   SC_SDEF_TYP_FLOAT  },
   { "k_process_setup_fall",           SC_SDEF_TYP_FLOAT  },
   { "k_process_setup_rise",           SC_SDEF_TYP_FLOAT  },
   { "k_process_wire_cap",             SC_SDEF_TYP_FLOAT  },
   { "k_process_wire_res",             SC_SDEF_TYP_FLOAT  },
   { "k_temp_cell_leakage_power",      SC_SDEF_TYP_FLOAT  },
   { "k_temp_cell_power",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_cell_rise",               SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_cell_fall",               SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_fall_delay_intercept",    SC_SDEF_TYP_FLOAT  },
   { "k_temp_fall_pin_resistance",     SC_SDEF_TYP_FLOAT  },
   { "k_temp_fall_propagation",        SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_fall_transition",         SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_fall_wire_resistance",    SC_SDEF_TYP_FLOAT  },
   { "k_temp_fall_wor_emitter",        SC_SDEF_TYP_FLOAT  },
   { "k_temp_fall_wor_intercept",      SC_SDEF_TYP_FLOAT  },
   { "k_temp_hold_fall",               SC_SDEF_TYP_FLOAT  },
   { "k_temp_hold_rise",               SC_SDEF_TYP_FLOAT  },
   { "k_temp_drive_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_drive_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_internal_power",          SC_SDEF_TYP_FLOAT  },
   { "k_temp_intrinsic_fall",          SC_SDEF_TYP_FLOAT  },
   { "k_temp_intrinsic_rise",          SC_SDEF_TYP_FLOAT  },
   { "k_temp_slope_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_slope_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_min_period",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_min_pulse_width_high",    SC_SDEF_TYP_FLOAT  },
   { "k_temp_min_pulse_width_low",     SC_SDEF_TYP_FLOAT  },
   { "k_temp_pin_cap",                 SC_SDEF_TYP_FLOAT  },
   { "k_temp_pin_power",               SC_SDEF_TYP_FLOAT  },
   { "k_temp_recovery_fall",           SC_SDEF_TYP_FLOAT  },
   { "k_temp_recovery_rise",           SC_SDEF_TYP_FLOAT  },
   { "k_temp_rise_delay_intercept",    SC_SDEF_TYP_FLOAT  },
   { "k_temp_rise_pin_resistance",     SC_SDEF_TYP_FLOAT  },
   { "k_temp_rise_propagation",        SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_rise_transition",         SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_rise_wire_resistance",    SC_SDEF_TYP_FLOAT  },
   { "k_temp_rise_wor_emitter",        SC_SDEF_TYP_FLOAT  },
   { "k_temp_rise_wor_intercept",      SC_SDEF_TYP_FLOAT  },
   { "k_temp_setup_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_setup_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_wire_cap",                SC_SDEF_TYP_FLOAT  },
   { "k_temp_wire_res",                SC_SDEF_TYP_FLOAT  },
   { "k_volt_cell_leakage_power",      SC_SDEF_TYP_FLOAT  },
   { "k_volt_cell_power",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_cell_rise",               SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_cell_fall",               SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_fall_delay_intercept",    SC_SDEF_TYP_FLOAT  },
   { "k_volt_fall_pin_resistance",     SC_SDEF_TYP_FLOAT  },
   { "k_volt_fall_propagation",        SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_fall_transition",         SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_fall_wire_resistance",    SC_SDEF_TYP_FLOAT  },
   { "k_volt_fall_wor_emitter",        SC_SDEF_TYP_FLOAT  },
   { "k_volt_fall_wor_intercept",      SC_SDEF_TYP_FLOAT  },
   { "k_volt_hold_fall",               SC_SDEF_TYP_FLOAT  },
   { "k_volt_hold_rise",               SC_SDEF_TYP_FLOAT  },
   { "k_volt_drive_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_drive_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_internal_power",          SC_SDEF_TYP_FLOAT  },
   { "k_volt_intrinsic_fall",          SC_SDEF_TYP_FLOAT  },
   { "k_volt_intrinsic_rise",          SC_SDEF_TYP_FLOAT  },
   { "k_volt_slope_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_slope_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_min_period",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_min_pulse_width_high",    SC_SDEF_TYP_FLOAT  },
   { "k_volt_min_pulse_width_low",     SC_SDEF_TYP_FLOAT  },
   { "k_volt_pin_cap",                 SC_SDEF_TYP_FLOAT  },
   { "k_volt_pin_power",               SC_SDEF_TYP_FLOAT  },
   { "k_volt_recovery_fall",           SC_SDEF_TYP_FLOAT  },
   { "k_volt_recovery_rise",           SC_SDEF_TYP_FLOAT  },
   { "k_volt_rise_delay_intercept",    SC_SDEF_TYP_FLOAT  },
   { "k_volt_rise_pin_resistance",     SC_SDEF_TYP_FLOAT  },
   { "k_volt_rise_propagation",        SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_rise_transition",         SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_rise_wire_resistance",    SC_SDEF_TYP_FLOAT  },
   { "k_volt_rise_wor_emitter",        SC_SDEF_TYP_FLOAT  },
   { "k_volt_rise_wor_intercept",      SC_SDEF_TYP_FLOAT  },
   { "k_volt_setup_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_setup_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_wire_cap",                SC_SDEF_TYP_FLOAT  },
   { "k_volt_wire_res",                SC_SDEF_TYP_FLOAT  },
   { "leakage_power_unit",             SC_SDEF_TYP_STRING  },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_library[] =
{
   { "define", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_STRING, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END}},   
   { "capacitive_load_unit", { SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_IDENTIFIER, SC_CDEF_TYP_END }  },
   { "define_cell_area", { SC_CDEF_TYP_IDENTIFIER, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END }     },
   { "piece_define", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END }                                 },
   { "technology", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END }                                   },
   { "library_features", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END }                                   },
   { NULL, { 0 } }
};
sc_sdef_init_struct sc_sdef_library_symbol[] =
{
   { "in_osc_symbol",                  SC_SDEF_TYP_STRING  },
   { "in_port_symbol",                 SC_SDEF_TYP_STRING  },
   { "inout_osc_symbol",               SC_SDEF_TYP_STRING  },
   { "inout_port_symbol",              SC_SDEF_TYP_STRING  },
   { "logic_0_symbol",                 SC_SDEF_TYP_STRING  },
   { "logic_1_symbol",                 SC_SDEF_TYP_STRING  },
   { "logic_1_symbol",                 SC_SDEF_TYP_STRING  },
   { "out_port_symbol",                SC_SDEF_TYP_STRING  },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_library_symbol[] =
{
   /* ... */
   { NULL, { 0 } }
};


sc_sdef_init_struct sc_sdef_lu_table_template[] =     /* new */
{
   { "variable_1",                        SC_SDEF_TYP_IDENTIFIER },
   { "variable_2",                        SC_SDEF_TYP_IDENTIFIER },
   { "variable_3",                        SC_SDEF_TYP_IDENTIFIER },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_lu_table_template[] =     /* new */
{
   { "index_1", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { "index_3", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};


sc_sdef_init_struct sc_sdef_memory[] =     
{
   { "type",                              SC_SDEF_TYP_STRING     },
   { "adress_width",                      SC_SDEF_TYP_FLOAT      },
   { "word_width",                        SC_SDEF_TYP_FLOAT      },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_memory[] =     
{
  { NULL, { 0 } }
};


sc_sdef_init_struct sc_sdef_operating_conditions[] =
{
   { "process",                        SC_SDEF_TYP_FLOAT      },
   { "temperature",                    SC_SDEF_TYP_FLOAT      },
   { "tree_type",                      SC_SDEF_TYP_STRING },
   { "voltage",                        SC_SDEF_TYP_FLOAT      },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_operating_conditions[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_output_voltage[] =
{
   { "vol",                            SC_SDEF_TYP_FLOAT      },
   { "voh",                            SC_SDEF_TYP_FLOAT      },
   { "vomin",                          SC_SDEF_TYP_FLOAT      },
   { "vomax",                          SC_SDEF_TYP_FLOAT      },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_output_voltage[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_parameterized_cell[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_parameterized_cell[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_parameterized_pin[] =
{
   { "disabled",                       SC_SDEF_TYP_BOOLEAN },
   { "pin_properties",                 SC_SDEF_TYP_STRING  },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_parameterized_pin[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_pin[] =
{
   { "capacitance",                         SC_SDEF_TYP_FLOAT      },
   { "clock",                               SC_SDEF_TYP_STRING     },
   { "connection_class",                    SC_SDEF_TYP_STRING     },
   { "direction",                           SC_SDEF_TYP_STRING     },
   { "drive_current",                       SC_SDEF_TYP_FLOAT      },
   { "driver_type",                         SC_SDEF_TYP_STRING     },
   { "emitter_count",                       SC_SDEF_TYP_INTEGER    },
   { "fall_current_slope_after_treshhold",  SC_SDEF_TYP_FLOAT      },
   { "fall_current_slope_before_treshhold", SC_SDEF_TYP_FLOAT      },
   { "fall_time_after_treshhold",           SC_SDEF_TYP_FLOAT      },
   { "fall_time_before_treshhold",          SC_SDEF_TYP_FLOAT      },
   { "fall_wor_emitter",                    SC_SDEF_TYP_FLOAT      },
   { "fall_wor_intercept",                  SC_SDEF_TYP_FLOAT      },
   { "fanout_load",                         SC_SDEF_TYP_FLOAT      },
   { "function",                            SC_SDEF_TYP_STRING     },
   { "internal_node",                       SC_SDEF_TYP_STRING     },
   { "hysteresis",                          SC_SDEF_TYP_BOOLEAN    },
   { "input_voltage",                       SC_SDEF_TYP_STRING     },
   { "is_pad",                              SC_SDEF_TYP_BOOLEAN    },
   { "max_capacitance",                     SC_SDEF_TYP_FLOAT      },
   { "max_fanout",                          SC_SDEF_TYP_FLOAT      },
   { "max_transition",                      SC_SDEF_TYP_FLOAT      },
   { "min_period",                          SC_SDEF_TYP_FLOAT      },
   { "min_pulse_width_high",                SC_SDEF_TYP_FLOAT      },
   { "min_pulse_width_low",                 SC_SDEF_TYP_FLOAT      },
   { "multicell_pad_pin",                   SC_SDEF_TYP_BOOLEAN    },
   { "multiple_drivers_legal",              SC_SDEF_TYP_BOOLEAN    },
   { "output_voltage",                      SC_SDEF_TYP_STRING     },
   { "pin_power",                           SC_SDEF_TYP_FLOAT      },
   { "prefer_tied",                         SC_SDEF_TYP_BOOLEAN    },
   { "pulling_current",                     SC_SDEF_TYP_FLOAT      },
   { "pulling_resistance",                  SC_SDEF_TYP_FLOAT      },
   { "rise_current_slope_after_treshhold",  SC_SDEF_TYP_FLOAT      },
   { "rise_current_slope_before_treshhold", SC_SDEF_TYP_FLOAT      },
   { "rise_time_after_treshhold",           SC_SDEF_TYP_FLOAT      },
   { "rise_time_before_treshhold",          SC_SDEF_TYP_FLOAT      },
   { "rise_wor_emitter",                    SC_SDEF_TYP_FLOAT      },
   { "rise_wor_intercept",                  SC_SDEF_TYP_FLOAT      },
   { "slew-control",                        SC_SDEF_TYP_IDENTIFIER }, /* ??? */
   { "slew_control",                        SC_SDEF_TYP_IDENTIFIER },
   { "signal_type",                         SC_SDEF_TYP_STRING     },
   { "three_state",                         SC_SDEF_TYP_STRING     },
   { "timing_label_mpw_low",                SC_SDEF_TYP_STRING     },
   { "timing_label_mpw_high",               SC_SDEF_TYP_STRING     },
   { "vhdl_name",                           SC_SDEF_TYP_STRING     },
   { "wire_capitance",                      SC_SDEF_TYP_FLOAT      },
   { "wired_connection_class",              SC_SDEF_TYP_STRING     },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_pin[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_power_lut_template[] =     
{
   { "variable_1",                        SC_SDEF_TYP_IDENTIFIER },
   { "variable_2",                        SC_SDEF_TYP_IDENTIFIER },
   { "variable_3",                        SC_SDEF_TYP_IDENTIFIER },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_power_lut_template[] =     
{
   { "index_1", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { "index_3", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_rise_constraint[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_rise_constraint[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_rise_propagation[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_rise_propagation[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_rise_power[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_rise_power[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_rise_transition[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_rise_transition[] =
{
   { "index_1", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "index_2", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { "values", { SC_CDEF_TYP_MSTRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_routing_track[] =
{
   { "tracks",            SC_SDEF_TYP_INTEGER  },
   { "total_track_area",  SC_SDEF_TYP_FLOAT    },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_routing_track[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_scaled_cell[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_scaled_cell[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_scaling_factors[] =
{
   { "aux_no_pulldown_pin_property",   SC_SDEF_TYP_STRING  },
   { "bus_naming_style",               SC_SDEF_TYP_STRING  },
   { "comment",                        SC_SDEF_TYP_STRING  },
   { "current_unit",                   SC_SDEF_TYP_STRING  },
   { "date",                           SC_SDEF_TYP_STRING  },
   { "delay_model",                    SC_SDEF_TYP_STRING },
   { "in_place_swap_mode",             SC_SDEF_TYP_IDENTIFIER },
   { "max_wired_emitters",             SC_SDEF_TYP_INTEGER },
   { "multiple_driver_legal",          SC_SDEF_TYP_BOOLEAN },
   { "nom_process",                    SC_SDEF_TYP_FLOAT   },
   { "nom_temperature",                SC_SDEF_TYP_FLOAT   },
   { "nom_voltage",                    SC_SDEF_TYP_FLOAT   },
   { "nonpaired_twin_inc_delay_func",  SC_SDEF_TYP_STRING  },
   { "no_pulldown_pin_property",       SC_SDEF_TYP_STRING  },
   { "power_unit",                     SC_SDEF_TYP_FLOAT   },
   { "pulling_resistance_unit",        SC_SDEF_TYP_STRING  },
   { "revision",                       SC_SDEF_TYP_FLOAT   },  /* also string */
   { "simulation",                     SC_SDEF_TYP_BOOLEAN },
   { "time_unit",                      SC_SDEF_TYP_STRING  },
   { "unconnected_pin_property",       SC_SDEF_TYP_STRING  },
   { "voltage_unit",                   SC_SDEF_TYP_STRING  },
   { "wired_logic_function",           SC_SDEF_TYP_IDENTIFIER},
   { "default_cell_power",             SC_SDEF_TYP_FLOAT  },
   { "default_cell_leakage_power",     SC_SDEF_TYP_FLOAT  },
   { "default_connection_class",       SC_SDEF_TYP_STRING },
   { "default_conection_class",        SC_SDEF_TYP_STRING },    /* wrong...!!!??? */
   { "default_emitter_count",          SC_SDEF_TYP_INTEGER},
   { "default_fall_delay_intercept",   SC_SDEF_TYP_FLOAT  },
   { "default_fall_nonpaired_twin",    SC_SDEF_TYP_FLOAT  },
   { "default_fall_pin_resistance",    SC_SDEF_TYP_FLOAT  },
   { "default_fall_wire_resistance",   SC_SDEF_TYP_FLOAT  },
   { "default_fall_wor_emitter",       SC_SDEF_TYP_FLOAT  },
   { "default_fall_wor_intercept",     SC_SDEF_TYP_FLOAT  },
   { "default_fanout_load",            SC_SDEF_TYP_FLOAT  },
   { "default_inout_pin_cap",          SC_SDEF_TYP_FLOAT  },
   { "default_inout_pin_fall_res",     SC_SDEF_TYP_FLOAT  },
   { "default_inout_pin_rise_res",     SC_SDEF_TYP_FLOAT  },
   { "default_input_pin_cap",          SC_SDEF_TYP_FLOAT  },
   { "default_intrinsic_fall",         SC_SDEF_TYP_FLOAT  },
   { "default_intrinsic_rise",         SC_SDEF_TYP_FLOAT  },
   { "default_max_fanout",             SC_SDEF_TYP_FLOAT  },
   { "default_max_transition",         SC_SDEF_TYP_FLOAT  },
   { "default_operating_conditions",   SC_SDEF_TYP_STRING },
   { "default_output_pin_cap",         SC_SDEF_TYP_FLOAT  },
   { "default_output_pin_fall_res",    SC_SDEF_TYP_FLOAT  },
   { "default_output_pin_rise_res",    SC_SDEF_TYP_FLOAT  },
   { "default_pin_limit",              SC_SDEF_TYP_INTEGER},
   { "default_pin_power",              SC_SDEF_TYP_INTEGER},
   { "default_rise_delay_intercept",   SC_SDEF_TYP_FLOAT  },
   { "default_rise_nonpaired_twin",    SC_SDEF_TYP_FLOAT  },
   { "default_rise_pin_resistance",    SC_SDEF_TYP_FLOAT  },
   { "default_rise_wire_resistance",   SC_SDEF_TYP_FLOAT  },
   { "default_rise_wor_emitter",       SC_SDEF_TYP_FLOAT  },
   { "default_rise_wor_intercept",     SC_SDEF_TYP_FLOAT  },
   { "default_slope_fall",             SC_SDEF_TYP_FLOAT  },
   { "default_slope_rise",             SC_SDEF_TYP_FLOAT  },
   { "default_wire_load",              SC_SDEF_TYP_STRING },
   { "default_wire_load_mode",         SC_SDEF_TYP_STRING },
   { "default_wire_load_capacitance",  SC_SDEF_TYP_FLOAT  },
   { "default_wire_load_resistance",   SC_SDEF_TYP_FLOAT  },
   { "default_wire_load_area",         SC_SDEF_TYP_FLOAT  },
   { "k_process_cell_leakage_power",   SC_SDEF_TYP_FLOAT  },
   { "k_process_cell_power",           SC_SDEF_TYP_FLOAT  },
   { "k_process_cell_rise",            SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_cell_fall",            SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_fall_delay_intercept", SC_SDEF_TYP_FLOAT  },
   { "k_process_fall_pin_resistance",  SC_SDEF_TYP_FLOAT  },
   { "k_process_fall_propagation",     SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_fall_transition",      SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_fall_wire_resistance", SC_SDEF_TYP_FLOAT  },
   { "k_process_fall_wor_emitter",     SC_SDEF_TYP_FLOAT  },
   { "k_process_fall_wor_intercept",   SC_SDEF_TYP_FLOAT  },
   { "k_process_hold_fall",            SC_SDEF_TYP_FLOAT  },
   { "k_process_hold_rise",            SC_SDEF_TYP_FLOAT  },
   { "k_process_drive_fall",           SC_SDEF_TYP_FLOAT  },
   { "k_process_drive_rise",           SC_SDEF_TYP_FLOAT  },
   { "k_process_internal_power",       SC_SDEF_TYP_FLOAT  },
   { "k_process_intrinsic_fall",       SC_SDEF_TYP_FLOAT  },
   { "k_process_intrinsic_rise",       SC_SDEF_TYP_FLOAT  },
   { "k_process_slope_fall",           SC_SDEF_TYP_FLOAT  },
   { "k_process_slope_rise",           SC_SDEF_TYP_FLOAT  },
   { "k_process_min_period",           SC_SDEF_TYP_FLOAT  },
   { "k_process_min_pulse_width_high", SC_SDEF_TYP_FLOAT  },
   { "k_process_min_pulse_width_low",  SC_SDEF_TYP_FLOAT  },
   { "k_process_pin_cap",              SC_SDEF_TYP_FLOAT  },
   { "k_process_pin_power",            SC_SDEF_TYP_FLOAT  },
   { "k_process_recovery_fall",        SC_SDEF_TYP_FLOAT  },
   { "k_process_recovery_rise",        SC_SDEF_TYP_FLOAT  },
   { "k_process_rise_delay_intercept", SC_SDEF_TYP_FLOAT  },
   { "k_process_rise_pin_resistance",  SC_SDEF_TYP_FLOAT  },
   { "k_process_rise_propagation",     SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_rise_transition",      SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_process_rise_wire_resistance", SC_SDEF_TYP_FLOAT  },
   { "k_process_rise_wor_emitter",     SC_SDEF_TYP_FLOAT  },
   { "k_process_rise_wor_intercept",   SC_SDEF_TYP_FLOAT  },
   { "k_process_setup_fall",           SC_SDEF_TYP_FLOAT  },
   { "k_process_setup_rise",           SC_SDEF_TYP_FLOAT  },
   { "k_process_wire_cap",             SC_SDEF_TYP_FLOAT  },
   { "k_process_wire_res",             SC_SDEF_TYP_FLOAT  },
   { "k_temp_cell_leakage_power",      SC_SDEF_TYP_FLOAT  },
   { "k_temp_cell_power",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_cell_rise",               SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_cell_fall",               SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_fall_delay_intercept",    SC_SDEF_TYP_FLOAT  },
   { "k_temp_fall_pin_resistance",     SC_SDEF_TYP_FLOAT  },
   { "k_temp_fall_propagation",        SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_fall_transition",         SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_fall_wire_resistance",    SC_SDEF_TYP_FLOAT  },
   { "k_temp_fall_wor_emitter",        SC_SDEF_TYP_FLOAT  },
   { "k_temp_fall_wor_intercept",      SC_SDEF_TYP_FLOAT  },
   { "k_temp_hold_fall",               SC_SDEF_TYP_FLOAT  },
   { "k_temp_hold_rise",               SC_SDEF_TYP_FLOAT  },
   { "k_temp_drive_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_drive_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_internal_power",          SC_SDEF_TYP_FLOAT  },
   { "k_temp_intrinsic_fall",          SC_SDEF_TYP_FLOAT  },
   { "k_temp_intrinsic_rise",          SC_SDEF_TYP_FLOAT  },
   { "k_temp_slope_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_slope_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_min_period",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_min_pulse_width_high",    SC_SDEF_TYP_FLOAT  },
   { "k_temp_min_pulse_width_low",     SC_SDEF_TYP_FLOAT  },
   { "k_temp_pin_cap",                 SC_SDEF_TYP_FLOAT  },
   { "k_temp_pin_power",               SC_SDEF_TYP_FLOAT  },
   { "k_temp_recovery_fall",           SC_SDEF_TYP_FLOAT  },
   { "k_temp_recovery_rise",           SC_SDEF_TYP_FLOAT  },
   { "k_temp_rise_delay_intercept",    SC_SDEF_TYP_FLOAT  },
   { "k_temp_rise_pin_resistance",     SC_SDEF_TYP_FLOAT  },
   { "k_temp_rise_propagation",        SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_rise_transition",         SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_temp_rise_wire_resistance",    SC_SDEF_TYP_FLOAT  },
   { "k_temp_rise_wor_emitter",        SC_SDEF_TYP_FLOAT  },
   { "k_temp_rise_wor_intercept",      SC_SDEF_TYP_FLOAT  },
   { "k_temp_setup_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_setup_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_temp_wire_cap",                SC_SDEF_TYP_FLOAT  },
   { "k_temp_wire_res",                SC_SDEF_TYP_FLOAT  },
   { "k_volt_cell_leakage_power",      SC_SDEF_TYP_FLOAT  },
   { "k_volt_cell_power",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_cell_rise",               SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_cell_fall",               SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_fall_delay_intercept",    SC_SDEF_TYP_FLOAT  },
   { "k_volt_fall_pin_resistance",     SC_SDEF_TYP_FLOAT  },
   { "k_volt_fall_propagation",        SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_fall_transition",         SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_fall_wire_resistance",    SC_SDEF_TYP_FLOAT  },
   { "k_volt_fall_wor_emitter",        SC_SDEF_TYP_FLOAT  },
   { "k_volt_fall_wor_intercept",      SC_SDEF_TYP_FLOAT  },
   { "k_volt_hold_fall",               SC_SDEF_TYP_FLOAT  },
   { "k_volt_hold_rise",               SC_SDEF_TYP_FLOAT  },
   { "k_volt_drive_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_drive_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_internal_power",          SC_SDEF_TYP_FLOAT  },
   { "k_volt_intrinsic_fall",          SC_SDEF_TYP_FLOAT  },
   { "k_volt_intrinsic_rise",          SC_SDEF_TYP_FLOAT  },
   { "k_volt_slope_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_slope_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_min_period",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_min_pulse_width_high",    SC_SDEF_TYP_FLOAT  },
   { "k_volt_min_pulse_width_low",     SC_SDEF_TYP_FLOAT  },
   { "k_volt_pin_cap",                 SC_SDEF_TYP_FLOAT  },
   { "k_volt_pin_power",               SC_SDEF_TYP_FLOAT  },
   { "k_volt_recovery_fall",           SC_SDEF_TYP_FLOAT  },
   { "k_volt_recovery_rise",           SC_SDEF_TYP_FLOAT  },
   { "k_volt_rise_delay_intercept",    SC_SDEF_TYP_FLOAT  },
   { "k_volt_rise_pin_resistance",     SC_SDEF_TYP_FLOAT  },
   { "k_volt_rise_propagation",        SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_rise_transition",         SC_SDEF_TYP_FLOAT  }, /* new */
   { "k_volt_rise_wire_resistance",    SC_SDEF_TYP_FLOAT  },
   { "k_volt_rise_wor_emitter",        SC_SDEF_TYP_FLOAT  },
   { "k_volt_rise_wor_intercept",      SC_SDEF_TYP_FLOAT  },
   { "k_volt_setup_fall",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_setup_rise",              SC_SDEF_TYP_FLOAT  },
   { "k_volt_wire_cap",                SC_SDEF_TYP_FLOAT  },
   { "k_volt_wire_res",                SC_SDEF_TYP_FLOAT  },
   { "leakage_power_unit",             SC_SDEF_TYP_STRING },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_scaling_factors[] =
{
   { NULL, { 0 } }
};


sc_sdef_init_struct sc_sdef_state[] =
{
   { "clocked_on",                     SC_SDEF_TYP_STRING },
   { "clocked_on_also",                SC_SDEF_TYP_STRING },
   { "force_00",                       SC_SDEF_TYP_STRING },
   { "force_01",                       SC_SDEF_TYP_STRING },
   { "force_10",                       SC_SDEF_TYP_STRING },
   { "force_11",                       SC_SDEF_TYP_STRING },
   { "next_state",                     SC_SDEF_TYP_STRING },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_state[] =
{
   { NULL, { 0 } }
};


sc_sdef_init_struct sc_sdef_statetable[] =
{
   { "table",                          SC_SDEF_TYP_STRING },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_statetable[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_symbol[] =
{
   { "lanscape",                      SC_SDEF_TYP_BOOLEAN },
   { "ripped_bits_property",          SC_SDEF_TYP_STRING  },
   { "ripped_pin",                    SC_SDEF_TYP_STRING  },
   { "template",                      SC_SDEF_TYP_STRING  },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_symbol[] =
{
   { "arc",    { SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "circle", { SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "line",   { SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "pin",    { SC_CDEF_TYP_STRING, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { "set_infinite_template_location" , { SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "set_minimum_boundary" , { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_END } },
   { "set_usable_area", { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_END } },
   { "sub_symbol", { SC_CDEF_TYP_STRING, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_END } },
   { "text" , { SC_CDEF_TYP_STRING, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { "variable" , { SC_CDEF_TYP_STRING, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_test_cell[] =
{
   { "___dummy", SC_SDEF_TYP_INTEGER },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_test_cell[] =
{
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_timing[] =
{
   { "fall_resistance",              SC_SDEF_TYP_FLOAT      },
   { "rise_resistance",              SC_SDEF_TYP_FLOAT      },
   { "intrinsic_fall",               SC_SDEF_TYP_FLOAT      },
   { "intrinsic_rise",               SC_SDEF_TYP_FLOAT      },
   { "related_pin",                  SC_SDEF_TYP_STRING     },
   { "slope_fall",                   SC_SDEF_TYP_FLOAT      },
   { "slope_rise",                   SC_SDEF_TYP_FLOAT      },
   { "timing_type",                  SC_SDEF_TYP_STRING     },
   { "timing_sense",                 SC_SDEF_TYP_STRING     },
   { "timing_label",                 SC_SDEF_TYP_STRING     },
   { "when",                         SC_SDEF_TYP_STRING     },
   { "when_start",                   SC_SDEF_TYP_STRING     },
   { "when_end",                     SC_SDEF_TYP_STRING     },
   { "sdf_cond",                     SC_SDEF_TYP_STRING     },
   { "sdf_cond_start",               SC_SDEF_TYP_STRING     },
   { "sdf_cond_end",                 SC_SDEF_TYP_STRING     },
   { "sdf_edges",                    SC_SDEF_TYP_STRING     },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_timing[] =
{
   { "fall_delay_intercept", { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "fall_nonpaired_twin" , { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "fall_pin_resistance" , { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "fall_wire_resistance", { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "rise_delay_intercept", { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "rise_nonpaired_twin" , { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "rise_pin_resistance" , { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "rise_wire_resistance", { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_timing_range[] =
{
   { "faster_factor",                SC_SDEF_TYP_FLOAT      },
   { "slower_factor",                SC_SDEF_TYP_FLOAT      },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_timing_range[] =
{
  { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_synop_type[] =
{
   { "base_type",                    SC_SDEF_TYP_STRING     },
   { "bit_from",                     SC_SDEF_TYP_FLOAT      },
   { "bit_to",                       SC_SDEF_TYP_FLOAT      },
   { "bit_width",                    SC_SDEF_TYP_FLOAT      },
   { "data_type",                    SC_SDEF_TYP_STRING     },
   { "downto",                       SC_SDEF_TYP_BOOLEAN    },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_synop_type[] =
{
  { NULL, { 0 } }
};


/* ... */

sc_sdef_init_struct sc_sdef_wire_load[] =
{
   { "area",                         SC_SDEF_TYP_FLOAT      },
   { "capacitance",                  SC_SDEF_TYP_FLOAT      },
   { "resistance",                   SC_SDEF_TYP_FLOAT      },
   { "slope",                        SC_SDEF_TYP_FLOAT      },
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_wire_load[] =
{
   { "fanout_length", { SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_wire_load_selection[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_wire_load_selection[] =
{
   { "wire_load_from_area", { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_STRING, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};

sc_sdef_init_struct sc_sdef_wire_load_table[] =
{
   { NULL, 0 }
};

sc_cdef_init_struct sc_cdef_wire_load_table[] =
{
   { "fanout_length",       { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "fanout_capacitance",  { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "fanout_resistance",   { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { "fanout_area",         { SC_CDEF_TYP_INTEGER, SC_CDEF_TYP_FLOAT, SC_CDEF_TYP_END } },
   { NULL, { 0 } }
};



/*-----------------------------------------------------------------------------*/

sc_sdl_init_struct sc_sdl_init_data[] =
{
   {  sc_str_bus                  , sc_sdef_bus                 , sc_cdef_bus                  },
   {  sc_str_bundle               , sc_sdef_bundle              , sc_cdef_bundle               },
   {  sc_str_cell                 , sc_sdef_cell                , sc_cdef_cell                 },
   {  sc_str_cell_enum            , sc_sdef_cell_enum           , sc_cdef_cell_enum            },
   {  sc_str_cell_fall            , sc_sdef_cell_fall           , sc_cdef_cell_fall            },
   {  sc_str_cell_rise            , sc_sdef_cell_rise           , sc_cdef_cell_rise            },
   {  sc_str_fall_constraint      , sc_sdef_fall_constraint     , sc_cdef_fall_constraint      },
   {  sc_str_fall_power           , sc_sdef_fall_power          , sc_cdef_fall_power           },
   {  sc_str_fall_propagation     , sc_sdef_fall_propagation    , sc_cdef_fall_propagation     },
   {  sc_str_fall_transition      , sc_sdef_fall_transition     , sc_cdef_fall_transition      },
   {  sc_str_ff                   , sc_sdef_ff                  , sc_cdef_ff                   },
   {  sc_str_ff_bank              , sc_sdef_ff_bank             , sc_cdef_ff_bank              },
   {  sc_str_input_voltage        , sc_sdef_input_voltage       , sc_cdef_input_voltage        },
   {  sc_str_internal_power       , sc_sdef_internal_power      , sc_cdef_internal_power       },
   {  sc_str_latch                , sc_sdef_latch               , sc_cdef_latch                },
   {  sc_str_latch_bank           , sc_sdef_latch_bank          , sc_cdef_latch_bank           },
   {  sc_str_layer                , sc_sdef_layer               , sc_cdef_layer                },
   {  sc_str_library              , sc_sdef_library             , sc_cdef_library              },
   {  sc_str_library_symbol       , sc_sdef_library_symbol      , sc_cdef_library_symbol       },
   {  sc_str_lu_table_template    , sc_sdef_lu_table_template   , sc_cdef_lu_table_template    },
   {  sc_str_memory               , sc_sdef_memory              , sc_cdef_memory               },
   {  sc_str_operating_conditions , sc_sdef_operating_conditions, sc_cdef_operating_conditions },
   {  sc_str_output_voltage       , sc_sdef_output_voltage      , sc_cdef_output_voltage       },
   {  sc_str_parameterized_cell   , sc_sdef_parameterized_cell  , sc_cdef_parameterized_cell   },
   {  sc_str_parameterized_pin    , sc_sdef_parameterized_pin   , sc_cdef_parameterized_pin    },
   {  sc_str_pin                  , sc_sdef_pin                 , sc_cdef_pin                  },
   {  sc_str_power_lut_template   , sc_sdef_power_lut_template  , sc_cdef_power_lut_template   },
   {  sc_str_rise_constraint      , sc_sdef_rise_constraint     , sc_cdef_rise_constraint      },
   {  sc_str_rise_power           , sc_sdef_rise_power          , sc_cdef_rise_power           },
   {  sc_str_rise_propagation     , sc_sdef_rise_propagation    , sc_cdef_rise_propagation     },
   {  sc_str_rise_transition      , sc_sdef_rise_transition     , sc_cdef_rise_transition      },
   {  sc_str_routing_track        , sc_sdef_routing_track       , sc_cdef_routing_track        },
   {  sc_str_scaled_cell          , sc_sdef_scaled_cell         , sc_cdef_scaled_cell          },
   {  sc_str_scaling_factors      , sc_sdef_scaling_factors     , sc_cdef_scaling_factors      },
   {  sc_str_state                , sc_sdef_state               , sc_cdef_state                },
   {  sc_str_statetable           , sc_sdef_statetable          , sc_cdef_statetable           },
   {  sc_str_symbol               , sc_sdef_symbol              , sc_cdef_symbol               },
   {  sc_str_test_cell            , sc_sdef_test_cell           , sc_cdef_test_cell            },
   {  sc_str_timing               , sc_sdef_timing              , sc_cdef_timing               },
   {  sc_str_timing_range         , sc_sdef_timing_range        , sc_cdef_timing_range         },
   {  sc_str_type                 , sc_sdef_synop_type          , sc_cdef_synop_type           },
   {  sc_str_wire_load            , sc_sdef_wire_load           , sc_cdef_wire_load            },
   {  sc_str_wire_load_selection  , sc_sdef_wire_load_selection , sc_cdef_wire_load_selection  },
   {  sc_str_wire_load_table      , sc_sdef_wire_load_table     , sc_cdef_wire_load_table      },
   /* ... */
   { NULL, NULL }
};
