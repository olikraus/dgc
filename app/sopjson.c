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
  107   Internal error
  
  Notes:
    - PLA data is stored as an array of strings. Each line (product term) is one string in the array.
      plaReference may look like this:   "plaReference": ["0-00 1", "1-11 1"]
    - 
  
  {
    inCnt: 99,
    outCnt: 1,
    plaReference: [ "<product>", "<product>", ..., "<product>"] ,
    plaInputList: [ [ "<product>", "<product>", ..., "<product>"] , ..., [ "<product>", "<product>", ..., "<product>"]   ],
    
    plaIntersectionList: [ ],
    cmpOutputList[ { isRefSubSet:0, isRefSuperSet:0, isRefEqual:0, isRefOverlap:0, plaRefExt:"ref#In"} ] 
    plaOuputUnion: "str",
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
dclist dcl_read;        // used to read dcl from JSON file
dclist dcl_input_list_union;       // union of the plaInputList array
dclist dcl_input_reference;       // content from the plaReference member
dclist dcl_tmp;                         // temporary dcl list
dclist dcl_a;                         // temporary dcl list
dclist dcl_b;                         // temporary dcl list

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

  Argument:
    pla:        cJSON array

  returns error code:
    0: all ok

  Note:
    - Result will be read into the global "dcl_read" object.
    - dclSCC() will be applied to "dcl_read"

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
  
  dclSCC(&pi, dcl_read);
  
  return 0;
}

int sop_json_dcl_write(cJSON *pla, dclist dcl)
{
  int i, cnt;
  cJSON *cube;
  if ( cJSON_IsArray(pla) == 0 )
  {
    p("Error: PLA is not an array\n");
    return 107;  /* internal error */
  }
  cnt = dclCnt(dcl);
  for( i = 0; i < cnt; i++ )
  {
    cube = cJSON_CreateString(dcToStr(&pi, dclGet(dcl, i), " ", ""));
    if ( cube == NULL ) return 102; /* memory error */
    if ( cJSON_AddItemToArray(pla, cube) == 0 ) return 102; /* memory error */      
  }
  return 0;
}

int sop_json_add_dcl_to_object(cJSON *obj, const char *name, dclist dcl)
{
  cJSON *pla = cJSON_CreateArray();
  if ( pla == NULL ) return 102; /* memory error */
  if ( cJSON_AddItemToObject(obj, name, pla) == 0 ) return 102; /* memory error */
  return sop_json_dcl_write(pla, dcl);
}



/*===================================================================*/
/* 
  setup 
  - check & create problem info
  - create global DCLs
  - create output members in the JSON object

*/

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
  
  /* create problem info structure */
  
  pinfoInitInOut(&pi, in, out);
  
  /* setup initial cube list */
  
  if ( dclInitVA(6, &dcl_a, &dcl_b, &dcl_tmp, &dcl_read, &dcl_input_list_union, &dcl_input_reference) == 0 )
    return 102;
  
  /* create output elements in the cJSON structure */

  o = cJSON_GetObjectItemCaseSensitive(json, "cmpOutputList");
  if ( o == NULL )
  {
    cJSON_AddArrayToObject(json, "cmpOutputList");
  }
  else
  {
    p("Error: JSON member 'cmpOutputList' exists in the input and can't be created.\n");
    return 105;    
  }
  
  
  
  return 0;
}

/*===================================================================*/
/* execution */

/*
  
  Search for the "plaInputList" of the JSON and calculate the
  union over all PLA inside the "plaInputList". Store the result
  in global object "dcl_input_list_union".

  Finally the result is stored in a new member "plaOuputUnion" for the json argmument.

*/
int sop_json_input_union(cJSON *json)
{
  int code;
  int cnt;
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
  
  cnt = 0;
  dclClear(dcl_input_list_union);
  
  cJSON_ArrayForEach(e, list)
  {
    p("Reading dcl %d\n", cnt);
    code = sop_json_dcl_read(e);
    if ( code > 0 ) return code;
    dclShow(&pi, dcl_read);
    
    dclSCCUnion(&pi, dcl_input_list_union, dcl_read);
    
    ++cnt;
  }

  dclMinimize(&pi, dcl_input_list_union);
  //p("dcl_input_list_union:\n", cnt);
  //dclShow(&pi, dcl_input_list_union);

  code = sop_json_add_dcl_to_object(json, "plaOuputUnion", dcl_input_list_union);
  if ( code > 0 )
    return code;

  
  return 0;
}


/*

  Description:
    Execute several operation between two PLA objects ("ref" and "e")
    Store the result of those operations into the JSON object "result".
    The following PLA results will be stored:
      result:"subtractFromRef"  --> ref - e
      result:"isRefSubSet"      --> true if "ref" is equal or a subset of "e"
      result:"reducedByRef" --> e - ref
      result:"isRefSuperSet" --> true if "e" is equal or a subset of "ref"
      result:"isRefEqual"      --> true of both "isRefSubSet" and "isRefSuperSet" are true (which means that "e" and "ref" describe the same set)
      result:"refIntersection" --> the intersection PLA between "e" and "ref", this can be the empty set.
      result:"isRefOverlap"  --> true, if "refIntersection" is not empty
    



*/

int sop_json_add_result(cJSON *result, dclist ref, dclist e)
{
  int code;
  int is_ref_subset;
  int is_ref_superset;
  
  if ( dclCopy(&pi, dcl_a, ref) == 0 ) return 102; // memory error    
  if ( dclCopy(&pi, dcl_b, e) == 0 ) return 102; // memory error
  if ( dclSubtract(&pi, dcl_a, dcl_b) == 0 ) return 102; // memory error
  
  is_ref_subset = dclCnt(dcl_a) == 0 ? 1 : 0;
  
  /* dclIsSubsetList: is the 3rd arg a subset of the 2nd arg  */
  /* is_ref_subset = dclIsSubsetList(&pi, dcl_read, dcl_input_reference); */
  if ( is_ref_subset )
    cJSON_AddTrueToObject(result, "isRefSubSet");
  else
    cJSON_AddFalseToObject(result, "isRefSubSet");

  code = sop_json_add_dcl_to_object(result, "subtractFromRef", dcl_a);
  if ( code > 0 )
    return code;
  
  
  if ( dclCopy(&pi, dcl_a, e) == 0 ) return 102; // memory error
  if ( dclCopy(&pi, dcl_b, ref) == 0 ) return 102; // memory error    
  if ( dclSubtract(&pi, dcl_a, dcl_b) == 0 ) return 102; // memory error
  
  is_ref_superset = dclCnt(dcl_a) == 0 ? 1 : 0;
  /* is_ref_superset = dclIsSubsetList(&pi, dcl_input_reference, dcl_read); */
  if ( is_ref_superset )
    cJSON_AddTrueToObject(result, "isRefSuperSet");
  else
    cJSON_AddFalseToObject(result, "isRefSuperSet");

  code = sop_json_add_dcl_to_object(result, "reducedByRef", dcl_a);
  if ( code > 0 )
    return code;

  
  if ( is_ref_superset && is_ref_subset )
    cJSON_AddTrueToObject(result, "isRefEqual");
  else
    cJSON_AddFalseToObject(result, "isRefEqual");
    
  /* intersection? */    
  dclIntersectionList(&pi, dcl_tmp, ref, e);
  if ( dclCnt(dcl_tmp) != 0 )
    cJSON_AddTrueToObject(result, "isRefOverlap");
  else
    cJSON_AddFalseToObject(result, "isRefOverlap");

  code = sop_json_add_dcl_to_object(result, "refIntersection", dcl_tmp);
  if ( code > 0 )
    return code;
  
  //p("intersection %d\n", cnt);
  //dclShow(&pi, dcl_tmp);
  
  return 0;
}


int sop_json_reference_intersection(cJSON *json)
{
  cJSON *list;
  cJSON *reference;
  cJSON *e;
  cJSON *cmp_output_list;
  cJSON *result_object;
  int cnt = 0;
  int code;
  
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

  reference = cJSON_GetObjectItemCaseSensitive(json, "plaReference");
  if ( reference == NULL )
  {
    p("Error: JSON member 'plaReference' is missing\n");
    return 105;    
  }
  
  if ( cJSON_IsArray(reference) == 0 )
  {
    p("Error: JSON member 'plaReference' exits, but is not an array\n");
    return 105;    
  }
  
  
  cmp_output_list = cJSON_GetObjectItemCaseSensitive(json, "cmpOutputList");
  if ( list == NULL )
  {
    p("Internal error: JSON member 'cmpOutputList' is missing\n");
    return 107;
  }
  
  if ( cJSON_IsArray(cmp_output_list) == 0 )
  {
    p("Internal error: JSON member 'cmpOutputList' exits, but is not an array\n");
    return 107;
  }
 
  /* get the reference DCL */
  
  code = sop_json_dcl_read(reference);
  if ( code > 0 ) return code;
  dclCopy(&pi, dcl_input_reference, dcl_read);
  
  /* clear the output array */
  
  while( cJSON_GetArraySize(cmp_output_list) > 0 )
  {
    cJSON_DeleteItemFromArray(cmp_output_list, 0);
  }
  
  /* loop over the input list of DCLs */
  
  cJSON_ArrayForEach(e, list)
  {
    /* read the PLA */
    code = sop_json_dcl_read(e);
    if ( code > 0 ) return code;
        
    /* create a result structure */
    result_object = cJSON_CreateObject();
    if ( result_object == NULL ) return 102;            // memory error
    if ( !cJSON_AddItemToArray(cmp_output_list, result_object) ) return 102;

    /* do the actual calculation*/
    code = sop_json_add_result(result_object, dcl_input_reference, dcl_read);
    if ( code > 0 ) return code;
    
    ++cnt;
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
  
  code = sop_json_reference_intersection(json);
  if ( code > 0 ) return code;

  s = cJSON_Print(json);    
  puts(s);

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

