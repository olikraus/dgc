/*

  tree.h

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

#ifndef _TREE_H
#define _TREE_H

#include <stdio.h>
#include <stdarg.h>

typedef union _tr_node_data_union tr_node_data_union;
typedef struct _tr_node_struct tr_node_struct;
typedef struct _tr_node_struct *tr_node;


union _tr_node_data_union
{
  char *s;
  int v;
};

#define TR_NODE_TYPE_ILLEGAL 0
#define TR_NODE_TYPE_NONE 1
#define TR_NODE_TYPE_CSTR 2
/* #define TR_NODE_TYPE_ASTR 3 */
#define TR_NODE_TYPE_VALUE 4


struct _tr_node_struct
{
  int type;
  tr_node_data_union d;
  int next;
  int down;
};

/* node array */
#define TR_NA_INITIAL_LIST_SIZE 128
#define TR_NA_EXPAND_LIST_SIZE 128
struct _tr_na_struct
{
  tr_node_struct *list;
  int cnt;
  int max;
  int search_start;
};
typedef struct _tr_na_struct *tr_na;

#define tr_na_N(tr_na, pos) ((tr_na)->list+(pos))

#define TR_POOL_ARRAY_BCNT 4
#define TR_POOL_ARRAY_BPOS 26
#define TR_POOL_ARRAY_CNT (1<<TR_POOL_ARRAY_BCNT)
#define TR_POOL_ARRAY_PMASK ((1<<TR_POOL_ARRAY_BPOS)-1)
struct _tr_pool_struct
{
  tr_na na[TR_POOL_ARRAY_CNT];
  int current_na;
};
typedef struct _tr_pool_struct *tr_pool;

#define tr_pool_N(pool, pos) tr_na_N((pool)->na[(pos)>>TR_POOL_ARRAY_BPOS], \
  (pos)&TR_POOL_ARRAY_PMASK)

struct _tr_str_struct
{
  char **list;
  int max;
  int cnt;
  int is_case_in_sensitive;
  size_t mem_usage_of_strings;
};
typedef struct _tr_str_struct *tr_str;

#define TREE_ERR_BUF_LEN 1024
struct _tree_struct
{
  tr_str str_cache;
  tr_pool pool;
  
  FILE *fp;
  int current_char;
  
  char *buf_ptr;
  size_t buf_size; 
  
  unsigned char_cnt;
  unsigned line_cnt;
  
  void (*err_fn)(void *data, char *fmt, va_list va);
  void *err_data;
  char err_buf[TREE_ERR_BUF_LEN];
};
typedef struct _tree_struct *tree;

#define tree_N(tree, pos) tr_pool_N((tree)->pool, pos)

#define tree_Next(tree, pos) (tree_N((tree),(pos))->next)
#define tree_Down(tree, pos) (tree_N((tree),(pos))->down)

tree tree_Open();
void tree_Close(tree t);

void tree_SetErrFn(tree t, void (*err_fn)(void *data, char *fmt, va_list va), void *data);

int tree_AllocEmpty(tree t);
int tree_AllocStr(tree t, char *s);
int tree_AllocValue(tree t, int v);
void tree_Free(tree t, int n);

int tree_SetStr(tree t, int n, char *s);
void tree_SetValue(tree t, int n, int v);

char *tree_RegisterString(tree t, char *s);

int tree_ScanEdifAtom(tree t);
int tree_ScanEdifList(tree t);
int tree_ScanEdifAtomOrList(tree t);
int tree_ScanEdif(tree t);
int tree_ReadEdif(tree t, const char *name);

int tree_Search(tree t, int n, char *s);
int tree_SearchNext(tree t, int n, char *s);
int tree_SearchChild(tree t, int n, char *s);

void tree_ShowLine(tree t, int n);

#endif
