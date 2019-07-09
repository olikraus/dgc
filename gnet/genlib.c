/*

  genlib.c
  
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


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "genlib.h"
#include "b_ff.h"
#include "mwc.h"

int gl_dummy_cb(gl_type gl, int msg, void *user_data)
{
  return 1;
}

gl_type gl_Open()
{
  gl_type gl;
  gl = (gl_type)malloc(sizeof(struct _gl_struct));
  if ( gl != NULL )
  {
    memset(gl, 0, sizeof(struct _gl_struct));
    gl->latch_name[0] = '\0';
    gl->cb = gl_dummy_cb;
    gl->fp = NULL;
    return gl;
  }
  return NULL;
}

int gl_Call(gl_type gl, int msg)
{
  if ( gl->cb(gl, msg, gl->user_data) == 0 )
    return 0;
  return 1;
}

void gl_Close(gl_type gl)
{
  gl_Call(gl, GL_MSG_CB_END);
  if ( gl->fp != NULL )
    fclose(gl->fp);
  free(gl);
}

void gl_Error(gl_type gl, char *msg, ...)
{
  va_list va;
  va_start(va, msg);
  vprintf(msg, va);
  puts("");
  va_end(va);
}

int gl_SetFile(gl_type gl, const char *filename)
{
  if ( gl->fp != NULL )
    fclose(gl->fp);
  gl->block_state = GL_BLOCK_STATE_NONE;
  gl->fp = b_fopen(filename, NULL, ".genlib", "r");
  if ( gl->fp == NULL )
    return gl_Error(gl, "Can not open file '%s'.", filename), 0;
  gl->curr_char = getc(gl->fp);
  return 1;
}

int gl_SetCB(gl_type gl, int (*cb)(gl_type gl, int msg, void *user_data), void *user_data)
{
  gl_Call(gl, GL_MSG_CB_END);
  gl->user_data = user_data;
  gl->cb = cb;
  if ( gl_Call(gl, GL_MSG_CB_START) == 0 )
    return 0;
  return 1;
}

int gl_NextChar(gl_type gl)
{
  if ( gl->curr_char == EOF )
    return 0;
  gl->curr_char = getc(gl->fp);
  if ( gl->curr_char == EOF )
    return 0;
  return 1;
}

void gl_SkipSpace(gl_type gl)
{
  for(;;)
  {
    while ( gl->curr_char == '#' )
    {
      for(;;)
      {
        if ( gl->curr_char == EOF )
          break;
        if ( gl_NextChar(gl) == 0 )
          break;
        if ( gl->curr_char == '\n' )
        {
          gl_NextChar(gl);
          break;
        }
      }
    }
    if ( gl->curr_char == EOF )
      break;
    if ( gl->curr_char > ' ' )
      break;
    if ( gl_NextChar(gl) == 0 )
      break;
  }
}

int gl_ReadDecimal(gl_type gl, double *val, int *cnt, int is_check_sign)
{ 
  double v = 0;
  int c = 0;
  double sign = 1;
  
  if ( is_check_sign != 0 )
  {
    if ( gl->curr_char == '-' )
    {
      sign = -1;
      if ( gl_NextChar(gl) == 0 )
        return 0;
    }
    if ( gl->curr_char == '+' )
    {
      sign = +1;
      if ( gl_NextChar(gl) == 0 )
        return 0;
    }
  }
  if ( gl->curr_char < '0' || gl->curr_char > '9' )
      return 0;
  while( gl->curr_char >= '0' && gl->curr_char <= '9' )
  {
    v = v*10.0 + (double)(gl->curr_char-'0');
    c++;
    if ( gl_NextChar(gl) == 0 )
      break;
  }
  v = v*sign;
  if ( val != NULL )
    *val = v;
  if ( cnt != NULL )
    *cnt = c;
  return 1;
}

int gl_ReadFloat(gl_type gl, double *val)
{
  int c = 0;
  double pre = 0.0, post = 0.0;
  if ( gl->curr_char != '.' )
    if ( gl_ReadDecimal(gl, &pre, NULL, 1) == 0 )
      return 0;
  if ( gl->curr_char == '.' )
  {
    if ( gl_NextChar(gl) == 0 )
      return 0;
    if ( gl_ReadDecimal(gl, &post, &c, 0) == 0 )
      return 0;
    pre = pre + post * pow(10.0, (double)-c);
  }
  if ( gl->curr_char == 'e' || gl->curr_char == 'E' )
  {
    if ( gl_NextChar(gl) == 0 )
      return 0;
    if ( gl_ReadDecimal(gl, &post, NULL, 1) == 0 )
      return 0;
    pre = pre * pow(10.0, post);
  }
  
  if ( val != NULL )
    *val = pre;
    
  gl_SkipSpace(gl);
  return 1;
}

int gl_is_symf(gl_type gl)
{
  if ( gl->curr_char >= 'a' && gl->curr_char <= 'z' )
    return 1;
  if ( gl->curr_char >= 'A' && gl->curr_char <= 'Z' )
    return 1;
  if ( gl->curr_char == '$' )
    return 1;
  if ( gl->curr_char == '~' )
    return 1;
  if ( gl->curr_char == '_' )
    return 1;
  if ( gl->curr_char == '*' ) /* some file use '*' for 'ALL'? */
    return 1;
  return 0;
}

int gl_is_sym(gl_type gl)
{
  if ( gl_is_symf(gl) != 0 )
    return 1;
  if ( gl->curr_char >= '0' && gl->curr_char <= '9' )
    return 1;
  return 0;
}

int gl_ReadIdentifier(gl_type gl)
{
  int i = 0;
  gl->identifier[i] = '\0';
  if ( gl_is_symf(gl) != 0 )
  {
    do
    {
      if ( i < GL_IDENTIFIER_MAX-2 )
        gl->identifier[i++] = gl->curr_char;
      if ( gl_NextChar(gl) == 0 )
        break;
    } while( gl_is_sym(gl) != 0 );
    gl->identifier[i++] = '\0';
    gl_SkipSpace(gl);
    return 1;
  }
  gl_SkipSpace(gl);
  return 0;
}

int gl_ReadString(gl_type gl)
{
  int i = 0;
  if ( gl->curr_char != '\"' )
    return gl_ReadIdentifier(gl);
  if ( gl_NextChar(gl) == 0 )
    return 0;
  
  gl->identifier[i] = '\0';
  do
  {
    if ( i < GL_IDENTIFIER_MAX-2 )
      gl->identifier[i++] = gl->curr_char;
    if ( gl_NextChar(gl) == 0 )
      return 0;
  } while( gl->curr_char != '\"' );
  gl->identifier[i++] = '\0';
  gl_NextChar(gl);
  gl_SkipSpace(gl);
  return 1;
}


int gl_ReadGate(gl_type gl)
{
  int i = 0;
  
  /* gate name */

  if ( gl_ReadString(gl) == 0 )  
    return gl_Error(gl, "Gate: Missing name."), 0;
  strcpy(gl->gate_name, gl->identifier);
  
  /* gate area */
  
  if ( gl_ReadFloat(gl, &(gl->gate_area)) == 0 )
    return gl_Error(gl, "Gate '%s': Missing area.", gl->gate_name), 0;

  /*
  printf("GATE %s %lg\n", gl->gate_name, gl->gate_area);
  */

  /* gate function */
    
  gl->gate_fn[i] = '\0';
  for(;;)
  {
    if ( gl->curr_char == EOF )
      return gl_Error(gl, "Gate '%s': Missinig ';' for function termination.", gl->gate_name), 0;
    if ( gl->curr_char == ';' )
      break;
    if ( i < GL_FUNCTION_MAX-2 )
    {
      gl->gate_fn[i++] = gl->curr_char;
      gl->gate_fn[i] = '\0';
    }
    gl_NextChar(gl);
  } 
  gl_NextChar(gl);
  gl_SkipSpace(gl);
  return 1;
}

int gl_ReadPin(gl_type gl)
{
  /* pin name */

  if ( gl_ReadIdentifier(gl) == 0 )  
    return gl_Error(gl, "Pin: Missing name."), 0;
  strcpy(gl->pin_name, gl->identifier);

  gl->pin_input_load = 0.0;
  gl->pin_max_load = 0.0;
  gl->pin_rise_block_delay = 0.0;
  gl->pin_rise_fanout_delay = 0.0;
  gl->pin_fall_block_delay = 0.0;
  gl->pin_fall_fanout_delay = 0.0;

  gl->pin_phase[0] = '\0';
  if ( gl_ReadIdentifier(gl) == 0 )  
    return gl_Error(gl, "Pin '%s': Missing phase.", gl->pin_name), 0;
  strcpy(gl->pin_phase, gl->identifier);
  
  if ( gl_ReadFloat(gl, &(gl->pin_input_load)) == 0 )
    return gl_Error(gl, "Pin '%s': Missing input load.", gl->pin_name), 1;

  if ( gl_ReadFloat(gl, &(gl->pin_max_load)) == 0 )
    return gl_Error(gl, "Pin '%s': Missing max load.", gl->pin_name), 1;

  if ( gl_ReadFloat(gl, &(gl->pin_rise_block_delay)) == 0 )
    return gl_Error(gl, "Pin '%s': Missing rise block delay.", gl->pin_name), 1;

  if ( gl_ReadFloat(gl, &(gl->pin_rise_fanout_delay)) == 0 )
    return gl_Error(gl, "Pin '%s': Missing rise fanout delay.", gl->pin_name), 1;

  if ( gl_ReadFloat(gl, &(gl->pin_fall_block_delay)) == 0 )
    return gl_Error(gl, "Pin '%s': Missing fall block delay.", gl->pin_name), 1;

  if ( gl_ReadFloat(gl, &(gl->pin_fall_fanout_delay)) == 0 )
    return gl_Error(gl, "Pin '%s': Missing fall fanout delay.", gl->pin_name), 1;

  return 1;
}

int gl_ReadLatch(gl_type gl)
{
  int i = 0;
  
  /* latch name */

  if ( gl_ReadString(gl) == 0 )  
    return gl_Error(gl, "Latch: Missing name."), 0;
  strcpy(gl->latch_name, gl->identifier);
  
  /* latch area */
  
  if ( gl_ReadFloat(gl, &(gl->latch_area)) == 0 )
    return gl_Error(gl, "Latch '%s': Missing area.", gl->latch_name), 0;

  /*
  printf("LATCH %s %lg\n", gl->latch_name, gl->latch_area);
  */

  /* latch function */
    
  gl->latch_fn[i] = '\0';
  for(;;)
  {
    if ( gl->curr_char == EOF )
      return gl_Error(gl, "Latch '%s': Missinig ';' for function termination.", gl->latch_name), 0;
    if ( gl->curr_char == ';' )
      break;
    if ( i < GL_FUNCTION_MAX-2 )
    {
      gl->latch_fn[i++] = gl->curr_char;
      gl->latch_fn[i] = '\0';
    }
    gl_NextChar(gl);
  } 
  gl_NextChar(gl);
  gl_SkipSpace(gl);
  return 1;
}

int gl_ReadSeq(gl_type gl)
{
  /* latch input */

  if ( gl_ReadIdentifier(gl) == 0 )  
    return gl_Error(gl, "Latch '%s': Missing input name.", gl->latch_name), 0;
  strcpy(gl->latch_input, gl->identifier);

  /* latch output */

  if ( gl_ReadIdentifier(gl) == 0 )  
    return gl_Error(gl, "Latch '%s': Missing output name.", gl->latch_name), 0;
  strcpy(gl->latch_output, gl->identifier);

  /* latch type */

  if ( gl_ReadIdentifier(gl) == 0 )  
    return gl_Error(gl, "Latch '%s': Missing type.", gl->latch_name), 0;
  strcpy(gl->latch_type, gl->identifier);

  return 1;  
}

int gl_ReadControl(gl_type gl)
{
  /* latch control pin */

  if ( gl_ReadIdentifier(gl) == 0 )  
    return gl_Error(gl, "Latch '%s': Missing control name.", gl->latch_name), 0;
  strcpy(gl->latch_ctl_name, gl->identifier);

  if ( gl_ReadFloat(gl, &(gl->latch_ctl_input_load)) == 0 )
    return gl_Error(gl, "Latch '%s': Missing input load.", gl->latch_name), 0;

  if ( gl_ReadFloat(gl, &(gl->latch_ctl_max_load)) == 0 )
    return gl_Error(gl, "Latch '%s': Missing max load.", gl->latch_name), 0;

  if ( gl_ReadFloat(gl, &(gl->latch_ctl_rise_block_delay)) == 0 )
    return gl_Error(gl, "Latch '%s': Missing rise block delay.", gl->latch_name), 0;

  if ( gl_ReadFloat(gl, &(gl->latch_ctl_rise_fanout_delay)) == 0 )
    return gl_Error(gl, "Latch '%s': Missing rise fanout delay.", gl->latch_name), 0;

  if ( gl_ReadFloat(gl, &(gl->latch_ctl_fall_block_delay)) == 0 )
    return gl_Error(gl, "Latch '%s': Missing fall block delay.", gl->latch_name), 0;

  if ( gl_ReadFloat(gl, &(gl->latch_ctl_fall_fanout_delay)) == 0 )
    return gl_Error(gl, "Latch '%s': Missing fall fanout delay.", gl->latch_name), 0;

  return 1;  
}

int gl_ReadConstraint(gl_type gl)
{
  /* latch pin */

  if ( gl_ReadIdentifier(gl) == 0 )  
    return gl_Error(gl, "Latch '%s': Missing constraint pin name.", gl->latch_name), 0;
  strcpy(gl->latch_constraint_name, gl->identifier);

  if ( gl_ReadFloat(gl, &(gl->latch_constaint_setup_time)) == 0 )
    return gl_Error(gl, "Latch '%s': Missing setup time.", gl->latch_name), 0;

  if ( gl_ReadFloat(gl, &(gl->latch_constaint_hold_time)) == 0 )
    return gl_Error(gl, "Latch '%s': Missing hold time.", gl->latch_name), 0;

  return 1;  
}


int gl_EndBlock(gl_type gl)
{
  switch(gl->block_state)
  {
    case GL_BLOCK_STATE_GATE:
      if ( gl_Call(gl, GL_MSG_GATE_END) == 0 )
        return 0;
      break;
    case GL_BLOCK_STATE_LATCH:
      if ( gl_Call(gl, GL_MSG_LATCH_END) == 0 )
        return 0;
      break;
  }
  gl->block_state = GL_BLOCK_STATE_NONE;
  return 1;
}

int gl_StartBlock(gl_type gl, int block_state)
{
  switch(block_state)
  {
    case GL_BLOCK_STATE_GATE:
      gl->block_state = block_state;
      if ( gl_Call(gl, GL_MSG_GATE_START) == 0 )
        return 0;
      break;
    case GL_BLOCK_STATE_LATCH:
      gl->block_state = block_state;
      if ( gl_Call(gl, GL_MSG_LATCH_START) == 0 )
        return 0;
      break;
    default:
      return 0;
  }
  return 1;
}

int gl_ReadKey(gl_type gl)
{
  if ( gl_ReadIdentifier(gl) == 0 )  
    return gl_Error(gl, "Expecting a valid keyword."), 0;
  if ( strcmp(gl->identifier, "gate") == 0 || strcmp(gl->identifier, "GATE") == 0 )
  {
    if ( gl_EndBlock(gl) == 0 )
      return 0;
    if ( gl_ReadGate(gl) == 0 ) 
      return 0;
    if ( gl_StartBlock(gl, GL_BLOCK_STATE_GATE) == 0 )
      return 0;
    return 1;
  }
  if ( strcmp(gl->identifier, "pin") == 0 || strcmp(gl->identifier, "PIN") == 0 )
  {
    if ( gl_ReadPin(gl) == 0 )
      return 0;
    switch( gl->block_state )
    {
      case GL_BLOCK_STATE_GATE:
        if ( gl_Call(gl, GL_MSG_GATE_PIN) == 0 )
          return 0;
        break;
      case GL_BLOCK_STATE_LATCH:
        if ( gl_Call(gl, GL_MSG_LATCH_PIN) == 0 )
          return 0;
        break;
      default:
        return 0;
    }
    return 1;
  }
  if ( strcmp(gl->identifier, "latch") == 0 || strcmp(gl->identifier, "LATCH") == 0 )
  {
    if ( gl_EndBlock(gl) == 0 )
      return 0;
    if ( gl_ReadLatch(gl) == 0 ) 
      return 0;
    if ( gl_StartBlock(gl, GL_BLOCK_STATE_LATCH) == 0 )
      return 0;
    return 1;
  }
  if ( strcmp(gl->identifier, "seq") == 0 || strcmp(gl->identifier, "SEQ") == 0 )
  {
    if ( gl_ReadSeq(gl) == 0 )
      return 0;
    if ( gl_Call(gl, GL_MSG_LATCH_SEQ) == 0 )
      return 0;
    return 1;
  }
  if ( strcmp(gl->identifier, "control") == 0 || strcmp(gl->identifier, "CONTROL") == 0 )
  {
    if ( gl_ReadControl(gl) == 0 )
      return 0;
    if ( gl_Call(gl, GL_MSG_LATCH_CONTROL) == 0 )
      return 0;
    return 1;
  }
  if ( strcmp(gl->identifier, "constraint") == 0 || strcmp(gl->identifier, "CONSTRAINT") == 0 )
  {
    if ( gl_ReadConstraint(gl) == 0 )
      return 0;
    if ( gl_Call(gl, GL_MSG_LATCH_CONSTRAINT) == 0 )
      return 0;
    return 1;
  }
  return gl_Error(gl, "Unknown keyword '%s'.", gl->identifier), 0;
}

int gl_ReadAll(gl_type gl)
{
  gl_SkipSpace(gl);
  for(;;)
  {
    if ( gl->curr_char == EOF )
      break;
    if ( gl_ReadKey(gl) == 0 )
      return 0;
  }
  if ( gl_EndBlock(gl) == 0 )
    return 0;
  return 1;
}


/*
int main()
{
  gl_type gl;
  gl = gl_Open();
  gl_SetFile(gl, "test.genlib");
  gl_ReadAll(gl);
  return 0;
}
*/

