/*

   SC_BEXPR.H

   pharser for synopsys boolean expressions

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
    
   17.07.96  Oliver Kraus

*/

#ifndef _SC_BEXPR_H
#define _SC_BEXPR_H

#include "sc_io.h"
#include "sc_strl.h"

/* ben = boolean expression node */

#define SC_BEN_FN_NONE 0

#define SC_BEN_FN_0     10
#define SC_BEN_FN_1     11
#define SC_BEN_FN_ID    12
#define SC_BEN_FN_TRI   13
#define SC_BEN_FN_BUF   14

#define SC_BEN_FN_XOR   100
#define SC_BEN_FN_AND   101
#define SC_BEN_FN_OR    102
#define SC_BEN_FN_DFF   103
#define SC_BEN_FN_DL    104

#define SC_BEN_FN_EQV   200
#define SC_BEN_FN_NAND  201
#define SC_BEN_FN_NOR   202
#define SC_BEN_FN_NDFF  203 
#define SC_BEN_FN_NDL   204


#define SC_BEN_MAX 32
struct _sc_ben_struct
{
   int is_invert;
   int fn;
   int cnt;
   int max;       /* this is always SC_BEN_MAX */
   int ref;       /* reference to the id */
   char *id;
   struct _sc_ben_struct *list[SC_BEN_MAX];
};
typedef struct _sc_ben_struct sc_ben_struct;
typedef struct _sc_ben_struct *sc_ben_type;

typedef struct _sc_be_struct *sc_be_type;
struct _sc_be_struct
{
   char *def;
   char *pos;
   sc_ben_type ben;
   char str[SC_IO_STR_LEN];
   void *eval_data;
   int (*eval_fn)(void *data, int ref);
   sc_strl_type strl;
};
typedef struct _sc_be_struct sc_be_struct;

char *sc_be_identifier(char *s);
sc_be_type sc_be_Open(void);
void sc_be_Close(sc_be_type be);
int sc_be_GenerateTree(sc_be_type be, char *def);
char *sc_be_GetSilcSynForm(sc_be_type be);
char *sc_be_GetSimpleCombinational(sc_be_type be);
char *sc_be_GetMDLConnection(sc_be_type be, char *outname, int *start);
char *sc_be_GetMDLPart(sc_be_type be, int *start);
char *sc_be_GetMDLPins(sc_be_type be, char *outname, int *start);
int _sc_bc_RegisterIdentifier(sc_be_type be, sc_ben_type ben);
int sc_bc_RegisterIdentifier(sc_be_type be);
void sc_bc_SetEvalFn(sc_be_type be, int (*eval_fn)(void *data, int ref), void *data);
int sc_bc_Eval(sc_be_type be);

/*
   -1 pin not found
   0  not inverted
   1  inverted
   2  xor
*/
int sc_be_GetEffect(sc_be_type be, char *in);

#endif

