/*

  b_scan.c
  
  scanner

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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "b_scan.h"
#include "mwc.h"

/*---------------------------------------------------------------------------*/

static int internal_b_scan_set_name(char **s, const char *name)
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

/*---------------------------------------------------------------------------*/

b_scankey_type b_scankey_OpenEmpty()
{
  b_scankey_type sk;
  sk = (b_scankey_type)malloc(sizeof(struct _b_scankey_struct));
  if ( sk != NULL )
  {
    sk->name = NULL;
    sk->id = -1;
    return sk;
  }
  return NULL;
}


void b_scankey_Close(b_scankey_type sk)
{
  if ( sk->name != NULL )
    free(sk->name);
  free(sk);
}

int b_scankey_SetName(b_scankey_type sk, const char *name)
{
  if ( internal_b_scan_set_name(&(sk->name), name) == 0 )
    return 0;
  sk->len = strlen(name);
  return 1;
}

void b_scankey_SetId(b_scankey_type sk, int id)
{
  sk->id = id;
}

b_scankey_type b_scankey_Open(int id, const char *name)
{
  b_scankey_type sk;
  sk = b_scankey_OpenEmpty();
  if ( sk != NULL )
  {
    if ( b_scankey_SetName(sk, name) != 0 )
    {
      b_scankey_SetId(sk, id);
      return sk;
    }
    b_scankey_Close(sk);
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/


b_scan_type b_scan_Open()
{
  b_scan_type sc;
  sc = (b_scan_type)malloc(sizeof(struct _b_scan_struct));
  if ( sc != NULL )
  {
    sc->keys = b_set_Open();
    if ( sc->keys != NULL )
    {
      sc->filename = NULL;
      sc->fp = NULL;
      sc->curr = EOF;
      sc->str_val = NULL;
      sc->str_pos = 0;
      sc->str_max = 0;
      return sc;
    }
    free(sc);
  }
  return NULL;
}

void b_scan_Close(b_scan_type sc)
{
  int i = -1;
  while( b_scan_LoopKeys(sc, &i) != 0 )
    b_scankey_Close(b_scan_GetKey(sc, i));
    
  internal_b_scan_set_name(&(sc->filename), NULL);
  if ( sc->str_val != NULL )
    free(sc->str_val);
  b_set_Close(sc->keys);
  free(sc);
}

int b_scan_Next(b_scan_type sc)
{
  if ( sc->curr == EOF )
    return 0;
  sc->curr = getc(sc->fp);
  if ( sc->curr == EOF )
    return 0;
  return 1;
}

void b_scan_SkipSpace(b_scan_type sc)
{
  for(;;)
  {
    if ( sc->curr == EOF )
      break;
    if ( sc->curr > ' ')
      break;
    b_scan_Next(sc);
  }
}

int b_scan_AddKey(b_scan_type sc, int id, const char *name)
{
  b_scankey_type sk;
  sk = b_scankey_Open(id, name);
  if ( sk != NULL )
  {
    if ( b_set_Add(sc->keys, sk) >= 0 )
    {
      return 1;
    }
    b_scankey_Close(sk);
  }
  return 0;
}

const char * b_scan_GetNameById(b_scan_type sc, int id)
{
  int i = -1;
  while( b_scan_LoopKeys(sc, &i) != 0 )
    if ( b_scan_GetKey(sc, i)->id == id )
      return b_scan_GetKey(sc, i)->name;
  return NULL;
}

int b_scan_SetFileName(b_scan_type sc, const char *name)
{
  if ( internal_b_scan_set_name(&(sc->filename), name) != 0 )
  {
    sc->fp = fopen(sc->filename, "r");
    if ( sc->fp != NULL )
    {
      sc->curr = getc(sc->fp);
      b_scan_SkipSpace(sc);
      return 1;
    }
    internal_b_scan_set_name(&(sc->filename), NULL);
  }
  return 0;
}


void b_scan_ResetStr(b_scan_type sc)
{
  sc->str_pos = 0;
}

#define B_SCAN_STR_EXTEND 4

int b_scan_AddStrChar(b_scan_type sc, int c)
{
  if ( sc->str_val == NULL )
  {
    sc->str_val = (char *)malloc(B_SCAN_STR_EXTEND);
    if ( sc->str_val == NULL )
      return 0;
    sc->str_max = B_SCAN_STR_EXTEND;
  }
  else if ( sc->str_pos+1 < sc->str_max )
  {
    void *ptr;
    ptr = realloc(sc->str_val, sc->str_max+B_SCAN_STR_EXTEND);
    if ( ptr == NULL )
      return 0;
    sc->str_val = (char *)ptr;
    sc->str_max += B_SCAN_STR_EXTEND;
  }
  assert(sc->str_pos+1 < sc->str_max);
  sc->str_val[sc->str_pos] = c;
  sc->str_pos++;
  sc->str_val[sc->str_pos] = '\0';
  return 1;
}

/* returns position number for an exact match */
/* returns -2 if nothing matches */
/* returns -1 if more char's are required */
int b_scan_CheckKeys(b_scan_type sc)
{
  b_scankey_type sk;
  int i = -1;
  int cnt = 0;
  while( b_scan_LoopKeys(sc, &i) != 0 )
  {
    sk = b_scan_GetKey(sc, i);
    if ( sc->str_pos == sk->len )
    {
      if ( strcmp( sk->name, sc->str_val ) == 0 ) 
        return i;
    }
    else if ( sc->str_pos < sk->len )
    {
      if ( strncmp( sk->name, sc->str_val,  sc->str_pos) == 0 ) 
        cnt++;
    }
  }
  if ( cnt == 0 )
    return -2;
  return -1;
}


static int b_scan_IsSymF(b_scan_type sc)
{
  if ( sc->curr >= 'a' && sc->curr <= 'z' )
    return 1;
  if ( sc->curr >= 'A' && sc->curr <= 'Z' )
    return 1;
  if ( sc->curr == '_' )
    return 1;
  return 0;
}

static int b_scan_IsSym(b_scan_type sc)
{
  if ( b_scan_IsSymF(sc) )
    return 1;
  if ( sc->curr >= '0' && sc->curr <= '9' )
    return 1;
  return 0;
}

#define IS_ID  0x02
#define IS_KEY 0x04
int b_scan_KeyIdentifier(b_scan_type sc)
{
  int whats_it = IS_KEY|IS_ID;
  int key_pos;
  int result;

  b_scan_ResetStr(sc);
  
  while(whats_it != 0)
  {
    if ( sc->curr == EOF )
      if ( (whats_it&IS_ID) != 0 )
        return B_SCAN_TOK_ID;
  
    if ( b_scan_AddStrChar(sc, sc->curr) == 0 )
    {
      return B_SCAN_TOK_ERR;
    }
    if ( (whats_it&IS_KEY) != 0 )
    {
      key_pos = b_scan_CheckKeys(sc);
      if ( key_pos >= 0 )
      {
        b_scan_Next(sc);
        if ( b_scan_GetKey(sc, key_pos)->id != B_SCAN_TOK_LINE_COMMENT )
          b_scan_SkipSpace(sc);
        sc->token = b_scan_GetKey(sc, key_pos)->id;
        return sc->token;
      }
      if ( key_pos == -2 ) 
        whats_it &= ~IS_KEY;
    }
    if ( (whats_it&IS_ID) != 0 )
    {
      if ( sc->str_pos == 1 )
        result = b_scan_IsSymF(sc);
      else
        result = b_scan_IsSym(sc);

      if (result == 0)
      {
        if ( whats_it == IS_ID && sc->str_pos > 1 )
        {
          sc->str_val[sc->str_pos-1] = '\0';
          b_scan_SkipSpace(sc);
          return B_SCAN_TOK_ID;
        }
        whats_it &= ~IS_ID;
      }
    }
    b_scan_Next(sc);
  }
  return B_SCAN_TOK_ERR;
}

int b_scan_Number(b_scan_type sc)
{
  sc->num_val = 0;
  while( sc->curr >= '0' && sc->curr <= '9' )
  {
    sc->num_val *= 10;
    sc->num_val += sc->curr - '0';
    b_scan_Next(sc);
  }
  b_scan_SkipSpace(sc);
  return B_SCAN_TOK_VAL;
}

int b_scan_String(b_scan_type sc)
{
  if ( sc->curr != '\"' )
    return B_SCAN_TOK_ERR;
  b_scan_ResetStr(sc);
  for(;;)
  {
    b_scan_Next(sc);
    if ( sc->curr == EOF  )
    {
      return B_SCAN_TOK_ERR;
    }
    if ( sc->curr == '\"' )
      break;
    if ( b_scan_AddStrChar(sc, sc->curr) == 0 )
    {
      return B_SCAN_TOK_ERR;
    }
    
  }
  b_scan_Next(sc);
  b_scan_SkipSpace(sc);
  return B_SCAN_TOK_STR;
}

int b_scan_Token(b_scan_type sc)
{
  int t;
  for(;;)
  {
    if ( sc->curr == EOF )
      return B_SCAN_TOK_EOF;
    if ( sc->curr >= '0' && sc->curr <= '9' )
      return b_scan_Number(sc);
    if ( sc->curr == '\"' )
      return b_scan_String(sc);
    t = b_scan_KeyIdentifier(sc);
    if ( t == B_SCAN_TOK_LINE_COMMENT )
    {
      for(;;)
      {
        if ( sc->curr == EOF )
          return B_SCAN_TOK_EOF;
        if ( sc->curr == '\n' )
          break;
        b_scan_Next(sc);
      }
      b_scan_SkipSpace(sc);
    }
    else
    {
      break;
    }
  }
  return t;
}

#ifdef B_SCAN_MAIN
int main()
{
  int t;
  b_scan_type sc;
  sc = b_scan_Open();
  b_scan_AddKey(sc, 0, ";");
  b_scan_SetFileName(sc, "tmp");
  for(;;)
  {
    t = b_scan_Token(sc);
    if ( t == B_SCAN_TOK_EOF )
    {
      puts("EOF");
      break;
    }
    if ( t == B_SCAN_TOK_ERR )
    {
      puts("ERROR");
      break;
    }
    if ( t == B_SCAN_TOK_VAL )
      printf("%d ", sc->num_val);
    else if ( t == B_SCAN_TOK_STR || t == B_SCAN_TOK_ID || t >= 0  )
      printf("%d:%s ", t, sc->str_val);
  }
  b_scan_Close(sc);
  return 1;
}

#endif
