/*
  
  Sum of Product with JSON interface
  
  Part of the Digital Gate Project
  
  Copyright (C) 2023 Oliver Kraus (olikraus@yahoo.com)
  cJSON library from https://github.com/DaveGamble/cJSON (MIT License)
  
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
  
  
  Error Codes
  101   JSON error
  102   Memory allocation failed
  103   File IO
  104   File not found (fopen failed)
*/

#include <stdio.h>
#include <stdlib.h>
#include <cJSON.h>
#include "dcube.h"
#include "config.h"

int sopjsonstring(const char *s)
{
  cJSON *json = cJSON_Parse(s);
  if (json == NULL)
  {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL)
      {
          fprintf(stderr, "Error before: %s\n", error_ptr);
      }
      return 101;
  }
  
  return 0;
}

int sopjsonfile(const char *filename)
{
  /* Source: https://stackoverflow.com/questions/2029103/correct-way-to-read-a-text-file-into-a-buffer-in-c */
  int error_code;
  char *source = NULL;
  FILE *fp = fopen(filename, "r");
  if (fp != NULL) 
  {
      /* Go to the end of the file. */
      if (fseek(fp, 0L, SEEK_END) == 0) 
      {
        /* Get the size of the file. */
        long bufsize = ftell(fp);
        if (bufsize > 0) 
        {
          source = malloc(sizeof(char) * (bufsize + 1));
          if ( source != NULL )
          {
            /* goto start of the file. */
            if (fseek(fp, 0L, SEEK_SET) == 0) 
            {  
              /* read file into memory. */
              size_t len = fread(source, sizeof(char), bufsize, fp);
              if ( ferror( fp ) != 0 ) 
              {
                  error_code = 103;
                  fputs("File read error", stderr);
              } 
              else 
              {
                source[len] = '\0'; 
                error_code = sopjsonstring(source);
              }
            } //fseek
            else
            {
              error_code = 103;
            }
            
            free(source); 
          } // malloc
          else
          {
            error_code = 102;
          }
          
        } // ftell
        else
        {
          error_code = 103;
        }
      } // fseek
      else
      {
        error_code = 103;
      }
      fclose(fp);
  } // fopen 
  else
  {
    error_code = 104;
  }
  return error_code;
}

void help(void)
{
  puts("sopjson <jsonfile>");
  puts("sopjson Copyright (c) 2023 olikraus@gmail.com");
  puts("cJSON Copyright (c) 2009-2017 Dave Gamble and cJSON contributors");
}

int main(int argc, char **argv)
{
  const char *json_file_name = NULL;
  int error_code;
  
  if ( argc <= 1 )
  {
    help();
    exit(0);
  }
  
  argv++;       /* skip program name */
  while( *argv != NULL )
  {
    if ( **argv == '-' )
    {
    }
    else if ( json_file_name == NULL )
    {
      json_file_name = *argv;
    }
    else
    {
      printf("Argument '%s' ignored\n", *argv);
    }
    argv++;
  }
  
  error_code = sopjsonfile(json_file_name);
  printf("error code = %d\n", error_code);
}

