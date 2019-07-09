/*

  tree.c

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
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "tree.h"
#include "mwc.h"

/*---------------------------------------------------------------------------*/

static void tr_na_init_node(tr_na na, int pos)
{
  na->list[pos].type = TR_NODE_TYPE_ILLEGAL;
  na->list[pos].d.v = 0;
  na->list[pos].next = -1;
  na->list[pos].down = -1;
}

static void tr_na_init_range(tr_na na, int start, int cnt)
{
  int i;
  for( i = start; i < start+cnt; i++ )
    tr_na_init_node(na, i);
}

static int tr_na_expand(tr_na na)
{
  void *p;  
  assert(na->list != NULL);
  assert(na->max > 0);
  p = realloc(na->list, sizeof(tr_node_struct)*(na->max+TR_NA_EXPAND_LIST_SIZE));
  if ( p == NULL )
    return 0;
  na->list = (tr_node_struct *)p;
  na->max += TR_NA_EXPAND_LIST_SIZE;
  tr_na_init_range(na, na->max-TR_NA_EXPAND_LIST_SIZE, TR_NA_EXPAND_LIST_SIZE);
  na->search_start = na->max-TR_NA_EXPAND_LIST_SIZE;
  return 1;
}

tr_na tr_na_Open()
{
  tr_na na;
  na = (tr_na)malloc(sizeof(struct _tr_na_struct));
  if ( na != NULL )
  {
    na->list = (tr_node_struct *)malloc(sizeof(tr_node_struct)*TR_NA_INITIAL_LIST_SIZE);
    if ( na->list != NULL )
    {
      na->max = TR_NA_INITIAL_LIST_SIZE;
      na->cnt = 0;
      na->search_start = 0;
      tr_na_init_range(na, 0, TR_NA_INITIAL_LIST_SIZE);
      return na;
    }
    free(na);
  }
  return NULL;
}

void tr_na_Close(tr_na na)
{
  free(na->list);
  free(na);
}


int tr_na_Alloc(tr_na na)
{
  int i;
  assert(na->cnt <= na->max);
  if ( na->cnt == na->max )
    if ( tr_na_expand(na) == 0 )
      return -1;
  assert(na->cnt < na->max);
  
  for(i = na->search_start; i < na->max; i++ )
  {
    if ( na->list[i].type == TR_NODE_TYPE_ILLEGAL )
    {
      tr_na_init_node(na, i);
      na->list[i].type = TR_NODE_TYPE_NONE;
      na->cnt++;
      return i;
    }
  }

  for(i = 0; i < na->search_start; i++ )
  {
    if ( na->list[i].type == TR_NODE_TYPE_ILLEGAL )
    {
      na->list[i].type = TR_NODE_TYPE_NONE;
      na->cnt++;
      return i;
    }
  }
  assert(0);
  return -1;
}

int tr_na_IsElement(tr_na na, tr_node n)
{
  if ( n >= na->list && n <= na->list+na->max )
    return 1;
  return 0;
}

void tr_na_Free(tr_na na, int n)
{
  tr_na_N(na, n)->type = TR_NODE_TYPE_ILLEGAL;
  na->cnt--;
}

size_t tr_na_GetMemUsage(tr_na na)
{
  return sizeof(struct _tr_na_struct)+na->max*sizeof(tr_node_struct);
}

int tr_na_Cnt(tr_na na)
{
  return na->cnt;
}

/*---------------------------------------------------------------------------*/

tr_pool tr_pool_Open()
{
  tr_pool p;
  int i;
  p = (tr_pool)malloc(sizeof(struct _tr_pool_struct));
  if ( p != NULL )
  {
    p->current_na = 0;
    for( i = 0; i < TR_POOL_ARRAY_CNT; i++ )
      p->na[i] = NULL;
      
    for( i = 0; i < TR_POOL_ARRAY_CNT; i++ )
    {
      p->na[i] = tr_na_Open();
      if ( p->na[i] == NULL )
       break;
    }
    if ( i >=  TR_POOL_ARRAY_CNT)
    {
      return p;
    }
    for( i = 0; i < TR_POOL_ARRAY_CNT; i++ )
      if ( p->na[i] != NULL )
        tr_na_Close(p->na[i]);
    free(p);
  }
  return NULL;
}

void tr_pool_Close(tr_pool p)
{
  int i;
  for( i = 0; i < TR_POOL_ARRAY_CNT; i++ )
    if ( p->na[i] != NULL )
      tr_na_Close(p->na[i]);
  free(p);
}

int tr_pool_Alloc(tr_pool p)
{
  int pos;
  pos = tr_na_Alloc(p->na[p->current_na]);
  if ( pos < 0 )
    return -1;
  pos |= (p->current_na<<TR_POOL_ARRAY_BPOS);
  p->current_na++;
  if ( p->current_na >= TR_POOL_ARRAY_CNT )
    p->current_na = 0;
  return pos;
}

void tr_pool_Free(tr_pool p, int n)
{
  tr_na_Free(p->na[n>>TR_POOL_ARRAY_BPOS], n&TR_POOL_ARRAY_PMASK);
}

size_t tr_pool_GetMemUsage(tr_pool p)
{
  int i;
  size_t sum = sizeof(struct _tr_pool_struct);
  for( i = 0; i < TR_POOL_ARRAY_CNT; i++ )
    sum += tr_na_GetMemUsage(p->na[i]);
  return sum;
}

int tr_pool_Cnt(tr_pool p)
{
  int i;
  int sum = 0;
  for( i = 0; i < TR_POOL_ARRAY_CNT; i++ )
    sum += tr_na_Cnt(p->na[i]);
  return sum;
}

/*---------------------------------------------------------------------------*/

#define TR_STR_EXPAND 16

int tr_str_init(tr_str tstr)
{
  tstr->mem_usage_of_strings = 0;
  tstr->cnt = 0;
  tstr->max = 0;
  tstr->list = (char **)malloc(TR_STR_EXPAND*sizeof(void *));
  if ( tstr->list == NULL )
    return 0;
  tstr->max = TR_STR_EXPAND;
  return 1;
}

int tr_str_expand(tr_str tstr)
{
  void *ptr;
  ptr = realloc(tstr->list, (tstr->max+TR_STR_EXPAND)*sizeof(void *));
  if ( ptr == NULL )
    return 0;
  tstr->list = (char **)ptr;
  tstr->max += TR_STR_EXPAND;
  return 1;
}

tr_str tr_str_Open()
{
  tr_str tstr;
  tstr = (tr_str)malloc(sizeof(struct _tr_str_struct));
  if ( tstr != NULL )
  {
    if ( tr_str_init(tstr) != 0 )
    {
      return tstr;
    }
    free(tstr);
  }
  return NULL;
}

void tr_str_Clear(tr_str tstr)
{
  int i;
  for( i = 0; i < tstr->cnt; i++ )
  {
    free(tstr->list[i]);
    tstr->list[i] = NULL;
  }
  tstr->cnt = 0;
}

void tr_str_Close(tr_str tstr)
{
  tr_str_Clear(tstr);
  free(tstr->list);
  tstr->max = 0;
  free(tstr);
}

/* returns position or -1 */
static int tr_str_append(tr_str tstr, char *ptr)
{
  while( tstr->cnt >= tstr->max )
    if ( tr_str_expand(tstr) == 0 )
      return -1;
  
  tstr->list[tstr->cnt] = strdup(ptr);
  if ( tstr->list[tstr->cnt] == NULL )
    return -1;
  tstr->mem_usage_of_strings += strlen(ptr)+1;
  tstr->cnt++;
  return tstr->cnt-1;
}

/*
void tr_str_DelByPos(tr_str tstr, int pos)
{
  while( pos+1 < tstr->cnt )
  {
    tstr->list[pos] = tstr->list[pos+1];
    pos++;
  }
  if ( tstr->cnt > 0 )
    tstr->cnt--;
  tstr->mem_usage_of_strings
}
*/

/* 0: error */
int tr_str_InsByPos(tr_str tstr, char *ptr, int pos)
{
  int i;
  if ( tr_str_append(tstr, ptr) < 0 )
    return 0;
  assert(tstr->cnt > 0 );
  i = tstr->cnt-1;
  ptr = tstr->list[i];
  while( i > pos + 4 )
  {
    tstr->list[i-0] = tstr->list[i-1];
    tstr->list[i-1] = tstr->list[i-2];
    tstr->list[i-2] = tstr->list[i-3];
    tstr->list[i-3] = tstr->list[i-4];
    i-=4;
  }
  while( i > pos )
  {
    tstr->list[i] = tstr->list[i-1];
    i--;
  }
  tstr->list[i] = ptr;
  return 1;
}

int tr_str_compare(tr_str tstr, char *s, char *t)
{
  return strcasecmp(s, t);
}

int tr_str_FindInsPos(tr_str tstr, char *key)
{
  int l,u,m;
  int result;

  if ( tstr->cnt == 0 )
    return 0;

  l = 0;
  u = tstr->cnt;
  while(l < u)
  {
    m = (size_t)((l+u)/2);
    result = tr_str_compare(tstr, tstr->list[m], key);
    if ( result < 0 )
       l = m+1;
    else if ( result > 0 )
       u = m;
    else
       return m;
  }
  return u;
}

int tr_str_FindPos(tr_str tstr, char *key)
{
  int pos;
  pos = tr_str_FindInsPos(tstr, key);
  if ( pos >= tstr->cnt )
    return -1;
  if ( tr_str_compare(tstr, tstr->list[pos], key) != 0 )
    return -1;
  return pos;
}

int tr_str_Add(tr_str tstr, char *key)
{
  int pos;
  pos = tr_str_FindInsPos(tstr, key);
  if ( pos < tstr->cnt )
    if ( tr_str_compare(tstr, tstr->list[pos], key) == 0 )
     return pos;
  if ( tr_str_InsByPos(tstr, key, pos) != 0 )
    return pos;
  return -1;
}

char *tr_str_Cache(tr_str tstr, char *s)
{
  int pos;
  pos = tr_str_Add(tstr, s);
  if ( pos < 0 )
    return NULL;
  return tstr->list[pos];
}

size_t tr_str_GetMemUsage(tr_str tstr)
{
  return sizeof(struct _tr_str_struct)+
    tstr->mem_usage_of_strings+
    tstr->max*sizeof(char *);
}


/*---------------------------------------------------------------------------*/

void tree_err_fn(void *data, char *fmt, va_list va)
{
  vprintf(fmt, va);
  puts("");
}

void tree_SetErrFn(tree t, void (*err_fn)(void *data, char *fmt, va_list va), void *data)
{
  t->err_fn = err_fn;
  t->err_data = data;
}

tree tree_Open()
{
  tree t;
  t = malloc(sizeof(struct _tree_struct));
  if ( t != NULL )
  {
    tree_SetErrFn(t, tree_err_fn, NULL);
  
    t->buf_ptr = NULL;
    t->buf_size = 0; 

    t->pool = tr_pool_Open();
    if ( t->pool != NULL )
    {
      t->str_cache = tr_str_Open();
      if ( t->str_cache != NULL )
      {
        return t;
      }
      tr_pool_Close(t->pool);
    }
    free(t);
  }
  return NULL;
}

void tree_Close(tree t)
{
  if ( t->buf_ptr != NULL )
    free(t->buf_ptr);
  tr_pool_Close(t->pool);
  tr_str_Close(t->str_cache);
  free(t);
}

void tree_dot_error(tree t, char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  t->err_fn(t->err_data, fmt, va);
  va_end(va);
}

void tree_ErrorVA(tree t, char *fmt, va_list va)
{
  vsnprintf(t->err_buf, TREE_ERR_BUF_LEN, fmt, va);
  t->err_buf[TREE_ERR_BUF_LEN-1] = '\0';
  tree_dot_error(t, "line %d: %s", (int)t->line_cnt, t->err_buf);
}

void tree_Error(tree t, char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  tree_ErrorVA(t, fmt, va);
  va_end(va);
}


static int tree_extend_buf(tree t, size_t cnt)
{
  void *p;
  if ( cnt <= t->buf_size  )
    return 1;
  cnt += 31;
  cnt &= ~31;
  if ( t->buf_ptr == NULL )
    p = malloc(cnt);
  else
    p = realloc(t->buf_ptr, cnt);
  if ( p == NULL )
  {
    tree_Error(t, "out of memory (tree_extend_buf)");
    return 0;
  }
  t->buf_ptr = (char *)p;
  t->buf_size = cnt;
  return 1;
}

static int _tree_SetBufChar(tree t, int pos, int c)
{
  if ( pos < t->buf_size  )
  {
    t->buf_ptr[pos] = c;
    return 1;
  }
  if ( tree_extend_buf(t, pos+1) != 0 )
  {
    t->buf_ptr[pos] = c;
    return 1;
  }
  return 0;
}

#define tree_SetBufChar(t, pos, c) \
  (((pos) < (t)->buf_size)? \
  ((t)->buf_ptr[pos] = (c),1):\
  _tree_SetBufChar((t),(pos),(c)))
  


void tree_Free(tree t, int n)
{
  tr_pool_Free(t->pool, n);
  if ( tree_Down(t,n) >= 0 )
    tree_Free(t, tree_Down(t,n));
  if ( tree_Next(t,n) >= 0 )
    tree_Free(t, tree_Next(t,n));
}

char *tree_RegisterString(tree t, char *s)
{
  char *cs = tr_str_Cache(t->str_cache, s);
  if ( s != NULL && cs == NULL )
    tree_Error(t, "out of memory (tree_RegisterString)");
  return cs;
}

int tree_SetStr(tree t, int n, char *s)
{
  tree_N(t,n)->d.s = tree_RegisterString(t, s);
  if ( tree_N(t,n)->d.s == NULL )
  {
    tree_N(t,n)->type = TR_NODE_TYPE_NONE;
    return 0;
  }
  tree_N(t,n)->type = TR_NODE_TYPE_CSTR;
  return 1;
}

void tree_SetValue(tree t, int n, int v)
{
  tree_N(t,n)->d.v = v;
  tree_N(t,n)->type = TR_NODE_TYPE_VALUE;
}

int tree_AllocEmpty(tree t)
{
  int pos = tr_pool_Alloc(t->pool);
  if ( pos < 0 )
  {
    tree_Error(t, "out of memory (tree_AllocEmpty)");
    return -1;
  }
  return pos;
}

int tree_AllocStr(tree t, char *s)
{
  int n;
  n = tree_AllocEmpty(t);
  if ( n >= 0 )
  {
    if ( tree_SetStr(t, n, s) != 0 )
    {
      return n;
    }
    tree_Free(t, n);
  }
  return -1;
}

int tree_AllocValue(tree t, int v)
{
  int n;
  n = tree_AllocEmpty(t);
  if ( n < 0 )
    return -1;
  tree_SetValue(t, n, v);
  return n;
}

size_t tree_GetMemUsage(tree t)
{
  return tr_pool_GetMemUsage(t->pool)+tr_str_GetMemUsage(t->str_cache)+
    sizeof(struct _tree_struct);
}

int tree_GetNodeCnt(tree t)
{
  return tr_pool_Cnt(t->pool);
}

/* requires, that the string was registered with tree_RegisterString */
int tree_Search(tree t, int n, char *s)
{
  while( n >= 0 )
  {  
    if ( tree_N(t, n)->d.s == s )
      return n;
    n = tree_Next(t, n);
  }
  return -1;
}

/* requires, that the string was registered with tree_RegisterString */
int tree_SearchNext(tree t, int n, char *s)
{
  return tree_Search(t, tree_Next(t, n), s);
}

/* requires, that the string was registered with tree_RegisterString */
int tree_SearchChild(tree t, int n, char *s)
{
  return tree_Search(t, tree_Down(t, n), s);
}


/*---------------------------------------------------------------------------*/

static void tree_get_next(tree t)
{
  t->current_char = getc(t->fp);
  t->char_cnt++;
  /*
  if ( (t->char_cnt & 0x03fff) == 0 )
  {
    printf("%d \r", (int)t->char_cnt); fflush(stdout);
  }
  */
}

static int tree_is_space(int c)
{
  if ( c > '\0' && c <= ' ' )
    return 1;
  return 0;
}

static void tree_skip_space(tree t)
{
  while( tree_is_space(t->current_char) )
  {
    if ( t->current_char == '\n' )
      t->line_cnt++;
    tree_get_next(t);
  }
}

static int tree_read_unsigned_number(tree t)
{
  int v = 0;
  while( t->current_char >= '0' && t->current_char <= '9' )
  {
    v = v*10 + t->current_char - '0';
    tree_get_next(t);
  }
  return v;
}

static int tree_edif_string_token(tree t)
{
  int i = 0;
  int state = 0;
  int c;
  if ( t->current_char != '\"' )
  {
    tree_Error(t, "missing \"");
    return 0;
  }
  tree_get_next(t);
  for(;;)
  {
    switch(state)
    {
      case 0: /* normal string characters */
        if ( t->current_char == EOF )
          state = 3;
        else if ( t->current_char == '\"' )
        {
          tree_get_next(t);
          state = 4;
        }
        else if ( t->current_char == '%' )
        {
          tree_get_next(t);
          state = 1;
        }
        else
        {
          if ( t->current_char >= '\0' &&  t->current_char < ' ' )
            tree_Error(t, "illegal string character");
          
          if ( tree_SetBufChar(t, i, t->current_char) == 0 )
          {
            tree_Error(t, "out of memory (string)");
            return 0;
          }
          i++;
          tree_get_next(t);
        }
        break;
      case 1: /* ascii character */
        if ( t->current_char == EOF )
          state = 3;
        else if (t->current_char == '\"')
          state = 3;
        else if (t->current_char == '%' )
        {
          tree_get_next(t);
          state = 0;
        }
        else
        {
          tree_skip_space(t);
          if ( t->current_char < '0' || t->current_char > '9' )
          {
            tree_Error(t, "illegal ascii character syntax");
            return 0;
          }
          c = tree_read_unsigned_number(t);
          if ( tree_SetBufChar(t, i, c) == 0 )
          {
            tree_Error(t, "out of memory (string, ascii char)");
            return 0;
          }
          i++;
        }
        break;
      case 3: /* string terminates abnormal */
        tree_Error(t, "unexpected end of string");
        return 0;
      case 4: /* normal string termination*/
        if ( tree_SetBufChar(t, i, (int)(unsigned char)'\0') == 0 )
        {
          tree_Error(t, "out of memory (string, '\\0')");
          return 0;
        }
        tree_skip_space(t);
        return 1;
    }
  }
    
  /* does not leave for-loop */
}

static int tree_is_identifier_char(int c)
{
  if ( tree_is_space(c) != 0 )
    return 0;
  if ( c == EOF )
    return 0;
  if ( c == '\"' )
    return 0;
  if ( c == '(' )
    return 0;
  if ( c == ')' )
    return 0;
  return 1;
}

static int tree_edif_str(tree t)
{
  int i = 0;
  if ( t->current_char == '\"' )
    return tree_edif_string_token(t);
  
  while( tree_is_identifier_char(t->current_char) != 0 )
  {
    if ( tree_SetBufChar(t, i, t->current_char) == 0 )
    {
      tree_Error(t, "out of memory (identifier)");
      return 0;
    }
    tree_get_next(t);
    i++;
  }
  if ( tree_SetBufChar(t, i, (int)(unsigned char)'\0') == 0 )
    return 0;
  tree_skip_space(t);
  return 1;
}

int tree_AllocEdifStr(tree t, char *s)
{
  char *e = s;
  int v;
  v = (int)strtol(s, &e, 10);
  if ( e == s+strlen(s) )
    return tree_AllocValue(t, v);
  return tree_AllocStr(t, s);
}

int tree_ScanEdifAtom(tree t)
{
  if ( tree_edif_str(t) == 0 )
    return -1;
    
  return tree_AllocEdifStr(t, t->buf_ptr);
}

int tree_ScanEdifList(tree t)
{
  int parent;
  int node;
  int curr;
  
  if ( t->current_char != '(' )
  {
    tree_Error(t, "expected '('");
    return -1;
  }
  tree_get_next(t);
  tree_skip_space(t);
  
  parent = tree_ScanEdifAtom(t);
  if ( parent < 0 )
    return -1;

  node = -1;
  while( t->current_char != ')' )
  {
    curr = tree_ScanEdifAtomOrList(t);
    if ( curr < 0 )
    {
      tree_Free(t, parent);
      return -1;
    }
    if ( t->current_char == EOF )
    {
      tree_Error(t, "unexpected EOF");
      return -1;
    }
    
    if ( node < 0 )
      tree_N(t,parent)->down = curr;
    else
      tree_N(t,node)->next = curr;
    
    node = curr;
  }
  tree_get_next(t);
  tree_skip_space(t);
  
  return parent;
}

int tree_ScanEdifAtomOrList(tree t)
{
  if ( t->current_char == '(' )
    return tree_ScanEdifList(t);
  return tree_ScanEdifAtom(t);
}

int tree_ScanEdif(tree t)
{
  tree_skip_space(t);
  return tree_ScanEdifAtomOrList(t);
}

int tree_ReadEdif(tree t, const char *name)
{
  int n;
  t->char_cnt = 0;
  t->line_cnt = 1;
  t->fp = fopen(name, "r");
  if ( t->fp == NULL )
    return -1;
  tree_get_next(t);
  n = tree_ScanEdif(t);
  fclose(t->fp);
  /*
  printf("memory usage: %u bytes\n", (unsigned)tree_GetMemUsage(t));
  printf("number of nodes: %u\n", (unsigned)tree_GetNodeCnt(t));
  printf("number of strings: %u/%u\n", (unsigned)t->str_cache->cnt, 
    (unsigned)tr_str_GetMemUsage(t->str_cache));
  */
  return n;
}

void tree_ShowLine(tree t, int n)
{
  if ( tree_Down(t, n) >= 0 )
  {
    printf(" (%s", tree_N(t, n)->d.s);
    n = tree_Down(t, n);
    while( n >= 0 )
    {
      tree_ShowLine(t, n);
      n = tree_Next(t, n);
    }
    printf(")");
  }
  else
  {
    switch(tree_N(t, n)->type)
    {
      case TR_NODE_TYPE_CSTR:
        printf(" %s", tree_N(t, n)->d.s);
        break;
      case TR_NODE_TYPE_VALUE:
        printf(" %d", tree_N(t, n)->d.v);
        break;
    }
  }
}

#ifdef TREE_MAIN
int main()
{
  tree t;
  int n;
  t = tree_Open();
  if ( t != NULL )
  {
    n = tree_ReadEdif(t, "test.edif");
    tree_Close(t);
  }
  return 0;
}
#endif

