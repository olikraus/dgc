/*

   SC_IO.H

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
   
   A simple scanner for the library files
   
*/

#ifndef _SC_IO_H
#define _SC_IO_H

#include <stdio.h>

#define SC_IO_ID_LEN 1024
#define SC_IO_STR_LEN 4096
#define SC_IO_ERR_LEN 256

struct _sc_io_struct
{
   FILE *fp;
   char *fname;
   long fend, fcurr;
   int pm;
   int curr_char;
   char identifier[SC_IO_ID_LEN];
   char string[SC_IO_STR_LEN];
   char err_str[SC_IO_ERR_LEN];
   long row, col;
};
typedef struct _sc_io_struct sc_io_struct;
typedef struct _sc_io_struct *sc_io_type;

#define sc_io_GetCurr(io) ((io)->curr_char)

sc_io_type sc_io_Open(const char *name);
void sc_io_Close(sc_io_type io);
int sc_io_GetNext(sc_io_type io);
int sc_io_Unget(sc_io_type io, int c);
int sc_io_SkipSpace(sc_io_type io);
char *sc_io_GetIdentifier(sc_io_type io);
long sc_io_GetDec(sc_io_type io);
double sc_io_GetDouble(sc_io_type io);
char *sc_io_GetPosStr(sc_io_type io);
void sc_io_Error(sc_io_type io, char *fmt, ...);
char *sc_io_GetString(sc_io_type io);
char *sc_io_GetMString(sc_io_type io);

#endif
