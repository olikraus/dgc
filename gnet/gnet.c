/*

  Gate Net (gnet.c)
  
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

  ToDo:
  17.11.00  update gcellGetMemUsage with clist
  20.11.00  rename ..NL.. to ..Cell..
            --> 20.11.00 done
  20.11.00  NULL pointer for node_name and net_name
  06.12.00  gnc_AddCellNetPort() muss unique sein
  
  
  
  
  gnc_Log:
    Prio   Desc 
    0      sygate.c: gnc_SynthDigitalCell
    0      synthfsm: gnc_SynthDoor
    0      sylib.c: synlib_do_msg_grp_end, Import done
    1      sylopt.c: gnc_InvertInputs
    1      bbb.c: gnc_IdentifyAndMarkBBBs, Cell Identification
    1      sydelay: min max delays...
    2      sylib.c: synlib_do_msg_grp_end, Import failed
    2      syfsm.c: gnc_fsm_log_fn
    2      sylopt.c: gnc_OptLibrary
    3      bbb.c: gnc_find_bbb, Cell Selection
    3      syfsm.c: gnc_SynthGenericDoorToLib
    3      syfsm.c: gnc_SetFSMPortsWithoutClock
    3      gnetlv.c: capacitance per transition
    4      syrecu.c: gnc_SynthOnSet, Synthesis steps
    4      gnet.c: Import type
    4      wiga.c: wge_Open
    3      gnetdcex.c: gnc_do_synthesis_verification steps
    5      syfsm.c: gnc_SynthClockedFSM, gnc_SynthAsynchronousFSM
    5      syrecu.c: gnc_SynthDCL
    6      toplevel (commandline) messages
    6      gnetlv.c ctc_device: Analysis result

*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "gnet.h"
#include "mwc.h"
#include "b_io.h"
#include "b_ff.h"

/*! \defgroup gncinit gnc Initalisation */
/*! \defgroup gnccell gnc Cell Management */
/*! \defgroup gncnet gnc Cell Netlist Management */
/*! \defgroup gncexport gnc Cell Export */
/*! \defgroup gncsynth gnc Synthesis */



void glvInit(glv_struct *glv);  /* gnetlv.c */


/*----------------------------------------------------------------------------*/

static int internal_gnet_set_name(char **s, const char *name)
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

static size_t internal_gnet_get_name_mem_usage(char *s)
{
  if ( s == NULL )
    return 0;
  return strlen(s);
}

/*----------------------------------------------------------------------------*/

int gportSetName(gport p, const char *name)
{
  return internal_gnet_set_name(&(p->name), name);
}

gport gportOpen(int type, const char *name)
{
  gport p;
  p = (gport)malloc(sizeof(struct _gport_struct));
  if ( p != NULL )
  {
    p->type = type;
    p->name = NULL;
    p->fn = GPORT_FN_LOGIC;
    p->is_inverted = 0;
    p->input_load = 1.0;
    p->poti_set = b_set_Open();
    if ( p->poti_set != NULL )
    {
      if ( gportSetName(p, name) != 0 )
      {
        glvInit(&(p->lv));
        return p;
      }
      b_set_Close(p->poti_set);
    }
    free(p);
  }
  return NULL;
}

void gportClose(gport p)
{
  int i = -1;
  while( b_set_WhileLoop(p->poti_set, &i) != 0 )
    gpotiClose((gpoti)b_set_Get(p->poti_set, i));
  b_set_Close(p->poti_set);
  gportSetName(p, NULL);
  free(p);
}

size_t gportGetMemUsage(gport p)
{
  return sizeof(struct _gport_struct)+
    internal_gnet_get_name_mem_usage(p->name)+
    b_set_GetAllMemUsage(p->poti_set, (size_t (*)(void *))gpotiGetMemUsage);
}

void gportSetFn(gport p, int fn, int is_inverted)
{
  p->fn = fn;
  p->is_inverted = is_inverted;
}

int gportGetFn(gport p)
{
  return p->fn;
}

int gportIsInverted(gport p)
{
  return p->is_inverted;
}

void gportSetInputLoad(gport p, double cap)
{
  p->input_load = cap;
}

double gportGetInputLoad(gport p)
{
  return p->input_load;
}


static int gport_write_poti(FILE *fp, void *el, void *ud)
{
  return gpotiWrite((gpoti)el, fp);
}

int gportWrite(gport p, FILE *fp)
{ 
  if ( b_io_WriteInt(fp, p->type) == 0 )                              return 0;
  if ( b_io_WriteInt(fp, p->fn) == 0 )                                return 0;
  if ( b_io_WriteInt(fp, p->is_inverted) == 0 )                       return 0;
  if ( b_io_WriteString(fp, p->name) == 0 )                           return 0;
  if ( b_io_WriteDouble(fp, p->input_load) == 0 )                     return 0;
  if ( p->poti_set != NULL )
  {
    if ( b_io_WriteInt(fp, 1) == 0 )                                  return 0;
    if ( b_set_Write(p->poti_set, fp, gport_write_poti, NULL) == 0 )  return 0;
  }
  else
  {
    if ( b_io_WriteInt(fp, 0) == 0 )                                  return 0;
  }
  return 1;
}

static void *gport_read_poti(FILE *fp, void *ud)
{
  gpoti pt = gpotiOpen();
  if ( pt == NULL )
    return NULL;
  if ( gpotiRead(pt, fp) == 0 )
  {
    gpotiClose(pt);
    return NULL;
  }
  return pt;
}


int gportRead(gport p, FILE *fp)
{
  int is_poti_set = 0;
  if ( b_io_ReadInt(fp, &(p->type)) == 0 )           return 0;
  if ( b_io_ReadInt(fp, &(p->fn)) == 0 )             return 0;
  if ( b_io_ReadInt(fp, &(p->is_inverted)) == 0 )    return 0;
  gportSetName(p, NULL);
  if ( b_io_ReadAllocString(fp, &(p->name)) == 0 )   return 0;
  if ( b_io_ReadDouble(fp, &(p->input_load)) == 0 )  return 0;
  if ( b_io_ReadInt(fp, &(is_poti_set)) == 0 )       return 0;
  if ( is_poti_set != 0 )
  {
    if ( b_set_Read(p->poti_set, fp, gport_read_poti, NULL) == 0 )   return 0;
  }
  
  return 1;
}

gpoti gportFindGPOTI(gport p, int port_ref)
{
  int i = -1;
  
  while( b_set_WhileLoop(p->poti_set, &i) != 0 )
  {
    if ( ((gpoti)b_set_Get(p->poti_set, i))->related_port_ref == port_ref )
      return (gpoti)b_set_Get(p->poti_set, i);
  }
  return NULL;
}

static gpoti gport_add_gpoti(gport p, int port_ref)
{
  gpoti poti;
  poti = gpotiOpen();
  if ( poti != NULL )
  {
    poti->related_port_ref = port_ref;
    if ( b_set_Add(p->poti_set, poti) >= 0 )
    {
      return poti;
    }
    gpotiClose(poti);
  }
  return NULL;
}

gpoti gportGetGPOTI(gport p, int port_ref, int is_create)
{
  gpoti poti;
  if ( is_create == 0 )
  {
    poti = gportFindGPOTI(p, port_ref);
    if ( poti != NULL )
      return poti;
    return gportFindGPOTI(p, -1);
  }
  poti = gportFindGPOTI(p, port_ref);
  if ( poti == NULL )
    return gport_add_gpoti(p, port_ref);
  return poti;
}

/*----------------------------------------------------------------------------*/

gjoin gjoinOpen(int node, int port)
{
  gjoin j;
  j = (gjoin)malloc(sizeof(struct _gjoin_struct));
  if ( j != NULL )
  {
    j->node = node;
    j->port = port;
    return j;
  }
  return NULL;
}

void gjoinClose(gjoin j)
{
  j->node = -1;
  j->port = -1;
  free(j);
}

size_t gjoinGetMemUsage(gjoin p)
{
  return sizeof(struct _gjoin_struct);
}

int gjoinWrite(gjoin j, FILE *fp)
{
  if ( b_io_WriteInt(fp, j->node) == 0 )        return 0;
  if ( b_io_WriteInt(fp, j->port) == 0 )        return 0;
  return 1;
}

int gjoinRead(gjoin j, FILE *fp)
{
  if ( b_io_ReadInt(fp, &(j->node)) == 0 )      return 0;
  if ( b_io_ReadInt(fp, &(j->port)) == 0 )      return 0;
  return 1;
}


/*----------------------------------------------------------------------------*/

int gnetSetName(gnet net, const char *name)
{
  return internal_gnet_set_name(&(net->name), name);
}


gnet gnetOpen()
{
  gnet net;
  net = (gnet)malloc(sizeof(struct _gnet_struct));
  if ( net != NULL )
  {
    net->user_val = -1;
    net->name = NULL;
    net->join_set = b_set_Open();
    if ( net->join_set != NULL )
    {
      glvInit(&(net->lv));
      return net;
    }
    free(net);
  }
  return NULL;
}


void gnetClose(gnet net)
{
  int join_ref = -1;
  while( gnetLoopJoinRef(net, &join_ref) != 0 )
    gjoinClose(gnetGetGJOIN(net, join_ref));
    
  gnetSetName(net, NULL);
  
  b_set_Close(net->join_set);
  net->join_set = NULL;
  free(net);
}

/* returns join_ref or -1 if port was not found */
/* find a join_ref that is connected to a 'port' of 'node' */
/* if 'node' is -1 a parent port is searched */
/* 'port' = -1 is a wildcard for any port of the node */
int gnetFindPort(gnet net, int node, int port)
{
  int join_ref = -1;
  gjoin join;
  
  if ( port < 0 )
  {
    while( gnetLoopJoinRef(net, &join_ref) != 0 )
    {
      join = gnetGetGJOIN(net, join_ref);
      if ( join->node == node )
        return join_ref;
    }
  }
  else
  {
    while( gnetLoopJoinRef(net, &join_ref) != 0 )
    {
      join = gnetGetGJOIN(net, join_ref);
      if ( join->node == node && join->port == port )
        return join_ref;
    }
  }
  return -1;
}

/* how often is node_ref used inside the net? */
int gnetGetNodeCnt(gnet net, int node_ref)
{
  int join_ref = -1;
  int cnt = 0;
  while( gnetLoopJoinRef(net, &join_ref) != 0 )
    if ( gnetGetGJOIN(net, join_ref)->node == node_ref )
      cnt++;
  return cnt;
}

/* returns join_ref */
int gnetAddPort(gnet net, int node, int port)
{
  int join_ref;
  gjoin join;
  join_ref = gnetFindPort(net, node, port);
  if ( join_ref >= 0 )
    return join_ref;
  join = gjoinOpen(node, port);
  if ( join != NULL )
  {
    join_ref = b_set_Add(net->join_set, join);
    if ( join_ref >= 0 )
    {
      return join_ref;
    }
    gjoinClose(join);
  }
  return -1;
}

/* should be renamed to gnetDelJoin() */
void gnetDelPort(gnet net, int join_ref)
{
  gjoin join;
  join = gnetGetGJOIN(net, join_ref);
  b_set_Del(net->join_set, join_ref);
  gjoinClose(join);
}

void gnetDelPortByNodeAndPort(gnet e, int node, int port)
{
  int join_ref;
  join_ref = gnetFindPort(e, node, port);
  if ( join_ref >= 0 )
    gnetDelPort(e, join_ref);
}

size_t gnetGetMemUsage(gnet net)
{
  size_t m = sizeof(struct _gnet_struct);
  int join_ref = -1;
  
  while( gnetLoopJoinRef(net, &join_ref) != 0 )
    m += gjoinGetMemUsage(gnetGetGJOIN(net, join_ref));
  
  m += b_set_GetMemUsage(net->join_set);
  m += internal_gnet_get_name_mem_usage(net->name);
  return m;
}

int gnetGetPortCnt(gnet net)
{
  return b_set_Cnt(net->join_set);
}

static int gnet_write_el(FILE *fp, void *el, void *ud)
{
  return gjoinWrite((gjoin)el, fp);
}

int gnetWrite(gnet net, FILE *fp)
{ 
  if ( b_io_WriteString(fp, net->name) == 0 )                       return 0;
  if ( b_set_Write(net->join_set, fp, gnet_write_el, NULL) == 0 )   return 0;
  return 1;
}

static void *gnet_read_el(FILE *fp, void *ud)
{
  gjoin j = gjoinOpen(-1, -1);
  if ( j == NULL )
    return NULL;
  if ( gjoinRead(j, fp) == 0 )
    return gjoinClose(j), (void *)NULL;
  return j;
}

int gnetRead(gnet net, FILE *fp)
{
  gnetSetName(net, NULL);
  if ( b_io_ReadAllocString(fp, &(net->name)) == 0 )              return 0;
  if ( b_set_Read(net->join_set, fp, gnet_read_el, NULL) == 0 )   return 0;
  return 1;
}


/*----------------------------------------------------------------------------*/

int gnodeSetName(gnode n, const char *name)
{
  return internal_gnet_set_name(&(n->name), name);
}

gnode gnodeOpen()
{
  gnode n;
  n = (gnode)malloc(sizeof(struct _gnode_struct));
  if ( n != NULL )
  {
    n->cell_ref = -1;
    n->name = NULL;
    n->flag = 0;
    n->is_do_not_touch = 0;
    n->net_refs = b_il_Open();
    if ( n->net_refs != NULL )
    {
      return n;
    }
    free(n);
  }
  return NULL;
}

void gnodeClose(gnode n)
{
  b_il_Close(n->net_refs);
  n->net_refs = NULL;
  gnodeSetName(n, NULL);
  n->cell_ref = -1;
  free(n);
}

int gnodeAddNet(gnode n, int net_ref, int port_ref)
{
  if ( b_il_AddUnique(n->net_refs, net_ref) < 0 )
    return 0;
  return 1;
}

/* port_ref == -1 is used as a wild card... */
void gnodeDelNet(gnode n, int net_ref, int port_ref)
{
  b_il_DelByVal(n->net_refs, net_ref);
}

size_t gnodeGetMemUsage(gnode n)
{
  return sizeof(struct _gnode_struct)+
    internal_gnet_get_name_mem_usage(n->name)+b_il_GetMemUsage(n->net_refs);
}

int gnodeWrite(gnode n, FILE *fp)
{ 
  if ( b_io_WriteInt(fp, n->cell_ref) == 0 )      return 0;
  if ( b_io_WriteString(fp, n->name) == 0 )       return 0;
  return 1;
}

int gnodeRead(gnode n, FILE *fp)
{
  if ( b_io_ReadInt(fp, &(n->cell_ref)) == 0 )        return 0;
  gnodeSetName(n, NULL);
  if ( b_io_ReadAllocString(fp, &(n->name)) == 0 )    return 0;
  return 1;
}


/*----------------------------------------------------------------------------*/

void gnlDelNet(gnl nl, int net_ref);
void gnlDelCellNetJoin(gnl nl, int net_ref, int join_ref);

int gnl_node_cmp_fn(void *data, int el, const void *key)
{
  gnl nl = (gnl)data;
  return strcmp(gnlGetGNODE(nl, el)->name, (const char *)key);
}

int gnl_net_cmp_fn(void *data, int el, const void *key)
{
  gnl nl = (gnl)data;
  return strcmp(gnlGetGNET(nl, el)->name, (const char *)key);
}

gnl gnlOpen()
{
  gnl nl;
  nl = (gnl)malloc(sizeof(struct _gnl_struct));
  if ( nl != NULL )
  {
    nl->node_set = NULL;
    nl->node_by_name = NULL;
    nl->net_set = NULL;
    nl->net_by_name = NULL;
    
    nl->node_set = b_set_Open();
    nl->node_by_name = b_rdic_Open();
    nl->net_set = b_set_Open();
    nl->net_by_name = b_rdic_Open();
    nl->net_refs = b_il_Open();
    
    if ( nl->node_set != NULL && nl->node_by_name != NULL && 
         nl->net_set != NULL && nl->net_by_name != NULL && 
         nl->net_refs != NULL )
    {
      b_rdic_SetCmpFn(nl->node_by_name, gnl_node_cmp_fn, (void *)nl);
      b_rdic_SetCmpFn(nl->net_by_name, gnl_net_cmp_fn, (void *)nl);
      return nl;
    }
    
    if (nl->node_set != NULL) b_set_Close(nl->node_set);
    if (nl->node_by_name != NULL) b_rdic_Close(nl->node_by_name);
    if (nl->net_set != NULL) b_set_Close(nl->net_set);
    if (nl->net_by_name != NULL) b_rdic_Close(nl->net_by_name);
    if (nl->net_refs != NULL) b_il_Close(nl->net_refs);
    
    free(nl);
  }
  return NULL;
}

void gnlClear(gnl nl)
{
  int node_ref = -1;
  int net_ref = -1;

  b_il_Clear(nl->net_refs);
  
  while( gnlLoopNodeRef(nl, &node_ref) != 0 )
    gnodeClose(gnlGetGNODE(nl, node_ref));
  b_set_Clear(nl->node_set);
  b_rdic_Clear(nl->node_by_name);
  
  while( gnlLoopNetRef(nl, &net_ref) != 0 )
    gnetClose(gnlGetGNET(nl, net_ref));
  b_set_Clear(nl->net_set);
  b_rdic_Clear(nl->net_by_name);
}

void gnlClose(gnl nl)
{
  gnlClear(nl);
  b_set_Close(nl->node_set);
  b_rdic_Close(nl->node_by_name);
  b_set_Close(nl->net_set);
  b_rdic_Close(nl->net_by_name);
  b_il_Close(nl->net_refs);
  free(nl);
}

int gnlGetNetCnt(gnl nl)
{
  return b_set_Cnt(nl->net_set);
}

/* returns node_ref or -1 */
int gnlFindNode(gnl nl, const char *name)
{
  int node_ref;

  if ( name == NULL )
    return -1;
  node_ref = b_rdic_Find(nl->node_by_name, (const void *)name);
  assert((node_ref >= 0?strcmp(name, gnlGetGNODE(nl,node_ref)->name):0) == 0);
  return node_ref;
}

void gnlDelNode(gnl nl, int node_ref)
{
  char *name;

  int net_ref;
  int join_ref;
  gnet net;
  gjoin join;
  b_il_type net_refs;
  int loop;
  
  assert(node_ref >= 0);

  /* assume that all possible references from othere net's are stored */
  /* in the cache */
  net_refs = gnlGetGNODE(nl, node_ref)->net_refs;
  loop = -1;
  net_ref = -1;
  while( b_il_Loop(net_refs, &loop, &net_ref) != 0 )
  {
    net = gnlGetGNET(nl, net_ref);
    join_ref = -1;
    while( gnetLoopJoinRef(net, &join_ref) != 0 )
    {
      join = gnetGetGJOIN(net, join_ref);
      if ( join->node == node_ref )
      {
        /* should I use gnetDelPort or gnlDelCellNetJoin? */
        /* using gnlDelCellNetJoin would be overkill, because all */
        /* join_ref's with this node_ref are deleted anyway */
        /* gnlDelCellNetJoin(nl, net_ref, join_ref); */
        gnetDelPort(net, join_ref); 

        /* ... and from the cell connections */
        /* b_il_DelByVal(nl->net_refs, net_ref); */

        if ( gnetGetPortCnt(net) == 0 )
        {
          /* I commented the following line out some day      */
          /* but the line is required, ... well at least as   */
          /* long gnc_CheckCellNetDriver() does not recognice */
          /* empty net structures. ok, 30. may 2001           */
          gnlDelNet(nl, net_ref);
          /* obviously, we had to break out of the join-loop here */
          /* ok 28 dec 2001 */
          break;
        }
      }
    }
  }
    
  name = gnlGetGNODE(nl, node_ref)->name;
  if ( name != NULL )
    b_rdic_Del(nl->node_by_name, (void *)name);
    
  gnodeClose(gnlGetGNODE(nl, node_ref));
  b_set_Del(nl->node_set, node_ref);
}

int gnlAddNode(gnl nl, const char *name, int cell_ref)
{
  gnode node;
  int node_ref;
  node_ref = gnlFindNode(nl, name);
  if ( node_ref >= 0 )
  {
    if ( gnlGetGNODE(nl, node_ref)->cell_ref != cell_ref )  
      return -1;
    return node_ref;
  }
    
  node = gnodeOpen();
  if ( node != NULL )
  {
    node->cell_ref = cell_ref;
    if ( gnodeSetName(node, name) != 0 )
    {
      node_ref = b_set_Add(nl->node_set, node);
      if ( node_ref >= 0 )
      {
        if ( name != NULL )
        {
          if ( b_rdic_Ins(nl->node_by_name, node_ref, name) != 0 )
          {
            assert(gnlFindNode(nl, name) == node_ref);
            return node_ref;
          }
        }
        else
        {
          return node_ref;
        }
        b_set_Del(nl->node_set, node_ref);
      }
    }
    gnodeClose(node);
  }
  return -1;
}

/* returns net_ref or -1 */
int gnlFindNetByName(gnl nl, const char *name)
{
  int net_ref = -1;
  gnet net;
  while( gnlLoopNetRef(nl, &net_ref) != 0 )
  {
    net = gnlGetGNET(nl, net_ref);
    if ( strcmp(net->name, name) == 0 )
      return net_ref;
  }
  return -1;
}

/* returns net_ref or -1 */
static int gnlStupidFindNetByPort(gnl nl, int node_ref, int port_ref)
{
  int net_ref = -1;
  int join_ref = -1;
  gjoin join;
  while( gnlLoopNetRef(nl, &net_ref) != 0 )
  {
    join_ref = -1;
    while( gnlLoopJoinRef(nl, net_ref, &join_ref) != 0 )
    {
      join = gnlGetGJOIN(nl, net_ref, join_ref);
      if ( join->node == node_ref && join->port == port_ref )
        return net_ref;
    }
  }
  return -1;
}

static int gnlCleverFindNetByPort(gnl nl, int node_ref, int port_ref)
{
  int net_ref = -1;
  int join_ref = -1;
  int loop = -1;
  b_il_type net_refs;
  
  gjoin join;
  
  if ( node_ref >= 0 )
    net_refs = gnlGetGNODE(nl, node_ref)->net_refs;
  else
    net_refs = nl->net_refs;
    
  while( b_il_Loop(net_refs, &loop, &net_ref) != 0 )
  {
    join_ref = -1;
    while( gnlLoopJoinRef(nl, net_ref, &join_ref) != 0 )
    {
      join = gnlGetGJOIN(nl, net_ref, join_ref);
      if ( join->node == node_ref && join->port == port_ref )
        return net_ref;
    }
  }
  return -1;
}

/* returns net_ref or -1 */
int gnlFindNetByPort(gnl nl, int node_ref, int port_ref)
{
  int net_ref;

  net_ref = gnlCleverFindNetByPort(nl, node_ref, port_ref);
  if ( net_ref < 0 )
  {
    net_ref = gnlStupidFindNetByPort(nl, node_ref, port_ref);
    /* 11 jan 2002: remove stupid search some day! It should not */
    /* be required any more. */
    assert( net_ref < 0 );
  }

  return net_ref;
}

/* returns net_ref or -1 */
int gnlFindNetByNode(gnl nl, int node_ref)
{
  int net_ref = -1;
  int join_ref = -1;
  gjoin join;
  while( gnlLoopNetRef(nl, &net_ref) != 0 )
  {
    join_ref = -1;
    while( gnlLoopJoinRef(nl, net_ref, &join_ref) != 0 )
    {
      join = gnlGetGJOIN(nl, net_ref, join_ref);
      if ( join->node == node_ref )
        return net_ref;
    }
  }
  return -1;
}

int gnlAddNet(gnl g, const char *name)
{
  gnet net;
  int net_ref;
  net = gnetOpen();
  if ( net != NULL )
  {
    if ( gnetSetName(net, name) != 0 )
    {
      net_ref = b_set_Add(g->net_set, net);
      if ( net_ref >=0 )
      {
        return net_ref;
      }
    }
    gnetClose(net);
  }
  return -1;
}

void gnlDelNet(gnl nl, int net_ref)
{
  char *name;
  int node_ref = -1;
  int join_ref = -1;

  assert( net_ref >= 0 );
  assert( net_ref < nl->net_set->list_max );
  assert( gnlGetGNET(nl, net_ref) != NULL );
  
  /* remove the net_refs from each node... */
  /* hmmm... replaced the following code... */
  /*
  while( gnlLoopNodeRef(nl, &node_ref) != 0 )
    gnodeDelNet(gnlGetGNODE(nl, node_ref), net_ref, -1);
  */
    
  /* remove only those net_refs from nodes, that are part of the net */
  while( gnlLoopJoinRef(nl, net_ref, &join_ref) != 0 )
  {
    node_ref = gnlGetGJOIN(nl, net_ref, join_ref)->node;
    if ( node_ref >= 0 )
      gnodeDelNet(gnlGetGNODE(nl, node_ref), net_ref, -1);
  }
  
  /* ... and from the cell connections */
  b_il_DelByVal(nl->net_refs, net_ref);
  
  /* delete the net */
  name = gnlGetGNET(nl, net_ref)->name;
  if ( name != NULL )
    b_rdic_Del(nl->net_by_name, (void *)name);
  gnetClose(gnlGetGNET(nl, net_ref));
  b_set_Del(nl->net_set, net_ref);
}

void gnlDelEmptyNet(gnl nl)
{
  int net_ref = -1;
  gnet net;
  
  while( gnlLoopNetRef(nl, &net_ref) != 0 )
  {
    net = gnlGetGNET(nl, net_ref);
    if ( b_set_Cnt(net->join_set) == 0 )
      gnlDelNet(nl, net_ref);
  }
}

/* deletes net_ref2 */
int gnlMergeNets(gnl nl, int net_ref, int net_ref2)
{
  int join_ref = -1;
  gnet net2 = gnlGetGNET(nl, net_ref2);
  gjoin join;
  while( gnetLoopJoinRef(net2, &join_ref) != 0 )
  {
    join = gnetGetGJOIN(net2, join_ref);
    if ( gnlAddNetPort(nl, net_ref, join->node, join->port, 0) < 0 )
      return 0;
  }
  gnlDelNet(nl, net_ref2);
  return 1;
}

/*
  node_ref: specifies a node of the parents node array.
  port_ref: a reference to the port of a cell of the specified node.
  node_ref can be -1

  returns join_ref
*/
int gnlAddNetPort(gnl nl, int net_ref, int node_ref, int port_ref, int is_unique)
{
  gnet net;
  gnode node;
  assert( port_ref >= 0 );
  assert( net_ref >= 0 );
  assert( net_ref < nl->net_set->list_max );
  
  /* usuallay all node/port pairs must be unique, but checking this,  */
  /* might be a great slow down, so this is optional. The 'is_unique' */
  /* flag advices this function to check for existing pairs. One */
  /* additional problem is that another net might be deleted, if */
  /* a pair exists and both nets are merged */
  
  if ( is_unique != 0 )
  {
    int existing_net_ref = gnlFindNetByPort(nl, node_ref, port_ref);
    /* there is nothing to do, if the pair does not exist */
    /* Do not do anything if the pair exists in the target net */
    if ( existing_net_ref >= 0 && existing_net_ref != net_ref )
    {
      /* existing_net_ref will be deleted */
      if ( gnlMergeNets(nl, net_ref, existing_net_ref) == 0 )
        return 0;
    }
  }


  /* inform the node about the new connection */
  /* this is only used for speed up!!! */
  if ( node_ref >= 0 )
  {
    node = gnlGetGNODE(nl, node_ref);
    assert( node != NULL );
    if ( gnodeAddNet(node, net_ref, port_ref) < 0 )
      return -1;
  }
  else
  {
    if ( b_il_AddUnique(nl->net_refs, net_ref) < 0 )
      return 0;
  }

  /* assign the connection to the 'net' structure */  
  net = gnlGetGNET(nl, net_ref);
  assert( net != NULL );
  return gnetAddPort(net, node_ref, port_ref);
}

/*
  the net_refs cache is indeed only a cache.
  It might appear to be exact, but it fails
  in cases where two ports of the same node are 
  connected to the same net. In this case, the 
  following delete function would remove the net 
  although there is still a connection to the removed net.
  
  11. jan 2002: Decided to store the cache entry until
  the last port of a node is remove from a net.
*/
void gnlDelCellNetJoin(gnl nl, int net_ref, int join_ref)
{
  gnet net;
  int node_ref;
  assert( net_ref >= 0 );
  assert( net_ref < nl->net_set->list_max );

  net = gnlGetGNET(nl, net_ref);
  assert( net != NULL );

  /* inform the node about the deletion of its connection */  
  node_ref = gnetGetGJOIN(net, join_ref)->node;
  if ( node_ref >= 0 )  /* only if this is an existing node (not a parent) */
  {
    int port_ref = gnetGetGJOIN(net, join_ref)->port;
    gnode node = gnlGetGNODE(nl, node_ref);
    if ( gnetGetNodeCnt(net, node_ref) == 1 )  /* 11. jan 2002 */
      gnodeDelNet(node, net_ref, port_ref);
  }
  else
  {
    if ( gnetGetNodeCnt(net, -1) == 1 )     /* 11. jan 2002 */
      b_il_DelByVal(nl->net_refs, net_ref);
  }
  
  /* now, delete the connection */
  gnetDelPort(net, join_ref);
  
}

int gnlGetMemUsage(gnl nl)
{
  int i;
  size_t m = sizeof(struct _gnl_struct);
  
  i = -1;
  while( gnlLoopNodeRef(nl, &i) )
    m += gnodeGetMemUsage(gnlGetGNODE(nl, i));
    
  i = -1;
  while( gnlLoopNetRef(nl, &i) )
    m += gnetGetMemUsage(gnlGetGNET(nl, i));
    
  m += b_set_GetMemUsage(nl->node_set);
  m += b_set_GetMemUsage(nl->net_set);
  
  m += b_il_GetMemUsage(nl->net_refs);
  
  return m;
}

static int gnl_write_node_el(FILE *fp, void *el, void *ud)
{
  return gnodeWrite((gnode)el, fp);
}

static int gnl_write_net_el(FILE *fp, void *el, void *ud)
{
  return gnetWrite((gnet)el, fp);
}

int gnlWrite(gnl nl, FILE *fp)
{ 
  if ( nl == NULL )
    return b_io_WriteInt(fp, 0);
  if ( b_io_WriteInt(fp, -1) == 0 )                                   return 0;
  if ( b_set_Write(nl->node_set, fp, gnl_write_node_el, NULL) == 0 ) return 0;
  if ( b_set_Write(nl->net_set, fp, gnl_write_net_el, NULL) == 0 )   return 0;
  return 1;
}

static void *gnl_read_node_el(FILE *fp, void *ud)
{
  gnode node = gnodeOpen();
  if ( node == NULL )
    return NULL;
  if ( gnodeRead(node, fp) == 0 )
    return gnodeClose(node), (void *)NULL;  
  return node;
}

static void *gnl_read_net_el(FILE *fp, void *ud)
{
  gnet net = gnetOpen();
  if ( net == NULL )
    return NULL;
  if ( gnetRead(net, fp) == 0 )
    return gnetClose(net), (void *)NULL;
  return net;
}

int gnl_read(gnl nl, FILE *fp)
{
  int i;
    
  b_rdic_Clear(nl->node_by_name);
  b_rdic_Clear(nl->net_by_name);

  if ( b_set_Read(nl->node_set, fp, gnl_read_node_el, NULL) == 0 )   return 0;
  if ( b_set_Read(nl->net_set, fp, gnl_read_net_el, NULL) == 0 )     return 0;
  
  i = -1;
  while( gnlLoopNodeRef(nl, &i) != 0 )
    if ( gnlGetGNODE(nl,i)->name != NULL )
      if ( b_rdic_Ins(nl->node_by_name, i, gnlGetGNODE(nl,i)->name) == 0 )
        return 0;

  i = -1;
  while( gnlLoopNetRef(nl, &i) != 0 )
    if ( gnlGetGNET(nl,i)->name != NULL )
      if ( b_rdic_Ins(nl->net_by_name, i, gnlGetGNET(nl,i)->name) == 0 )
        return 0;
  
  return 1;
}

int gnlRead(gnl nl, FILE *fp)
{
  int chk;
  if ( b_io_ReadInt(fp, &chk) == 0 )
    return 0;
  if ( chk == 0 )
    return 1;
    
  return gnl_read(nl, fp);
}

gnl gnlOpenRead(FILE *fp)
{
  gnl nl;
  int chk;
  if ( b_io_ReadInt(fp, &chk) == 0 )
    return NULL;
  if ( chk == 0 )
    return NULL;
  nl = gnlOpen();
  if ( nl == NULL )
    return NULL;
  if ( gnl_read(nl, fp) == 0 )
    return gnlClose(nl), (gnl)NULL;
  return nl;
}

/*----------------------------------------------------------------------------*/

void gciomClear(gciom ciom)
{
  int i;
  for( i = 0; i < ciom->in_cnt*ciom->out_cnt; i++ )
  {
    ciom->is_used[i] = 0;
    ciom->min_delay[i] = 0.0;
    ciom->max_delay[i] = 0.0;
  }
  ciom->max = 0.0;
  ciom->min = -1.0;
}

gciom gciomOpen(int in, int out)
{
  gciom ciom;
  ciom = (gciom)malloc(sizeof(struct _gciom_struct));
  if ( ciom != NULL )
  {
    ciom->in_cnt = in;
    ciom->out_cnt = out;
    ciom->min_delay = (double *)malloc(sizeof(double)*in*out);
    if ( ciom->min_delay != NULL )
    {
      ciom->max_delay = (double *)malloc(sizeof(double)*in*out);
      if ( ciom->max_delay != NULL )
      {
        ciom->is_used = (char *)malloc(sizeof(char)*in*out);
        if ( ciom->is_used != NULL )
        {
          gciomClear(ciom);
          return ciom;
        }
        free(ciom->max_delay);
      }
      free(ciom->min_delay);
    }
    free(ciom);
  }
  return NULL;
}

void gciomClose(gciom ciom)
{
  free(ciom->is_used);
  free(ciom->min_delay);
  free(ciom->max_delay);
  free(ciom);
}

int gciomWrite(gciom ciom, FILE *fp)
{
  int i, cnt;

  if ( ciom == NULL )
  {
    if ( b_io_WriteInt(fp, 0) == 0 )  
      return 0;
    return 1;
  }
  
  if ( b_io_WriteInt(fp, 1) == 0 )               return 0;
  if ( b_io_WriteInt(fp, ciom->in_cnt) == 0 )    return 0;
  if ( b_io_WriteInt(fp, ciom->out_cnt) == 0 )   return 0;
  cnt = ciom->in_cnt * ciom->out_cnt;
  for( i = 0; i < cnt; i++ )
  {
    if ( b_io_WriteDouble(fp, ciom->min_delay[i]) == 0 )  return 0;
    if ( b_io_WriteDouble(fp, ciom->max_delay[i]) == 0 )  return 0;
    if ( b_io_WriteChar(fp, ciom->is_used[i]) == 0 )      return 0;
  }
  if ( b_io_WriteDouble(fp, ciom->max) == 0 )  return 0;
  if ( b_io_WriteDouble(fp, ciom->min) == 0 )  return 0;
  return 1;
}

int gciomRead(gciom *ciom, FILE *fp)
{
  int in, out;
  int i, cnt;
  
  if ( *ciom != NULL )
    gciomClose(*ciom);
  *ciom = NULL;
  
  
  if ( b_io_ReadInt(fp, &(in)) == 0 )    return 0;
  if ( in == 0 )
    return 1;
  
  if ( b_io_ReadInt(fp, &(in)) == 0 )    return 0;
  if ( b_io_ReadInt(fp, &(out)) == 0 )   return 0;
  
  *ciom = gciomOpen(in, out);
  if ( *ciom == NULL )
    return 0;
  
  cnt = in*out;
  for( i = 0; i < cnt; i++ )
  {
    if ( b_io_ReadDouble(fp, &((*ciom)->min_delay[i])) == 0 )  return 0;
    if ( b_io_ReadDouble(fp, &((*ciom)->max_delay[i])) == 0 )  return 0;
    if ( b_io_ReadChar(fp, &((*ciom)->is_used[i])) == 0 )      return 0;
  }
  if ( b_io_ReadDouble(fp, &((*ciom)->max)) == 0 )  return 0;
  if ( b_io_ReadDouble(fp, &((*ciom)->min)) == 0 )  return 0;
  return 1;
}

size_t gciomGetMemUsage(gciom ciom)
{
  size_t m = sizeof(struct _gciom_struct);
  m += ciom->in_cnt*ciom->out_cnt*(2*sizeof(double)+sizeof(char));
  return m;
}


/*----------------------------------------------------------------------------*/

int gcellSetName(gcell cell, const char *name)
{
  return internal_gnet_set_name(&(cell->name), name);
}

int gcellSetLibrary(gcell cell, const char *library)
{
  return internal_gnet_set_name(&(cell->library), library);
}

int gcellSetDescription(gcell cell, const char *desc)
{
  return internal_gnet_set_name(&(cell->desc), desc);
}


/*
int gcellGetGPORTCnt(gcell t)
{
  return b_pl_GetCnt(t->port_list);
}
*/

/*
gpin gcellGetGPORT(gcell t, int pos)
{
  return (gport)b_pl_GetVal(t->port_list, pos);
}
*/

gcell gcellOpen(const char *name, const char *library)
{
  gcell cell;
  cell = (gcell)malloc(sizeof(struct _gcell_struct));
  if ( cell != NULL )
  {
    cell->nl = NULL;
    cell->name = NULL;
    cell->library = NULL;
    cell->desc = NULL;
    cell->id = GCELL_ID_UNKNOWN;
    cell->area = 0.0;
    cell->in_cnt = 0;
    cell->out_cnt = 0;
    cell->flag = 0;
    
    cell->register_width = 0;
    cell->cl_on = NULL;
    cell->cl_dc = NULL;
    cell->cl_off = NULL;
    cell->pi = NULL;

    cell->ciom = NULL;
    cell->port_set = b_set_Open();
    if ( cell->port_set != NULL )
    {
      if ( gcellSetName(cell, name) != 0 )
      {
        if ( gcellSetLibrary(cell, library) != 0 )
        {
          return cell;
        }
        gcellSetName(cell, NULL);
      }
      b_set_Close(cell->port_set);
    }
    free(cell);
  }
  return NULL;
}

gnl gcellGetGNL(gcell cell, int is_create)
{
  if ( cell->nl != NULL )
    return cell->nl;
  if ( is_create == 0 )
    return NULL;
  cell->nl = gnlOpen();
  return cell->nl;
}

int gcellGetNetCnt(gcell cell)
{
  gnl nl = gcellGetGNL(cell, 0);
  if ( nl == NULL )
    return 0;
  return gnlGetNetCnt(nl);
}

void gcellClear(gcell cell)
{
  int i = -1;
  
  if ( cell->nl != NULL )
  {
    gnlClose(cell->nl);
    cell->nl = NULL;
  }
  
  while(gcellLoopPortRef(cell, &i))
    gportClose(gcellGetGPORT(cell, i));
    
  b_set_Clear(cell->port_set);
  if ( cell->cl_on != NULL )
    dclDestroy(cell->cl_on);
  cell->cl_on = NULL;
  if ( cell->cl_off != NULL )
    dclDestroy(cell->cl_off);
  cell->cl_off = NULL;
  if ( cell->cl_dc != NULL )
    dclDestroy(cell->cl_dc);
  cell->cl_dc = NULL;
  if ( cell->pi != NULL )
    pinfoClose(cell->pi);
  cell->pi = NULL;
  if ( cell->ciom != NULL )
    gciomClose(cell->ciom);
  cell->ciom = NULL;
}

void gcellClose(gcell cell)
{
  gcellClear(cell);
  gcellSetName(cell, NULL);
  gcellSetLibrary(cell, NULL);
  gcellSetDescription(cell, NULL);
  b_set_Close(cell->port_set);
  free(cell);
}

/* returns positon or -1 */
int gcellAddGPORT(gcell cell, gport p)
{
  return b_set_Add(cell->port_set, p);
}

void gcellCalcInOutCnt(gcell cell)
{
  int port_ref = -1;
  cell->in_cnt = 0;
  cell->out_cnt = 0;
  while( gcellLoopPortRef(cell, &port_ref) != 0 )
  {
    switch( gcellGetGPORT(cell, port_ref)->type )
    {
      case GPORT_TYPE_IN: 
        cell->in_cnt++;
        break;
      case GPORT_TYPE_OUT:
        cell->out_cnt++;
        break;
    }
  }
}

/* returns 0 or 1 */
int gcellSetGPORT(gcell cell, int pos, gport p)
{
  gport prev_port = NULL;
  if ( pos < cell->port_set->list_max )
    prev_port = gcellGetGPORT(cell, pos);
  if ( prev_port != NULL )
    gportClose(prev_port);
  if ( b_set_Set(cell->port_set, pos, p) == 0 )
    return 0;
  gcellCalcInOutCnt(cell);
  return 1;
}

/* returns position or -1 */
int gcellAddPortByName(gcell cell, int port_type, const char *port_name)
{
  gport p;
  int pos;
  switch(port_type)
  {
    case GPORT_TYPE_IN: 
      cell->in_cnt++;
      break;
    case GPORT_TYPE_OUT:
      cell->out_cnt++;
      break;
  }
  p = gportOpen(port_type, port_name);
  if ( p != NULL )
  {
    pos = gcellAddGPORT(cell, p);
    if ( pos >= 0 )
    {
      return pos;
    }
    gportClose(p);
  }
  switch(port_type)
  {
    case GPORT_TYPE_IN: 
      cell->in_cnt--;
      break;
    case GPORT_TYPE_OUT:
      cell->out_cnt--;
      break;
  }
  return -1;
}

/* returns 0 or 1, does not update pinfo */
int gcellSetPortByName(gcell cell, int pos, int port_type, const char *port_name)
{
  gport p;
  p = gportOpen(port_type, port_name);
  if ( p == NULL )
    return 0;
  if ( gcellSetGPORT(cell, pos, p) == 0 )
  {
    gportClose(p);
    return 0;
  }
  return 1;
}

/* returns port_ref */
int gcellFindPort(gcell cell, const char *port_name)
{
  int i = -1;
  while(gcellLoopPortRef(cell, &i))
    if ( strcmp( gcellGetGPORT(cell, i)->name, port_name ) == 0 )
      return i;
  return -1;
}

/* returns the first port port_ref with the specified type */
int gcellFindPortByType(gcell cell, int type)
{
  int i = -1;
  while(gcellLoopPortRef(cell, &i))
    if ( gcellGetGPORT(cell, i)->type == type )
      return i;
  return -1;
}

/* returns the first port port_ref with the specified type */
int gcellFindPortByFn(gcell cell, int fn)
{
  int i = -1;
  while(gcellLoopPortRef(cell, &i))
    if ( gcellGetGPORT(cell, i)->fn == fn )
      return i;
  return -1;
}

/* returns the first port port_ref with the specified type */
int gcellGetNthPort(gcell cell, int n, int type)
{
  int i = -1;
  while(gcellLoopPortRef(cell, &i))
  {
    if ( gcellGetGPORT(cell, i)->type == type )
    {
      if ( n == 0 )
        return i;
      n--;
    }
  }
  return -1;
}


void gcellSetPortFn(gcell cell, int port_ref, int fn, int is_inverted)
{
  assert(port_ref >= 0);
  assert(port_ref < cell->port_set->list_max);
  assert(gcellGetGPORT(cell, port_ref) != NULL);
  gportSetFn(gcellGetGPORT(cell, port_ref), fn, is_inverted);
}

int gcellGetPortFn(gcell cell, int port_ref)
{
  assert(port_ref >= 0);
  assert(port_ref < cell->port_set->list_max);
  assert(gcellGetGPORT(cell, port_ref) != NULL);
  return gportGetFn(gcellGetGPORT(cell, port_ref));
}

int gcellIsPortInverted(gcell cell, int port_ref)
{
  assert(port_ref >= 0);
  assert(port_ref < cell->port_set->list_max);
  assert(gcellGetGPORT(cell, port_ref) != NULL);
  return gportIsInverted(gcellGetGPORT(cell, port_ref));
}

/* return the delay structure from port_ref to related_port_ref */
gpoti gcellGetPortGPOTI(gcell cell, int port_ref, int related_port_ref, int is_create)
{
  assert(port_ref >= 0);
  assert(port_ref < cell->port_set->list_max);
  assert(gcellGetGPORT(cell, port_ref) != NULL);
  return gportGetGPOTI(gcellGetGPORT(cell, port_ref), related_port_ref, is_create);
}


void gcellSetArea(gcell cell, double area)
{
  cell->area = area;
}

double gcellGetArea(gcell cell)
{
  return cell->area;
}

void gcellSetId(gcell cell, int id)
{
  cell->id = id;
}

int gcellGetId(gcell cell)
{
  return cell->id;
}

void gcellSetPortInputLoad(gcell cell, int port_ref, double cap)
{
  assert(port_ref >= 0);
  assert(port_ref < cell->port_set->list_max);
  assert(gcellGetGPORT(cell, port_ref) != NULL);
  gportSetInputLoad(gcellGetGPORT(cell, port_ref), cap);
}

double gcellGetPortInputLoad(gcell cell, int port_ref)
{
  assert(port_ref >= 0);
  assert(port_ref < cell->port_set->list_max);
  assert(gcellGetGPORT(cell, port_ref) != NULL);
  return gportGetInputLoad(gcellGetGPORT(cell, port_ref));
}


int gcellGetInCnt(gcell cell)
{
  return cell->in_cnt;
}


size_t gcellGetMemUsage(gcell cell)
{
  size_t m = sizeof(struct _gcell_struct);
  int i;

  m += internal_gnet_get_name_mem_usage(cell->name);
  m += internal_gnet_get_name_mem_usage(cell->library);
  m += internal_gnet_get_name_mem_usage(cell->desc);
  
  i = -1;
  while(gcellLoopPortRef(cell, &i))
    m += gportGetMemUsage(gcellGetGPORT(cell, i));
  m += b_set_GetMemUsage(cell->port_set);
  
  if ( cell->nl != NULL )
    m += gnlGetMemUsage(cell->nl);
    
  if ( cell->ciom != NULL )
    m += gciomGetMemUsage(cell->ciom);
    
  return m;
}


static int gcell_write_port_el(FILE *fp, void *el, void *ud)
{
  return gportWrite((gport)el, fp);
}

int gcellWrite(gcell cell, FILE *fp, const char *libname)
{ 
  
  if ( b_io_WriteInt(fp, 0x4c4c4543) == 0 )                            return 0;
  if ( b_io_WriteString(fp, cell->name) == 0 )                         return 0;
  if ( libname == NULL )
    if ( b_io_WriteString(fp, cell->library) == 0 )                    return 0;
  if ( b_io_WriteString(fp, cell->desc) == 0 )                         return 0;
  if ( b_set_Write(cell->port_set, fp, gcell_write_port_el, NULL) == 0 )return 0;
  if ( gnlWrite(cell->nl, fp) == 0 ) return 0;
  if ( b_io_WriteInt(fp, cell->id) == 0 )                              return 0;
  if ( b_io_WriteInt(fp, cell->in_cnt) == 0 )                          return 0;
  if ( b_io_WriteInt(fp, cell->out_cnt) == 0 )                         return 0;
  
  if ( b_io_WriteDouble(fp, cell->area) == 0 )                         return 0;
  if ( b_io_WriteUnsignedInt(fp, cell->flag) == 0 )                    return 0;
  if ( b_io_WriteInt(fp, cell->register_width) == 0 )                  return 0;
  
  if ( pinfoWrite(cell->pi, fp) == 0 )                                 return 0;
  if ( dclWriteBin(cell->pi, cell->cl_on, fp) == 0 )                   return 0;
  if ( dclWriteBin(cell->pi, cell->cl_dc, fp) == 0 )                   return 0;
  if ( dclWriteBin(cell->pi, cell->cl_off, fp) == 0 )                  return 0;

  
  if ( gciomWrite(cell->ciom, fp) == 0 )                               return 0;
  return 1;
}

static void *gcell_read_port_el(FILE *fp, void *ud)
{
  gport port = gportOpen(GPORT_TYPE_BI, NULL);
  if ( port == NULL )
    return NULL;
  if ( gportRead(port, fp) == 0 )
    return gportClose(port), (void *)NULL;
  return port;
}

int gcellRead(gcell cell, FILE *fp, const char *libname)
{
  int magic;
  if ( b_io_ReadInt(fp, &magic) == 0 )
    return 0;
  if ( magic != 0x4c4c4543 )                                           
    return 0;
  gcellSetName(cell, NULL);
  if ( gcellSetLibrary(cell, libname) == 0 )                           
    return 0;
  gcellSetDescription(cell, NULL);
  if ( b_io_ReadAllocString(fp, &(cell->name)) == 0 )                  
    return 0;
  if ( libname == NULL )
    if ( b_io_ReadAllocString(fp, &(cell->library)) == 0 )             
      return 0;
  if ( b_io_ReadAllocString(fp, &(cell->desc)) == 0 )                  
    return 0;
  if ( b_set_Read(cell->port_set, fp, gcell_read_port_el, NULL) == 0 ) return 0;
  if ( cell->nl != NULL )
    gnlClose(cell->nl);
  cell->nl = gnlOpenRead(fp);
  if ( b_io_ReadInt(fp, &(cell->id)) == 0 )                            
    return 0;
  if ( b_io_ReadInt(fp, &(cell->in_cnt)) == 0 )                        
    return 0;
  if ( b_io_ReadInt(fp, &(cell->out_cnt)) == 0 )                       
    return 0;
  
  if ( b_io_ReadDouble(fp, &(cell->area)) == 0 )                       
    return 0;
  if ( b_io_ReadUnsignedInt(fp, &(cell->flag)) == 0 )                  
    return 0;
  if ( b_io_ReadInt(fp, &(cell->register_width)) == 0 )                
    return 0;

  if ( pinfoRead(&(cell->pi), fp) == 0 )                               
    return 0;
  if ( dclReadBin(cell->pi, &(cell->cl_on), fp) == 0 )                 
    return 0;
  if ( dclReadBin(cell->pi, &(cell->cl_dc), fp) == 0 )                 
    return 0;
  if ( dclReadBin(cell->pi, &(cell->cl_off), fp) == 0 )                
    return 0;
  
  if ( gciomRead(&(cell->ciom), fp) == 0 )                             
    return 0;
  return 1;
}

int gcellInitDelay(gcell cell)
{
  if ( cell == NULL )
    return 0;
  if ( cell->ciom != NULL )
    gciomClose(cell->ciom);
  cell->ciom = gciomOpen(b_set_Cnt(cell->port_set), cell->out_cnt);
  if ( cell->ciom == 0 )
    return 0;
  return 1;
}

static int gcell_port_ref_to_index(gcell cell, int port_ref, int type)
{
  int i, j;
  gport port;
  
  assert(gcellGetGPORT(cell, port_ref)->type == type);
  
  i = -1;
  j = 0;
  while( gcellLoopPortRef(cell, &i) != 0 )
  {
    if ( i == port_ref )
      return j;
    port = gcellGetGPORT(cell, i);
    if ( port->type == type )
      j++;
  }
  return -1;
}

static int gcell_get_io_matrix_pos(gcell cell, int in_out_port_ref, int out_port_ref)
{
  int in_out_idx;
  int out_idx;

  if ( cell == NULL )
    return -1;
  if ( cell->ciom == NULL )
    return -1;
  if ( cell->ciom->in_cnt != b_set_Cnt(cell->port_set) || cell->ciom->out_cnt != cell->out_cnt )
    return -1;
  
  in_out_idx = in_out_port_ref;
  if ( in_out_idx < 0 )
    return -1;
  out_idx = gcell_port_ref_to_index(cell, out_port_ref, GPORT_TYPE_OUT);
  if ( out_idx < 0 )
    return -1;
  return in_out_idx*cell->out_cnt+out_idx;
}

int gcellSetDelay(gcell cell, int in_out_port_ref, int out_port_ref, double delay)
{
  int pos = gcell_get_io_matrix_pos(cell, in_out_port_ref, out_port_ref);
  if ( pos < 0 )
    return 0;
  if ( cell->ciom->max < delay )
    cell->ciom->max = delay;

  if ( cell->ciom->min < 0.0 )
    cell->ciom->min = delay;
  else if ( delay >= 0.0 && cell->ciom->min > delay )
    cell->ciom->min = delay;

  if ( cell->ciom->is_used[pos] == 0 )
  {
    cell->ciom->min_delay[pos] = delay;
    cell->ciom->max_delay[pos] = delay;
    cell->ciom->is_used[pos] = 1;
  }
  else
  {
    if ( cell->ciom->min_delay[pos] > delay )
      cell->ciom->min_delay[pos] = delay;
    if ( cell->ciom->max_delay[pos] < delay )
      cell->ciom->max_delay[pos] = delay;
  }
  return 1;
}

double gcellGetDelayMin(gcell cell, int in_out_port_ref, int out_port_ref)
{
  int pos = gcell_get_io_matrix_pos(cell, in_out_port_ref, out_port_ref);
  if ( pos < 0 )
    return 0;
  return cell->ciom->min_delay[pos];
}

double gcellGetDelayMax(gcell cell, int in_out_port_ref, int out_port_ref)
{
  int pos = gcell_get_io_matrix_pos(cell, in_out_port_ref, out_port_ref);
  if ( pos < 0 )
    return 0;
  return cell->ciom->max_delay[pos];
}

int gcellIsDelayUsed(gcell cell, int in_out_port_ref, int out_port_ref)
{
  int pos = gcell_get_io_matrix_pos(cell, in_out_port_ref, out_port_ref);
  if ( pos < 0 )
    return 0;
  return cell->ciom->is_used[pos];
}

double gcellGetDelayTotalMax(gcell cell)
{
  if ( cell->ciom == NULL )
    return 0.0;
  return cell->ciom->max;
}

double gcellGetDelayTotalMin(gcell cell)
{
  if ( cell->ciom == NULL )
    return 0.0;
  return cell->ciom->min;
}



/*----------------------------------------------------------------------------*/

int glibSetName(glib lib, const char *name)
{
  return internal_gnet_set_name(&(lib->name), name);
}

glib glibOpen(const char *name)
{
  glib lib;
  lib = (glib)malloc(sizeof(struct _glib_struct));
  if ( lib != NULL )
  {
    lib->name = NULL;
    lib->cell_cnt = 0;
    if ( glibSetName(lib, name) != 0 )
    {
      return lib;
    }
    free(lib);
  }
  return NULL;
}

void glibClose(glib lib)
{
  glibSetName(lib, NULL);
  free(lib);
}


int glibWrite(glib lib, FILE *fp)
{ 
  if ( b_io_WriteString(fp, lib->name) == 0 )                         return 0;
  if ( b_io_WriteInt(fp, lib->cell_cnt) == 0 )                        return 0;
  return 1;
}

int glibRead(glib lib, FILE *fp)
{
  glibSetName(lib, NULL);
  if ( b_io_ReadAllocString(fp, &(lib->name)) == 0 )                  return 0;
  if ( b_io_ReadInt(fp, &(lib->cell_cnt)) == 0 )                      return 0;
  return 1;
}

/*----------------------------------------------------------------------------*/

void gnc_ErrorVA(gnc nc, const char *fmt, va_list va)
{
  nc->err_fn(nc->err_data, fmt, va);
}

void gnc_Error(gnc nc, const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  gnc_ErrorVA(nc, fmt, va);
  va_end(va);
}

void gnc_default_log_fn(void *data, const char *fmt, va_list va)
{
  vprintf(fmt, va);
  puts("");
}

void gnc_SetLogFn(gnc nc, void (*log_fn)(void *data, const char *fmt, va_list va), void *data)
{
  nc->log_fn = log_fn;
  nc->log_data = data;
}

void gnc_LogVA(gnc nc, int prio, const char *fmt, va_list va)
{
  if ( prio >= nc->log_level )
  {
    nc->log_fn(nc->log_data, fmt, va);
  }
}

void gnc_Log(gnc nc, int prio, const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  gnc_LogVA(nc, prio, fmt, va);
  va_end(va);
}

/* contains triples char*, pinfo *, dclist */
void gnc_LogDCL(gnc nc, int prio, char *prefix, int cnt, ...)
{
  va_list va;
  int line = 0;
  int line_cnt;
  int width[10];
  char buf[1024*2];
  int i;
  char *s;
  pinfo *pi;
  dclist cl;
  line_cnt = 0;
  va_start(va, cnt);
  for( i = 0; i < cnt; i++ )
  {
    s = va_arg(va, char *);
    pi = va_arg(va, pinfo *);
    cl = va_arg(va, dclist);
    width[i] = strlen(s)+1;
    if ( width[i] < pi->in_cnt+pi->out_cnt+3 )
      width[i] = pi->in_cnt+pi->out_cnt+3;
    if ( line_cnt < dclCnt(cl) )
      line_cnt = dclCnt(cl);
  }
  va_end(va);
  
  if ( line_cnt > 10000 )
    return;

  va_start(va, cnt);
  buf[0] = '\0';
  for( i = 0; i < cnt; i++ )
  {
    s = va_arg(va, char *);
    pi = va_arg(va, pinfo *);
    cl = va_arg(va, dclist);
    sprintf(buf+strlen(buf), "%*s", -width[i], s);
  }
  va_end(va);
  gnc_Log(nc, prio, "%s%s", prefix, buf);

  for( line = 0; line < line_cnt; line++ )
  {
    va_start(va, cnt);
    buf[0] = '\0';
    for( i = 0; i < cnt; i++ )
    {
      s = va_arg(va, char *);
      pi = va_arg(va, pinfo *);
      cl = va_arg(va, dclist);
      if ( line < dclCnt(cl))
        sprintf(buf+strlen(buf), "%*s", width[i], dcToStr(pi, dclGet(cl, line), " ", "  "));
      else
        sprintf(buf+strlen(buf), "%*s", width[i], "");
    }
    va_end(va);
    gnc_Log(nc, prio, "%s%s", prefix, buf);
  }
}

void gnc_default_err_fn(void *data, const char *fmt, va_list va)
{
  vprintf(fmt, va);
  puts("");
}

void gnc_SetErrFn(gnc nc, void (*err_fn)(void *data, const char *fmt, va_list va), void *data)
{
  nc->err_fn = err_fn;
  nc->err_data = data;
}

int gnc_SetName(gnc nc, const char *name)
{
  return internal_gnet_set_name(&(nc->name), name);
}

char *gnc_GetName(gnc nc)
{
  return nc->name;
}

struct _gnc_key_struct
{
  char *cell;
  char *library;
};
typedef struct _gnc_key_struct gnc_key_struct;


int gnc_cell_cmp_fn(void *el, void *_key)
{
  gcell gc = (gcell)el;
  gnc_key_struct *key = (gnc_key_struct *)_key;
  int r;
  
  if ( gc->library == NULL && key->library == NULL )
    return strcmp(gc->name, key->cell);
  if ( gc->library == NULL )
    return -1;
  if ( key->library == NULL )
    return 1;
  r = strcmp(gc->library, key->library);
  if ( r != 0 )
    return r;
  return strcmp(gc->name, key->cell);
}

int gnc_Init(gnc nc)
{
  int i;
  nc->wge = NULL;
  for( i = 0; i < GCELL_ID_MAX; i++ )
  {
    nc->bbb_cell_ref[i] = -1;
    nc->bbb_disable[i] = 0;
  }

  gnc_SetErrFn(nc, gnc_default_err_fn, NULL);
  gnc_SetLogFn(nc, gnc_default_log_fn, NULL);
  nc->name = NULL;

  nc->name_clk = NULL;
  nc->name_set = NULL;
  nc->name_clr = NULL;
  
  if ( internal_gnet_set_name(&(nc->name_clk), GPORT_NAME_CLK) == 0 ) return 0;
  if ( internal_gnet_set_name(&(nc->name_set), GPORT_NAME_SET) == 0 ) return 0;
  if ( internal_gnet_set_name(&(nc->name_clr), GPORT_NAME_CLR) == 0 ) return 0;
  
  return 1;
}

/*!
  \ingroup gncinit
  Create a new gnc (gate net collection) object. A gnc object
  is a list of cells. Each cell has some properties:
    - a unique cell name
    - a library name (cells with the same library name are grouped together)
    - an optional netlist
    - an optional boolean description
    
  \return A pointer to the new gnc structure or NULL if a memory error occured. 
  \see gnc_Close()
*/
gnc gnc_Open()
{
  gnc nc;
  nc = (gnc)malloc(sizeof(struct _gnc_struct));
  if ( nc != NULL )
  {
    if ( gnc_Init(nc) != 0 )
    {
      nc->cell_set = b_set_Open();
      if ( nc->cell_set != NULL )
      {
        nc->lib_set = b_set_Open();
        if ( nc->lib_set != NULL )
        {
          if ( syparam_Init(&(nc->syparam)) != 0 )
          {
            nc->log_level = 4;
            nc->max_asynchronous_delay_depth = 100;
            nc->simulation_loop_abort = 500;
            nc->async_delay_cell_ref = -1;
            return nc;
          }
          b_set_Close(nc->lib_set);
        }
        b_set_Close(nc->cell_set);
      }
    }
    internal_gnet_set_name(&(nc->name_clk), NULL);
    internal_gnet_set_name(&(nc->name_set), NULL);
    internal_gnet_set_name(&(nc->name_clr), NULL);
    free(nc); 
  }
  return NULL;
}

/*!
  \ingroup gncinit
  Destroy a gnc object. All memory occupied by the object is released.
  \param nc A pointer to a gnc structure.
  \see gnc_Open()
*/
void gnc_Close(gnc nc)
{
  int i;
  
  internal_gnet_set_name(&(nc->name_clk), NULL);
  internal_gnet_set_name(&(nc->name_set), NULL);
  internal_gnet_set_name(&(nc->name_clr), NULL);
  
  if ( nc->wge != NULL )
    wge_Close(nc->wge);
  nc->wge = NULL;
  
  gnc_SetName(nc, NULL);
  
  i = -1;
  while( gnc_LoopCell(nc, &i) )
    gcellClose(gnc_GetGCELL(nc, i));

  i = -1;
  while( gnc_LoopLib(nc, &i) )
    glibClose(gnc_GetGLIB(nc, i));

  b_set_Close(nc->cell_set);
  b_set_Close(nc->lib_set);
  
  free(nc);
}

static int gnc_write_cell_el(FILE *fp, void *el, void *ud)
{
  return gcellWrite((gcell)el, fp, NULL);
}

static int gnc_write_lib_el(FILE *fp, void *el, void *ud)
{
  return glibWrite((glib)el, fp);
}

int gnc_Write(gnc nc, FILE *fp)
{ 
  if ( b_io_WriteString(fp, nc->name) == 0 )                           return 0;
  if ( b_set_Write(nc->cell_set, fp, gnc_write_cell_el, NULL) == 0 )   return 0;
  if ( b_set_Write(nc->lib_set, fp, gnc_write_lib_el, NULL) == 0 )     return 0;
  if ( b_io_WriteIntArray(fp, GCELL_ID_MAX, nc->bbb_cell_ref) == 0 )   return 0;
  if ( b_io_WriteIntArray(fp, GCELL_ID_MAX, nc->bbb_disable) == 0 )    return 0;
  if ( b_io_WriteInt(fp, nc->log_level) == 0 )                         return 0;
  return 1;
}

int gnc_WriteBin(gnc nc, char *name)
{
  FILE *fp;
  fp = fopen(name, "wb");
  if ( fp == NULL )
    return 0;
  if ( gnc_Write(nc, fp) == 0 )
    return fclose(fp), 0;
  return fclose(fp), 1;
}

static void *gnc_read_cell_el(FILE *fp, void *ud)
{
  gcell cell = gcellOpen(NULL, NULL);
  if ( cell == NULL )
    return NULL;
  if ( gcellRead(cell, fp, NULL) == 0 )
    gcellClose(cell);
    return NULL;
  return cell;
}

static void *gnc_read_lib_el(FILE *fp, void *ud)
{
  glib lib = glibOpen(NULL);
  if ( lib == NULL )
    return NULL;
  if ( glibRead(lib, fp) == 0 )
  {
    glibClose(lib);
    return NULL;
  }
  return lib;
}

int gnc_Read(gnc nc, FILE *fp)
{
  gnc_SetName(nc, NULL);
  if ( b_io_ReadAllocString(fp, &(nc->name)) == 0 )                    return 0;
  if ( b_set_Read(nc->cell_set, fp, gnc_read_cell_el, NULL) == 0 )     return 0;
  if ( b_set_Read(nc->lib_set, fp, gnc_read_lib_el, NULL) == 0 )       return 0;
  if ( b_io_ReadIntArray(fp, GCELL_ID_MAX, nc->bbb_cell_ref) == 0 )    return 0;
  if ( b_io_ReadIntArray(fp, GCELL_ID_MAX, nc->bbb_disable) == 0 )     return 0;
  if ( b_io_ReadInt(fp, &(nc->log_level)) == 0 )                       return 0;
  
  return 1;
}

int gnc_ReadBin(gnc nc, char *name)
{
  FILE *fp;
  fp = fopen(name, "rb");
  if ( fp == NULL )
    return 0;
  if ( gnc_Read(nc, fp) == 0 )
    return fclose(fp), 0;
  return fclose(fp), 1;
}

int gnc_FindLib(gnc nc, const char *library)
{
  int lib_ref = -1;
  glib lib;
  while( gnc_LoopLib(nc, &lib_ref) )
  {
    lib = gnc_GetGLIB(nc, lib_ref);
    if ( library == NULL )
    {
      if ( lib->name == NULL )
        return lib_ref;
    }
    else
    {
      if ( lib->name == NULL )
      {
        if ( library == NULL )
          return lib_ref;
      }
      else 
      {
        if ( strcmp(lib->name, library) == 0 )
          return lib_ref;
      }
    }
  }
  return -1;
}

void gnc_DelLib(gnc nc, const char *library)
{
  int cell_ref = -1;
  while( gnc_LoopCell(nc, &cell_ref) != 0 )
  {
    if ( gnc_GetCellLibraryName(nc, cell_ref) == NULL )
    {
      if ( library == NULL )
        gnc_DelCell(nc, cell_ref);
    }
    else
    {
      if ( strcmp(library, gnc_GetCellLibraryName(nc, cell_ref)) == 0 )
        gnc_DelCell(nc, cell_ref);
    }
  }
}

static int gnc_AddLib(gnc nc, const char *library)
{
  int lib_ref;
  glib lib;
  
  lib_ref = gnc_FindLib(nc, library);
  if ( lib_ref >= 0 )
    return lib_ref;
    
  lib = glibOpen(library);
  if ( lib != NULL )
  {
    lib_ref = b_set_Add(nc->lib_set, lib);
    if ( lib_ref >= 0 )
    {
      return lib_ref;
    }
    glibClose(lib);
  }
  return -1;
}

char *gnc_GetLibName(gnc nc, int lib_ref)
{
  return gnc_GetGLIB(nc, lib_ref)->name;
}

/* returns 0 (false) or 1 (true) */
static int gnc_register_cell(gnc nc, gcell cell)
{
  glib lib;
  int lib_ref;
  lib_ref = gnc_AddLib(nc, cell->library);
  if ( lib_ref < 0 )
    return 0;
  lib = gnc_GetGLIB(nc, lib_ref);
  lib->cell_cnt++;
  return 1;
}

/* returns 0 (false) or 1 (true) */
static int gnc_RegisterCell(gnc nc, int cell_ref)
{
  assert(cell_ref >= 0);
  return gnc_register_cell(nc, gnc_GetGCELL(nc, cell_ref));
}

static void gnc_UnregisterCell(gnc nc, int cell_ref)
{
  gcell cell;
  glib lib;
  int lib_ref;
  assert(cell_ref >= 0);
  cell = gnc_GetGCELL(nc, cell_ref);
  lib_ref = gnc_AddLib(nc, cell->library);
  if ( lib_ref < 0 )
    return;
  lib = gnc_GetGLIB(nc, lib_ref);
  assert(lib->cell_cnt > 0);
  lib->cell_cnt--;
  
  if ( lib->cell_cnt <= 0 )
  {
    glibClose(lib);
    b_set_Del(nc->lib_set, lib_ref);
  }
}

gnl gnc_GetGNL(gnc nc, int cell_ref, int is_create)
{
  assert(cell_ref >= 0);
  return gcellGetGNL(gnc_GetGCELL(nc, cell_ref), is_create);
}

gnode gnc_GetCellGNODE(gnc nc, int cell_ref, int node_ref)
{
  /* gnl nl = gnc_GetGNL(nc, cell_ref, 0); */
  gnl nl = gnc_GetGCELL(nc, cell_ref)->nl;  /* only for speed improvements */
  if ( nl == NULL )
    return NULL;
  return gnlGetGNODE(nl, node_ref);
}

gnet gnc_GetCellGNET(gnc nc, int cell_ref, int net_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return NULL;
  if ( net_ref < 0 )
    return NULL;
  assert( net_ref >= 0 );
  assert( net_ref < nl->net_set->list_max );
  return gnlGetGNET(nl, net_ref);
}


int gnc_FindCell(gnc nc, const char *name, const char *library)
{
  gcell cell;
  int i = -1;
  while( gnc_LoopCell(nc, &i) )
  {
    cell = gnc_GetGCELL(nc, i);
    if ( library == NULL )
    {
      if ( cell->library == NULL )
        if ( strcmp(cell->name, name) == 0 )
          return i;
    }
    else
    {
      if ( cell->library != NULL )
        if ( strcmp(cell->library, library) == 0 )
          if ( strcmp(cell->name, name) == 0 )
            return i;
    }
  }
  return -1;
}

/*!
  \ingroup gnccell
  A gnc object contains cells. Each cell must have a unique name.
  This functions adds a new cell to the gnc object \a nc if there
  is no other cell with the name \a name. If such a cell exists,
  this function will return the \a cell_ref of the cell with the
  name \a name.
  
  \param nc A pointer to a gnc structure.
  \param name The unique name of the cell.
  \param library The library name of the cell (can be \c NULL).
  
  \return A \a cell_ref handle to the new or existing cell or \c -1 if a
    memory error occured.
    
  \see gnc_Open()
  \see gnc_DelCell()
*/
int gnc_AddCell(gnc nc, const char *name, const char *library)
{
  gcell cell;
  int cell_ref;
  
  cell_ref = gnc_FindCell(nc, name, library);
  if ( cell_ref >= 0 )
    return cell_ref;

  cell = gcellOpen(name, library);
  if ( cell != NULL )
  {
    cell_ref = b_set_Add(nc->cell_set, cell);
    if ( cell_ref >= 0 )
    {
      if ( gnc_RegisterCell(nc, cell_ref) != 0 )
      {
        return cell_ref;
      }
      b_set_Del(nc->cell_set, cell_ref);
    }
    gcellClose(cell);
  }
  return -1;
}

/*!
  \ingroup gnccell
  A gnc object contains cells. 
  This functions deletes an existing cell. \a cell_ref must be 
  a reference handle of an existing cell. Such a handle is 
  returned by \c gnc_AddCell().
  
  \param nc A pointer to a gnc structure.
  \param cell_ref A cell reference handle
    
  \see gnc_Open()
  \see gnc_AddCell()
*/
void gnc_DelCell(gnc nc, int cell_ref)
{
  gcell cell;
  int i;
  
  assert(cell_ref >= 0);
  
  cell = gnc_GetGCELL(nc, cell_ref);
  if ( cell == NULL )
    return;

  /* delete all nodes, that refer to this cell_ref */    
  i = -1;
  while( gnc_LoopCell(nc, &i) != 0 )
    if ( gnc_GetGNL(nc, i, 0) != NULL )
      gnc_DelCellNodeCell(nc, i, cell_ref);

  /* decrement library count */
  gnc_UnregisterCell(nc, cell_ref);

  /* remove it from the set of cells */  
  b_set_Del(nc->cell_set, cell_ref);
  
  /* discard the cell object */
  gcellClose(cell);

  /* prevent synthesis algorithms from using this cell_ref */
  /* hmmm... one should perhaps start another selection process */
  for( i = 0; i < GCELL_ID_MAX; i++ )
    if ( nc->bbb_cell_ref[i] == cell_ref )
      nc->bbb_cell_ref[i] = -1;
}

void gnc_ClearCell(gnc nc, int cell_ref)
{
  gcell cell;
  int i;
  
  assert(cell_ref >= 0);
  
  cell = gnc_GetGCELL(nc, cell_ref);
  if ( cell == NULL )
    return;

  gcellClear(cell);
  
  for( i = 0; i < GCELL_ID_MAX; i++ )
    if ( nc->bbb_cell_ref[i] == cell_ref )
      nc->bbb_cell_ref[i] = -1;
}


int gnc_LoopLibCell(gnc nc, int lib_ref, int *cell_ref)
{
  char *cell_lib_name;
  char *library_name = gnc_GetLibName(nc, lib_ref);
  for(;;)
  {
    if ( gnc_LoopCell(nc, cell_ref) == 0 )
      break; /* return 0; */
    cell_lib_name = gnc_GetCellLibraryName(nc, *cell_ref);
    if ( library_name == NULL )
    {
      if ( cell_lib_name == NULL )
        return 1;
    }
    else
    {
      if ( cell_lib_name != NULL )
        if ( strcmp(cell_lib_name, library_name) == 0 )
          return 1;
    }
  }
  return 0;
}

/* returns position or -1 */
int gnc_AddCellPort(gnc nc, int cell_ref, int port_type, const char *port_name)
{
  gcell cell;
  int pos;
  cell = gnc_GetGCELL(nc,cell_ref);
  pos = gcellAddPortByName(cell, port_type, port_name);
  if ( pos < 0 )
    gnc_Error(nc, "gnc_AddCellPort: Can not add port '%s'.\n", port_name);
  return pos;
}

/* returns 0 or 1, does not update pinfo */
int gnc_SetCellPort(gnc nc, int cell_ref, int pos, int port_type, const char *port_name)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  if ( gcellSetPortByName(cell, pos, port_type, port_name) == 0 )
  {
    gnc_Error(nc, "gnc_SetCellPort: Out of Memory (gcellSetPortByName).");
    return 0;
  }
  return 1;
}

void gnc_SetCellPortFn(gnc nc, int cell_ref, int port_ref, int fn, int is_inverted)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  gcellSetPortFn(cell, port_ref, fn, is_inverted);
}

int gnc_GetCellPortFn(gnc nc, int cell_ref, int port_ref)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellGetPortFn(cell, port_ref);
}

int gnc_IsCellPortInverted(gnc nc, int cell_ref, int port_ref)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellIsPortInverted(cell, port_ref);
}

void gnc_SetCellPortInputLoad(gnc nc, int cell_ref, int port_ref, double cap)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  gcellSetPortInputLoad(cell, port_ref, cap);
}

double gnc_GetCellPortInputLoad(gnc nc, int cell_ref, int port_ref)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellGetPortInputLoad(cell, port_ref);
}

/* return the delay structure from port_ref to related_port_ref */
gpoti gnc_GetCellPortGPOTI(gnc nc, int cell_ref, int port_ref, int related_port_ref, int is_create)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellGetPortGPOTI(cell, port_ref, related_port_ref, is_create);
}

void gnc_SetCellArea(gnc nc, int cell_ref, double area)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  gcellSetArea(cell, area);
}

double gnc_GetCellArea(gnc nc, int cell_ref)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellGetArea(cell);
}

void gnc_SetCellId(gnc nc, int cell_ref, int id)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  gcellSetId(cell, id);
}

int gnc_GetCellId(gnc nc, int cell_ref)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellGetId(cell);
}

int gnc_GetCellInCnt(gnc nc, int cell_ref)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellGetInCnt(cell);
}


void gnc_ClearAllCellsFlag(gnc nc)
{
  int i = -1;
  gcell cell;
  while( gnc_LoopCell(nc, &i) )
  {
    cell = gnc_GetGCELL(nc, i);
    cell->flag = 0;
  }
}

int gnc_IsCellFlag(gnc nc, int cell_ref)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return cell->flag;
}

void gnc_SetCellFlag(gnc nc, int cell_ref)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  cell->flag = 1;
}

/* returns port_ref */
int gnc_FindCellPort(gnc nc, int cell_ref, const char *port_name)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellFindPort(cell, port_name);
}

/* returns port_ref */
int gnc_FindCellPortByType(gnc nc, int cell_ref, int type)
{
  gcell cell;
  assert(cell_ref >= 0);
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellFindPortByType(cell, type);
}

/* returns port_ref */
int gnc_FindCellPortByFn(gnc nc, int cell_ref, int fn)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellFindPortByFn(cell, fn);
}

/* returns port_ref */
int gnc_GetNthPort(gnc nc, int cell_ref, int n, int type)
{
  gcell cell;
  cell = gnc_GetGCELL(nc,cell_ref);
  return gcellGetNthPort(cell, n, type);
}

/* alias AddInstance, creates a new NL */
/* returns node_ref or -1 */
int gnc_AddCellNode(gnc nc, int cell_ref, const char *node_name, int node_cell_ref)
{
  int ret;
  gnl nl = gnc_GetGNL(nc, cell_ref, 1);
  if ( node_cell_ref < 0 )
  {
    gnc_Error(nc, "gnc_AddCellNode: Illegal cell.");
    return 0;
  }
  
  if ( nl == NULL )
  {
    return -1;
  }
  ret = gnlAddNode(nl, node_name, node_cell_ref);
  if ( ret < 0 )
  {
    gnc_Error(nc, "gnc_AddCellNode: Can not create node (memory error or cell_ref mismatch).");
  }
  
  return ret;
}

void gnc_DelCellNode(gnc nc, int cell_ref, int node_ref)
{
  gnl nl;
  
  nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return; /* the cell does not contain a net */
  
  gnlDelNode(nl, node_ref);
}

/* delete all nodes with a certain cell_ref */
void gnc_DelCellNodeCell(gnc nc, int cell_ref, int node_cell_ref)
{
  int node_ref = -1;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
  {
    if ( gnc_GetCellNodeCell(nc, cell_ref, node_ref) == node_cell_ref )
      gnc_DelCellNode(nc, cell_ref, node_ref);
  }
}

/* returns node_ref */
int gnc_FindCellNode(gnc nc, int cell_ref, const char *node_name)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if (  nl == NULL )
    return -1;
  return gnlFindNode(nl, node_name);
}

/* returns net_ref or -1, creates a new NL */
int gnc_AddCellNet(gnc nc, int cell_ref, const char *net_name)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 1);
  if (  nl == NULL )
    return -1;
  return gnlAddNet(nl, net_name);
}

void gnc_DelCellNet(gnc nc, int cell_ref, int net_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if (  nl == NULL )
    return;
  gnlDelNet(nl, net_ref);
}

void gnc_DelCellEmptyNet(gnc nc, int cell_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if (  nl == NULL )
    return;
  gnlDelEmptyNet(nl);
}

void gnc_DelEmptyNet(gnc nc)
{
  int cell_ref = -1;
  while( gnc_LoopCell(nc, &cell_ref) != 0 )
    gnc_DelCellEmptyNet(nc, cell_ref);
}


/* returns net_ref or -1 */
int gnc_FindCellNet(gnc nc, int cell_ref, const char *net_name)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if (  nl == NULL )
    return -1;
  return gnlFindNetByName(nl, net_name);
}

/* returns net_ref or -1 */
/* this is a clever search function */
int gnc_FindCellNetByPort(gnc nc, int cell_ref, int node_ref, int port_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if (  nl == NULL )
    return -1;
  return gnlFindNetByPort(nl, node_ref, port_ref);
}

/* returns net_ref or -1 */
int gnc_FindCellNetByNode(gnc nc, int cell_ref, int node_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if (  nl == NULL )
    return -1;
  return gnlFindNetByNode(nl, node_ref);
}

/* returns join_ref */
/* previously gnc_AddCellNetPort */
int gnc_AddCellNetJoin(gnc nc, int cell_ref, int net_ref, int node_ref, int port_ref, int is_unique)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 1);
  if (  nl == NULL )
    return -1;
  return gnlAddNetPort(nl, net_ref, node_ref, port_ref, is_unique);
}

void gnc_DelCellNetJoin(gnc nc, int cell_ref, int net_ref, int join_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if (  nl == NULL )
    return;
  gnlDelCellNetJoin(nl, net_ref, join_ref);
}

void gnc_DelCellNetJoinByPort(gnc nc, int cell_ref, int net_ref, int node_ref, int port_ref)
{
  int join_ref = gnc_FindCellNetJoin(nc, cell_ref, net_ref, node_ref, port_ref);
  if ( join_ref >= 0 )
    gnc_DelCellNetJoin(nc, cell_ref, net_ref, join_ref);
}

/* returns join_ref or -1 if port was not found */
/* find a join_ref that is connected to a 'port' of 'node' */
/* if 'node' is -1 a parent port is searched */
/* 'port' = -1 is a wildcard for any port of the node */
int gnc_FindCellNetJoin(gnc nc, int cell_ref, int net_ref, int node_ref, int port_ref)
{
  gnet net = gnc_GetCellGNET(nc, cell_ref, net_ref);
  if ( net == NULL )
    return 0;
  return gnetFindPort(net, node_ref, port_ref);
}

/*
 * int gnc_FindCellNetPort(gnc nc, int cell_ref, int net_ref, int node_ref, int port_ref)
 * {
 *   return gnc_FindCellNetJoin(nc, cell_ref, net_ref, node_ref, port_ref);
 * }
 */



/* returns cell_ref */
int gnc_GetCellByNode(gnc nc, int cell_ref, int node_ref)
{
  return gnc_GetCellNodeCell(nc, cell_ref, node_ref);
  /*
  gnode node;
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if (  nl == NULL || node_ref < 0 )
    return -1;
  node = gnlGetGNODE(nl, node_ref);
  return node->cell_ref;
  */
}

int gnc_GetCellNetCnt(gnc nc, int cell_ref)
{
  gcell cell;
  assert(cell_ref >= 0);
  cell = gnc_GetGCELL(nc, cell_ref);
  if ( cell == NULL )
    return 0;
  return gcellGetNetCnt(cell);
}

double gnc_CalcCellNetNodeArea(gnc nc, int cell_ref)
{
  double area = 0.0;
  int node_ref = -1;
  gnode node;
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if (  nl == NULL )
    return 0.0;
  while( gnlLoopNodeRef(nl, &node_ref) != 0 )
  {
    node = gnlGetGNODE(nl, node_ref);
    area += gnc_GetCellArea(nc, node->cell_ref);
  }
  return area;
}

int gnc_GetLibraryNLCnt(gnc nc, int lib_ref)
{
  gcell cell;
  int cell_ref = -1;
  int cnt = 0;
  
  while(gnc_LoopLibCell(nc, lib_ref, &cell_ref))
  {
    cell = gnc_GetGCELL(nc, cell_ref);
    
    if ( gcellGetNetCnt(gnc_GetGCELL(nc, cell_ref)) > 0 )
      cnt++;
  }
  
  return cnt;
}

int gnc_GetCellPortCnt(gnc nc, int cell_ref)
{
  return b_set_Cnt(gnc_GetGCELL(nc, cell_ref)->port_set);
}

int gnc_GetCellPortMax(gnc nc, int cell_ref)
{
  return b_set_Max(gnc_GetGCELL(nc, cell_ref)->port_set);
}

const char *gnc_GetCellPortName(gnc nc, int cell_ref, int port_ref)
{
  gcell c = gnc_GetGCELL(nc, cell_ref);
  gport p;
  if ( port_ref < 0 )
    return NULL;
  assert(port_ref >= 0);
  assert(port_ref < c->port_set->list_max);
  if ( gcellGetGPORT(c, port_ref) == NULL ) 
    return NULL;
  p = gcellGetGPORT(c, port_ref);
  return p->name;
}

int gnc_SetCellPortName(gnc nc, int cell_ref, int port_ref, const char *name)
{
  gcell c = gnc_GetGCELL(nc, cell_ref);
  gport p;
  if ( port_ref < 0 )
    return 0;
  assert(port_ref >= 0);
  assert(port_ref < c->port_set->list_max);
  assert(gcellGetGPORT(c, port_ref) != NULL);
  p = gcellGetGPORT(c, port_ref);
  return gportSetName(p, name);
}

int gnc_GetCellPortType(gnc nc, int cell_ref, int port_ref)
{
  gcell c = gnc_GetGCELL(nc, cell_ref);
  gport p;
  assert(port_ref >= 0);
  assert(port_ref < c->port_set->list_max);
  assert(gcellGetGPORT(c, port_ref) != NULL);
  p = gcellGetGPORT(c, port_ref);
  return p->type;
}

void gnc_SetCellPortType(gnc nc, int cell_ref, int port_ref, int type)
{
  gcell c = gnc_GetGCELL(nc, cell_ref);
  gport p;
  assert(port_ref >= 0);
  assert(port_ref < c->port_set->list_max);
  assert(gcellGetGPORT(c, port_ref) != NULL);
  p = gcellGetGPORT(c, port_ref);
  p->type = type;
}

char *gnc_GetCellLibraryName(gnc nc, int cell_ref)
{
  return gnc_GetGCELL(nc, cell_ref)->library;
}

const char *gnc_GetCellName(gnc nc, int cell_ref)
{
  return gnc_GetGCELL(nc, cell_ref)->name;
}

const char *gnc_GetCellNameStr(gnc nc, int cell_ref)
{
  if ( gnc_GetCellName(nc, cell_ref) == NULL )
    return "???";
  return gnc_GetCellName(nc, cell_ref);
}

/*
int gnc_GetCellNodeCnt(gnc nc, int cell_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return 0;
  return b_dic_GetCnt(nl->node_set);
}
*/

char *gnc_GetCellNodeName(gnc nc, int cell_ref, int node_ref)
{
  static char s[16];
  gnode node;
  node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
  if ( node == NULL )
    return NULL;
  if ( node->name == NULL )
  {
    sprintf(s, "I%08d", node_ref);
    return s;
  }
  return node->name;
}

int gnc_GetCellNodeCell(gnc nc, int cell_ref, int node_ref)
{
  gnode node;
  node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
  if ( node == NULL )
    return -1;
  return node->cell_ref;
}

void gnc_ClearCellNodeFlags(gnc nc, int cell_ref)
{
  int node_ref = -1;
  gnode node;
  while( gnc_LoopCellNode(nc, cell_ref, &node_ref) != 0 )
  {
    node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
    node->flag = 0;
  }
}

unsigned gnc_GetCellNodeFlag(gnc nc, int cell_ref, int node_ref)
{
  gnode node;
  node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
  return node->flag;
}

void gnc_SetCellNodeFlag(gnc nc, int cell_ref, int node_ref, unsigned val)
{
  gnode node;
  node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
  node->flag = val;
}

int gnc_IsCellNodeDoNotTouch(gnc nc, int cell_ref, int node_ref)
{
  gnode node;
  node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
  if ( node->is_do_not_touch == 0 )
    return 0;
  return 1;
}

void gnc_SetCellNodeDoNotTouch(gnc nc, int cell_ref, int node_ref)
{
  gnode node;
  node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
  node->is_do_not_touch = 1;
}

int gnc_LoopCellNodePortType(gnc nc, int cell_ref, int node_ref, int type, int *port_ref, int *net_ref, int *join_ref)
{
  int node_cell_ref;
  assert(node_ref >= 0);
  node_cell_ref = gnc_GetCellNodeCell(nc, cell_ref, node_ref);

  for(;;)
  {
    if ( gnc_LoopCellPort(nc, node_cell_ref, port_ref) == 0 )
      break;
    if ( gnc_GetCellPortType(nc, node_cell_ref, *port_ref) == type )
    {
      /* old code */
/*
 *       *net_ref = -1;
 *       while( gnc_LoopCellNet(nc, cell_ref, net_ref) != 0 )
 *       {
 *         *join_ref = gnc_FindCellNetPort(nc, cell_ref, *net_ref, node_ref, *port_ref);
 *         if ( *join_ref >= 0 )
 *           return 1;
 *       }
 */
      /* do the new clever search method */
      *net_ref = gnc_FindCellNetByPort(nc, cell_ref, node_ref, *port_ref);
      if ( *net_ref >= 0 )
      {
        *join_ref = gnc_FindCellNetJoin(nc, cell_ref, *net_ref, node_ref, *port_ref);
        if ( *join_ref >= 0 )
          return 1;
      }
    }
  }
  return 0;
}

int gnc_AddCellNodeTypeNet(gnc nc, int cell_ref, int node_ref, int type, b_il_type il)
{
  int port_ref = -1;
  int net_ref = -1;
  int join_ref = -1;
  
  while( gnc_LoopCellNodePortType(nc, cell_ref, node_ref, type, &port_ref, &net_ref, &join_ref) != 0 )
  {
    if ( b_il_Add(il, net_ref) < 0 )
    {
      gnc_Error(nc, "gnc_GetCellNodeInputNet: Out of Memory (b_il_Add).");
      return 0;
    }
  }
  return 1;
}

int gnc_GetCellNodeFirstOutputNet(gnc nc, int cell_ref, int node_ref)
{
  int port_ref = -1;
  int net_ref = -1;
  int join_ref = -1;
  
  if ( gnc_LoopCellNodePortType(nc, cell_ref, node_ref, GPORT_TYPE_OUT, &port_ref, &net_ref, &join_ref) == 0 )
    return -1;
  return net_ref;
}

int gnc_GetCellNodeFirstInputNet(gnc nc, int cell_ref, int node_ref)
{
  int port_ref = -1;
  int net_ref = -1;
  int join_ref = -1;
  
  if ( gnc_LoopCellNodePortType(nc, cell_ref, node_ref, GPORT_TYPE_IN, &port_ref, &net_ref, &join_ref) == 0 )
    return -1;
  return net_ref;
}

int gnc_GetCellNodeInputNet(gnc nc, int cell_ref, int node_ref, b_il_type il)
{
  b_il_Clear(il);
  return gnc_AddCellNodeTypeNet(nc, cell_ref, node_ref, GPORT_TYPE_IN, il);
}

int gnc_GetCellNodeOutputNet(gnc nc, int cell_ref, int node_ref, b_il_type il)
{
  b_il_Clear(il);
  return gnc_AddCellNodeTypeNet(nc, cell_ref, node_ref, GPORT_TYPE_OUT, il);
}

int gnc_AddCellNodeInputNodes(gnc nc, int cell_ref, int node_ref, b_il_type il)
{
  int port_ref = -1;
  int net_ref = -1;
  int join_ref = -1;
  int input_node_ref;
  
  while( gnc_LoopCellNodePortType(nc, cell_ref, node_ref, GPORT_TYPE_IN, &port_ref, &net_ref, &join_ref) != 0 )
  {
    if ( net_ref >= 0 ) 
    {
      /* input_node_ref = gnc_FindCellNetOutputNode(nc, cell_ref, net_ref); */
      input_node_ref = gnc_FindCellNetNodeByDriver(nc, cell_ref, net_ref);
      if ( input_node_ref >= 0 )
      {
        if ( b_il_AddUnique(il, input_node_ref) < 0 )
        {
          gnc_Error(nc, "gnc_AddCellNodeInputNodes: Out of Memory (b_il_Add).");
          return 0;
        }
      }
    }
  }
  return 1;
}



char *gnc_GetCellNodeCellName(gnc nc, int cell_ref, int node_ref)
{
  gnode node;
  gcell cell;
  node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
  if ( node == NULL )
    return NULL;
  cell = gnc_GetGCELL(nc, node->cell_ref);
  return cell->name;
}

char *gnc_GetCellNodeCellLibraryName(gnc nc, int cell_ref, int node_ref)
{
  gnode node;
  node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
  if ( node == NULL )
    return NULL;
  return gnc_GetCellLibraryName(nc, node->cell_ref);
}

/*
int gnc_GetCellNetCnt(gnc nc, int cell_ref)
{
  gnl nl = gnc_GetGNL(nc, cell_ref, 0);
  if ( nl == NULL )
    return 0;
  return b_set_Cnt(nl->net_set);
}
*/

char *gnc_GetCellNetName(gnc nc, int cell_ref, int net_ref)
{
  static char s[16];
  gnet net = gnc_GetCellGNET(nc, cell_ref, net_ref);
  if ( net == NULL )
    return NULL;
  if ( net->name == NULL )
  {
    sprintf(s, "N%08d", net_ref);
    return s;
  }
  return net->name;
}

gjoin gnc_GetCellNetGJOIN(gnc nc, int cell_ref, int net_ref, int join_ref)
{
  gnet net = gnc_GetCellGNET(nc, cell_ref, net_ref);
  if ( net == NULL )
    return NULL;
  assert(join_ref >= 0);
  return gnetGetGJOIN(net, join_ref);
}

int gnc_GetCellNetPort(gnc nc, int cell_ref, int net_ref, int join_ref)
{
  gjoin join = gnc_GetCellNetGJOIN(nc, cell_ref, net_ref, join_ref);
  if ( join == NULL )
    return -1;
  return join->port;
}

int gnc_GetCellNetNode(gnc nc, int cell_ref, int net_ref, int join_ref)
{
  gjoin join = gnc_GetCellNetGJOIN(nc, cell_ref, net_ref, join_ref);
  if ( join == NULL )
    return -1;
  return join->node;
}

char *gnc_GetCellNetNodeName(gnc nc, int cell_ref, int net_ref, int join_ref)
{
  int node_ref = gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
  if ( node_ref < 0 )
    return NULL;
  return gnc_GetCellNodeName(nc, cell_ref, node_ref);
}

/* return cell_ref */
int gnc_GetCellNetNodeCell(gnc nc, int cell_ref, int net_ref, int join_ref)
{
  gnode node;
  int node_ref = gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
  if ( node_ref < 0 )
    return cell_ref;
  node = gnc_GetCellGNODE(nc, cell_ref, node_ref);
  return node->cell_ref;
}

const char *gnc_GetCellNetNodeCellName(gnc nc, int cell_ref, int net_ref, int join_ref)
{
  cell_ref = gnc_GetCellNetNodeCell(nc, cell_ref, net_ref, join_ref);
  if ( cell_ref < 0 )
    return NULL;
  return gnc_GetCellName(nc, cell_ref);
}

gport gnc_GetCellNetGPORT(gnc nc, int cell_ref, int net_ref, int join_ref)
{
  gcell c;
  gport p;
  gjoin j;
  int port_ref;
  j = gnc_GetCellNetGJOIN(nc, cell_ref, net_ref, join_ref);
  if ( j == NULL )
    return NULL;
  port_ref = j->port;
  
  /* if this is a port of the parent, do not change cell_ref */
  /* if not, replace cell_ref by the cell_ref of the node */
  if ( j->node >= 0 )
    cell_ref = gnc_GetCellNodeCell(nc, cell_ref, j->node);
  /*
  port_ref = gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref);
  cell_ref = gnc_GetCellNetNodeCell(nc, cell_ref, net_ref, join_ref);
  */
  if ( cell_ref < 0 || port_ref < 0)
    return NULL;
  c = gnc_GetGCELL(nc, cell_ref);
  assert(port_ref >= 0);
  assert(port_ref < c->port_set->list_max);
  p = gcellGetGPORT(c, port_ref);
  assert(p != NULL);
  return p;
}

const char *gnc_GetCellNetPortName(gnc nc, int cell_ref, int net_ref, int join_ref)
{
  gport p = gnc_GetCellNetGPORT(nc, cell_ref, net_ref, join_ref);
  if ( p == NULL )
    return NULL;
  return p->name;
}

int gnc_GetCellNetPortType(gnc nc, int cell_ref, int net_ref, int join_ref)
{
  gport p = gnc_GetCellNetGPORT(nc, cell_ref, net_ref, join_ref);
  if ( p == NULL )
    return GPORT_TYPE_BI;
  return p->type;
}

int gnc_FindCellNetJoinByPortType(gnc nc, int cell_ref, int net_ref, int type)
{
  int join_ref = -1;
  while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
  {
    if ( gnc_GetCellNetPortType(nc, cell_ref, net_ref, join_ref) == type )
      return join_ref;
  }
  return -1;
}

int gnc_FindCellNetNodeByPortType(gnc nc, int cell_ref, int net_ref, int type)
{
  int join_ref = -1;
  join_ref = gnc_FindCellNetJoinByPortType(nc, cell_ref, net_ref, type);
  if ( join_ref < 0 )
    return -1;
  return gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
}

int gnc_FindCellNetJoinByDriver(gnc nc, int cell_ref, int net_ref)
{
  int join_ref = -1;
  while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
  {
    if ( gnc_GetCellNetPortType(nc, cell_ref, net_ref, join_ref) == GPORT_TYPE_OUT )
    {
      if ( gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref) >= 0 )
        return join_ref;
    }
    else if ( gnc_GetCellNetPortType(nc, cell_ref, net_ref, join_ref) == GPORT_TYPE_IN )
    {
      if ( gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref) < 0 )
        return join_ref;
    }
  }
  return -1;
}

int gnc_FindCellNetNodeByDriver(gnc nc, int cell_ref, int net_ref)
{
  int join_ref = -1;
  join_ref = gnc_FindCellNetJoinByDriver(nc, cell_ref, net_ref);
  if ( join_ref < 0 )
    return -1;
  return gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
}

int gnc_FindCellNetPortByDriver(gnc nc, int cell_ref, int net_ref)
{
  int join_ref = -1;
  join_ref = gnc_FindCellNetJoinByDriver(nc, cell_ref, net_ref);
  if ( join_ref < 0 )
    return -1;
  return gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref);
}

int gnc_CheckCellNetDriver(gnc nc, int cell_ref)
{
  int net_ref = -1;
  while(gnc_LoopCellNet(nc, cell_ref, &net_ref) != 0)
  {
    if ( gnc_FindCellNetJoinByDriver(nc, cell_ref, net_ref) < 0 )
    {
      int join_ref = -1;
      int port_ref;
      int node_ref;
      const char *cell_name;
      const char *port_name;
      gnc_Error(nc, "gnc_CheckCellNetDriver: Internal Error (Net %d not driven).", net_ref);

      gnc_Log(nc, 0, "gnc_CheckCellNetDriver: join  name/port");
      while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
      {
        port_ref = gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref);
        node_ref = gnc_GetCellNetPort(nc, cell_ref, net_ref, join_ref);
        cell_name = gnc_GetCellNetNodeCellName(nc, cell_ref, net_ref, join_ref);
        port_name = gnc_GetCellNetPortName(nc, cell_ref, net_ref, join_ref);
        gnc_Log(nc, 0, "gnc_CheckCellNetDriver: %4d  %s/%s", join_ref, cell_name, port_name);
      }
      
      return 0;
    }
  }
  return 1;
}

int gnc_GetCellNetPortCnt(gnc nc, int cell_ref, int net_ref)
{
  gnet net = gnc_GetCellGNET(nc, cell_ref, net_ref);
  if ( net == NULL )
    return -1;
  return gnetGetPortCnt(net);
}

int gnc_FindCellNetCell(gnc nc, int cell_ref, int net_ref, int req_cell_ref)
{
  int join_ref = -1;
  while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
    if ( gnc_GetCellNetNodeCell(nc, cell_ref, net_ref, join_ref) == req_cell_ref )
      return join_ref;
  return -1;
}

int gnc_FindCellNetOutputNode(gnc nc, int cell_ref, int net_ref)
{
  int join_ref = -1;
  int node_ref;
  while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
  {
    node_ref = gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
    if ( node_ref >= 0 )
      if ( gnc_GetCellNetPortType(nc, cell_ref, net_ref, join_ref) == GPORT_TYPE_OUT )
        return node_ref;
  }
  return -1;
}

int gnc_FindFirstCellNetInputNode(gnc nc, int cell_ref, int net_ref)
{
  int join_ref = -1;
  int node_ref;
  while( gnc_LoopCellNetJoin(nc, cell_ref, net_ref, &join_ref) != 0 )
  {
    node_ref = gnc_GetCellNetNode(nc, cell_ref, net_ref, join_ref);
    if ( node_ref >= 0 )
      if ( gnc_GetCellNetPortType(nc, cell_ref, net_ref, join_ref) == GPORT_TYPE_IN )
        return node_ref;
  }
  return -1;
}

/*
  A cell, that has no net 
          and is not referenced
          and does not have an id
  can be deleted
*/
int gnc_IsCellUseless(gnc nc, int cell_ref)
{
  int cell_pos;
  int node_pos;
  
  if ( gnc_GetCellLibraryName(nc, cell_ref) != NULL )
    if ( strcmp(gnc_GetCellLibraryName(nc, cell_ref), GNET_GENERIC_LIB_NAME ) == 0 )
      return 1;
  if ( gnc_GetGNL(nc, cell_ref, 0) != NULL )
    return 0;
  if ( gnc_GetCellId(nc, cell_ref) > 0 )
    return 0;
  
  cell_pos = -1;
  while( gnc_LoopCell(nc, &cell_pos) )
  {
    if ( cell_pos != cell_ref )
    {
      node_pos = -1;
      while( gnc_LoopCellNode(nc, cell_pos, &node_pos) )
      {
        if ( cell_ref == gnc_GetCellNodeCell(nc, cell_pos, node_pos) )
        {
          return 0;
        }
      }
    }
  }
  return 1;
}

void gnc_Clean(gnc nc)
{
  int i = -1;
  
  gnc_DelEmptyNet(nc);
  
  while( gnc_LoopCell(nc, &i) )
  {
    if ( gnc_IsCellUseless(nc, i) != 0 )
      gnc_DelCell(nc, i);
  }
}

/*
  A toplevel cell is not referenced by any other cells
*/
int gnc_IsTopLevelCell(gnc nc, int cell_ref)
{
  int cell_pos;
  int node_pos;
  
  if ( gnc_GetGNL(nc, cell_ref, 0) == NULL )
    return 0;

  cell_pos = -1;
  while( gnc_LoopCell(nc, &cell_pos) )
  {
    if ( cell_pos != cell_ref )
    {
      node_pos = -1;
      while( gnc_LoopCellNode(nc, cell_pos, &node_pos) )
      {
        if ( cell_ref == gnc_GetCellNodeCell(nc, cell_pos, node_pos) )
        {
          return 0;
        }
      }
    }
  }
  return 1;
}

int gnc_GetTopLevelCellCnt(gnc nc)
{
  int i = -1;
  int top_cells = 0;
  while( gnc_LoopCell(nc, &i) )
  {
    if ( gnc_IsTopLevelCell(nc, i) != 0 )
      top_cells++;
  }
  return top_cells;
}


size_t gnc_GetMemUsage(gnc nc)
{
  int i = -1;
  size_t m = sizeof(struct _gnc_struct);
  while( gnc_LoopCell(nc, &i) )
    m += gcellGetMemUsage(gnc_GetGCELL(nc,i));
  m += b_set_GetMemUsage(nc->cell_set);
  return m;
}

#define GNC_LIB_MAGIC 0x0004c4744

struct gnc_merge_el_struct
{
  gnc nc;
  const char *library_name;
};

static void *gnc_read_libcell_el(FILE *fp, void *ud)
{
  struct gnc_merge_el_struct *mes = (struct gnc_merge_el_struct *)ud;
  gcell cell = gcellOpen(NULL, NULL);
  if ( cell == NULL )
    return NULL;
  if ( gcellRead(cell, fp, mes->library_name) == 0 )
  {
    gcellClose(cell);
    return NULL;
  }
  if ( gnc_register_cell(mes->nc, cell) == 0 )
  {
    gcellClose(cell);
    return NULL;
  }
  return cell;
}

char *gnc_ReadBinaryLibraryFP(gnc nc, FILE *fp, const char *library_name)
{
  struct gnc_merge_el_struct mes;
  static char buf[256];
  char *file_lib_name = NULL;
  int magic;

  if ( b_io_ReadInt(fp, &magic) == 0 )                                 
    return NULL;
  if ( magic != GNC_LIB_MAGIC )
    return NULL;
  
  if ( b_io_ReadAllocString(fp, &(file_lib_name)) == 0 )               
    return NULL;
  if ( file_lib_name == NULL )
    return NULL;

  gnc_Log(nc, 4, "Internal library name is '%s' (dgc binary library).", file_lib_name);
    
  strncpy(buf, file_lib_name, 256);
  buf[255] = '\0';
  free(file_lib_name);
  file_lib_name = buf;
  
  if ( library_name == NULL )
    library_name = file_lib_name;
  
  gnc_DelLib(nc, library_name);

  mes.nc = nc;
  mes.library_name = library_name;
  if ( b_set_ReadMerge(nc->cell_set, fp, gnc_read_libcell_el, &mes) == 0 )     
    return NULL;
  
  return file_lib_name;
}

static int gnc_write_libcell_el(FILE *fp, void *el, void *ud)
{
  gcell c = (gcell)el;
  if ( c->library == NULL )
    return 1;
  if ( strcmp(c->library, (const char *)ud) != 0 )
    return 1;
  return gcellWrite(c, fp, (const char *)ud);
}

int gnc_WriteBinaryLibraryFP(gnc nc, FILE *fp, const char *libary_name)
{
  if ( libary_name == NULL )
    return 0;
  if ( b_io_WriteInt(fp, GNC_LIB_MAGIC) == 0 )
    return 0;
  if ( b_io_WriteString(fp, libary_name) == 0 )
    return 0;

  /* hmm... we had to remove the const qualifier for a moment */
  if ( b_set_Write(nc->cell_set, fp, gnc_write_libcell_el, (void *)libary_name) == 0 )
    return 0;
  
  return 1;
}

char *gnc_ReadBinaryLibrary(gnc nc, const char *name, const char *libary_name)
{
  FILE *fp;
  char *result;
  fp = b_fopen(name, NULL, ".dgl", "r");
  if ( fp == NULL )
    return NULL;
  result = gnc_ReadBinaryLibraryFP(nc, fp, libary_name);
  fclose(fp);
  return result;
}

int gnc_WriteBinaryLibrary(gnc nc, const char *name, const char *libary_name)
{
  FILE *fp;
  int result;
  fp = fopen(name, "w");
  if ( fp == NULL )
    return 0;
  result = gnc_WriteBinaryLibraryFP(nc, fp, libary_name);
  fclose(fp);
  return result;
}

int gnc_IsValidBinaryLibrary(gnc nc, const char *name)
{
  FILE *fp;
  int magic;
  fp = b_fopen(name, NULL, ".dgl", "r");
  if ( fp == NULL )
    return 0;
  if ( b_io_ReadInt(fp, &magic) == 0 )                                 
  {
    fclose(fp);
    return 0;
  }
  fclose(fp);
  if ( magic != GNC_LIB_MAGIC )
    return 0;
  
  return 1;
}

/*!
  \ingroup gncinit
  
  A library is an external file, that contains the functional
  description of some cells. This function reads an external
  library into a gnc object. For each cell description found inside
  the library, a corresponding cell is created in the gnc object.
  
  The cell-name of such a created cell is derived from the library.
  The libarary-name is depends on the argument \a library_name.
  If \a library_name is \c NULL and a library name is available for
  the library, all cells get that library name.
  For example, if a library has the name \c x and contains the
  cells \c a, \c b and \c c, than three cells are created with
  the cell-name \c a, cell-name \c b and cell-name \c c. All these
  cells have the library name \c x.
  
  If \a library_name not \c NULL \a library_name is used instead of
  the internal name. One could say: All imported cells are renamed
  to \a library_name.
  For example, if a library has the name \c x and contains the
  cells \c a, \c b and \c c, than three cells are created with
  the cell-name \c a, cell-name \c b and cell-name \c c. All these
  cells have the library name \c y if \a library_name is \c "y".
  
  Often the cells of a library are used to build larger cells:
  In this case the cells of a library are used as basic building
  blocks for some other cells. 
  To make the cells available for other cells, one must call
  \c gnc_ApplyBBBs().
  
  This function returns the library name, that had been used to
  create the cells.
  
  \param nc A pointer to a gnc structure.
  \param name A valid filename.
  \param library_name The new library name of the cell or \c NULL.
  \param is_msg Print potential error messages (usually 0 = false)
  
  \return The library name that had been used for the cells or \c NULL
  if an error occured (memory or file error).
    
  \see gnc_Open()
  \see gnc_AddCell()
  \see gnc_ApplyBBBs()
  \see gnc_SynthByFile()
*/
char *gnc_ReadLibrary(gnc nc, const char *name, const char *library_name, int is_msg)
{
  char *result;

  if ( library_name != NULL )
    if ( library_name[0] == '\0' )
      library_name = NULL;
    
  if ( gnc_IsValidBinaryLibrary(nc, name) != 0 )
  {
    gnc_Log(nc, 4, "Gate library '%s' is a binary dgc library.", name);
    result = gnc_ReadBinaryLibrary(nc, name, library_name);
  }
  else if ( IsValidSynopsysLibraryFile(name, is_msg) != 0 )
  {
    gnc_Log(nc, 4, "Gate library '%s' is a synopsys library sourcecode.", name);
    result = gnc_ReadSynopsysLibrary(nc, name, library_name);
  }
  else if ( IsValidGenlibFile(name) != 0 )
  {
    gnc_Log(nc, 4, "Gate library '%s' is a genlib file.", name);
    result = gnc_ReadGenlibLibrary(nc, name, library_name);
  }
  else
  {
    gnc_Error(nc, "Gate library '%s' is not a valid file. Supported formats are: Synopsys, Genlib, DGC Binary.", name);
    return NULL;
  }
  
  if ( result == NULL )
  {
    gnc_Error(nc, "Can not read gate library '%s'. (Syntax error? IO error?)", name);
    return NULL;
  }
  
  return result;
}

