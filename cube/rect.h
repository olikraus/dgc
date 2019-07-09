/*

  rect.h
  
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

#ifndef _RECT_H
#define _RECT_H

#include "dcube.h"

typedef unsigned int r_int;
#define R_BITS_PER_WORD (sizeof(r_int)*8)
#define R_SYM_PER_WORD (R_BITS_PER_WORD/2)

#define R_DIMENSIONS 2

typedef struct _rmatrix_struct *rmatrix;


struct _rect_struct
{
  short cnt[R_DIMENSIONS];
  r_int *assignment[R_DIMENSIONS];
};
typedef struct _rect_struct rect;

int rInit(rmatrix rm, rect *r);
void rDestroy(rect *r);

rect *rOpen(rmatrix rm);
void rClose(rect *r);

void rClearDim        (rmatrix rm, int d, rect *r);
void rClear           (rmatrix rm, rect *r);
void rNotAndDim       (rmatrix rm, int d, rect *r, rect *a, rect *b);
void rNotAnd          (rmatrix rm, rect *r, rect *a, rect *b);
void rOrDim           (rmatrix rm, int d, rect *r, rect *a, rect *b);
void rOr              (rmatrix rm, rect *r, rect *a, rect *b);
void rAndDim          (rmatrix rm, int d, rect *r, rect *a, rect *b);
void rAnd             (rmatrix rm, rect *r, rect *a, rect *b);
int rIsIntersectionDim(rmatrix rm, int d, rect *a, rect *b);
int rRealCntDim       (rmatrix rm, int d, rect *r);

/* Ist b Teilmenge von a? */
int rIsSubSetDim(rmatrix rm, int d, rect *a, rect *b);
int rIsSubSet(rmatrix rm, rect *a, rect *b);
int rIsEqual(rmatrix rm, rect *a, rect *b);

void rCopyDim(rmatrix rm, int d, rect *dest, rect *src);
void rCopy(rmatrix rm, rect *dest, rect *src);
void rSet(rect *r, int dim, int val, int symbol);
int rGet(rect *r, int dim, int val);

void rFillDim(int dim, rect *r, int from, int to, int symbol);
void rFill(rect *r, int x1, int x2, int y1, int y2, int symbol);

int rCheck(rmatrix rm, rect *r);
int rIsEmpty(rmatrix rm, rect *r);

void rShowDimLine(rmatrix rm, rect *r, int dim);
void rShowLine(rmatrix rm, rect *r);

struct _rlist_struct
{
  rect *list;
  int max;
  int cnt;
};
typedef struct _rlist_struct *rlist;

#define rlGet(rl,pos) ((rl)->list+(pos))
#define rlCnt(rl) ((rl)->cnt)

rlist rlOpen(void);
void rlClear(rlist rl);
void rlClose(rlist rl);
int rlExpandTo(rmatrix rm, rlist rl, int max);
void rlDel(rmatrix rm, rlist rl, int pos);
int rlAddEmpty(rmatrix rm, rlist rl);
int rlAdd(rmatrix rm, rlist rl, rect *r);

void rlSet(rmatrix rm, rlist rl, int pos, rect *r);
int rlCopy(rmatrix rm, rlist dest, rlist src);
int rlIsDisjunctY(rmatrix rm, rlist rl, int pos);
int rlFindMaxRectangle(rmatrix rm, rlist rl);
int rlDisjunctY(rmatrix rm, rlist rl);

/* limited rlist */

struct _lrlist_struct
{
  int limit;            /* maximum number of elements */
  int pos_min;          /* Position des kleinsten rechtecks */
  int pos_max;          /* Position des groessten rechtecks */
  rlist rl;
};
typedef struct _lrlist_struct *lrlist;

#define lrlGet(lrl, pos) rlGet((lrl)->rl, (pos))
#define lrlCnt(lrl) rlCnt((lrl)->rl)
#define lrlGetMinPos(lrl) ((lrl)->pos_min)
#define lrlGetMaxPos(lrl) ((lrl)->pos_max)

void lrlClear(lrlist lrl);
void lrlSetLimit(lrlist lrl, int limit);      /* also calls lrlClear() */
lrlist lrlOpen();
void lrlClose(lrlist lrl);

int lrlIsSubSetElement(rmatrix rm, lrlist lrl, rect *r);
int lrlIsEqualElement(rmatrix rm, lrlist lrl, rect *r);
void lrlDel(rmatrix rm, lrlist lrl, int pos);
int lrlPut(rmatrix rm, lrlist lrl, rect *r);  /* returns 0 for error */
int lrlCopy(rmatrix rm, lrlist dest, lrlist src);
int lrlShow(rmatrix rm, lrlist lrl);
void lrlDelLowerEqualWidth(rmatrix rm, lrlist lrl, int width);
void lrlDelLowerEqualHeight(rmatrix rm, lrlist lrl, int height);
int lrlDisjunctY(rmatrix rm, lrlist lrl);


/* rectangle matrix */
struct _rmatrix_struct
{
  int dim[R_DIMENSIONS];
  int word_dim[R_DIMENSIONS]; /* (dim[i]+R_SYM_PER_WORD-1)/R_SYM_PER_WORD */
  int word_product;   /* word_width*word_heigth */
  r_int *xy[R_DIMENSIONS];
  rect buffer;
};


void rm_set_xy(rmatrix rm, int x, int y, int value);
void rm_set_yx(rmatrix rm, int x, int y, int value);
int rm_get_xy(rmatrix rm, int x, int y);
int rm_get_yx(rmatrix rm, int x, int y);


#define rmGetRow(rm, y) ((rm)->xy[0]+(y)*(rm)->word_dim[0])
#define rmGetCol(rm, x) ((rm)->xy[1]+(x)*(rm)->word_dim[1])

void rmClear(rmatrix rm);
rmatrix rmOpen(int width, int height);
void rmClose(rmatrix rm);

void rmFillByCL(rmatrix rm, pinfo *pi, dclist cl);
void rmShow(rmatrix rm);

void rmExtendToMaxHeight(rmatrix rm, rect *r);
void rmExtendToMaxWidth(rmatrix rm, rect *r);
int rmIsMaxWidth(rmatrix rm, rect *r);
int rmIsMaxHeight(rmatrix rm, rect *r);

void rmExtendToMaxHeightArea(rmatrix rm, rect *r, rect *area);
int rmIsMaxHeightArea(rmatrix rm, rect *r, rect *area);
void rmExtendToMaxWidthArea(rmatrix rm, rect *r, rect *area);
int rmIsMaxWidthArea(rmatrix rm, rect *r, rect *area);


int rmExtendXY(rmatrix rm, lrlist lrl, lrlist primes, rect *r, rect *area);
int rmExtendList(rmatrix rm, lrlist dest, lrlist src, rect *area);

int rmFindInitialList(rmatrix rm, lrlist lrl, int col, rlist exclude);
int rmFindInitialListRange(rmatrix rm, lrlist lrl, int low_x, int high_x);
int rmFindInitialListArea(rmatrix rm, lrlist lrl, rect *area);
int rmFindInitialListAreaMaxWidth(rmatrix rm, lrlist lrl, rect *area);
void rmFillWithCol(rmatrix rm, rect *r, int col, int s);

int rmAddRectColumn(rmatrix rm, rect *r, int column, int symbol);

#endif /* _RECT_H */
