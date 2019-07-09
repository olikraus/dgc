/*

  fsmp.c

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

  
  at the moment, this file is unused, I think...

*/

#include <stdlib.h>
#include <ctype.h>
#include "fsm.h"
#include "mwc.h"

#define FSMP_IDENTIFIER_LEN 256
#define FSMP_STRING_LEN (1024*16)

#define FSMP_TOK_EOF        0
#define FSMP_TOK_BEGIN      1
#define FSMP_TOK_END        2
#define FSMP_TOK_IDENTIFIER 3
#define FSMP_TOK_STRING     4
#define FSMP_TOK_SEMICOLON  6

#define FSMP_KEY_FSM        0
#define FSMP_KEY_STATE      1
#define FSMP_KEY_NEXT       2
#define FSMP_KEY_OUT        3
#define FSMP_KEY_IF         4

struct _fsmp_key_id_struct
{
  char *key;
  int id;
};
struct _fsmp_key_id_struct fsmp_keys[] = 
{
  {"fsm",     FSMP_KEY_FSM},
  {"state",   FSMP_KEY_STATE},
  {"next",    FSMP_KEY_NEXT},
  {"out",     FSMP_KEY_OUT},
  {"if",      FSMP_KEY_IF},
  {NULL, -1},
};


struct _fsmp_struct
{
  fsm_type fsm;
  
  /* contains the name of the current state */
  char curr_state[FSMP_IDENTIFIER_LEN];
  
  /* contains the name of the next state within the current state */
  char next_state[FSMP_IDENTIFIER_LEN];

  /* input buffer */
  char string[FSMP_STRING_LEN];
  char identifier[FSMP_IDENTIFIER_LEN];
  int token;
  
  /* input stream */
  FILE *fp;
  int current_char;
  int line_cnt;
};
typedef struct _fsmp_struct *fsmp_type;



fsmp_type fsmp_Open(fsm_type fsm)
{
  fsmp_type fsmp;
  fsmp = (fsmp_type)malloc(sizeof(struct _fsmp_struct));
  if ( fsmp != NULL )
  {
    fsmp->fsm = fsm;
    fsmp->fp = NULL;
    fsmp->current_char = EOF;
    return fsmp;
  }
  return NULL;
}

void fsmp_Close(fsmp_type fsmp)
{
  if ( fsmp->fp != NULL )
    fclose(fsmp->fp);
  free(fsmp);
}

void fsmp_Next(fsmp_type fsmp)
{
  fsmp->current_char = getc(fsmp->fp);
}

#define fsmp_Curr(fsmp) ((fsmp)->current_char)


void fsmp_SkipSpace(fsmp_type fsmp)
{
  for(;;)
  {
    if ( fsmp_Curr(fsmp) == '\n' )
    {
      fsmp->line_cnt++;
    }
    else if ( fsmp_Curr(fsmp) > ' ' || fsmp_Curr(fsmp) == EOF )
    {
      break;
    }
    fsmp_Next(fsmp);
  }
}

int fsmp_OpenInputFile(fsmp_type fsmp, char *name)
{
  fsmp->fp = fopen(name, "rb");
  if ( fsmp->fp == NULL )
    return 0;
  fsmp->line_cnt = 1;
  fsmp->current_char = getc(fsmp->fp);
  fsmp_SkipSpace(fsmp);
  return 1;
}

void fsmp_read_identifier(fsmp_type fsmp)
{
  int i = 0;
  for(;;)
  {
    if ( i < FSMP_IDENTIFIER_LEN-1 );
      fsmp->identifier[i++] = fsmp_Curr(fsmp);
    fsmp_Next(fsmp);
    if ( fsmp_Curr(fsmp) == EOF )
      break;
    if ( !((fsmp_Curr(fsmp) <= 'a' && fsmp_Curr(fsmp) <= 'z') || 
           (fsmp_Curr(fsmp) >= 'A' && fsmp_Curr(fsmp) <= 'Z') || 
           (fsmp_Curr(fsmp) >= '0' && fsmp_Curr(fsmp) <= '9') || 
           fsmp_Curr(fsmp) == '_' ) )
      break;
  }
  fsmp->identifier[i++] = '\0';
}

unsigned char fsmp_get_escaped_char(fsmp_type fsmp)
{
  unsigned char c;
  int i;
  if ( fsmp_Curr(fsmp) == '\\' )
  {
    fsmp_Next(fsmp);
    if ( fsmp_Curr(fsmp) == EOF )
    {
      return '\\';
    }
    switch(fsmp_Curr(fsmp))
    {
      case 'a': c = '\a'; fsmp_Next(fsmp); break;
      case 'b': c = '\b'; fsmp_Next(fsmp); break;
      case 'f': c = '\f'; fsmp_Next(fsmp); break;
      case 'n': c = '\n'; fsmp_Next(fsmp); break;
      case 'r': c = '\r'; fsmp_Next(fsmp); break;
      case 't': c = '\t'; fsmp_Next(fsmp); break;
      case 'v': c = '\v'; fsmp_Next(fsmp); break;
      case '\?': c = '\?'; fsmp_Next(fsmp); break;
      case '\'': c = '\''; fsmp_Next(fsmp); break;
      case '\"': c = '\"'; fsmp_Next(fsmp); break;
      case '\\': c = '\\'; fsmp_Next(fsmp); break;
      case 'x':
        c = 0;
        fsmp_Next(fsmp);
        for( i = 0; i < 2; i++ )
        {
          if ( fsmp_Curr(fsmp) == EOF )
            break;
          if ( !(isdigit(fsmp_Curr(fsmp)) != 0 ||
               (toupper(fsmp_Curr(fsmp)) >= 'A' &&
                toupper(fsmp_Curr(fsmp)) <= 'F'   )) )
          {
            break;
          }
          c *= 16;
          if ( fsmp_Curr(fsmp) <= '9' )
            c += fsmp_Curr(fsmp)-'0';
          else
            c += toupper(fsmp_Curr(fsmp))-'A'+10;
          fsmp_Next(fsmp);
        }
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
        c = 0;
        for( i = 0; i < 3; i++ )
        {
          if ( fsmp_Curr(fsmp) == '\0' )
            break;
          if ( fsmp_Curr(fsmp) <= '0' || fsmp_Curr(fsmp) >= '7' )
          {
            break;
          }
          c *= 8;
          c += fsmp_Curr(fsmp)-'0';
          fsmp_Next(fsmp);
        }
        break;
      default:
        c = fsmp_Curr(fsmp);
        fsmp_Next(fsmp);
        break;
    }
  }
  else
  {
    c = fsmp_Curr(fsmp);
    fsmp_Next(fsmp);
  }
  return c;
}


void fsmp_read_string(fsmp_type fsmp)
{
  int i = 0;
  
  for(;;)
  {
    if ( fsmp_Curr(fsmp) != '\"' )
    {
      fsmp->string[i] = '\0';
      return;
    }
    fsmp_Next(fsmp);
    for(;;)
    {
      if ( fsmp_Curr(fsmp) == EOF )
        break;
      if ( fsmp_Curr(fsmp) == '\"' )
        break;
      if ( i < FSMP_STRING_LEN-1 )
        fsmp->string[i++] = fsmp_get_escaped_char(fsmp);
      else
        fsmp_get_escaped_char(fsmp);
    }
    fsmp_SkipSpace(fsmp);
  }
}

int fsmp_read_token(fsmp_type fsmp)
{
  switch(fsmp_Curr(fsmp))
  {
    case EOF : fsmp->token = FSMP_TOK_EOF;       fsmp_Next(fsmp);        break;
    case '{' : fsmp->token = FSMP_TOK_BEGIN;     fsmp_Next(fsmp);        break;
    case '}' : fsmp->token = FSMP_TOK_END;       fsmp_Next(fsmp);        break;
    case ';' : fsmp->token = FSMP_TOK_SEMICOLON; fsmp_Next(fsmp);        break;
    case '\"': fsmp->token = FSMP_TOK_STRING;    fsmp_read_string(fsmp); break;
    default:
      if ( (fsmp_Curr(fsmp) >= 'a' && fsmp_Curr(fsmp) <= 'z') || 
           (fsmp_Curr(fsmp) >= 'A' && fsmp_Curr(fsmp) <= 'Z') || 
           fsmp_Curr(fsmp) == '_' )
      {
        fsmp->token = FSMP_TOK_IDENTIFIER;
        fsmp_read_identifier(fsmp);
      }
      break;
  }
  fsmp_SkipSpace(fsmp);
  return fsmp->token;
}



