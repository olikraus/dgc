/*

   SC_UTIL.C

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
   
   29.06.96  Oliver Kraus
   
*/

#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "sc_io.h"
#include "mwc.h"

int sc_ipatmat(char *raw, char *pat)
{
   if ( *pat == '\0' )
      return *raw == '\0';                  /*  *raw == '\0' ? 1 : 0  */

   if (*pat != '*')                         /* if pattern is not a '*'*/
   {
      if (*raw == '\0')                     /*  if end of raw then    */
          return( 0 ) ;                     /*     mismatch           */
      while ((*pat == '?') || (tolower((int)(unsigned char)*pat) == tolower((int)(unsigned char)*raw)))  
                                            /*  if chars match then   */
      {
         raw++; pat++;
         if ( *pat == '\0' )
            return *raw == '\0';            /* *raw == '\0' ? 1 : 0   */
      }
   }  /* no else !!! */                     /* no match, if pat is not '*' */
   if (*pat == '*')                         /* if pattern is a '*'    */
   {
      while(*pat == '*')                    /* ignore more '*'        */
         pat++;
      if (*(pat  ) == '\0')                 /*    if it is end of pat */
         return( 1 ) ;                      /*    then match          */

      while( *raw != '\0' )
      {                                     /*    else hunt for match */
         if( tolower((int)(unsigned char)*raw)==tolower((int)(unsigned char)*pat) ||
             *(pat) == '?'  )               /*         or wild card   */
         {
            if (sc_ipatmat(++raw, pat+1) != 0)  /*      if found,match    */
                 return( 1 ) ;              /*        rest of pat     */
         }
         else
         {
            raw++;
         }
      }
   }
   return( 0 ) ;                            /*  no match found        */
}

int sc_strcmp(char *s1, char *s2)
{
   if ( s1 == s2 )
      return 0;
   return strcmp(s1, s2);
}

char *sc_strupr(char *s)
{
   static char t[SC_IO_STR_LEN];
   int i = 0;
   if ( s == NULL )
      return NULL;
   if ( s == t )
      return t;
   for(;;)
   {
      t[i] = (char)toupper((int)s[i]);
      if ( s[i] == '\0' )
         break;
      i++;
   }
   return t;
}

char *sc_strlwr(char *s)
{
   static char t[SC_IO_STR_LEN];
   int i = 0;
   if ( s == NULL )
      return NULL;
   if ( s == t )
      return t;
   for(;;)
   {
      t[i] = (char)tolower((int)s[i]);
      if ( s[i] == '\0' )
         break;
      i++;
   }
   return t;
}



int sc_is_symf(int c)
{
   if ( c >= 'A' && c <= 'Z' )
      return 1;
   if ( c >= 'a' && c <= 'z' )
      return 1;
   if ( c == '_' )
      return 1;
   return 0;
}

int sc_is_sym(int c)
{
   if ( sc_is_symf(c) != 0 )
      return 1;
   if ( c >= '0' && c <= '9' )
      return 1;
   if ( c == '.' )
      return 1;
   if ( c == '-' )
      return 1;
   return 0;
}

char *sc_identifier(char *s)
{
   static char str[SC_IO_STR_LEN];
   int i=0;
   if ( sc_is_symf((int)(unsigned char)*s) != 0 )
   {
      do
      {
         assert(i < SC_IO_STR_LEN-1);
         str[i] = *s;
         i++;
         s++;
      } while( sc_is_sym((int)(unsigned char)*s) != 0 );
   }
   str[i] = '\0';
   return str;
}

char *sc_skip_space(char *s)
{
   if ( s == NULL )
      return NULL;
   for(;;)
   {
      if ( *s == '\0' )
         break;
      if ( *s > ' ' )
         break;
      s++;
   }
   return s;
}
