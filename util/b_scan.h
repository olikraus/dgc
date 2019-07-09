/*

  b_scan.h
  
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

#ifndef _B_SCAN_H
#define _B_SCAN_H

#include <stdio.h>
#include "b_set.h"

struct _b_scankey_struct
{
  int id;
  char *name;
  size_t len;
};
typedef struct _b_scankey_struct *b_scankey_type;

struct _b_scan_struct
{
  b_set_type keys;
  char *filename;
  FILE *fp;
  
  int curr;
  
  char *str_val;
  size_t str_pos;
  size_t str_max;
  
  int num_val;
  
  int token;
};
typedef struct _b_scan_struct *b_scan_type;


#define b_scan_LoopKeys(sc, pos) b_set_WhileLoop((sc)->keys, (pos))
#define b_scan_GetKey(sc, pos) ((b_scankey_type)b_set_Get((sc)->keys, (pos)))

#define B_SCAN_TOK_ERR -1
#define B_SCAN_TOK_NONE -2
#define B_SCAN_TOK_VAL -3
#define B_SCAN_TOK_ID  -4
#define B_SCAN_TOK_STR -5
#define B_SCAN_TOK_EOF -6
#define B_SCAN_TOK_LINE_COMMENT -7

b_scan_type b_scan_Open();
void b_scan_Close(b_scan_type sc);
int b_scan_AddKey(b_scan_type sc, int id, const char *name);
const char * b_scan_GetNameById(b_scan_type sc, int id);
int b_scan_SetFileName(b_scan_type sc, const char *name);
int b_scan_Token(b_scan_type sc);

#endif /* _B_SCAN_H */
