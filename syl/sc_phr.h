/*

   SC_PHR.C
   
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
    
   01.07.96  Oliver Kraus    
   
*/

#ifndef _SC_PHR_C
#define _SC_PHR_C

#include "sc_sld.h"
#include "sc_io.h"

#define PURE_FN


#ifndef PURE_FN

#include "sc_strl.h"
#include "sc_drive.h"
#include "sc_bc.h"
#include "sc_list.h"

#endif


#define SC_MSG_AUX_START 1
#define SC_MSG_AUX_END   0

#define SC_MSG_IND_START 900
#define SC_MSG_IND_END   901
#define SC_MSG_IND_DO    902

#define SC_MSG_GRP_START 1000
#define SC_MSG_GRP_END   1001

#define SC_MSG_ATTRIB    1111

#define SC_PHR_GRP_DEPTH_MAX 16
#define SC_PHR_AUX_MAX 10


#define SC_PHR_ARG_STR     0
#define SC_PHR_ARG_DOUBLE  1
#define SC_PHR_ARG_LONG    2

#define SC_PHR_ARGS SC_CDEF_ARGS

union sc_phr_arg_union
{
   double d;
   long   l;
   char   s[SC_IO_STR_LEN];
};

struct _sc_phr_arg_struct
{
   union sc_phr_arg_union u;
   int typ;
};
typedef struct _sc_phr_arg_struct sc_phr_arg_struct;

struct _sc_phr_indicator_struct
{
   long fsize;       /* total file size   */
   long fpos;        /* current file pos  */
   int pm;           /* per 1000 value    */
};
typedef struct _sc_phr_indicator_struct sc_phr_indicator_struct;

struct _sc_phr_grp_struct
{
   int idx;
   char *name;
   char *s1;
   char *s2;
   long val;
};
typedef struct _sc_phr_grp_struct sc_phr_grp_struct;

struct _sc_phr_aux_struct
{
   int (*aux_fn)(void *, int msg, void *data);
   void *aux_user_data;
   void *aux_local_data;
   int is_aux;
};
typedef struct _sc_phr_aux_struct sc_phr_aux_struct;

#define SC_PHR_OUT_FILE_MAX 16

struct _sc_phr_struct
{
   sc_io_type io;
   sc_sld_type sld;
   sc_phr_grp_struct grp_stack[SC_PHR_GRP_DEPTH_MAX];
   int grp_depth;
   sc_phr_aux_struct aux_list[SC_PHR_AUX_MAX];
   int aux_pos;
   int is_relax;
   FILE *out_fp[SC_PHR_OUT_FILE_MAX];
   int is_do_mdl_files;
   sc_phr_arg_struct arg_list[SC_PHR_ARGS];
   int arg_cnt;
   
   char *sdef1_name;
   
   int is_error_msg;

#ifndef PURE_FN
   sc_drive_type fanout_list;
   sc_bc_type bc;                   /* buffer chain list */

   
   double default_tt;               /* default transition time in ns */
   double sl_factor;                /* picofarad */
   double wire_cap;                 /* wire_cap */
   double dtt;                      /* default transition time (nano sec) */
#endif
};
typedef struct _sc_phr_struct sc_phr_struct;
typedef struct _sc_phr_struct *sc_phr_type;

#define sc_phr_GetSLD(phr) ((phr)->sld)
#define sc_phr_GetGrpDepth(phr) ((phr)->grp_depth)
#define sc_phr_GetGrpStruct(phr,idx) ((phr)->grp_stack+(idx))
#define sc_phr_GetCurrGrpStruct(phr) ((phr)->grp_stack+((phr)->grp_depth)-1)
#define sc_phr_GetLastGrpStruct(phr) ((phr)->grp_stack+((phr)->grp_depth)-2)
#define sc_phr_GetOutFP(phr, no) ((phr)->out_fp[no])
#define sc_phr_SetOutFP(phr, no, fp) ((phr)->out_fp[no] = (fp))

#define sc_phr_GetAuxNo(phr,pos) ((phr)->aux_list+(pos))
#define sc_phr_GetAux(phr) ((phr)->aux_list+((phr)->aux_pos))
#define sc_phr_GetUserData(phr) (sc_phr_GetAux(phr)->aux_user_data)
#define sc_phr_GetLocalData(phr) (sc_phr_GetAux(phr)->aux_local_data)
#define sc_phr_SetLocalData(phr,data) \
   (sc_phr_GetAux(phr)->aux_local_data = (data))

#define sc_phr_GetArgCnt(phr) ((phr)->arg_cnt)
#define sc_phr_GetArg(phr, idx) ((phr)->arg_list+(idx))

#define sc_phr_GetIO(phr) ((phr)->io)
#define sc_phr_GetCurr(phr) sc_io_GetCurr(sc_phr_GetIO(phr))
#define sc_phr_GetNext(phr) sc_io_GetNext(sc_phr_GetIO(phr))
#define sc_phr_Unget(phr,c) sc_io_Unget(sc_phr_GetIO(phr), (c))
#define sc_phr_SkipSpace(phr) sc_io_SkipSpace(sc_phr_GetIO(phr))
#define sc_phr_GetIdentifier(phr) sc_io_GetIdentifier(sc_phr_GetIO(phr))
#define sc_phr_GetDec(phr) sc_io_GetDec(sc_phr_GetIO(phr))
#define sc_phr_GetDouble(phr) sc_io_GetDouble(sc_phr_GetIO(phr))
#define sc_phr_GetString(phr) sc_io_GetString(sc_phr_GetIO(phr))
#define sc_phr_GetMString(phr) sc_io_GetMString(sc_phr_GetIO(phr))

int sc_phr_SetAux(sc_phr_type phr, 
   int (*fn)(sc_phr_type phr, int msg, void *data), void *user_data);
sc_phr_type sc_phr_Open(const char *name);
void sc_phr_Close(sc_phr_type phr);
int sc_phr_ReadFanout(sc_phr_type phr, char *filename);
int sc_phr_Do(sc_phr_type phr);
void sc_phr_Error(sc_phr_type phr, char *fmt, ...);
int sc_phr_printf(sc_phr_type phr, int no, char *fmt, ...);
int sc_phr_SetOutfile(sc_phr_type phr, int no, char *name);


#endif
