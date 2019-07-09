/*

  bms2kiss.c
  
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

#include "fsm.h"
#include "config.h"

int main(int argc, char **argv)
{
  fsm_type fsm;
  
  if ( argc <= 2 )
  {
    puts("Conversion from burst mode format to KISS format");
    puts(COPYRIGHT);
    puts(FREESOFT);
    puts(REDIST);
    printf("Usage: %s bms-file kiss-file\n", argv[0]);
    return 1;
  }
  
  fsm = fsm_Open();
  if ( fsm != NULL )
  {
    if ( fsm_ReadBMS(fsm, argv[1]) != 0 )
    {
      fsm_WriteKISS(fsm, argv[2]);
    }
    else
    {
      printf("Can not read BMS file '%s'.\n", argv[1]);
    }
    
    fsm_Close(fsm);
  }
  else
  {
    puts("Can not create state machine.");
  }
 
  return 0; 
}
