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
  0             all ok
  101   JSON parse error
  102   Memory allocation failed
  103   File IO
  104   File not found (fopen failed)
  105   SOP JSON grammer error
  106   PLA Parser failed
  
{
  inCnt: 99,
  outCnt: 1,
  plaReference: "str",
  plaInputList: [ ],
  
  plaIntersectionList: [ ],
  cmpInputList[ { isSubSet:0, isSuperSet:0, isEqual:0, isOverlap:0, plaRefExt:"ref#In"} ] 
  plaInputUnion: "str",
  cmpInputUnion: { isSubSet:0, isSuperSet:0, isEqual:0, isOverlap:0, plaRefExt:"ref#In"};
}
  
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <cJSON.h>
#include "dcube.h"
#include "config.h"

/*===================================================================*/
/* global variables */


pinfo pi;       // global problem info from JSON
dclist dcl_read;

/*===================================================================*/
/* util functions */

void p(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

/*===================================================================*/
/* dcl load */

/*
  read a JSON pla object into "pi_read" and "dcl_read" objects

  This function will ensure that the pla has a fitting problem information

  returns error code:
    0: all ok

*/
int sop_json_dcl_read(cJSON *pla)
{
  dcube *c_on = &(pi.tmp[3]);
  dcube *c_dc = &(pi.tmp[4]);
  char *s;
  cJSON *line;
  
  if ( dcl_read != NULL )
    dclRealClear(dcl_read);
  //if ( cl_dc != NULL )
  //  dclRealClear(cl_dc);
  
  
  if ( pla == NULL )
  {
    p("Error: PLA is NULL\n");
    return 105;
  }
  
  if ( cJSON_IsArray(pla) == 0 )
  {
    p("Error: PLA is not an array\n");
    return 105;
  }
  
  cJSON_ArrayForEach(line, pla)
  {
    if ( cJSON_IsString(line) == 0 )
    {
      p("Error: PLA line is not a string\n");
      return 105;
    }
    s = cJSON_GetStringValue(line);
    
    if ( dcSetAllByStr(&pi, pi.in_cnt, pi.out_cnt, c_on, c_dc, s) == 0 )
    {
      p("Error: PLA line '%s' syntax error (in=%d, out=%d)\n", s, pi.in_cnt, pi.out_cnt);
      return 106;    
    }
    if ( dcl_read != NULL )
      if ( dcIsOutIllegal(&pi, c_on) == 0 )
        if ( dclAdd(&pi, dcl_read, c_on) < 0 )
        {
          p("Error: PLA line memory allocation error (line '%s')\n", s);
          return 102;
        }    
  }
  
  return 0;
}




/*===================================================================*/
/* setup */

int sop_json_init(cJSON *json)
{
  int in, out;
  cJSON *o;
  if ( cJSON_IsObject(json) == 0 )
  {
    p("Error: JSON root object is not an object\n");
    return 105;
  }
  
  /* inCnt */
  
  o = cJSON_GetObjectItemCaseSensitive(json, "inCnt");
  if ( o == NULL )
  {
    p("Error: JSON member 'inCnt' is missing\n");
    return 105;    
  }
  if ( cJSON_IsNumber(o) == 0 )
  {
    p("Error: JSON member 'inCnt' exists, but is not a number\n");
    return 105;    
  }
  in  = (int)cJSON_GetNumberValue(o);

  /* outCnt */
  
  o = cJSON_GetObjectItemCaseSensitive(json, "outCnt");
  if ( o == NULL )
  {
    p("Error: JSON member 'outCnt' is missing\n");
    return 105;    
  }
  if ( cJSON_IsNumber(o) == 0 )
  {
    p("Error: JSON member 'outCnt' exists, but is not a number\n");
    return 105;    
  }
  out  = (int)cJSON_GetNumberValue(o);
  
  pinfoInitInOut(&pi, in, out);
  
  if ( dclInitVA(1, &dcl_read) == 0 )
    return 102;

  
  return 0;
}

/*===================================================================*/
/* execution */

int sop_json_input_union(cJSON *json)
{
  int code;
  cJSON *list;
  cJSON *e;
  
  list = cJSON_GetObjectItemCaseSensitive(json, "plaInputList");
  if ( list == NULL )
  {
    p("Error: JSON member 'plaInputList' is missing\n");
    return 105;    
  }
  
  if ( cJSON_IsArray(list) == 0 )
  {
    p("Error: JSON member 'plaInputList' exits, but is not an array\n");
    return 105;    
  }
  
  cJSON_ArrayForEach(e, list)
  {
    code = sop_json_dcl_read(e);
    if ( code > 0 ) return code;
    dclShow(&pi, dcl_read);
  }
  
  return 0;
}

/*===================================================================*/
/* main */

int sop_json_string(const char *s)
{
  int code;
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
  else
  {
    const char *s;
    p("Info: JSON read success\n");
    s = cJSON_Print(json);    
    puts(s);
  }
  
  code = sop_json_init(json);
  if ( code > 0 ) return code;
  code = sop_json_input_union(json);
  if ( code > 0 ) return code;
  return 0;
}

int sop_json_file(const char *filename)
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
                p("Info: %ld bytes read from file %s\n", (unsigned long)(len), filename);
                source[len] = '\0'; 
                error_code = sop_json_string(source);
              }
            } //fseek
            else
            {
              error_code = 103;
              p("'fseek' failed for file %s\n", filename);
            }
            
            free(source); 
          } // malloc
          else
          {
            error_code = 102;
            p("Error: 'malloc(%ld)' failed for file %s\n", (unsigned long)(bufsize+1), filename);
          }
        } // ftell
        else
        {
          error_code = 103;
          p("Error: 'ftell' failed for file %s\n", filename);
        }
      } // fseek
      else
      {
        error_code = 103;
        p("Error: Seek failed for file %s\n", filename);
      }
      fclose(fp);
  } // fopen 
  else
  {
    error_code = 104;
    p("Error: File %s not found\n", filename);
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
  
  error_code = sop_json_file(json_file_name);
  printf("error code = %d\n", error_code);
}

