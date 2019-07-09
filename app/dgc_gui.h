/*

  dgc_gui.h

  graphical user interface for the dgc application

  Copyright (C) 2001 Claudia Spircu (claudia@lrs.e-technik.uni-erlangen.de)

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

#ifndef _DGC_GUI_H
#define _DGC_GUI_H

#include "gnet.h"

/* remember to modify these when modifing the menu */
#define FILE_OPEN 0

#define BOOL_PLA 0
#define BOOL_BEX 1

#define NET_EDIF 0
#define NET_VHDL 1
#define NET_XNF 2

#define FILE_SAVE_LOG 5

#define LIBRARY_OPEN 0

#define EDIT_ENC 0
#define EDIT_SYNTH 1
#define EDIT_LIB 3
#define EDIT_LOG 5

#define SYNTHESIS 3

#define WIN_LOG_CLR 0
#define WIN_ERR_CLR 1

struct user_data
{
  char file_name[1024];
  int log_level;
  
  gnc nc;
  
  int state_encoding;         /* encoding type */
  int state_encoding_old;     /* for the dialog box */
  int is_async;         /* state machine type */
  int is_async_old;
  int reset_type;       /* reset type */
  int reset_type_old;
  int t_is_neca;        /* net cache optimization type */
  int t_is_neca_old;
  int t_is_generic;         /* generic cell optimization type */
  int t_is_generic_old;
  int t_is_library;       /* technology optimization type */
  int t_is_library_old;
  int t_is_multi_level;       /* multilevel optimization type */
  int t_is_multi_level_old;
  int t_is_2l_min;      /* 2 level minimization type */
  int t_is_2l_min_old; 
  int t_is_dly_path;      /* delay path construction */
  int t_is_dly_path_old; 
  int t_is_flatten;
  int t_is_flatten_old;
  int t_is_fbo;
  int t_is_fbo_old;
  int t_is_delay;
  int t_is_delay_old;
  
  char disable_cells[1024*2];
  char import_lib_name[1024];
  char target_library_name[1024];
  char cell_library_name[1024];
  char cell_name[1024];
  
  char view_name[1024];
  char arch_name[1024];
  
  int cell_ref;
  
  /* for the file open and file save dialog boxes */
  GtkFileSelection *fo;
  GtkFileSelection *lo;
  GtkFileSelection *fs;
  
  /* for the encoding parameters dialog box */
  GtkDialog *dialEnc;
  
  /* for the synthesis parameters dialog box */
  GtkDialog *dialSynth;
  
  /* for the library parameters dialog box */
  GtkDialog *dialLib;
  GtkDialog *dialLib1;
  GtkDialog *dialLog;
  GtkWidget *text_tln;
  GtkWidget *text_cln;
  GtkWidget *text_cn;
  GtkWidget *text_vn;
  GtkWidget *text_an;
  
  /* the text window */
  GtkWidget *text_window;
  char *text_text;
  
  /* for the log window */
  GtkWidget *log_window;
  char *log_text;
  
  /* for the err window */
  GtkWidget *err_window;
  char *err_text;
  
  GtkWidget *spin;
};


/* for the menus */
static void fileOpenClb(GtkObject *, gpointer);

static void boolPlaClb(GtkObject *, gpointer);
static void boolBexClb(GtkObject *, gpointer);

static void netEdifClb(GtkObject *, gpointer);
static void netVhdlClb(GtkObject *, gpointer);
static void netXnfClb(GtkObject *, gpointer);

static void fileSaveLogClb(GtkObject *, gpointer);

static void libOpenClb(GtkObject *, gpointer);

static void editEncClb(GtkObject *, gpointer);
static void editSynthClb(GtkObject *, gpointer);
static void editLibClb(GtkObject *, gpointer);
static void editLogClb(GtkObject *, gpointer);

static void synthesisClb(GtkObject *, gpointer);

static void winLogClrClb(GtkObject *, gpointer);
static void winErrClrClb(GtkObject *, gpointer);


static void helpAboutClb(GtkObject *, gpointer);

/* for the File Open dialog box */
static void fileOpenOkClb( GtkWidget *w, gpointer data);
static void fileOpenCancelClb( GtkWidget *w, gpointer data);

/* for the Library Open dialog box */
static void libOpenOkClb( GtkWidget *w, gpointer data);
static void libOpenCancelClb( GtkWidget *w, gpointer data);

/* for the File Save dialog box */
static void boolPlaOkClb( GtkWidget *w, gpointer data);
static void boolPlaCancelClb( GtkWidget *w, gpointer data);
static void boolBexOkClb( GtkWidget *w, gpointer data);
static void boolBexCancelClb( GtkWidget *w, gpointer data);


static void netEdifOkClb( GtkWidget *w, gpointer data);
static void netEdifCancelClb( GtkWidget *w, gpointer data);
static void netVhdlOkClb( GtkWidget *w, gpointer data);
static void netVhdlCancelClb( GtkWidget *w, gpointer data);
static void netXnfOkClb( GtkWidget *w, gpointer data);
static void netXnfCancelClb( GtkWidget *w, gpointer data);

/* for the Encode dialog box */
static void encodeFanInClb(GtkWidget *w, gpointer data);
static void encodeIcAllClb(GtkWidget *w, gpointer data);
static void encodeIcPartClb(GtkWidget *w, gpointer data);
static void encodeSimpleClb(GtkWidget *w, gpointer data);
static void encodeOkClb(GtkWidget *w, gpointer data);
static void encodeCancelClb(GtkWidget *w, gpointer data);

/* for the Synthesis dialog box */
static void synthSyncClb(GtkWidget *w, gpointer data);
static void synthAsyncClb(GtkWidget *w, gpointer data);

static void synthResetLowClb(GtkWidget *w, gpointer data);
static void synthResetHighClb(GtkWidget *w, gpointer data);
static void synthNoResetClb(GtkWidget *w, gpointer data);

static void synthNecaClb(GtkWidget *w, gpointer data);
static void synthNoNecaClb(GtkWidget *w, gpointer data);

static void synthGenericClb(GtkWidget *w, gpointer data);
static void synthNoGenericClb(GtkWidget *w, gpointer data);

static void synthTechnClb(GtkWidget *w, gpointer data);
static void synthNoTechnClb(GtkWidget *w, gpointer data);

static void synthMLevelClb(GtkWidget *w, gpointer data);
static void synthNoMLevelClb(GtkWidget *w, gpointer data);

static void synth2LevMinClb(GtkWidget *w, gpointer data);
static void synthNo2LevMinClb(GtkWidget *w, gpointer data);

static void synthOutFBClb(GtkWidget *w, gpointer data);
static void synthNoOutFBClb(GtkWidget *w, gpointer data);

static void synthDelayClb(GtkWidget *w, gpointer data);
static void synthNoDelayClb(GtkWidget *w, gpointer data);

static void synthesisOkClb(GtkWidget *w, gpointer data);
static void synthesisCancelClb(GtkWidget *w, gpointer data);

/* for the library parameters dialog box */
static void libOkClb(GtkWidget *w, gpointer data);
static void libCancelClb(GtkWidget *w, gpointer data);
static void libOkClb1(GtkWidget *w, gpointer data);
static void libCancelClb1(GtkWidget *w, gpointer data);

static void logOkClb(GtkWidget *w, gpointer data);
static void logCancelClb(GtkWidget *w, gpointer data);

/* for deleting and exiting */
static gint eventDeleteClb(GtkWidget *widget, GdkEvent *event, gpointer data);
static gint eventDestroyClb(GtkWidget *widget, GdkEvent *event, gpointer data);

/* Functions for creating the widges */

struct user_data * graphic_NewUData();
GtkWidget* makeSynthTable( gpointer data);
void makeStateMenu(GtkWidget *frame, gpointer data);
void makeResetMenu(GtkWidget *frame, gpointer data);
void makeNetMenu(GtkWidget *frame, gpointer data);
void makeGenericMenu(GtkWidget *frame, gpointer data);
void makeTechnMenu(GtkWidget *frame, gpointer data);
void makeMultiMenu(GtkWidget *frame, gpointer data);
void make2LevelMenu(GtkWidget *frame, gpointer data);
void makeDlyPath(GtkWidget *frame, gpointer data);
void makeDelayCorrMenu(GtkWidget *frame, gpointer data);;
void makeOutFBMenu(GtkWidget *frame, gpointer data);

void makeRadioBut(GtkWidget *frame, gpointer data);

void makeEntries(GtkWidget *frame, gpointer data);
void makeEntries1(GtkWidget *frame, gpointer data);

/* the synthesis function */
int doAll(gnc nc, gpointer data);

#endif /* _DGC_GUI_H */


