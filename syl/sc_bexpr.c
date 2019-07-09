/*

   SC_BEXPR.C

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

   pharser for synopsys boolean expressions

   17.07.96  Oliver Kraus



   cell-list: one cellname per line, blank lines are ignored, comment: ';' 
   function-file: cell-pattern ':' 
     outpin '=' <synopsys-expr> { ',' outpin '=' <synopsys-expr> }*
     operator: * AND       + OR
               & AND       | OR
               ^ XOR       ! NOT
               ' NOT       ~ NOT
               ? Tri-State (y=enable?input)
               < Buffer (y=<input)
     wildcard: * zero, one or more characters
               ? one character

*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sc_util.h"
#include "sc_bexpr.h"
#include "mwc.h"


sc_ben_type sc_ben_Open(int fn)
{
   sc_ben_type ben;
   ben = (sc_ben_type)malloc(sizeof(sc_ben_struct));
   if ( ben != NULL )
   {
      ben->fn = fn;
      ben->is_invert = 0;
      ben->cnt = 0;
      ben->max = SC_BEN_MAX;
      return ben;
   }
   return NULL;
}

void sc_ben_Close(sc_ben_type ben)
{
   int i;
   if ( ben != NULL )
   {
      for( i = 0; i < ben->cnt; i++ )
      {
         sc_ben_Close(ben->list[i]);
      }
      free(ben);
   }
}

void sc_ben_Append(sc_ben_type ben, sc_ben_type a_ben)
{
   assert(ben->cnt < ben->max);
   ben->list[ben->cnt++] = a_ben;
}

/*
   get_plain == 0:
      NOT AND --> NAND
      NOT OR --> NOR
*/
void sc_ben_InvertConverter(sc_ben_type ben, int get_plain)
{
   int i;
   if ( ben == NULL )
      return;
   switch(ben->fn)
   {
      case SC_BEN_FN_AND:
         if ( get_plain == 0 )
         {
            if ( ben->is_invert != 0 )
            {
               ben->fn = SC_BEN_FN_NAND;
               ben->is_invert = 0;
            }
         }
         break;
      case SC_BEN_FN_OR:
         if ( get_plain == 0 )
         {
            if ( ben->is_invert != 0 )
            {
               ben->fn = SC_BEN_FN_NOR;
               ben->is_invert = 0;
            }
         }
         break;
      case SC_BEN_FN_DFF:
         if ( get_plain == 0 )
         {
            if ( ben->is_invert != 0 )
            {
               ben->fn = SC_BEN_FN_NDFF;
               ben->is_invert = 0;
            }
         }
         break;
      case SC_BEN_FN_DL:
         if ( get_plain == 0 )
         {
            if ( ben->is_invert != 0 )
            {
               ben->fn = SC_BEN_FN_NDL;
               ben->is_invert = 0;
            }
         }
         break;
      case SC_BEN_FN_XOR:
         if ( get_plain == 0 )
         {
            if ( ben->is_invert != 0 )
            {
               ben->fn = SC_BEN_FN_EQV;
               ben->is_invert = 0;
            }
         }
         break;
      case SC_BEN_FN_NAND:
         if ( ben->is_invert != 0 || get_plain != 0 )
         {
            ben->fn = SC_BEN_FN_AND;
            ben->is_invert = !ben->is_invert;
         }
         break;
      case SC_BEN_FN_NOR:
         if ( ben->is_invert != 0 || get_plain != 0 )
         {
            ben->fn = SC_BEN_FN_OR;
            ben->is_invert = !ben->is_invert;
         }
         break;
      case SC_BEN_FN_NDFF:
         if ( ben->is_invert != 0 || get_plain != 0 )
         {
            ben->fn = SC_BEN_FN_DFF;
            ben->is_invert = !ben->is_invert;
         }
         break;
      case SC_BEN_FN_NDL:
         if ( ben->is_invert != 0 || get_plain != 0 )
         {
            ben->fn = SC_BEN_FN_DL;
            ben->is_invert = !ben->is_invert;
         }
         break;
      case SC_BEN_FN_EQV:
         if ( ben->is_invert != 0 || get_plain != 0 )
         {
            ben->fn = SC_BEN_FN_XOR;
            ben->is_invert = !ben->is_invert;
         }
         break;
   }
   for( i = 0; i < ben->cnt; i++ )
   {
      sc_ben_InvertConverter(ben->list[i], get_plain);
   }
}

/*-----------------------------------------------------------------------------*/

sc_ben_type sc_be_expr(sc_be_type be);


char *sc_be_identifier(char *s)
{
   return sc_strupr(sc_identifier(s));
}

char *sc_ben_GetGndVccIdentifier(sc_ben_type ben)
{
   static char s[SC_IO_STR_LEN];
   if ( ben->fn == SC_BEN_FN_0 )
      return "GND";
   if ( ben->fn == SC_BEN_FN_1 )
      return "VCC";
   if ( ben->fn == SC_BEN_FN_ID )
   {
      sprintf(s, "\"%s\"",sc_be_identifier(ben->id));
      return s;
   }
   return "";
}

sc_be_type sc_be_Open(void)
{
   sc_be_type be;
   be = (sc_be_type)malloc(sizeof(sc_be_struct));
   if ( be != NULL )
   {
      be->def = NULL;
      be->ben = NULL;
      be->strl = sc_strl_Open();
      if ( be->strl != NULL )
      {
        return be;
      }
      free(be);
   }
   return NULL;
}

void sc_be_Close(sc_be_type be)
{
   if ( be != NULL )
   {
      sc_strl_Close(be->strl);
      if ( be->ben != NULL )
      {
         sc_ben_Close(be->ben);
      }
      if ( be->def != NULL )
      {
         free(be->def);
      }
      free(be);
   }
}

void sc_be_SkipSpace(sc_be_type be)
{
   for(;;)
   {
      if ( *be->pos == '\0' )
         break;
      if ( *be->pos > ' ' )
         break;
      be->pos++;
   }
}

int sc_be_SetExpr(sc_be_type be, char *def)
{
   if ( be->def != NULL )
   {
      free(be->def);
   }
   be->def = malloc(strlen(def)+1);
   if ( be->def == NULL )
    return 0;
   strcpy(be->def, def);
   be->pos = be->def;   
   sc_be_SkipSpace(be);
   return 1;
}

sc_ben_type sc_be_dff(sc_be_type be, int typ)
{
   int i;
   sc_ben_type ret, arg;
   if ( (int)*be->pos != '(' )
   {
      puts("bool-expr: '(' expected");
      return NULL;
   }
   be->pos++;
   sc_be_SkipSpace(be);

   ret = sc_ben_Open(typ);
   if ( ret == NULL )
      return NULL;

   for(i = 0; i < 4; i++ )
   {  
      if ( i != 0 )
      {
         if ( (int)*be->pos != ',' )
         {
            puts("bool-expr: ',' expected");
            sc_ben_Close(ret);
            return NULL;
         }
         be->pos++;
         sc_be_SkipSpace(be);
      }      
      arg = sc_be_expr(be);
      sc_ben_Append(ret, arg);
   }
   
   if ( (int)*be->pos != ')' )
   {
      puts("bool-expr: ')' expected");
      sc_ben_Close(ret);
      return NULL;
   }
   be->pos++;
   sc_be_SkipSpace(be);
   return ret;
}


sc_ben_type sc_be_atom(sc_be_type be)
{
   sc_ben_type ret;
   switch((int)*be->pos)
   {
      case '\0':
         puts("bool-expr: illegal end");
         return NULL;      
      case '(':
         be->pos++;
         sc_be_SkipSpace(be);
         ret = sc_be_expr(be);
         if ( ret == NULL )
            return NULL;
         if ( *be->pos != ')' )
         {
            puts("bool-expr: ')' expected");
            return NULL;
         }
         be->pos++;
         sc_be_SkipSpace(be);
         break;
      case '!':
         be->pos++;
         sc_be_SkipSpace(be);
         ret = sc_be_atom(be);
         if ( ret == NULL )
            return NULL;
         ret->is_invert = !ret->is_invert;
         break;
      case '0':
         be->pos++;
         sc_be_SkipSpace(be);
         ret = sc_ben_Open(SC_BEN_FN_0);
         if ( ret == NULL )
            return NULL;
         break;         
      case '1':
         be->pos++;
         sc_be_SkipSpace(be);
         ret = sc_ben_Open(SC_BEN_FN_1);
         if ( ret == NULL )
            return NULL;
         break;         
      default:
         {
            char *s;
            char *pos = be->pos;
            s = sc_be_identifier(be->pos);
            if ( s[0] == '\0' )
            {
               puts("bool-expr: unknown identifier");
               return NULL;
            }
            be->pos += strlen(s);
            sc_be_SkipSpace(be);
            
            if ( strcmp(s, "DFF") == 0 )
            {
               ret = sc_be_dff(be, SC_BEN_FN_DFF);
            }
            else if ( strcmp(s, "DL") == 0 )
            {
               ret = sc_be_dff(be, SC_BEN_FN_DL);
            }
            else
            {
               ret = sc_ben_Open(SC_BEN_FN_ID);
               if ( ret == NULL )
                  return NULL;
               ret->id = pos;
            }
         }
         break;         
   }
   if ( *be->pos == '\'' || *be->pos == '~' )
   {
      be->pos++;
      sc_be_SkipSpace(be);
      if ( ret == NULL )
         return NULL;
      ret->is_invert = !ret->is_invert;
   }
   return ret;
}

sc_ben_type sc_be_buffer(sc_be_type be)
{
   sc_ben_type next, ret;
   ret = NULL;
   if( *be->pos == '<' )
   {
      be->pos++;
      sc_be_SkipSpace(be);
      next = sc_be_atom(be);
      if ( next == NULL )
         return NULL;
      ret = sc_ben_Open(SC_BEN_FN_BUF);
      if ( ret == NULL )
      {
         sc_ben_Close(next);
         return NULL;
      }
      sc_ben_Append(ret, next);
      return ret;
   }
   return sc_be_atom(be);
}



sc_ben_type sc_be_tristate(sc_be_type be)
{
   sc_ben_type first, next, ret;
   ret = NULL;
   first = sc_be_buffer(be);
   if ( first == NULL )
      return NULL;
   if( *be->pos == '?' )
   {
      be->pos++;
      sc_be_SkipSpace(be);
      next = sc_be_buffer(be);
      if ( ret == NULL )
      {
         ret = sc_ben_Open(SC_BEN_FN_TRI);
         if ( ret == NULL )
         {
            sc_ben_Close(first);
            return NULL;
         }
         sc_ben_Append(ret, first);
      }
      if ( next == NULL )
      {
         sc_ben_Close(ret);
         return NULL;
      }
      sc_ben_Append(ret, next);
   }
   if ( ret == NULL )
      return first;
   return ret;
}


sc_ben_type sc_be_xor(sc_be_type be)
{
   sc_ben_type first, next, ret;
   ret = NULL;
   first = sc_be_tristate(be);
   if ( first == NULL )
      return NULL;
   while( *be->pos == '^' )
   {
      be->pos++;
      sc_be_SkipSpace(be);
      next = sc_be_tristate(be);
      if ( ret == NULL )
      {
         ret = sc_ben_Open(SC_BEN_FN_XOR);
         if ( ret == NULL )
         {
            sc_ben_Close(first);
            return NULL;
         }
         sc_ben_Append(ret, first);
      }
      if ( next == NULL )
      {
         sc_ben_Close(ret);
         return NULL;
      }
      sc_ben_Append(ret, next);
   }
   if ( ret == NULL )
      return first;
   return ret;
}

sc_ben_type sc_be_and(sc_be_type be)
{
   sc_ben_type first, next, ret;
   ret = NULL;
   first = sc_be_xor(be);
   if ( first == NULL )
      return NULL;
   while( *be->pos == '&' 
      || *be->pos == '*'
      || *be->pos == '('
      || *be->pos == '\"'
      || *be->pos == '!'
      || sc_is_symf((int)*be->pos) != 0 )
   {
      if ( *be->pos == '&' || *be->pos == '*' )
      {
         be->pos++;
         sc_be_SkipSpace(be);
      }
      next = sc_be_xor(be);
      if ( ret == NULL )
      {
         ret = sc_ben_Open(SC_BEN_FN_AND);
         if ( ret == NULL )
         {
            sc_ben_Close(first);
            if ( next != NULL )
              sc_ben_Close(next);
            return NULL;
         }
         sc_ben_Append(ret, first);
      }
      if ( next == NULL )
      {
         sc_ben_Close(ret);
         return NULL;
      }
      sc_ben_Append(ret, next);
   }
   if ( ret == NULL )
      return first;
   return ret;
}

sc_ben_type sc_be_or(sc_be_type be)
{
   sc_ben_type first, next, ret;
   ret = NULL;
   first = sc_be_and(be);
   if ( first == NULL )
      return NULL;
   while( *be->pos == '+' 
      || *be->pos == '|' )
   {
      be->pos++;
      sc_be_SkipSpace(be);
      next = sc_be_and(be);
      if ( ret == NULL )
      {
         ret = sc_ben_Open(SC_BEN_FN_OR);
         if ( ret == NULL )
         {
            sc_ben_Close(first);
            if ( next != NULL )
              sc_ben_Close(next);
            return NULL;
         }
         sc_ben_Append(ret, first);
      }
      if ( next == NULL )
      {
         sc_ben_Close(ret);
         return NULL;
      }
      sc_ben_Append(ret, next);
   }
   if ( ret == NULL )
      return first;
   return ret;
}


sc_ben_type sc_be_expr(sc_be_type be)
{
   return sc_be_or(be);
}


int sc_be_GenerateTree(sc_be_type be, char *def)
{
   if ( be == NULL )
      return 0;
   if ( sc_be_SetExpr(be, def) == 0 )
      return 0;
   if ( be->ben != NULL )
      sc_ben_Close(be->ben);
   be->ben = sc_be_expr(be);
   if ( be->ben == NULL )
      return 0;
   sc_ben_InvertConverter(be->ben, 0);
   return 1;
}

void sc_be_ben_to_silc_syn(sc_be_type be, sc_ben_type ben, int *pos)
{
   int i;
   if ( ben->is_invert != 0 )
   {
      *pos += sprintf( be->str+(*pos), "(NOT " );
   }
   if ( ben->fn == SC_BEN_FN_ID )
   {
      *pos += sprintf( be->str+(*pos), "%s", sc_be_identifier(ben->id) );
   }
   else
   {
      *pos += sprintf( be->str+(*pos), "(" );
      switch(ben->fn)
      {
         case SC_BEN_FN_0:
            *pos += sprintf( be->str+(*pos), "0 " );
            break;
         case SC_BEN_FN_1:
            *pos += sprintf( be->str+(*pos), "1 " );
            break;
         case SC_BEN_FN_XOR:
            *pos += sprintf( be->str+(*pos), "XOR " );
            break;
         case SC_BEN_FN_AND:
            *pos += sprintf( be->str+(*pos), "AND " );
            break;
         case SC_BEN_FN_OR:
            *pos += sprintf( be->str+(*pos), "OR " );
            break;      
         case SC_BEN_FN_EQV:
            *pos += sprintf( be->str+(*pos), "EQV " );
            break;
         case SC_BEN_FN_NAND:
            *pos += sprintf( be->str+(*pos), "NAND " );
            break;
         case SC_BEN_FN_NOR:
            *pos += sprintf( be->str+(*pos), "NOR " );
            break;
         case SC_BEN_FN_TRI:
            /* puts("no silcsyn support for tri-states"); */
            *pos += sprintf( be->str+(*pos), "TRI-STATE " );
            break;
         case SC_BEN_FN_BUF:
            /* puts("no silcsyn support for buffer"); */
            *pos += sprintf( be->str+(*pos), "BUF " );
            break;
         case SC_BEN_FN_DFF:
            /* puts("no silcsyn support for buffer"); */
            *pos += sprintf( be->str+(*pos), "DFF " );
            break;
         case SC_BEN_FN_DL:
            /* puts("no silcsyn support for buffer"); */
            *pos += sprintf( be->str+(*pos), "DL " );
            break;
         case SC_BEN_FN_NDFF:
            /* puts("no silcsyn support for buffer"); */
            *pos += sprintf( be->str+(*pos), "NDFF " );
            break;
         case SC_BEN_FN_NDL:
            /* puts("no silcsyn support for buffer"); */
            *pos += sprintf( be->str+(*pos), "NDL " );
            break;
      }
      for( i = 0; i < ben->cnt; i++ )
      {
         sc_be_ben_to_silc_syn(be, ben->list[i], pos);
         if ( i+1 < ben->cnt )
         {
            *pos += sprintf( be->str+(*pos), " " );         
         }
      }
      *pos += sprintf( be->str+(*pos), ")" );
   }
   if ( ben->is_invert != 0 )
   {
      *pos += sprintf( be->str+(*pos), ")" );
   }
}

char *sc_be_GetSilcSynForm(sc_be_type be)
{
   int i = 0;
   if ( be == NULL )
      return "";
   if ( be->ben == NULL )
      return "";
   sc_ben_InvertConverter(be->ben, 1);     /* don't use NAND etc */
   sc_be_ben_to_silc_syn(be, be->ben, &i);
   be->str[i] = '\0';   
   return be->str;
}


/*
   *erg:
   Bit 0/1:   0 not inverted
              1 inverted
              2,3 XOR
   higher bits: found inversions
*/
void sc_be_ben_get_effect(sc_be_type be, sc_ben_type ben, char *in, unsigned *erg, int *cnt)
{
   int i;
   if ( ben->is_invert != 0 )
   {
      *erg = (*erg) ^ 1; /* invert lowest bit */
   }
   if ( ben->fn == SC_BEN_FN_ID )
   {
      if ( sc_strcmp(sc_be_identifier(ben->id), in) == 0 )
      {
         *erg = (*erg) << 2;
         (*cnt)++;
      }
   }
   else
   {
      switch(ben->fn)
      {
         case SC_BEN_FN_0:
            break;
         case SC_BEN_FN_1:
            break;
         case SC_BEN_FN_XOR:
            *erg = (*erg) | 2;
            break;
         case SC_BEN_FN_AND:
            break;
         case SC_BEN_FN_OR:
            break;      
         case SC_BEN_FN_EQV:
            *erg = (*erg) | 2;
            break;
         case SC_BEN_FN_NAND:
            *erg = (*erg) ^ 1; /* invert lowest bit */
            break;
         case SC_BEN_FN_NOR:
            *erg = (*erg) ^ 1; /* invert lowest bit */
            break;      
      }
      for( i = 0; i < ben->cnt; i++ )
      {
         sc_be_ben_get_effect(be, ben->list[i], in, erg, cnt);
      }
   }
}

/*
   -1 pin not found
   0  not inverted
   1  inverted
   2  xor
*/
int sc_be_GetEffect(sc_be_type be, char *in)
{
   unsigned erg = 0U;
   int cnt = 0;
   int i;
   sc_ben_InvertConverter(be->ben, 1);     /* don't use NAND etc */
   sc_be_ben_get_effect(be, be->ben, in, &erg, &cnt);
   erg>>=2;
   if ( cnt == 0 )
      return 2;
   for( i = 0; i < cnt; i++ )
   {
      if ( ((erg>>(i*2))&3) > 1 )
         return 2;
   }
   if ( erg == 0 )
      return 0;   /* not inverted */
   if ( erg == (unsigned)((0x055555555UL)>>(sizeof(unsigned long)*8-cnt*2)) )
      return 1;
   return 2;
}

char *sc_ben_GetOutPinName(sc_ben_type ben)
{
   if ( ben->fn == SC_BEN_FN_TRI )
   {
      return "Y0";
   }
   if ( ben->fn == SC_BEN_FN_DFF || ben->fn == SC_BEN_FN_DL )
   {
      return "Q";
   }
   if ( ben->fn == SC_BEN_FN_NDFF || ben->fn == SC_BEN_FN_NDL )
   {
      return "QN";
   }
   return "Y";
}

char *sc_ben_GetInPinName(sc_ben_type ben, int n)
{
   static char s[256];
   switch(ben->fn)
   {      
      case SC_BEN_FN_BUF:
         sprintf(s, "A");
         break;
      case SC_BEN_FN_TRI:
         switch(n)
         {
            case 0:
               sprintf(s, "OC");
               break;
            default:
               sprintf(s, "A%d", n-1);
               break;
         }
         break;
      case SC_BEN_FN_DFF:
      case SC_BEN_FN_NDFF:
      case SC_BEN_FN_DL:
      case SC_BEN_FN_NDL:
         switch(n)
         {
            case 0:
               sprintf(s, "PRN");
               break;
            case 1:
               sprintf(s, "D");
               break;
            case 2:
               sprintf(s, "CLK");
               break;
            case 3:
               sprintf(s, "CLRN");
               break;
            default:
               sprintf(s, "A%d", n-4);
               break;
         }
         break;
      default:
         sprintf(s, "A%d", n);
         break;
   }
   return s;
}

void sc_be_get_mdl_connect(sc_be_type be, sc_ben_type ben, char **s, int *curr)
{
   int i, mynum = *curr;


   /*cnt = 0;*/
   for( i = 0; i < ben->cnt; i++ )
   {   
      switch(ben->list[i]->fn)
      {
         case SC_BEN_FN_XOR:
         case SC_BEN_FN_AND:
         case SC_BEN_FN_OR:
         case SC_BEN_FN_EQV:
         case SC_BEN_FN_NAND:
         case SC_BEN_FN_NOR:
         case SC_BEN_FN_TRI:
         case SC_BEN_FN_BUF:
         case SC_BEN_FN_DFF:
         case SC_BEN_FN_NDFF:
         case SC_BEN_FN_DL:
         case SC_BEN_FN_NDL:
            (*curr)++;
            /*
            if ( ben->fn == SC_BEN_FN_TRI )
            {            
               if ( i == 0 )
                  (*s) += sprintf((*s),"    P%d.OC P%d.%s $\n", mynum, *curr, sc_ben_GetOutPinName(ben->list[i]));
               else
                  (*s) += sprintf((*s),"    P%d.A%d P%d.%s $\n", mynum, i-1, *curr, sc_ben_GetOutPinName(ben->list[i]));
            }
            else if ( ben->fn == SC_BEN_FN_BUF )
            {
               (*s) += sprintf(*s,"    P%d.A P%d.%s $\n", mynum, *curr, sc_ben_GetOutPinName(ben->list[i]));
            }
            else
            {
               (*s) += sprintf(*s,"    P%d.A%d P%d.%s $\n", mynum, i, *curr, sc_ben_GetOutPinName(ben->list[i]));
            }
            */
            (*s) += sprintf(*s,"    P%d.%s P%d.%s $\n", 
               mynum, sc_ben_GetInPinName(ben, i), 
               *curr, sc_ben_GetOutPinName(ben->list[i]));

            sc_be_get_mdl_connect(be, ben->list[i], s, curr);
            break;
         case SC_BEN_FN_ID:
         case SC_BEN_FN_0:
         case SC_BEN_FN_1:
            if ( ben->list[i]->is_invert != 0 )
            {
               (*curr)++;
               /* (*s) += sprintf((*s),"    P%d.A%d P%d.%s $\n", mynum, i, *curr, sc_ben_GetOutPinName(ben->list[i])); */
               (*s) += sprintf(*s,"    P%d.%s P%d.%s $\n", 
                  mynum, sc_ben_GetInPinName(ben, i), 
                  *curr, sc_ben_GetOutPinName(ben->list[i]));
               (*s) += sprintf((*s),"    P%d.A %s $\n", *curr, sc_ben_GetGndVccIdentifier(ben->list[i]));
            }
            else
            {
               /*
               if ( ben->fn == SC_BEN_FN_TRI )
               {            
                  if ( i == 0 )
                     (*s) += sprintf((*s),"    P%d.OC %s $\n", mynum, sc_ben_GetGndVccIdentifier(ben->list[i]));
                  else
                     (*s) += sprintf((*s),"    P%d.A%d %s $\n", mynum, i-1, sc_ben_GetGndVccIdentifier(ben->list[i]));
               }
               else if ( ben->fn == SC_BEN_FN_BUF )
               {
                  (*s) += sprintf((*s),"    P%d.A %s $\n", mynum, sc_ben_GetGndVccIdentifier(ben->list[i]));
               }
               else
               {
                  (*s) += sprintf((*s),"    P%d.A%d %s $\n", mynum, i, sc_ben_GetGndVccIdentifier(ben->list[i]));
               }
               */
               (*s) += sprintf(*s,"    P%d.%s %s $\n", 
                  mynum, sc_ben_GetInPinName(ben, i), 
                  sc_ben_GetGndVccIdentifier(ben->list[i]));
            }   
            break;
      }
      /* cnt++; */
   }
   
}

char *sc_be_GetMDLConnection(sc_be_type be, char *outname, int *start)
{
   static char t[8192];
   char *s = t;
   t[0] = '\0';
   sc_ben_InvertConverter(be->ben, 0);     /* use NAND etc */
   if (  be->ben->fn == SC_BEN_FN_0 ||
         be->ben->fn == SC_BEN_FN_1 ||
         be->ben->fn == SC_BEN_FN_ID )
   {
      if ( be->ben->is_invert != 0 )
      {
         s += sprintf(s,"    \"%s\" P%d.%s $\n", outname, *start, sc_ben_GetOutPinName(be->ben));
         s += sprintf(s,"    P%d.A %s $\n", *start, sc_ben_GetGndVccIdentifier(be->ben));
         (*start)++;
      }
      else
      {
         s += sprintf(s,"    \"%s\" %s$\n", outname, sc_ben_GetGndVccIdentifier(be->ben));
      }
   }
   else
   {
      s += sprintf(s,"    P%d.%s \"%s\" $\n", *start, sc_ben_GetOutPinName(be->ben), outname);
      sc_be_get_mdl_connect(be, be->ben, &s, start);
      (*start)++;
   } 
    
   return t;
}

void sc_be_get_mdl_pins(sc_be_type be, sc_ben_type ben, char **s, int *curr)
{
   int i;

   for( i = 0; i < ben->cnt; i++ )
   {   
      switch(ben->list[i]->fn)
      {
         case SC_BEN_FN_XOR:
         case SC_BEN_FN_AND:
         case SC_BEN_FN_OR:
         case SC_BEN_FN_EQV:
         case SC_BEN_FN_NAND:
         case SC_BEN_FN_NOR:
         case SC_BEN_FN_TRI:
         case SC_BEN_FN_BUF:
         case SC_BEN_FN_DFF:
         case SC_BEN_FN_NDFF:
         case SC_BEN_FN_DL:
         case SC_BEN_FN_NDL:
            (*curr)++;
            sc_be_get_mdl_pins(be, ben->list[i], s, curr);
            break;
         case SC_BEN_FN_ID:
            (*s) += sprintf((*s),"%s\n", sc_ben_GetGndVccIdentifier(ben->list[i]));
            break;
      }
   }   
}

char *sc_be_GetMDLPins(sc_be_type be, char *outname, int *start)
{
   static char t[8192];
   char *s = t;
   t[0] = '\0';
   sc_ben_InvertConverter(be->ben, 0);     /* use NAND etc */
   switch(be->ben->fn)
   {
      case SC_BEN_FN_0:
      case SC_BEN_FN_1:
      case SC_BEN_FN_ID:
         s+=sprintf(s,"\"%s\"\n",outname);  
         s+=sprintf(s,"%s\n",sc_ben_GetGndVccIdentifier(be->ben));
         break;
      default:
         s+=sprintf(s,"\"%s\"\n",outname);  
         sc_be_get_mdl_pins(be, be->ben, &s, start);
         (*start)++;
         break;
   }
   return t;
}


void sc_be_get_mdl_part(sc_be_type be, sc_ben_type ben, char **s, int *curr)
{
   int i, mynum = *curr;

   switch(ben->fn)
   {
      case SC_BEN_FN_0:
      case SC_BEN_FN_1:
      case SC_BEN_FN_ID:
         break;
      case SC_BEN_FN_XOR:
         (*s) += sprintf((*s),"    P%d XOR $\n", mynum);
         assert(ben->cnt == 2);
         break;
      case SC_BEN_FN_AND:
         (*s) += sprintf((*s),"    P%d AND/%d $\n", mynum, ben->cnt);
         break;
      case SC_BEN_FN_OR:
         (*s) += sprintf((*s),"    P%d OR/%d $\n", mynum, ben->cnt);
         break;      
      case SC_BEN_FN_EQV:
         (*s) += sprintf((*s),"    P%d NXOR $\n", mynum);
         assert(ben->cnt == 2);
         break;
      case SC_BEN_FN_NAND:
         (*s) += sprintf((*s),"    P%d NAND/%d $\n", mynum, ben->cnt);
         break;
      case SC_BEN_FN_NOR:
         (*s) += sprintf((*s),"    P%d NOR/%d $\n", mynum, ben->cnt);
         break;      
      case SC_BEN_FN_TRI:
         assert(ben->cnt == 2);
         (*s) += sprintf((*s),"    P%d 3S-H/1 $\n", mynum);
         break;
      case SC_BEN_FN_BUF:
         assert(ben->cnt == 1);
         (*s) += sprintf((*s),"    P%d BUF $\n", mynum);
         break;
      case SC_BEN_FN_DFF:
      case SC_BEN_FN_NDFF:
         assert(ben->cnt == 4);
         (*s) += sprintf((*s),"    P%d DFF-E $\n", mynum);
         break;
      case SC_BEN_FN_DL:
      case SC_BEN_FN_NDL:
         assert(ben->cnt == 4);
         (*s) += sprintf((*s),"    P%d DFF-L $\n", mynum);
         break;
   }   
   for( i = 0; i < ben->cnt; i++ )
   {
   
      switch(ben->list[i]->fn)
      {
         case SC_BEN_FN_XOR:
         case SC_BEN_FN_AND:
         case SC_BEN_FN_OR:
         case SC_BEN_FN_EQV:
         case SC_BEN_FN_NAND:
         case SC_BEN_FN_NOR:
         case SC_BEN_FN_TRI:
         case SC_BEN_FN_BUF:
         case SC_BEN_FN_DFF:
         case SC_BEN_FN_NDFF:
         case SC_BEN_FN_DL:
         case SC_BEN_FN_NDL:
            (*curr)++;
            sc_be_get_mdl_part(be, ben->list[i], s, curr);
            break;
         case SC_BEN_FN_ID:
         case SC_BEN_FN_0:
         case SC_BEN_FN_1:
            if ( ben->list[i]->is_invert != 0 )
            {
               (*curr)++;
               (*s) += sprintf((*s),"    P%d INV $\n", *curr);
            }
            break;
      }
   }
}

char *sc_be_GetMDLPart(sc_be_type be, int *start)
{
   static char t[8192];
   char *s = t;
   t[0] = '\0';
   sc_ben_InvertConverter(be->ben, 0);   /* use NAND etc */
   if ( be->ben->is_invert != 0 )
   {
      s += sprintf(s,"    P%d INV $\n", *start);
      (*start)++;
   }
   switch(be->ben->fn)
   {
      case SC_BEN_FN_0:
      case SC_BEN_FN_1:
      case SC_BEN_FN_ID:
         break;
      default:
         sc_be_get_mdl_part(be, be->ben, &s, start);
         (*start)++;
         break;
   }
   return t;
}

#ifdef newcode
#endif

char *sc_be_GetSimpleCombinational(sc_be_type be)
{
   /* int is_simple = 1; */
   int i;
   
   if ( be == NULL )
      return NULL;
   if ( be->ben == NULL )
      return NULL;
      
   sc_ben_InvertConverter(be->ben, 0);     /* we may use NAND etc */

   if ( be->ben->fn == SC_BEN_FN_ID )
   {
      if ( be->ben->is_invert != 0 )
         return "NOT";
      else
         return "BUFFER";
   }
   for( i = 0; i < be->ben->cnt; i++ )
   {
      if ( be->ben->list[i]->fn != SC_BEN_FN_ID )
         return NULL;
      if ( be->ben->list[i]->is_invert != 0 )
         return NULL;
   }
   switch(be->ben->fn)
   {
      case SC_BEN_FN_XOR:
         return "XOR";
      case SC_BEN_FN_AND:
         return "AND";
      case SC_BEN_FN_OR:
         return "OR";
      case SC_BEN_FN_EQV:
         return "EQV";
      case SC_BEN_FN_NAND:
         return "NAND";
      case SC_BEN_FN_NOR:
         return "NOR";
      case SC_BEN_FN_TRI:
         return "TRI";
   }
   return NULL;      
}

void sc_bc_SetEvalFn(sc_be_type be, int (*eval_fn)(void *data, int ref), void *data)
{
  be->eval_fn = eval_fn;
  be->eval_data = data;
}

int _sc_bc_Eval(sc_be_type be, sc_ben_type ben)
{
  int i;
  int val;
  int val2;

  switch(ben->fn)
  {
    case SC_BEN_FN_0: return 0;
    case SC_BEN_FN_1: return 1;
    case SC_BEN_FN_ID: 
      if ( ben->is_invert == 0 )
        return be->eval_fn(be->eval_data, ben->ref);
      return !be->eval_fn(be->eval_data, ben->ref);
  }
  
  if ( ben->cnt == 0 )
    return 0;

  val = _sc_bc_Eval(be, ben->list[0]);
  for( i = 1; i < ben->cnt; i++ )
  {  
    val2 = _sc_bc_Eval(be, ben->list[i]);
    switch(ben->fn)
    {
      case SC_BEN_FN_XOR:  val ^= val2; break;
      case SC_BEN_FN_AND:  val &= val2; break;
      case SC_BEN_FN_OR:   val |= val2; break;
      case SC_BEN_FN_EQV:  val ^= val2; break;
      case SC_BEN_FN_NAND: val &= val2; break;
      case SC_BEN_FN_NOR:  val |= val2; break;
    }
  }
  
  switch(ben->fn)
  {
    case SC_BEN_FN_EQV:
    case SC_BEN_FN_NAND:
    case SC_BEN_FN_NOR:
      if ( ben->is_invert != 0 )
        return val;
      return !val;
  }
  
  if ( ben->is_invert == 0 )
    return val;
  return !val;
}

int sc_bc_Eval(sc_be_type be)
{
  return _sc_bc_Eval(be, be->ben);
}


int _sc_bc_RegisterIdentifier(sc_be_type be, sc_ben_type ben)
{
  if ( ben->fn == SC_BEN_FN_ID )
  {
    int pos;
    char *id = sc_be_identifier(ben->id);
    if ( sc_strl_AddUnique(be->strl, id) == 0 )
      return 0;
    pos = sc_strl_Find(be->strl, id);
    if ( pos < 0 )
      return 0;
    ben->ref = pos;
  }
  else
  {
    int i;
    for( i = 0; i < ben->cnt; i++ )
    {  
      if ( _sc_bc_RegisterIdentifier(be, ben->list[i]) == 0 )
        return 0;
    }
  }
  return 1;
}

int sc_bc_RegisterIdentifier(sc_be_type be)
{
  sc_strl_Clear(be->strl);
  return _sc_bc_RegisterIdentifier(be, be->ben);
}




#ifdef BEX
int main(int argc, char *argv[])
{
   sc_be_type be;
   int start = 1;
   int rstart;
   if ( argc < 2 )
   {
      printf("usage: %s synopsys-boolean-expression [startval]\n", argv[0]);
      return 0;
   }
   if ( argc >= 3 )
   {
      start = atoi(argv[2]);
   }
   be = sc_be_Open();
   puts(argv[1]);
   sc_be_GenerateTree(be, argv[1]);
   puts(sc_be_GetSilcSynForm(be));
   rstart = start;
   puts("Parts");
   puts(sc_be_GetMDLPart(be, &rstart));
   rstart = start;
   puts("Connections");
   puts(sc_be_GetMDLConnection(be, "Y", &rstart));
   rstart = start;
   puts("Pins");
   puts(sc_be_GetMDLPins(be, "Y", &rstart));
   sc_be_Close(be);
   return 0;
}
#endif
