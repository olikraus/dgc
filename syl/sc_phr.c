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

   28.06.96  Oliver Kraus    
   
   
   26. sep 2001  Oliver Kraus: added is_error_msg flag to disable error messages
*/

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "sc_util.h"
#include "sc_data.h"
#include "sc_phr.h"
#include "mwc.h"


int sc_phr_all_groups(sc_phr_type phr, char *identifier);
int sc_phr_pop_grp(sc_phr_type phr);
void sc_phr_ClearAllAux(sc_phr_type phr);
int sc_phr_AddDoubleArg(sc_phr_type phr, double d);


sc_phr_type sc_phr_Open(const char *name)
{
   sc_phr_type phr;
   int i;
   phr = (sc_phr_type)malloc(sizeof(sc_phr_struct));
   if ( phr != NULL )
   {
      phr->io = sc_io_Open(name);
      if ( phr->io != NULL )
      {
         phr->sld = sc_sld_Open();
         if ( phr->sld != NULL )
         {
            if ( sc_sld_AddListDef(phr->sld, sc_sdl_init_data) != 0 )
            {
               for( i = 0; i < SC_PHR_AUX_MAX; i++ )
                  sc_phr_GetAuxNo(phr,i)->is_aux = 0;
               phr->grp_depth = 0;
               phr->aux_pos = 0;
               phr->is_relax = 1;
               phr->arg_cnt = 0;
               phr->is_error_msg = 1;
               for( i = 0; i < SC_PHR_OUT_FILE_MAX; i++ )
                  phr->out_fp[i] = NULL;
#ifndef PURE_FN
               phr->fanout_list = NULL;
               phr->bc = NULL;
               phr->sl_factor = 0.033;
               phr->wire_cap = 0.066;
               phr->default_tt = 0.80; 
#endif
               phr->is_do_mdl_files = 0;
               phr->sdef1_name = NULL;
               return phr;
            }
            sc_sld_Close(phr->sld);
         }
         sc_io_Close(phr->io);
      }
      free(phr);
   }
   return NULL;
}

void sc_phr_Close(sc_phr_type phr)
{
   int i;
   if ( phr != NULL )
   {
#ifndef PURE_FN
      if ( phr->fanout_list != NULL )
         sc_drive_Close(phr->fanout_list);
      if ( phr->bc != NULL )
         sc_bc_Close(phr->bc);
#endif
      sc_phr_ClearAllAux(phr);

      while( phr->grp_depth > 0 )
      {
        sc_phr_pop_grp(phr);
      }

      for( i = 0; i < SC_PHR_OUT_FILE_MAX; i++ )
         if ( phr->out_fp[i] != NULL && phr->out_fp[i] != stdout )
         {
            fclose(phr->out_fp[i]);
         }
      sc_sld_Close(phr->sld);
      sc_io_Close(phr->io);
      free(phr);
   }
}


void sc_phr_Error(sc_phr_type phr, char *fmt, ...)
{
   va_list l;
   va_start(l, fmt);
   if ( phr->is_error_msg != 0 )
   {
     printf("Error at %s: ", sc_io_GetPosStr(phr->io));
     vsprintf(phr->io->err_str, fmt, l);
     puts(phr->io->err_str);
   }
   va_end(l);
}

#ifndef PURE_FN
int sc_phr_ReadFanout(sc_phr_type phr, char *filename)
{
   phr->fanout_list = sc_drive_Open();
   if ( phr->fanout_list == 0 )
   {
      sc_phr_Error(phr, "out of memory for fanout file");
      return 0;
   }
   if ( sc_drive_AddFile(phr->fanout_list, filename) == 0 )
   {
      sc_phr_Error(phr, "problem with fanout-file '%s'", filename);
      return 0;
   }
   return 1;
}
#endif

#ifndef PURE_FN
int sc_phr_ReadBufferChain(sc_phr_type phr, char *filename)
{
   phr->bc = sc_bc_Open();
   if ( phr->bc == 0 )
   {
      sc_phr_Error(phr, "out of memory for bc file '%s'", filename);
      return 0;
   }
   if ( sc_bc_AddFile(phr->bc, filename) == 0 )
   {
      sc_phr_Error(phr, "problem with buffer-chain-file '%s'", filename);
      return 0;
   }
   return 1;
}
#endif


int sc_phr_SetOutfile(sc_phr_type phr, int no, char *name)
{
   FILE *fp;
   fp = fopen(name, "w");
   if ( fp == NULL )
   {
      sc_phr_Error(phr, "can not open outputfile '%s'", name);
      return 0;
   }
   sc_phr_SetOutFP(phr, no, fp);
   return 1;
}

int sc_phr_printf(sc_phr_type phr, int no, char *fmt, ...)
{
   int ret = 0;
   va_list l;
   va_start(l, fmt);
   if ( sc_phr_GetOutFP(phr, no) != NULL )
      ret = vfprintf(sc_phr_GetOutFP(phr, no), fmt, l);
   va_end(l);
   return ret;
}

int sc_phr_CallAuxNo(sc_phr_type phr, int idx, int msg, void *arg)
{
   sc_phr_aux_struct *aux;
   aux = sc_phr_GetAuxNo(phr, idx);
   phr->aux_pos = idx;
   if ( aux->is_aux != 0 )
      return aux->aux_fn((void *)phr, msg, arg);
   return 1;
}

int sc_phr_CallAux(sc_phr_type phr, int msg, void *arg)
{
   int i;
   int ret = 1;
   for( i = 0; i < SC_PHR_AUX_MAX; i++ )
      if ( sc_phr_CallAuxNo(phr, i, msg, arg) == 0 )
         ret = 0;
   if ( ret == 0 )
      sc_phr_Error(phr, "aux abort");
   return ret;
}

int sc_phr_SetAux(sc_phr_type phr, 
   int (*fn)(sc_phr_type phr, int msg, void *data), void *user_data)
{
   sc_phr_aux_struct *aux;
   int i;
   for( i = 0; i < SC_PHR_AUX_MAX; i++ )
   {
      aux = sc_phr_GetAuxNo(phr, i);
      if ( aux->is_aux == 0 )
      {
         aux->is_aux = 1;
         aux->aux_user_data = user_data;
         aux->aux_local_data = NULL;
         aux->aux_fn = (int (*)(void *, int, void *))fn;
         if ( sc_phr_CallAuxNo(phr, i, SC_MSG_AUX_START, NULL) == 0 )
         {
            aux->is_aux = 0;
            return 0;
         }
         return 1;
      }
   }   
   return 0;
}

void sc_phr_ClearAllAux(sc_phr_type phr)
{
   sc_phr_aux_struct *aux;
   int i;
   for( i = 0; i < SC_PHR_AUX_MAX; i++ )
   {
      sc_phr_CallAuxNo(phr, i, SC_MSG_AUX_END, NULL);
      aux = sc_phr_GetAuxNo(phr, i);
      aux->is_aux = 0;
   }      
}


sc_phr_indicator_struct *sc_phr_GetIndicatorStruct(sc_phr_type phr)
{
   static sc_phr_indicator_struct ind;
   ind.fsize = (long)phr->io->fend;
   ind.fpos = (long)phr->io->fcurr;
   ind.pm = phr->io->pm;
   return &ind;
}

int sc_phr_AddStrArg(sc_phr_type phr, char *s)
{
   if ( phr->arg_cnt >= SC_PHR_ARGS )
      return 0;
   phr->arg_list[phr->arg_cnt].typ = SC_PHR_ARG_STR;
   if ( s == NULL )
      strcpy(phr->arg_list[phr->arg_cnt].u.s, "");
   else
      strncpy(phr->arg_list[phr->arg_cnt].u.s, s, SC_IO_STR_LEN);
   phr->arg_list[phr->arg_cnt].u.s[SC_IO_STR_LEN-1] = '\0';
   phr->arg_cnt++;
   return 1;
}

int sc_phr_AddDoubleArg(sc_phr_type phr, double d)
{
   if ( phr->arg_cnt >= SC_PHR_ARGS )
      return 0;
   phr->arg_list[phr->arg_cnt].typ = SC_PHR_ARG_DOUBLE;
   phr->arg_list[phr->arg_cnt].u.d = d;
   phr->arg_cnt++;
   return 1;
}

int sc_phr_AddLongArg(sc_phr_type phr, long l)
{
   if ( phr->arg_cnt >= SC_PHR_ARGS )
      return 0;
   phr->arg_list[phr->arg_cnt].typ = SC_PHR_ARG_LONG;
   phr->arg_list[phr->arg_cnt].u.l = l;
   phr->arg_cnt++;
   return 1;
}

void sc_phr_ClrArgs(sc_phr_type phr)
{
   phr->arg_cnt = 0;
}

int sc_phr_GetCurrGroupIndex(sc_phr_type phr)
{
   assert(phr != NULL);
   assert(phr->grp_depth > 0);
   return phr->grp_stack[phr->grp_depth-1].idx;
}

int sc_phr_push_grp(sc_phr_type phr, int idx, char *s1, char *s2, long val)
{
   if ( phr->grp_depth >= SC_PHR_GRP_DEPTH_MAX )
   {
      sc_phr_Error(phr, "max group depth reached");
      return 0;
   }
   phr->grp_stack[phr->grp_depth].idx = idx;
   phr->grp_stack[phr->grp_depth].name =
      sc_sld_GetGroupNameByIndex(sc_phr_GetSLD(phr), idx);
   phr->grp_stack[phr->grp_depth].val = val;
   
   phr->grp_stack[phr->grp_depth].s1 = NULL;
   phr->grp_stack[phr->grp_depth].s2 = NULL;
   
   if ( s1 != NULL )
   {
      phr->grp_stack[phr->grp_depth].s1 = (char *)malloc(strlen(s1)+1);
      if ( phr->grp_stack[phr->grp_depth].s1 == NULL )
      {
         sc_phr_Error(phr, "memory");
         return 0;
      }
      strcpy( phr->grp_stack[phr->grp_depth].s1, sc_strupr(s1) );
   }

   if ( s2 != NULL )
   {
      phr->grp_stack[phr->grp_depth].s2 = (char *)malloc(strlen(s2)+1);
      if ( phr->grp_stack[phr->grp_depth].s2 == NULL )
      {
         if ( phr->grp_stack[phr->grp_depth].s1 != NULL )
         {
            free(phr->grp_stack[phr->grp_depth].s1);
         }
         sc_phr_Error(phr, "memory");
         return 0;
      }
      strcpy( phr->grp_stack[phr->grp_depth].s2, sc_strupr(s2) );
   }

   phr->grp_depth++;

   if ( sc_phr_CallAux(phr, SC_MSG_GRP_START,
               (void *)(phr->grp_stack+phr->grp_depth-1)) == 0 )
      return 0;

   return 1;
}

int sc_phr_pop_grp(sc_phr_type phr)
{
   assert( phr->grp_depth > 0 );
   if ( sc_phr_CallAux(phr, SC_MSG_GRP_END,
               (void *)(phr->grp_stack+phr->grp_depth-1)) == 0 )
      return 0;
   phr->grp_depth--;
   if ( phr->grp_stack[phr->grp_depth].s1 != NULL )
      free(phr->grp_stack[phr->grp_depth].s1);
   if ( phr->grp_stack[phr->grp_depth].s2 != NULL )
      free(phr->grp_stack[phr->grp_depth].s2);
   return 1;
}

char *sc_phr_get_grp_stack_str(sc_phr_type phr)
{
   static char s[1024];
   int i;
   s[0] = '\0';
   for( i = 0; i < phr->grp_depth; i++ )
   {
      sprintf(s+strlen(s), "/%s", phr->grp_stack[i].s1);
   }
   return s;
}


char *sc_phr_single_str_arg(sc_phr_type phr)
{
   char *s = "";
   if ( sc_phr_GetCurr(phr) == '(' )
   {
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
      /* s = sc_phr_GetIdentifier(phr); */
      s = sc_phr_GetString(phr);
      if ( sc_phr_GetCurr(phr) != ')' )
      {
         sc_phr_Error(phr, "missing ')' after single string argument '%s'", s);
         return NULL;
      }
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
   }
   return s;
}

int sc_phr_double_str_arg(sc_phr_type phr, char **s1, char **s2)
{
   static char t1[SC_IO_STR_LEN];
   static char t2[SC_IO_STR_LEN];
   char *s;
   
   t1[0] = '\0';
   t2[0] = '\0';
   
   if ( s1 != NULL )
      *s1 = t1;
   if ( s2 != NULL )
      *s2 = t2;
   
   if ( sc_phr_GetCurr(phr) == '(' )
   {
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
      s = sc_phr_GetString(phr);
      if ( s == NULL )
         return 0;
      strcpy( t1, s);
      if ( sc_phr_GetCurr(phr) != ',' )
      {
         sc_phr_Error(phr, "missing ','");
         return 0;
      }
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
      s = sc_phr_GetString(phr);
      if ( s == NULL )
         return 0;
      strcpy( t2, s);
      if ( sc_phr_GetCurr(phr) != ')' )
      {
         sc_phr_Error(phr, "missing ')'");
         return 0;
      }
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
   }
   return 1;
}

int sc_phr_triple_str_arg(sc_phr_type phr, char **s1, char **s2, long *val)
{
   static char t1[SC_IO_STR_LEN];
   static char t2[SC_IO_STR_LEN];
   static long v;
   char *s;
   
   t1[0] = '\0';
   t2[0] = '\0';
   
   if ( s1 != NULL )
      *s1 = t1;
   if ( s2 != NULL )
      *s2 = t2;
   
   if ( sc_phr_GetCurr(phr) == '(' )
   {
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
      s = sc_phr_GetString(phr);
      if ( s == NULL )
         return 0;
      strcpy( t1, s);
      if ( sc_phr_GetCurr(phr) != ',' )
      {
         sc_phr_Error(phr, "missing ','");
         return 0;
      }
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
      s = sc_phr_GetString(phr);
      if ( s == NULL )
         return 0;
      strcpy( t2, s);
      if ( sc_phr_GetCurr(phr) != ',' )
      {
         sc_phr_Error(phr, "missing ','");
         return 0;
      }
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
      v = sc_phr_GetDec(phr);      
      if ( sc_phr_GetCurr(phr) != ')' )
      {
         sc_phr_Error(phr, "missing ')'");
         return 0;
      }
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
   }
   if ( val != NULL )
      *val = v;
   return 1;
}


/*
   0: error
   1: process without error
   2: nothing done (attribut not found)
*/
int sc_phr_simple_attr(sc_phr_type phr, char *identifier)
{
   int curr_grp = sc_phr_GetCurrGroupIndex(phr);
   char *grp_name;
   sc_sdef_type sd;
   int attr_idx;
   int typ;
   char *name;
   
   sc_phr_ClrArgs(phr);   
   grp_name = sc_sld_GetGroupNameByIndex(phr->sld, curr_grp);
   sd = sc_sld_GetSDEFByName(phr->sld, grp_name);
   if ( sd == NULL )
      return 0;
   attr_idx = sc_sdef_GetIndex(sd, identifier);
   if ( attr_idx < 0 )
      return 2;
   typ = sc_sdef_GetAttrTypByIndex(sd, attr_idx);
   name = sc_sdef_GetAttrNameByIndex(sd, attr_idx);
   /*
   puts(identifier);
   if ( strcmp(identifier, "input_voltage") == 0 )
      puts("input_voltage");
   */
   if ( sc_phr_GetCurr(phr) != ':' )
   {
      sc_phr_Error(phr, "':' expected after simple attribute '%s'", identifier);
      return 0;
   }
   sc_phr_GetNext(phr);
   sc_phr_SkipSpace(phr);
   switch(typ)
   {
      case SC_SDEF_TYP_STRING:
         sc_phr_AddStrArg(phr, sc_phr_GetString(phr));
         /* printf("%s\n", sc_phr_GetString(phr)); */
         break;
      case SC_SDEF_TYP_MSTRING:
         sc_phr_AddStrArg(phr, sc_phr_GetMString(phr));
         /* printf("%s\n", sc_phr_GetString(phr)); */
         break;
      case SC_SDEF_TYP_INTEGER:
         sc_phr_AddLongArg(phr, sc_phr_GetDec(phr));
         break;
      case SC_SDEF_TYP_FLOAT:
         sc_phr_AddDoubleArg(phr, sc_phr_GetDouble(phr));
         break;
      case SC_SDEF_TYP_IDENTIFIER:
         sc_phr_AddStrArg(phr, sc_phr_GetIdentifier(phr));
         break;
      case SC_SDEF_TYP_BOOLEAN:
         sc_phr_AddStrArg(phr, sc_phr_GetIdentifier(phr));
         break;
      default:
         assert(0);
         break;
   }
   
   if ( sc_phr_GetCurr(phr) != ';' )
   {
      if ( phr->is_relax == 0 )
      {
         sc_phr_Error(phr, "';' expected after simple attribute");
         return 0;
      }
   }
   else
   {
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
   }
   
   return sc_phr_CallAux(phr, SC_MSG_ATTRIB, (void *)name);
}

/*
   0: error
   1: process without error
   2: nothing done (attribut not found)
*/
int sc_phr_complex_attr(sc_phr_type phr, char *identifier)
{
   int curr_grp = sc_phr_GetCurrGroupIndex(phr);
   char *grp_name;
   sc_cdef_type cd;
   int attr_idx;
   int *typ;
   char *name;
   
   sc_phr_ClrArgs(phr);   
   grp_name = sc_sld_GetGroupNameByIndex(phr->sld, curr_grp);
   cd = sc_sld_GetCDEFByName(phr->sld, grp_name);
   if ( cd == NULL )
      return 0;
   attr_idx = sc_cdef_GetIndex(cd, identifier);
   if ( attr_idx < 0 )
      return 2;
   typ = sc_cdef_GetAttrTypByIndex(cd, attr_idx);
   name = sc_cdef_GetAttrNameByIndex(cd, attr_idx);
   if ( sc_phr_GetCurr(phr) != '(' )
   {
      sc_phr_Error(phr, "'(' expected after complex attribute '%s'", identifier);
      return 0;
   }
   sc_phr_GetNext(phr);
   sc_phr_SkipSpace(phr);
   while( *typ != SC_CDEF_TYP_END )
   {
      switch(*typ)
      {
         case SC_CDEF_TYP_MSTRING:
            sc_phr_AddStrArg(phr, sc_phr_GetMString(phr));
            /* printf("%s\n", ); */
            break;
         case SC_CDEF_TYP_STRING:
            sc_phr_AddStrArg(phr, sc_phr_GetString(phr));
            /* printf("%s\n", ); */
            break;
         case SC_CDEF_TYP_INTEGER:
            sc_phr_AddLongArg(phr, sc_phr_GetDec(phr));
            break;
         case SC_CDEF_TYP_FLOAT:
            sc_phr_AddDoubleArg(phr, sc_phr_GetDouble(phr));
            break;
         case SC_CDEF_TYP_IDENTIFIER:
            sc_phr_AddStrArg(phr, sc_phr_GetIdentifier(phr));
            break;
      }
      typ++;
      if ( *typ != SC_CDEF_TYP_END )
      {
         if ( sc_phr_GetCurr(phr) != ',' )
         {
            sc_phr_Error(phr, "complex attribute: ',' expected");
            return 0;
         }         
         sc_phr_GetNext(phr);
         sc_phr_SkipSpace(phr);
      }
   }
   if ( sc_phr_GetCurr(phr) != ')' )
   {
      sc_phr_Error(phr, "')' expected after complex attribute");
      return 0;
   }
   sc_phr_GetNext(phr);
   sc_phr_SkipSpace(phr);
   if ( sc_phr_GetCurr(phr) != ';' )
   {
      if ( phr->is_relax == 0 )
      {
         sc_phr_Error(phr, "';' expected after simple attribute");
         return 0;
      }
   }
   else
   {
      sc_phr_GetNext(phr);
      sc_phr_SkipSpace(phr);
   }
   sc_phr_CallAux(phr, SC_MSG_ATTRIB, (void *)name);

   return 1;
}

/*
   0: error
   1: process without error
   2: nothing done
*/
int sc_phr_group(sc_phr_type phr, char *identifier, int idx)
{
   char *s;
   char *s1 = NULL;
   char *s2 = NULL;
   long val = 0;
   if ( identifier == NULL )
      return 0;
   if ( sc_strcmp(identifier, sc_sld_GetGroupNameByIndex(phr->sld, idx)) == 0 )
   {
      if ( sc_strcmp(identifier, sc_str_ff) == 0    ||
           sc_strcmp(identifier, sc_str_latch) == 0 ||
           sc_strcmp(identifier, sc_str_statetable) == 0 ||
           sc_strcmp(identifier, sc_str_state) == 0)
      {
         if ( sc_phr_double_str_arg(phr, &s1, &s2) == 0 )
            return 0;
      }
      else if ( sc_strcmp(identifier, sc_str_ff_bank) == 0 || 
                sc_strcmp(identifier, sc_str_latch_bank) == 0 )
      {
         if ( sc_phr_triple_str_arg(phr, &s1, &s2, &val) == 0 )
            return 0;
      }
      else
      {
         /*
         if ( sc_strcmp(identifier, "wire_load") == 0 )
            printf("wire_load\n");
         */
         s1 = sc_phr_single_str_arg(phr);
         if ( s1 == NULL )
            return 0;
      }
      if ( sc_phr_push_grp(phr, idx, s1, s2, val) == 0 )
         return 0;
      /* puts(sc_phr_get_grp_stack_str(phr)); */
      if ( sc_phr_GetCurr(phr) == '{' )
      {
         sc_phr_GetNext(phr);
         sc_phr_SkipSpace(phr);

         sc_phr_CallAux(phr, SC_MSG_IND_DO,
            (void *)sc_phr_GetIndicatorStruct(phr));

         for(;;)
         {
            s = sc_phr_GetIdentifier(phr);
            if ( s == NULL )
               return 0;
            if ( s[0] != '\0' )
            {
               int ret;
               ret = sc_phr_complex_attr(phr, s);
               if ( ret == 2 )
               {
                  ret = sc_phr_simple_attr(phr, s);
                  if ( ret == 2 )
                  {
                     ret = sc_phr_all_groups(phr, s);
                     if ( ret == 2 )
                     {
                        /* ... */
                        sc_phr_Error(phr, "unknown keyword '%s'", s);
                        return 0;
                     }
                  }
               }
               if ( ret == 0 )
                  return 0;
            }
            else
            {
               break;
            }
         }

         if ( sc_phr_GetCurr(phr) != '}' )
         {
            sc_phr_Error(phr, "'}' expected");
            return 0;
         }
         sc_phr_GetNext(phr);
         sc_phr_SkipSpace(phr);
         
         /* skip another ';' */
         if ( phr->is_relax != 0 && sc_phr_GetCurr(phr) == ';' )
         {
            sc_phr_GetNext(phr);
            sc_phr_SkipSpace(phr);
         }

      }
      if ( sc_phr_pop_grp(phr) == 0 )
        return 0;
      return 1;
   }
   return 2;
}

/*
   0: error
   1: process without error
   2: nothing done
*/
int sc_phr_all_groups(sc_phr_type phr, char *identifier)
{
   int idx;
   int ret;
   for( idx = 0; idx < sc_sld_GetSDEFCnt(phr->sld); idx++ )
   {
      ret = sc_phr_group(phr, identifier, idx);
      if ( ret == 0 )
         return 0;
      if ( ret == 1 )
         return 1;
   }
   return 2;
}

int sc_phr_file(sc_phr_type phr)
{
   char *s;
   int ret;

   sc_phr_CallAux(phr, SC_MSG_IND_START,
      (void *)sc_phr_GetIndicatorStruct(phr));

   while(phr->grp_depth > 0)
      if ( sc_phr_pop_grp(phr) == 0 )
        return 0;

   sc_phr_SkipSpace(phr);
   s = sc_phr_GetIdentifier(phr);
   if ( s == NULL )
      return 0;
   ret = sc_phr_all_groups(phr, s);
   if ( ret == 2 )
   {
      sc_phr_Error(phr, "unknown keyword '%s'", s);
      return 0;
   }

   sc_phr_CallAux(phr, SC_MSG_IND_DO,
      (void *)sc_phr_GetIndicatorStruct(phr));
   sc_phr_CallAux(phr, SC_MSG_IND_END,
      (void *)sc_phr_GetIndicatorStruct(phr));

   return ret;
}

int sc_phr_Do(sc_phr_type phr)
{
   int ret = sc_phr_file(phr);
   if ( ret == 0 )
   {
      sc_phr_Error(phr, "error found!");
   }
   return ret;
}

/*
int main(void)
{
   sc_phr_type phr;
   phr = sc_phr_Open("lsi_10k.lib");
   if ( phr != NULL )
   {
      sc_phr_Do(phr);
      sc_phr_Close(phr);
   }
   return 0;
}
*/

