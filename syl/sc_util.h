/*

   SC_UTIL.H
   
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

#ifndef _SC_UTIL_H
#define _SC_UTIL_H

int sc_ipatmat(char *raw, char *pat);
int sc_strcmp(char *s1, char *s2);
char *sc_strupr(char *s);
char *sc_strlwr(char *s);
int sc_is_symf(int c);
int sc_is_sym(int c);
char *sc_identifier(char *s);
char *sc_skip_space(char *s);

#endif /* _SC_UTIL_H */
