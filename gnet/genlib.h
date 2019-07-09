/*

  genlib.h
 
  parse sis genlib format
  
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

#ifndef _GENLIB_H
#define _GENLIB_H

#include <stdio.h>

#define GL_IDENTIFIER_MAX 256
#define GL_FUNCTION_MAX 2048

#define GL_BLOCK_STATE_NONE 0
#define GL_BLOCK_STATE_GATE 1
#define GL_BLOCK_STATE_LATCH 2

typedef struct _gl_struct *gl_type;

struct _gl_struct
{
  FILE *fp;
  int curr_char;
  char identifier[GL_IDENTIFIER_MAX];
  
  char gate_name[GL_IDENTIFIER_MAX];    /* valid after GATE_START */
  double gate_area;                     /* valid after GATE_START */
  char gate_fn[GL_FUNCTION_MAX];        /* valid after GATE_START */
  
  char pin_name[GL_IDENTIFIER_MAX];     /* valid after GATE_START, GATE_PIN */
  char pin_phase[GL_IDENTIFIER_MAX];    /* valid after GATE_START, GATE_PIN */
  double pin_input_load;                /* valid after GATE_START, GATE_PIN */
  double pin_max_load;                  /* valid after GATE_START, GATE_PIN */
  double pin_rise_block_delay;          /* valid after GATE_START, GATE_PIN */
  double pin_rise_fanout_delay;         /* valid after GATE_START, GATE_PIN */
  double pin_fall_block_delay;          /* valid after GATE_START, GATE_PIN */
  double pin_fall_fanout_delay;         /* valid after GATE_START, GATE_PIN */

  char latch_name[GL_IDENTIFIER_MAX];   /* valid after LATCH_START */
  double latch_area;                    /* valid after LATCH_START */
  char latch_fn[GL_FUNCTION_MAX];       /* valid after LATCH_START */
  
  char latch_input[GL_IDENTIFIER_MAX];
  char latch_output[GL_IDENTIFIER_MAX];
  char latch_type[GL_IDENTIFIER_MAX];

  char latch_ctl_name[GL_IDENTIFIER_MAX];
  double latch_ctl_input_load;
  double latch_ctl_max_load;
  double latch_ctl_rise_block_delay;
  double latch_ctl_rise_fanout_delay;
  double latch_ctl_fall_block_delay;
  double latch_ctl_fall_fanout_delay;

  char latch_constraint_name[GL_IDENTIFIER_MAX];
  double latch_constaint_setup_time;
  double latch_constaint_hold_time;
  
  int block_state;
  
  int (*cb)(gl_type gl, int msg, void *user_data);
  void *user_data;
};

#define GL_MSG_CB_START          500
#define GL_MSG_CB_END            501

#define GL_MSG_GATE_START       1000
#define GL_MSG_GATE_END         1001
#define GL_MSG_GATE_PIN         1002

#define GL_MSG_LATCH_START      2000
#define GL_MSG_LATCH_END        2001
#define GL_MSG_LATCH_SEQ        2002
#define GL_MSG_LATCH_PIN        2003
#define GL_MSG_LATCH_CONTROL    2004
#define GL_MSG_LATCH_CONSTRAINT 2005


gl_type gl_Open();
void gl_Close(gl_type gl);
int gl_SetCB(gl_type gl, int (*cb)(gl_type gl, int msg, void *user_data), void *user_data);
int gl_SetFile(gl_type gl, const char *filename);
int gl_ReadAll(gl_type gl);


#endif /* _GENLIB_H */
