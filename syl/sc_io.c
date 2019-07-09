/*

   SC_IO.C
   
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
   
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "sc_io.h"
#include "sc_util.h"
#include "mwc.h"
#include "b_ff.h"

sc_io_type sc_io_Open(const char *name)
{
   sc_io_type io;
   io = (sc_io_type)malloc(sizeof(sc_io_struct));
   if ( io != NULL )
   {
      io->fp = b_fopen(name, NULL, ".lib", "r");
      if ( io->fp != NULL )
      {
         fseek(io->fp, 0L, SEEK_END);
         io->fend = ftell(io->fp);
         fseek(io->fp, 0L, SEEK_SET);
         io->fcurr = 0L;
         io->pm = 0;
         io->fname = (char *)malloc(strlen(name)+1);
         if ( io->fname != NULL )
         {
            strcpy(io->fname, name);
            io->err_str[0] = '\0';
            io->row = 1;
            io->col = 1;
            sc_io_GetNext(io);
            return io;
         }
         fclose(io->fp);
      }
      free(io);
   }
   return NULL;
}


void sc_io_Close(sc_io_type io)
{
   if ( io != NULL )
   {
      free(io->fname);
      fclose(io->fp);
      free(io);
   }
}

int sc_io_GetNext(sc_io_type io)
{
   io->curr_char = getc(io->fp);
   io->col++;
   if ( io->curr_char == '\n' )
   {
      io->col = 1;
      io->row++;
      io->fcurr = ftell(io->fp);
      if ( io->fend == 0L )
      {
         io->pm = 0;
      }
      else
      {
         io->pm =
            (int)(((long)io->fcurr*1000L + (long)io->fcurr/2)/(long)io->fend);
      }
      /* printf("%4d\r", io->pm); fflush(stdout); */
   }
      
   return sc_io_GetCurr(io);
}

int sc_io_Unget(sc_io_type io, int c)
{
   ungetc(io->curr_char, io->fp);
   io->curr_char = c;
   return 1;
}

int sc_io_SkipSpace(sc_io_type io)
{
   int flag = 0;
   for(;;)
   {
      if ( sc_io_GetCurr(io) == EOF )
         return 0;
      while ( sc_io_GetCurr(io) == '/' )
      {
         if ( sc_io_GetNext(io) != '*' )
         {
            sc_io_Unget(io, (int)(unsigned char)'/');
            return 1;
         }
         flag = 0;
         for(;;)
         {
            while ( sc_io_GetCurr(io) == '*' )
            {
               sc_io_GetNext(io);
               if ( sc_io_GetCurr(io) == '/' )
               {
                  sc_io_GetNext(io);
                  flag = 1;
                  break;
               }
            }
            if ( flag != 0 )
               break;
            if ( sc_io_GetCurr(io) == EOF )
            {
               sc_io_Error(io, "EOF in comment");
               return 0;
            }
            sc_io_GetNext(io);
         }
      }
      if ( sc_io_GetCurr(io) > ' ' )
         return 1;
      sc_io_GetNext(io);
   }
}


char *sc_io_GetIdentifier(sc_io_type io)
{
   int i = 0;   
   char *s = io->identifier;

   if ( sc_is_symf(sc_io_GetCurr(io)) != 0 )
   {
      do
      {
         if ( i < SC_IO_ID_LEN-1 )
         {
            *s++ = (char)sc_io_GetCurr(io);
            i++;
         }
         sc_io_GetNext(io);
      } while( sc_is_sym(sc_io_GetCurr(io)) != 0 );
   }
   
   *s = '\0';
   sc_io_SkipSpace(io);
   return io->identifier;
}

static long sc_io_get_dec(sc_io_type io)
{
   long ret = 0L;
   while ( isdigit(sc_io_GetCurr(io)) != 0 )
   {
      ret *= 10L;
      ret += (long)(sc_io_GetCurr(io)-'0');
      sc_io_GetNext(io);
   }
   return ret;
}

long sc_io_GetDec(sc_io_type io)
{
   long ret = sc_io_get_dec(io);
   sc_io_SkipSpace(io);
   return ret;
}

double sc_io_GetDouble(sc_io_type io)
{
   double ret = 0.0;
   double m, v;
   
   v = 1.0;
   if ( sc_io_GetCurr(io) == '-' )
   {
      v = -1.0;
      sc_io_GetNext(io);
      sc_io_SkipSpace(io);
   }

   while ( isdigit(sc_io_GetCurr(io)) != 0 )
   {
      ret *= 10.0;
      ret += (double)(sc_io_GetCurr(io)-'0');
      sc_io_GetNext(io);
   }
   if ( sc_io_GetCurr(io) == '.' )
   {
      sc_io_GetNext(io);
      m = 1.0;
      while ( isdigit(sc_io_GetCurr(io)) != 0 )
      {
         m /= 10.0;
         ret += m*(double)(sc_io_GetCurr(io)-'0');
         sc_io_GetNext(io);
      }
   }
   sc_io_SkipSpace(io);
   return ret*v;
}

char *sc_io_GetPosStr(sc_io_type io)
{
   static char s[80];
   sprintf(s, "%06ld:%03ld", io->row, io->col );
   return s;
}

void sc_io_Error(sc_io_type io, char *fmt, ...)
{
   va_list l;
   va_start(l, fmt);
   printf("Error at %s: ", sc_io_GetPosStr(io));
   vsprintf(io->err_str, fmt, l);
   va_end(l);
}

char *sc_io_GetNumIdentifier(sc_io_type io)
{
   int i = 0;   
   char *s = io->identifier;

   if ( sc_is_sym(sc_io_GetCurr(io)) != 0 )
   {
      do
      {
         if ( i < SC_IO_ID_LEN-1 )
         {
            *s++ = (char)sc_io_GetCurr(io);
            i++;
         }
         sc_io_GetNext(io);
      } while( sc_is_sym(sc_io_GetCurr(io)) != 0 );
   }
   
   *s = '\0';
   sc_io_SkipSpace(io);
   return io->identifier;
}


char *sc_io_GetString(sc_io_type io)
{
   int i = 0;
   char *s;
   io->string[0] = '\0';
   s = io->string;
   
   if ( sc_is_sym(sc_io_GetCurr(io)) != 0 )
      return sc_io_GetNumIdentifier(io);

  /*      
   if ( sc_io_GetCurr(io) == '\\' )
   {
      sc_io_GetNext(io);
      sc_io_SkipSpace(io);
   }
   */

   
   while ( sc_io_GetCurr(io) == '\"' )
   {
      sc_io_GetNext(io);
      while( sc_io_GetCurr(io) != '\"' && sc_io_GetCurr(io) != EOF )
      {
         if ( i < SC_IO_STR_LEN-1 )
         {
            *s++ = (char)sc_io_GetCurr(io);
            i++;
         }
         sc_io_GetNext(io);
      }
      if ( sc_io_GetCurr(io) == EOF )
      {
         sc_io_Error(io, "EOF in string");
         return NULL;
      }
      sc_io_GetNext(io);
      sc_io_SkipSpace(io);
      while ( sc_io_GetCurr(io) == '\\' )
      {
         sc_io_GetNext(io);
         sc_io_SkipSpace(io);
         if ( i < SC_IO_STR_LEN-1 )
         {
            *s++ = (char)'\n';
            i++;
         }
      }
   }
   *s = '\0';
   return io->string;
}

char *sc_io_GetMString(sc_io_type io)
{
   int i = 0;
   char *s;
   io->string[0] = '\0';
   s = io->string;
   
   if ( sc_is_sym(sc_io_GetCurr(io)) != 0 )
      return sc_io_GetNumIdentifier(io);

   if ( sc_io_GetCurr(io) == '\\' )
   {
      sc_io_GetNext(io);
      sc_io_SkipSpace(io);
   }
   
   while ( sc_io_GetCurr(io) == '\"' )
   {
      sc_io_GetNext(io);
      while( sc_io_GetCurr(io) != '\"' && sc_io_GetCurr(io) != EOF )
      {
         if ( i < SC_IO_STR_LEN-1 )
         {
            *s++ = (char)sc_io_GetCurr(io);
            i++;
         }
         sc_io_GetNext(io);
      }
      if ( sc_io_GetCurr(io) == EOF )
      {
         sc_io_Error(io, "EOF in string");
         return NULL;
      }
      sc_io_GetNext(io);
      sc_io_SkipSpace(io);
      if ( sc_io_GetCurr(io) == ',' )
      {
         sc_io_GetNext(io);
         sc_io_SkipSpace(io);
      }
      while ( sc_io_GetCurr(io) == '\\' )
      {
         sc_io_GetNext(io);
         sc_io_SkipSpace(io);
         if ( i < SC_IO_STR_LEN-1 )
         {
            *s++ = (char)'\n';
            i++;
         }
      }
   }
   *s = '\0';
   return io->string;
}
