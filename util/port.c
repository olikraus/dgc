/*

  port.c
  
  Copyright (C) 2001 Oliver Kraus (olikraus@yahoo.com)

  This file is part of dgc.

  dgc is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  dgc is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  


  TODO: rename all port_xxx functions....
        use ./replace instead...

*/

#ifdef USE_OLD_CODE

#include <strings.h> /* strcasecmp */
#include <ctype.h>

int port_strcasecmp(const char *s1, const char *s2)
{
  return strcasecmp(s1, s2);
  /*
  unsigned char c1, c2;
 
  for(;;)
  {
    c1 = toupper(*(const unsigned char *)s1);
    c2 = toupper(*(const unsigned char *)s2);
    if (c1 == '\0')
      break;
    if ( c1 != c2 )
      break;
    s1++;
    s2++;
  }

  if ( c1 == '\0' && c2 != '\0' )
    return -1;
  if ( c1 != '\0' && c2 == '\0' )
    return 1;
  if ( c1 < c2 )
    return -1;
  if ( c1 > c2 )
    return 1;
  return 0;
  */
}

int port_strncasecmp(const char *s1, const char *s2, size_t n)
{
  unsigned char c1, c2;

  if ( n == 0 )
    return 0;
 
  do
  {
    c1 = toupper(*(const unsigned char *)s1);
    c2 = toupper(*(const unsigned char *)s2);
    if ( c1 == '\0' )
      break;
    if ( c1 != c2 )
      break;
    s1++;
    s2++;
    n--;
  } while( n > 0 );
  
  
  if ( c1 == '\0' && c2 != '\0' )
    return -1;
  if ( c1 != '\0' && c2 == '\0' )
    return 1;
  if ( c1 < c2 )
    return -1;
  if ( c1 > c2 )
    return 1;
  return 0;
}

#endif
