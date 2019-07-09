/*

  dgd.c
  
  A script language for gnet: Digital Gate Design
  
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


  Syntax of DGD format:

    all  := { <entity> }
    entity := 'entity' <name> '{' <body> '}'
    body := { <statements> }
    statements := <delay> <import> <port> <use> <join> <opt>
    delay := 'delay' <portname> <portname> <float> ';'
    import := 'import' <filename>  ';'
    port := 'port' <portlist> ';'
    portlist := { <portdesc> }
    portdesc := ['input'] ['output'] <name> 
    name := <string>|<identifier>
    use := 'use' <entityname> <localname> ';'
    join := 'join' { <portref> } ';'
    opt := 'option' { <id > } ';'
    portref := [<nodename> ':'] <portname> 

*/

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "b_scan.h"
#include "b_ff.h"
#include "gnet.h"
#include "dgd_opt.h"

struct _dgd_struct
{
  gnc nc;
  b_scan_type sc;
  int is_check;
  int is_valid;
  
  int hl_opt;
  int local_hl_opt;
  int synth_opt;
  int local_synth_opt;
  
  char *entity_name;
  char *import_file_name;
  char *use_cell_name;
  char *use_local_name;
  char *join_name1;
  char *join_name2;
  
  char *delay_in_name;
  char *delay_out_name;
  
  int cell_ref;
  
};
typedef struct _dgd_struct *dgd_type;

#define K_ENTITY 1000
#define K_IMPORT 1001
#define K_PORT   1002
#define K_INPUT  1003
#define K_OUTPUT 1004
#define K_USE    1005
#define K_JOIN   1006
#define K_OPTION 1007
#define K_DELAY  1008

#define K_SC     2000
#define K_COLON  2001
#define K_BEGIN  2010
#define K_END    2011

/*---------------------------------------------------------------------------*/

static int internal_dgd_set_name(char **s, const char *name)
{
  if ( *s != NULL )
    free(*s);
  *s = NULL;
  if ( name != NULL )
  {
    *s = strdup(name);
    if ( *s == NULL )
      return 0;
  }
  return 1;
}

/*---------------------------------------------------------------------------*/

int dgd_Init(dgd_type dgd)
{
  dgd->is_check = 0;
  dgd->is_valid = 1;

  dgd->hl_opt = GNC_HL_OPT_DEFAULT;
  dgd->synth_opt = GNC_SYNTH_OPT_DEFAULT;
  
  dgd->entity_name = NULL;
  dgd->import_file_name = NULL;
  dgd->use_cell_name = NULL;
  dgd->use_local_name = NULL;
  dgd->join_name1 = NULL;
  dgd->join_name2 = NULL;
  
  dgd->delay_in_name = NULL;
  dgd->delay_out_name = NULL;
  
  dgd->cell_ref = -1;
  
  if ( b_scan_AddKey(dgd->sc, K_ENTITY, "entity") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_IMPORT, "import") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_PORT, "port") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_USE, "use") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_JOIN, "join") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_INPUT, "input") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_OUTPUT, "output") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_OPTION, "option") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_DELAY, "delay") == 0 ) return 0;


  
  if ( b_scan_AddKey(dgd->sc, K_SC, ";") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_COLON, ":") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_BEGIN, "{") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, K_END, "}") == 0 ) return 0;
  if ( b_scan_AddKey(dgd->sc, B_SCAN_TOK_LINE_COMMENT, "#") == 0 ) return 0;
  return 1;
}

dgd_type dgd_Open(gnc nc)
{
  dgd_type dgd;
  dgd = (dgd_type)malloc(sizeof(struct _dgd_struct));
  if ( dgd != NULL )
  {
    dgd->nc = nc;
    dgd->sc = b_scan_Open();
    if ( dgd->sc != NULL )
    {
      if ( dgd_Init(dgd) != 0 )
      {
        return dgd;
      }
      b_scan_Close(dgd->sc);
    }
    free(dgd);
  }
  gnc_Error(nc, "DGD Error: Out of memory (init)");
  return NULL;
}

void dgd_Close(dgd_type dgd)
{
  internal_dgd_set_name(&(dgd->entity_name), NULL);
  internal_dgd_set_name(&(dgd->import_file_name), NULL);
  internal_dgd_set_name(&(dgd->use_cell_name), NULL);
  internal_dgd_set_name(&(dgd->use_local_name), NULL);
  internal_dgd_set_name(&(dgd->join_name1), NULL);
  internal_dgd_set_name(&(dgd->join_name2), NULL);
  internal_dgd_set_name(&(dgd->delay_in_name), NULL);
  internal_dgd_set_name(&(dgd->delay_out_name), NULL);

  b_scan_Close(dgd->sc);
  free(dgd);
}

int dgd_T(dgd_type dgd)
{
  int t = b_scan_Token(dgd->sc);

  return t;    
}

const char *dgd_S(dgd_type dgd)
{
  return dgd->sc->str_val;
}

int dgd_V(dgd_type dgd)
{
  return dgd->sc->num_val;
}

void dgd_ErrorExpectedToken(dgd_type dgd, int cnt, ...)
{
  const char *s;
  va_list va;
  int t;
  
  if ( dgd->is_check != 0 )
    return;

  if ( cnt == 0 )
  {
  }
  else if ( cnt == 1 )
  {
    va_start(va, cnt);
    t = va_arg(va, int);
    s = b_scan_GetNameById(dgd->sc, t);
    if ( s == NULL )
      s = "<unknown>";
    gnc_Error(dgd->nc, "DGD Error: Expected '%s'.", s);
    va_end(va);
  }
  else
  {
    va_start(va, cnt);
    gnc_Error(dgd->nc, "DGD Error: Expected one of:");
    while( cnt > 0 )
    {
      t = va_arg(va, int);
      s = b_scan_GetNameById(dgd->sc, t);
      if ( s == NULL )
        s = "<unknown>";
      gnc_Error(dgd->nc, "DGD Error:    '%s'", s);
      cnt--;
    }
    va_end(va);
  }
}

void dgd_ErrorFound(dgd_type dgd, const char *s)
{
  if ( dgd->is_check != 0 )
    return;
    
  gnc_Error(dgd->nc, "DGD Error: Found '%s'.", s);
}

int dgd_OptStrOrId(dgd_type dgd, char **dest, int t)
{
  if ( t == B_SCAN_TOK_STR || t == B_SCAN_TOK_ID )
  {
    if ( internal_dgd_set_name(dest, dgd_S(dgd)) == 0 )
    {
      gnc_Error(dgd->nc, "DGD Error: Out of memory (string %s).", dgd_S(dgd));
      return B_SCAN_TOK_ERR;
    }
  }
  return dgd_T(dgd);
  
}

int dgd_StrOrId(dgd_type dgd, char **dest, int t)
{
  if ( t == B_SCAN_TOK_STR || t == B_SCAN_TOK_ID )
  {
    if ( internal_dgd_set_name(dest, dgd_S(dgd)) == 0 )
    {
      gnc_Error(dgd->nc, "DGD Error: Out of memory (string %s).", dgd_S(dgd));
      return 0;
    }
    return 1;
  }
  gnc_Error(dgd->nc, "DGD Error: Expected string or identifier.");
  if ( t >= 0 )
    dgd_ErrorFound(dgd, dgd_S(dgd));
  
  return 0;
}

/* t: expected token, tt: current token */
int dgd_CheckT(dgd_type dgd, int t, int tt)
{
  if ( t == tt )
    return 1;
  dgd_ErrorExpectedToken(dgd, 1, t);
  if ( t == B_SCAN_TOK_ID || t >= 0 )
    dgd_ErrorFound(dgd, dgd_S(dgd));
  return 0;
}

int dgd_GetCellRef(dgd_type dgd)
{
  if ( dgd->cell_ref >= 0 )
    return dgd->cell_ref;
  gnc_Log(dgd->nc, 1, "DGD: Create cell '%s'.", dgd->entity_name);
  dgd->cell_ref = gnc_AddCell(dgd->nc, dgd->entity_name, NULL);
  return dgd->cell_ref;
}

int dgd_Import(dgd_type dgd)
{
  if ( dgd->cell_ref >= 0 )
  {
    gnc_Error(dgd->nc, "DGD Error: The 'import' command must not be used together with other synthesis commands.");
    return 0;
  }

  if ( dgd_StrOrId(dgd, &(dgd->import_file_name), dgd_T(dgd)) == 0 )
    return 0;

  if ( dgd_CheckT(dgd, K_SC, dgd_T(dgd)) == 0 )
    return 0;

  return 1;  
}

int dgd_Delay(dgd_type dgd)
{
  int v;
  
  if ( dgd_StrOrId(dgd, &(dgd->delay_in_name), dgd_T(dgd)) == 0 )
    return 0;

  if ( dgd_StrOrId(dgd, &(dgd->delay_out_name), dgd_T(dgd)) == 0 )
    return 0;

  if ( dgd_CheckT(dgd, B_SCAN_TOK_VAL, dgd_T(dgd)) == 0 )
    return 0;
    
  v = dgd_V(dgd);

  if ( dgd_CheckT(dgd, K_SC, dgd_T(dgd)) == 0 )
    return 0;

  if ( dgd_GetCellRef(dgd) < 0 )
    return 0;
    
  if ( dgd->cell_ref >= 0 )
    return gnc_BuildDelayPath(dgd->nc, dgd->cell_ref, (double)v, dgd->delay_in_name, dgd->delay_out_name);

  return 1;
}


int dgd_DoImport(dgd_type dgd)
{
  assert(dgd->import_file_name != NULL);
  
  dgd->cell_ref = gnc_SynthByFile(dgd->nc, 
    dgd->import_file_name, 
    dgd->entity_name, 
    NULL, 
    dgd->local_hl_opt, 
    dgd->local_synth_opt);
    
  if ( dgd->cell_ref < 0 )
  {
    gnc_Error(dgd->nc, "DGD Error: Import of file '%s' failed.", dgd->import_file_name);
  }

  return 1;
}

int dgd_CreatePort(dgd_type dgd, int port_type, const char *name)
{
  int cell_ref = dgd_GetCellRef(dgd);
  if ( cell_ref < 0 )
    return 0;
  gnc_Log(dgd->nc, 1, "DGD: Create port '%s' (cell '%s').", name, dgd->entity_name);
  if ( gnc_AddCellPort(dgd->nc, cell_ref, port_type, name) < 0 )
    return 0;
  return 1;
}

int dgd_Port(dgd_type dgd)
{
  int t;
  int port_type = GPORT_TYPE_IN;
  
  for(;;)
  {
    t = dgd_T(dgd);
    if ( t == K_INPUT )
      port_type = GPORT_TYPE_IN;
    else if ( t == K_OUTPUT )
      port_type = GPORT_TYPE_OUT;
    else if ( t == B_SCAN_TOK_ID || t == B_SCAN_TOK_STR )
    {
      if ( dgd_CreatePort(dgd, port_type, dgd_S(dgd)) == 0 )
        return 0;
    }
    else if ( t == K_SC )
    {
      break;
    }
    else 
    {
      dgd_ErrorExpectedToken(dgd, 3, K_INPUT, K_OUTPUT, K_SC);
      if ( t >= 0 )
        dgd_ErrorFound(dgd, dgd_S(dgd));
      return 0;
    }
  }
  return 1;
}

int dgd_Use(dgd_type dgd)
{
  int cell_ref;
  int node_cell_ref;
  int t;

  if ( dgd_StrOrId(dgd, &(dgd->use_cell_name), dgd_T(dgd)) == 0 )
    return 0;

  t = dgd_T(dgd);
  for(;;)
  {
    if ( dgd_StrOrId(dgd, &(dgd->use_local_name), t) == 0 )
      return 0;

    node_cell_ref = gnc_FindCell(dgd->nc, dgd->use_cell_name, NULL);
    if ( node_cell_ref < 0 )
    {
      gnc_Error(dgd->nc, "DGD Error: Cell/Entity '%s' not found.", dgd->use_cell_name);
      return 0;
    }

    cell_ref = dgd_GetCellRef(dgd);
    if ( cell_ref < 0 )
      return 0;

    gnc_Log(dgd->nc, 0, "DGD: Create component '%s' from cell/entity '%s'.", 
      dgd->use_local_name, dgd->use_cell_name);

    if ( gnc_AddCellNode(dgd->nc, cell_ref, dgd->use_local_name, node_cell_ref) < 0 )
      return 0;

    t = dgd_T(dgd);
    if ( t == K_SC )
      break;
  }

  return 1;
}

int dgd_CreateJoin(dgd_type dgd, int net_ref, const char *nodename, const char *portname)
{
  int cell_ref;
  int node_ref;
  int port_ref;
  int node_cell_ref;
  cell_ref = dgd_GetCellRef(dgd);
  if ( cell_ref < 0 )
    return -1;
  if ( net_ref < 0 )
    net_ref = gnc_AddCellNet(dgd->nc, cell_ref, NULL);
  if ( net_ref < 0 )
    return -1;
  if ( nodename == NULL )
  {
    node_ref = -1;
    port_ref = gnc_FindCellPort(dgd->nc, cell_ref, portname);
    if ( port_ref < 0 )
    {
      gnc_Error(dgd->nc, "DGD Error: Port '%s' not found (entity '%s').", dgd->entity_name);
      return -1;
    }
  }
  else
  {
    node_ref = gnc_FindCellNode(dgd->nc, cell_ref, nodename);
    if ( node_ref < 0 )
    {
      gnc_Error(dgd->nc, "DGD Error: Component '%s' not found.", nodename);
      return -1;
    }
    node_cell_ref = gnc_GetCellNodeCell(dgd->nc, cell_ref, node_ref);
    port_ref = gnc_FindCellPort(dgd->nc, node_cell_ref, portname);
    if ( port_ref < 0 )
    {
      gnc_Error(dgd->nc, "DGD Error: Port '%s' not found (node '%s', entity '%s').", 
        portname,
        nodename, 
        gnc_GetCellName(dgd->nc, node_cell_ref));
      return -1;
    }
  }

  if ( nodename != NULL )
    gnc_Log(dgd->nc, 0, "DGD: Create join for port '%s:%s'.", 
      nodename, portname);
  else  
    gnc_Log(dgd->nc, 0, "DGD: Create join for port '%s'.", 
      portname);

  if ( gnc_AddCellNetJoin(dgd->nc, cell_ref, net_ref, node_ref, port_ref, 1) < 0 )
    return -1;
  
  return net_ref;
}

int dgd_Join(dgd_type dgd)
{ 
  int t;
  int net_ref = -1;
  int state = 0;

  internal_dgd_set_name(&(dgd->join_name1), NULL);
  internal_dgd_set_name(&(dgd->join_name2), NULL);
  
  t = dgd_T(dgd);

  for(;;)
  {
    if ( state == 0 )   /* expect [<name> :] <name> */
    {
      if ( t == K_SC )
        break;
        
      /* there must be a name */
      if ( dgd_StrOrId(dgd, &(dgd->join_name1), t) == 0 )
        return 0;
      state = 1;
      t = dgd_T(dgd);
    }
    else if ( state == 1 ) 
    {
      if ( t == K_COLON )
        state = 2;    /* ':' found */
      else
        state = 3;    /* something else found */
    }
    else if ( state == 2 )
    {
      t = dgd_T(dgd);   /* skip the ':' */
      /* there must be an other name */
      if ( dgd_StrOrId(dgd, &(dgd->join_name2), t) == 0 )
        return 0;
      net_ref = dgd_CreateJoin(dgd, net_ref, dgd->join_name1, dgd->join_name2);
      if ( net_ref < 0 )
        return 0;
      t = dgd_T(dgd);
      state = 0;
    }
    else if ( state == 3 )  /* only one name has been read: a portname */
    {
      net_ref = dgd_CreateJoin(dgd, net_ref, NULL, dgd->join_name1);
      if ( net_ref < 0 )
        return 0;
      /* back to the start, without new token */
      state = 0;
    }
  }
  return 1;
  
}

int dgd_Option(dgd_type dgd)
{
  int t;

  t = dgd_T(dgd);

  for(;;)
  {
    if ( t == K_SC )
      break;
    else if ( t == B_SCAN_TOK_ID || t == B_SCAN_TOK_STR )
    {

      if ( strcmp(dgd_S(dgd), DGD_NECA       ) == 0 )
        dgd->local_synth_opt |= GNC_SYNTH_OPT_NECA;
      else if ( strcmp(dgd_S(dgd), DGD_NO_NECA    ) == 0 )
        dgd->local_synth_opt &= ~GNC_SYNTH_OPT_NECA;
      
      else if ( strcmp(dgd_S(dgd), DGD_GENERIC       ) == 0 )
        dgd->local_synth_opt |= GNC_SYNTH_OPT_GENERIC;
      else if ( strcmp(dgd_S(dgd), DGD_NO_GENERIC    ) == 0 )      
        dgd->local_synth_opt &= ~GNC_SYNTH_OPT_GENERIC;
        
      else if ( strcmp(dgd_S(dgd), DGD_LEVELS        ) == 0 )
        dgd->local_synth_opt |= GNC_SYNTH_OPT_LEVELS;
      else if ( strcmp(dgd_S(dgd), DGD_NO_LEVELS     ) == 0 )
        dgd->local_synth_opt &= ~GNC_SYNTH_OPT_LEVELS;

      else if ( strcmp(dgd_S(dgd), DGD_OUTPUTS       ) == 0 )
        dgd->local_synth_opt |= GNC_SYNTH_OPT_OUTPUTS;
      else if ( strcmp(dgd_S(dgd), DGD_NO_OUTPUTS    ) == 0 )
        dgd->local_synth_opt &= ~GNC_SYNTH_OPT_OUTPUTS;
        
      else if ( strcmp(dgd_S(dgd), DGD_LIBRARY       ) == 0 )
        dgd->local_synth_opt |= GNC_SYNTH_OPT_LIBARY;
      else if ( strcmp(dgd_S(dgd), DGD_NO_LIBRARY    ) == 0 )
        dgd->local_synth_opt &= ~GNC_SYNTH_OPT_LIBARY;
        
      else if ( strcmp(dgd_S(dgd), DGD_DLYPATH       ) == 0 )
        dgd->local_synth_opt |= GNC_SYNTH_OPT_DLYPATH;
      else if ( strcmp(dgd_S(dgd), DGD_NO_DLYPATH    ) == 0 )
        dgd->local_synth_opt &= ~GNC_SYNTH_OPT_DLYPATH;

      else if ( strcmp(dgd_S(dgd), DGD_HL_NO_RESET   ) == 0 )
      {
        dgd->local_hl_opt &= ~0x07;
        dgd->local_hl_opt |= GNC_HL_OPT_NO_RESET;
      }
      else if ( strcmp(dgd_S(dgd), DGD_HL_CLR_HIGH ) == 0 )
      {
        dgd->local_hl_opt &= ~0x07;
        dgd->local_hl_opt |= GNC_HL_OPT_CLR_HIGH;
      }
      else if ( strcmp(dgd_S(dgd), DGD_HL_CLR_LOW  ) == 0 )
      {
        dgd->local_hl_opt &= ~0x07;
        dgd->local_hl_opt |= GNC_HL_OPT_CLR_LOW;
      }

      else if ( strcmp(dgd_S(dgd), DGD_HL_FBO   ) == 0 )
        dgd->local_hl_opt |= GNC_HL_OPT_FBO;
      else if ( strcmp(dgd_S(dgd), DGD_HL_NO_FBO   ) == 0 )
        dgd->local_hl_opt &= ~GNC_HL_OPT_FBO;

      else if ( strcmp(dgd_S(dgd), DGD_HL_RSO   ) == 0 )
        dgd->local_hl_opt |= GNC_HL_OPT_RSO;
      else if ( strcmp(dgd_S(dgd), DGD_HL_NO_RSO   ) == 0 )
        dgd->local_hl_opt &= ~GNC_HL_OPT_RSO;

      else if ( strcmp(dgd_S(dgd), DGD_HL_MIN_STATE   ) == 0 )
        dgd->local_hl_opt |= GNC_HL_OPT_MIN_STATE;
      else if ( strcmp(dgd_S(dgd), DGD_HL_NO_MIN_STATE   ) == 0 )
        dgd->local_hl_opt &= ~GNC_HL_OPT_MIN_STATE;

      else if ( strcmp(dgd_S(dgd), DGD_HL_FLATTEN   ) == 0 )
        dgd->local_hl_opt |= GNC_HL_OPT_FLATTEN;
      else if ( strcmp(dgd_S(dgd), DGD_HL_NO_FLATTEN   ) == 0 )
        dgd->local_hl_opt &= ~GNC_HL_OPT_FLATTEN;

      else if ( strcmp(dgd_S(dgd), DGD_HL_CLOCK   ) == 0 )
        dgd->local_hl_opt |= GNC_HL_OPT_CLOCK;
      else if ( strcmp(dgd_S(dgd), DGD_HL_NO_CLOCK   ) == 0 )
        dgd->local_hl_opt &= ~GNC_HL_OPT_CLOCK;
      
      else
      {
        gnc_Error(dgd->nc, "DGD Error: Illegal option '%s' ignored.", dgd_S(dgd));
      }
      
      gnc_Log(dgd->nc, 1, "DGD: Found option '%s' (synth is %08x, hl is %08x).", dgd_S(dgd), dgd->local_synth_opt, dgd->local_hl_opt);

    }
    else
    {
      gnc_Error(dgd->nc, "DGD Error: Identifier or string expected for 'option'.");
      return 0;
    }
    t = dgd_T(dgd);
  }
  return 1;
}


int dgd_Entity(dgd_type dgd)
{
  int t;

  internal_dgd_set_name(&(dgd->entity_name), NULL);
  internal_dgd_set_name(&(dgd->import_file_name), NULL);
  internal_dgd_set_name(&(dgd->delay_in_name), NULL);
  internal_dgd_set_name(&(dgd->delay_out_name), NULL);
  
  dgd->cell_ref = -1;
  /*
  internal_dgd_set_name(&(dgd->use_cell_name), NULL);
  internal_dgd_set_name(&(dgd->use_local_name), NULL);
  */
  dgd->local_hl_opt = dgd->hl_opt;
  dgd->local_synth_opt = dgd->synth_opt;
  
  if ( dgd_StrOrId(dgd, &(dgd->entity_name), dgd_T(dgd)) == 0 )
    return 0;
    
  gnc_Log(dgd->nc, 4, "DGD: Parsing entity '%s'.", dgd->entity_name);

  if ( dgd_CheckT(dgd, K_BEGIN, dgd_T(dgd)) == 0 )
    return 0;
    
  for(;;)
  {
    t = dgd_T(dgd);
    if ( t == K_IMPORT )
    {
      if ( dgd_Import(dgd) == 0 )
        return 0;
    }
    else if ( t == K_DELAY )
    {
      if ( dgd_Delay(dgd) == 0 )
        return 0;
    }
    else if ( t == K_OPTION )
    {
      if ( dgd_Option(dgd) == 0 )
        return 0;
    }
    else if ( t == K_PORT )
    {
      if ( dgd_Port(dgd) == 0 )
        return 0;
    }
    else if ( t == K_USE )
    {
      if ( dgd_Use(dgd) == 0 )
        return 0;
    }
    else if ( t == K_JOIN )
    {
      if ( dgd_Join(dgd) == 0 )
        return 0;
    }
    else if ( t == K_END )
    {
      break;
    }
    else
    {
      dgd_ErrorExpectedToken(dgd, 7, K_IMPORT, K_DELAY, K_OPTION, K_PORT, K_USE, K_JOIN, K_END);
      if ( t == B_SCAN_TOK_ID || t >= 0 )
        dgd_ErrorFound(dgd, dgd_S(dgd));
      return 0;
    }
  }

  if ( dgd->import_file_name != NULL )
  {
    if ( dgd_DoImport(dgd) == 0 )
      return 0;
  }
  
  if ( dgd->cell_ref >= 0 )
  {
    int port_ref = -1;
    gnc_Log(dgd->nc, 3, "DGD: Ports of '%s' are...", dgd->entity_name);
    while( gnc_LoopCellPort(dgd->nc, dgd->cell_ref, &port_ref) != 0 )
    {
      gnc_Log(dgd->nc, 3, "DGD: Port '%s'.", 
        gnc_GetCellPortName(dgd->nc, dgd->cell_ref, port_ref));
    }
  }

  gnc_Log(dgd->nc, 4, "DGD: Synthesis of '%s' finished.", dgd->entity_name);

  return 1;
}

int dgd_All(dgd_type dgd)
{
  int t;
  for(;;)
  {
    t = dgd_T(dgd);
    if ( t == K_ENTITY )
    {
      if ( dgd_Entity(dgd) == 0 )
        return 0;
    }
    else if ( t == B_SCAN_TOK_EOF )
    {
      break;
    }
    else
    {
      dgd_ErrorExpectedToken(dgd, 1, K_ENTITY);
      if ( t == B_SCAN_TOK_ID || t >= 0  )
        dgd_ErrorFound(dgd, dgd_S(dgd));
      return 0;
    }
  }
  return 1;
}

/*---------------------------------------------------------------------------*/

int gnc_SynthDGD(gnc nc, const char *import_file_name, int hl_opt, int synth_opt)
{
  dgd_type dgd;
  int cell_ref = -1;
  char *s;

  gnc_Log(nc, 4, "DGD: Processing file '%s'.", import_file_name);

  dgd = dgd_Open(nc);
  if ( dgd == NULL )
    return -1;
    
  dgd->hl_opt = hl_opt;
  dgd->synth_opt = synth_opt;
  
  s = b_ff(import_file_name, NULL, ".dgd");
  if ( s == NULL )
    return dgd_Close(dgd), -1;
    
  if ( b_scan_SetFileName(dgd->sc, s) == 0 )
  {
    gnc_Log(nc, 4, "DGD: Error with file '%s'.", s);
    return free(s), dgd_Close(dgd), -1;
  }
  
  free(s);
  
  if ( dgd_All(dgd) == 0 )
    return dgd_Close(dgd), -1;
    
  cell_ref = dgd->cell_ref;

  if ( (hl_opt & GNC_HL_OPT_FLATTEN) == GNC_HL_OPT_FLATTEN )
  {
    gnc_Log(nc, 4, "DGD: Flatten hierarchy.");

    if ( gnc_FlattenCell(nc, cell_ref, 1) == 0 )
    {
      gnc_Error(nc, "DGD: Flatten failed.");
      return dgd_Close(dgd), -1;
    }
    gnc_DeleteUnusedNodes(nc, cell_ref);
    gnc_Clean(nc);
    gnc_CheckCellNetDriver(nc, cell_ref);
  }

  gnc_Log(nc, 4, "DGD: Finished (file '%s').", import_file_name);
    
  dgd_Close(dgd);
  return cell_ref;
}

