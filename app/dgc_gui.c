/*

  dgc_gui.c

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

#include <gnome.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>

#include "config.h"
#include "fsm.h"
#include "dcube.h"
#include "encode_func.h"
#include "fsmenc.h"
#include "dgc_gui.h"
#include "gnet.h"



GnomeUIInfo netMenu[] = 
{

  { GNOME_APP_UI_ITEM, "_Save as EDIF", "Save to an EDIF netlist file", netEdifClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL },
 
  { GNOME_APP_UI_ITEM, "_Save as VHDL", "Save to a VHDL netlist file", netVhdlClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL },

  { GNOME_APP_UI_ITEM, "_Save as XNF", "Save to a Xilinx netlist file", netXnfClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL },
    
  GNOMEUIINFO_END
};

GnomeUIInfo boolMenu[] = 
{

  { GNOME_APP_UI_ITEM, "_Save as PLA", "Save to a PLA file", boolPlaClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL },
 
  { GNOME_APP_UI_ITEM, "_Save as BEX", "Save to a BEX file", boolBexClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL },

    
  GNOMEUIINFO_END
};


GnomeUIInfo fileMenu[] = 
{
  { GNOME_APP_UI_ITEM, "_Open", "Reads a valid PLA, BEX, NEX, KISS, BMS or DGD file", fileOpenClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_OPEN, 0, 0, NULL },
 
  
  GNOMEUIINFO_SEPARATOR,

  GNOMEUIINFO_SUBTREE("Export as _netlist", netMenu),
  
  GNOMEUIINFO_SUBTREE("Export as _boolean function", boolMenu),
  
  GNOMEUIINFO_SEPARATOR,
  
   { GNOME_APP_UI_ITEM, "_Save Log Window", "Save the log window to a file", fileSaveLogClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL },
    
  GNOMEUIINFO_SEPARATOR,
 

  { GNOME_APP_UI_ITEM, "E_xit", "Exit the program", eventDestroyClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_EXIT, 0, 0, NULL },

   
  GNOMEUIINFO_END
};

GnomeUIInfo libraryMenu[] = 
{
  { GNOME_APP_UI_ITEM, "O_pen import gate library", "Open a gate library", libOpenClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_BOOK_OPEN, 0, 0, NULL },
   
  GNOMEUIINFO_END
};



GnomeUIInfo editMenu[] = 
{

  { GNOME_APP_UI_ITEM, "_Encoding Parameters...", "Edit the encoding parameters for the FSM", editEncClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PROPERTIES, 0, 0, NULL },

  { GNOME_APP_UI_ITEM, "_Synthesis Parameters...", "Edit the synthesis parameters", editSynthClb, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_PREFERENCES, 0, 0, NULL },
    
  GNOMEUIINFO_SEPARATOR,

  
  { GNOME_APP_UI_ITEM, "Li_brary Parameters...", "Edit the library parameters", editLibClb, NULL, NULL, 
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  
  GNOMEUIINFO_SEPARATOR,
  
  { GNOME_APP_UI_ITEM, "Log level...", "Modify the log level", editLogClb, NULL, NULL, 
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
 
   
  GNOMEUIINFO_END
};

GnomeUIInfo winMenu[] = 
{

  { GNOME_APP_UI_ITEM, "Clear _Log Window", "Clears the log window", winLogClrClb, NULL, NULL, 
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },

  { GNOME_APP_UI_ITEM, "Clear _Error Window", "Clears the error window", winErrClrClb, NULL, NULL, 
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
   
  GNOMEUIINFO_END
};

GnomeUIInfo helpMenu[] =
{
  /*GNOMEUIINFO_HELP("dgc"),*/
  
  GNOMEUIINFO_MENU_ABOUT_ITEM(helpAboutClb, NULL),

  GNOMEUIINFO_END
};

GnomeUIInfo mainMenu[] = 
{
  GNOMEUIINFO_SUBTREE("_File", fileMenu),
  
  GNOMEUIINFO_SUBTREE("_Library", libraryMenu),
  
  GNOMEUIINFO_SUBTREE("_Edit", editMenu),
  
  
  { GNOME_APP_UI_ITEM, "_Synthesis", "Make a synthesis of the circuit", synthesisClb, NULL, NULL, 
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },

  GNOMEUIINFO_SUBTREE("_Window", winMenu),
  
  GNOMEUIINFO_SUBTREE("_Help", helpMenu),

  GNOMEUIINFO_END
};


/* writing logs in the log window */
void gui_LogVA(void *ptr, char *fmt, va_list va)
{
  char log_line[1024];
  struct user_data *udata = (struct user_data *)ptr;
  
  vsprintf(log_line, fmt, va);
  strcat(udata->log_text, log_line);
  strcat(udata->log_text, "\n");
  gnome_less_show_string(GNOME_LESS(udata->log_window), udata->log_text);
  
}

/* writing errors in the err window */
void gui_ErrVA(void *ptr, char *fmt, va_list va)
{
  char err_line[1024];
  struct user_data *udata = (struct user_data *)ptr;
 
  
  vsprintf(err_line, fmt, va);
  strcat(udata->err_text, err_line);
  strcat(udata->err_text, "\n");
  gnome_less_show_string(GNOME_LESS(udata->err_window), udata->err_text);
  
}

int main(int argc, char *argv[])
{
  GtkWidget *app;
  GtkWidget *statusbar;
  GtkWidget *text, *log, *err, *paned, *panedh;
 
  char dir[1000];
  
  struct user_data *udata;
 
  gnome_init("dgc", "1.0", argc, argv);
  app = gnome_app_new("dgc", "Digital Gate Compiler");
  gtk_window_set_default_size(GTK_WINDOW(app), 300, 400);
  
  gtk_signal_connect(GTK_OBJECT(app), "delete_event", GTK_SIGNAL_FUNC(eventDeleteClb), NULL);
  gtk_signal_connect(GTK_OBJECT(app), "destroy", GTK_SIGNAL_FUNC(eventDestroyClb), NULL);
  
  fileMenu[FILE_OPEN].user_data = (gpointer) app;
  boolMenu[BOOL_PLA].user_data = (gpointer) app;
  boolMenu[BOOL_BEX].user_data = (gpointer) app;
  netMenu[NET_EDIF].user_data = (gpointer) app;
  netMenu[NET_VHDL].user_data = (gpointer) app;
  netMenu[NET_XNF].user_data = (gpointer) app;
  fileMenu[FILE_SAVE_LOG].user_data = (gpointer) app;

  libraryMenu[LIBRARY_OPEN].user_data = (gpointer) app;
  
  editMenu[EDIT_ENC].user_data = (gpointer) app;
  editMenu[EDIT_SYNTH].user_data = (gpointer) app;
  editMenu[EDIT_LIB].user_data = (gpointer) app;
  editMenu[EDIT_LOG].user_data = (gpointer) app;
  
  winMenu[WIN_LOG_CLR].user_data = (gpointer) app;
  winMenu[WIN_ERR_CLR].user_data = (gpointer) app;
 
  
  mainMenu[SYNTHESIS].user_data = (gpointer) app;
 
  gnome_app_create_menus(GNOME_APP(app), mainMenu);
  
  
  statusbar = gtk_statusbar_new();
  gnome_app_install_statusbar_menu_hints(GTK_STATUSBAR(statusbar), mainMenu);
  gnome_app_set_statusbar(GNOME_APP(app), statusbar);
  
  strcpy(dir, DATA_DIR);
  strcat(dir, "/help/dgc/C");

  gnome_help_file_path("dgc", dir);
  
  gtk_widget_show_all(app);
  
  /* make my initializtions */
  udata = graphic_NewUData();

  gtk_object_set_user_data(GTK_OBJECT(app), (gpointer) udata);

  udata->nc = gnc_Open();
  gnc_SetLogFn(udata->nc, gui_LogVA, udata);
  gnc_SetErrFn(udata->nc, gui_ErrVA, udata);

  text = gnome_less_new();
  udata->text_window = text;

  log = gnome_less_new();
  udata->log_window = log;

  err = gnome_less_new();
  udata->err_window = err;
  
  panedh = gtk_vpaned_new();
  gtk_paned_add1(GTK_PANED(panedh), log);
  gtk_paned_add2(GTK_PANED(panedh), err);
  
  
  paned = gtk_vpaned_new();
  gtk_paned_add1(GTK_PANED(paned), text);
  gtk_paned_add2(GTK_PANED(paned), panedh);

  gnome_app_set_contents(GNOME_APP(app), paned);
  gtk_widget_show_all(GTK_WIDGET(app));

 
  
  gtk_main();
  
  /* making my cleaning */
    
  if( udata->nc != NULL)
    gnc_Close(udata->nc);

  free(udata->text_text);  
  free(udata->log_text);  
  free(udata->err_text);  
  free(udata);

  exit(0);
}

/* ------------ fileOpenClb -----------------------------*/

static void fileOpenClb(GtkObject *object, gpointer data)
{
  /* data is a pointer to the application */
  GtkWidget * file_open;
  struct user_data *udata;
  char dir[500];
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  file_open = gtk_file_selection_new ("PLA, BEX, NEX, KISS, BMS, DGD File selection");
  udata->fo = GTK_FILE_SELECTION(file_open); 

      /* Connect the ok_button to dialog_file_open_ok_callback function */
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_open)->ok_button), "clicked", 
    GTK_SIGNAL_FUNC (fileOpenOkClb), data );
    
      /* Connect the cancel_button to dialog_file_open_cancel_callback function */
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_open)->cancel_button), "clicked", 
    GTK_SIGNAL_FUNC (fileOpenCancelClb), data);
  
  strcpy(dir, DATA_DIR);
  strcat(dir, "/");
  gtk_file_selection_set_filename (GTK_FILE_SELECTION(file_open), dir);
                                     
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_open));
    
  gtk_widget_show(file_open);
}


/* ------------ libOpenClb -----------------------------*/

static void libOpenClb(GtkObject *object, gpointer data)
{
  /* data is a pointer to the application */
  GtkWidget * lib_open;
  struct user_data *udata;
  char dir[500];
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  strcpy(udata->cell_library_name, "");

  lib_open = gtk_file_selection_new ("Gate library selection");
  udata->lo = GTK_FILE_SELECTION(lib_open); 

      /* Connect the ok_button to dialog_file_open_ok_callback function */
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (lib_open)->ok_button), "clicked", 
    GTK_SIGNAL_FUNC (libOpenOkClb), data );
    
      /* Connect the cancel_button to dialog_file_open_cancel_callback function */
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (lib_open)->cancel_button), "clicked", 
    GTK_SIGNAL_FUNC (libOpenCancelClb), data);
  
  strcpy(dir, DATA_DIR);
  strcat(dir, "/");
  gtk_file_selection_set_filename (GTK_FILE_SELECTION(lib_open), dir);
                                     
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(lib_open));
    
  gtk_widget_show(lib_open);
}


/* ------------ boolPlaClb ----------------------------- */

static void boolPlaClb(GtkObject *object, gpointer data)
{
  /* data is a pointer to the app */
  GtkWidget * file_save;
  struct user_data *udata;
  char dir[500];

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  if(strcmp(udata->file_name, "") == 0)
  {
    gnome_error_dialog("Please load a file first!");
    return;
  }
  
  if ( udata->cell_ref <= 0 )
  {
    gnome_error_dialog("Please perform a synthesis first!");
    return;
   
  }

  file_save = gtk_file_selection_new ("PLA File selection");
  udata->fs = GTK_FILE_SELECTION(file_save); 

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_save)->ok_button), "clicked", 
    GTK_SIGNAL_FUNC (boolPlaOkClb), data );

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_save)->cancel_button), "clicked", 
    GTK_SIGNAL_FUNC (boolPlaCancelClb), data);

  strcpy(dir, DATA_DIR);
  strcat(dir, "/");
  gtk_file_selection_set_filename (GTK_FILE_SELECTION(file_save), dir);

  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_save));

  gtk_widget_show(file_save);
}

/* ------------ boolBexClb ----------------------------- */

static void boolBexClb(GtkObject *object, gpointer data)
{
  /* data is a pointer to the app */
  GtkWidget * file_save;
  struct user_data *udata;
  char dir[500];

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  if(strcmp(udata->file_name, "") == 0)
  {
    gnome_error_dialog("Please load a file first!");
    return;
  }
  
  if(udata->cell_ref < 0)
  {
    gnome_error_dialog("Please perform a synthesis first!");
    return;
  
  }

  file_save = gtk_file_selection_new ("PLA File selection");
  udata->fs = GTK_FILE_SELECTION(file_save); 

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_save)->ok_button), "clicked", 
    GTK_SIGNAL_FUNC (boolBexOkClb), data );

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_save)->cancel_button), "clicked", 
    GTK_SIGNAL_FUNC (boolBexCancelClb), data);

  strcpy(dir, DATA_DIR);
  strcat(dir, "/");
  gtk_file_selection_set_filename (GTK_FILE_SELECTION(file_save), dir);

  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_save));

  gtk_widget_show(file_save);
}

/* ------------ netEdifClb ----------------------------- */

static void netEdifClb(GtkObject *object, gpointer data)
{
  /* data is a pointer to the app */
  GtkWidget * file_save;
  struct user_data *udata;
  char dir[500];

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  if(strcmp(udata->file_name, "") == 0)
  {
    gnome_error_dialog("Please load a file first!");
    return;
  }
  
  if(udata->cell_ref < 0)
  {
    gnome_error_dialog("Please perform a synthesis first!");
    return;

  }

  file_save = gtk_file_selection_new ("EDIF File selection");
  udata->fs = GTK_FILE_SELECTION(file_save); 

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_save)->ok_button), "clicked", 
    GTK_SIGNAL_FUNC (netEdifOkClb), data );

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_save)->cancel_button), "clicked", 
    GTK_SIGNAL_FUNC (netEdifCancelClb), data);

  strcpy(dir, DATA_DIR);
  strcat(dir, "/");
  gtk_file_selection_set_filename (GTK_FILE_SELECTION(file_save), dir);

  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_save));

  gtk_widget_show(file_save);
}

/* ------------ netVhdlClb ----------------------------- */

static void netVhdlClb(GtkObject *object, gpointer data)
{
  /* data is a pointer to the app */
  GtkWidget * file_save;
  struct user_data *udata;
  char dir[500];

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  if(strcmp(udata->file_name, "") == 0)
  {
    gnome_error_dialog("Please load a file first!");
    return;
  }
  
  if ( udata->cell_ref <= 0 )
  {
    gnome_error_dialog("Please perform a synthesis first!");
    return;
   
  }


  file_save = gtk_file_selection_new ("VHDL File selection");
  udata->fs = GTK_FILE_SELECTION(file_save); 

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_save)->ok_button), "clicked", 
    GTK_SIGNAL_FUNC (netVhdlOkClb), data );

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_save)->cancel_button), "clicked", 
    GTK_SIGNAL_FUNC (netVhdlCancelClb), data);

  strcpy(dir, DATA_DIR);
  strcat(dir, "/");
  gtk_file_selection_set_filename (GTK_FILE_SELECTION(file_save), dir);

  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_save));

  gtk_widget_show(file_save);
}

/* ------------ netXnfClb ----------------------------- */

static void netXnfClb(GtkObject *object, gpointer data)
{
  /* data is a pointer to the app */
  GtkWidget * file_save;
  struct user_data *udata;
  char dir[500];

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  if(strcmp(udata->file_name, "") == 0)
  {
    gnome_error_dialog("Please load a file first!");
    return;
  }
  
  if ( udata->cell_ref <= 0 )
  {
    gnome_error_dialog("Please perform a synthesis first!");
    return;
   
  }


  file_save = gtk_file_selection_new ("XNF File selection");
  udata->fs = GTK_FILE_SELECTION(file_save); 

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_save)->ok_button), "clicked", 
    GTK_SIGNAL_FUNC (netXnfOkClb), data );

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_save)->cancel_button), "clicked", 
    GTK_SIGNAL_FUNC (netXnfCancelClb), data);

  strcpy(dir, DATA_DIR);
  strcat(dir, "/");
  gtk_file_selection_set_filename (GTK_FILE_SELECTION(file_save), dir);

  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_save));

  gtk_widget_show(file_save);
}

/* ------------ fileSaveLogClb ----------------------------- */

static void fileSaveLogClb(GtkObject *object, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  char dir[500];

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  if(udata->log_window == NULL)
    return;
    
  strcpy(dir, DATA_DIR);
  strcat(dir, "/log.txt");
  gnome_less_write_file(GNOME_LESS(udata->log_window), dir);
  gnome_ok_dialog("Log window was written to \"log.txt\" file!");
}

/* ------------ synthesisClb ----------------------------- */

static void synthesisClb(GtkObject *object, gpointer data)
{
  struct user_data *udata;

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
 
  if(udata->nc != NULL)
    gnc_Close(udata->nc);
  udata->nc = gnc_Open();

  gnc_SetLogFn(udata->nc, gui_LogVA, udata);
  gnc_SetErrFn(udata->nc, gui_ErrVA, udata);

  strcpy(udata->log_text, "");
  gnome_less_show_string(GNOME_LESS(udata->log_window), udata->log_text);
  strcpy(udata->err_text, "");
  gnome_less_show_string(GNOME_LESS(udata->err_window), udata->err_text);

  
  if(udata->nc != NULL)
    if(doAll(udata->nc, data)==0)
      gnc_Error(udata->nc, "Couldn't do the synthesis!\n");
}


/* ------------ winLogClrClb ----------------------------- */

static void winLogClrClb(GtkObject *object, gpointer data)
{
  struct user_data *udata;

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  strcpy(udata->log_text,"");
  gnome_less_show_string(GNOME_LESS(udata->log_window), udata->log_text);
}

/* ------------ winErrClrClb ----------------------------- */

static void winErrClrClb(GtkObject *object, gpointer data)
{
  struct user_data *udata;

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  strcpy(udata->err_text,"");
  gnome_less_show_string(GNOME_LESS(udata->err_window), udata->err_text);
}

/* ----------------- DIALOG BOXES ----------------------- */

/* called by the OK button of the File Selection dialog box */
static void fileOpenOkClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  strcpy(udata->file_name, gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fo)));
  
  gnome_less_show_file(GNOME_LESS(udata->text_window),gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fo)) );
  
  strcpy(udata->log_text, "");
  strcpy(udata->err_text, "");
  gnome_less_show_string(GNOME_LESS(udata->log_window), udata->log_text);
  gnome_less_show_string(GNOME_LESS(udata->err_window), udata->err_text);
 

  gtk_widget_show_all(GTK_WIDGET(data));

  gtk_widget_destroy(GTK_WIDGET(udata->fo));
  udata->fo = NULL;

}

/* called by the Cancel button of the File Selection dialog box */
static void fileOpenCancelClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the main window */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gtk_widget_destroy(GTK_WIDGET(udata->fo));
  udata->fo = NULL;
}


/* called by the OK button of the File Selection library dialog box */
static void libOpenOkClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  GtkWidget *dialog, *vbox, *hbox, *label, *button;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  /* read the library */
   
  strcpy(udata->import_lib_name, gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->lo)));
  
  gtk_widget_destroy(GTK_WIDGET(udata->lo));
  udata->lo = NULL;
  
  
  dialog = gtk_dialog_new();
  udata->dialLib1 = GTK_DIALOG(dialog);
  gtk_window_set_title(GTK_WINDOW(dialog), "Rename library...");
  
  
  vbox = GTK_DIALOG(dialog)->vbox;
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
  label = gtk_label_new("Please input the new name if you want to rename the library:");
  gtk_box_pack_start(GTK_BOX (vbox), label, TRUE, FALSE, 10);

  label = gtk_label_new("(You can leave the text empty if you don't want to rename it)");
  gtk_box_pack_start(GTK_BOX (vbox), label, TRUE, FALSE, 10);
 
  makeEntries1(vbox, data); 
 

  hbox =   GTK_DIALOG(dialog)->action_area;
    
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
  
  button = gtk_button_new_with_label("OK");
  gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, FALSE, 20);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (libOkClb1), 
      data);
  
  button = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, FALSE, 20);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (libCancelClb1), 
      data);
  
  gtk_widget_show_all(dialog);

}

/* called by the Cancel button of the File Selection dialog box */
static void libOpenCancelClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the main window */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gtk_widget_destroy(GTK_WIDGET(udata->lo));
  udata->lo = NULL;
}



static void boolPlaOkClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  if ( gnc_GetGCELL(udata->nc, udata->cell_ref)->pi != NULL &&
       gnc_GetGCELL(udata->nc, udata->cell_ref)->cl_on != NULL )
  {
    gnc_Log(udata->nc, 6, "Write PLA file '%s'.", 
      gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs)));
    if ( gnc_WriteCellPLA(udata->nc, udata->cell_ref, 
      gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs))) == 0 )
    {
      gnc_Log(udata->nc, 6, "Failed with file '%s'.", 
        gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs)));
    }
  }
  else
  {
    gnc_Error(udata->nc, "Can not write PLA file (no SOP specification available).");
  }

  gtk_widget_destroy(GTK_WIDGET(udata->fs));
  udata->fs = NULL;
}

static void boolPlaCancelClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gtk_widget_destroy(GTK_WIDGET(udata->fs));
  udata->fs = NULL;

}

static void boolBexOkClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  if ( gnc_GetGCELL(udata->nc, udata->cell_ref)->pi != NULL &&
      gnc_GetGCELL(udata->nc, udata->cell_ref)->cl_on != NULL )
  {
    gnc_Log(udata->nc, 6, "Write BEX file '%s'.", 
      gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs)));
    if ( gnc_WriteCellBEX(udata->nc, udata->cell_ref, 
      gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs))) == 0 )
    {
      gnc_Log(udata->nc, 6, "Failed with file '%s'.", 
        gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs)));
    }
  }
  else
  {
    gnc_Error(udata->nc, "Can not write BEX file (no SOP specification available).");
  }


  gtk_widget_destroy(GTK_WIDGET(udata->fs));
  udata->fs = NULL;
}

static void boolBexCancelClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gtk_widget_destroy(GTK_WIDGET(udata->fs));
  udata->fs = NULL;

}
static void netEdifOkClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gnc_Log(udata->nc, 6, "Write EDIF file.");
  if ( gnc_WriteEdif(udata->nc, gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs)) , 
    udata->view_name) == 0 )
  {
    gnc_Error(udata->nc, "Couldn't write the EDIF file!");
  }
  
  gtk_widget_destroy(GTK_WIDGET(udata->fs));
  udata->fs = NULL;
}

static void netEdifCancelClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gtk_widget_destroy(GTK_WIDGET(udata->fs));
  udata->fs = NULL;

}

static void netVhdlOkClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gnc_Log(udata->nc, 6, "Write VHDL file.");
  if ( gnc_WriteVHDL(udata->nc, udata->cell_ref, gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs)), 
      udata->cell_name, "netlist", GNC_WRITE_VHDL_UPPER_KEY, -1, udata->cell_library_name) == 0 )
  {
    gnc_Log(udata->nc, 6, "Failed with file '%s'.", 
      gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs)));
  }
  
  gtk_widget_destroy(GTK_WIDGET(udata->fs));
  udata->fs = NULL;
}

static void netVhdlCancelClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gtk_widget_destroy(GTK_WIDGET(udata->fs));
  udata->fs = NULL;

}
static void netXnfOkClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  
  
  gnc_Log(udata->nc, 6, "Write XNF file.");
  if ( gnc_WriteXNF(udata->nc, udata->cell_ref, gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs))) == 0 )
  {
    gnc_Log(udata->nc, 6, "Failed with file '%s'.",
      gtk_file_selection_get_filename (GTK_FILE_SELECTION (udata->fs)) );
  }
  
  gtk_widget_destroy(GTK_WIDGET(udata->fs));
  udata->fs = NULL;
}

static void netXnfCancelClb( GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gtk_widget_destroy(GTK_WIDGET(udata->fs));
  udata->fs = NULL;

}

static void editEncClb(GtkObject *w, gpointer data)
{
  /* data is a pointer to the app */
  
  GtkWidget * dialog;
  GtkWidget * button;
  GtkWidget * label, *vbox, *hbox;
  
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

/*
 *   if(udata->fsm == NULL)
 *   {
 *     gnome_ok_dialog("Load fsm first\n"); 
 *   }
 *   else
 *   {
 */
    dialog = gtk_dialog_new();
    udata->dialEnc = GTK_DIALOG(dialog);
    gtk_window_set_title(GTK_WINDOW(dialog), "Encoding parameters...");
  
  
    vbox = GTK_DIALOG(dialog)->vbox;
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    
    label = gtk_label_new("Please select the following encoding parameters:");
    gtk_box_pack_start(GTK_BOX (vbox), label, TRUE, FALSE, 10);

  
    makeRadioBut(vbox, data); 
 

    hbox =   GTK_DIALOG(dialog)->action_area;
    
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
  
    button = gtk_button_new_with_label("OK");
    gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, FALSE, 20);
    gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (encodeOkClb), 
      data);
  
    button = gtk_button_new_with_label("Cancel");
    gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, FALSE, 20);
    gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (encodeCancelClb), 
      data);
  
    gtk_widget_show_all(dialog);
/*
 *   }
 */


}

static void editSynthClb(GtkObject *w, gpointer data)
{
  /* data is a pointer to the app */
  
  GtkWidget * dialog;
  GtkWidget *button;
  GtkWidget *box;
  GtkWidget *vbox, *label;
  GtkWidget *hbox;
  
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  dialog = gtk_dialog_new();
  udata->dialSynth = GTK_DIALOG(dialog);
  gtk_window_set_title(GTK_WINDOW(dialog), "Synthesis parameters...");
  
  vbox = GTK_DIALOG(dialog)->vbox;
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  
  label = gtk_label_new("Please select the following synthesis parameters:");
  gtk_box_pack_start(GTK_BOX (vbox), label, TRUE, FALSE, 10);
  
  box = makeSynthTable(data); 
  gtk_box_pack_start(GTK_BOX(vbox), box, TRUE, FALSE, 10);
  
 
  hbox = GTK_DIALOG(dialog)->action_area;
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 20);
  
  
  button = gtk_button_new_with_label("OK");
  gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (synthesisOkClb), 
    data);
  
  button = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (synthesisCancelClb), 
    data);
  
  gtk_widget_show_all(dialog);

}

static void editLibClb(GtkObject *w, gpointer data)
{
  /* data is a pointer to the app */
  
  GtkWidget * dialog;
  GtkWidget * button;
  GtkWidget * label, *vbox, *hbox;
  
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  dialog = gtk_dialog_new();
  udata->dialLib = GTK_DIALOG(dialog);
  gtk_window_set_title(GTK_WINDOW(dialog), "Library parameters...");
  
  
  vbox = GTK_DIALOG(dialog)->vbox;
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  
  label = gtk_label_new("Please input the following library parameters:");
  gtk_box_pack_start(GTK_BOX (vbox), label, TRUE, FALSE, 10);

  
  makeEntries(vbox, data); 
 

  hbox =   GTK_DIALOG(dialog)->action_area;
  
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
  
  button = gtk_button_new_with_label("OK");
  gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, FALSE, 20);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (libOkClb), 
    data);
  
  button = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, FALSE, 20);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (libCancelClb), 
    data);
  
  gtk_widget_show_all(dialog);
}

static void editLogClb(GtkObject *w, gpointer data)
{
  /* data is a pointer to the app */
  
  GtkWidget * dialog;
  GtkWidget * label, *vbox, *hbox, *button;

  GtkWidget *spin;
  GtkObject *adjustement;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  dialog = gtk_dialog_new();
  udata->dialLog = GTK_DIALOG(dialog);
  gtk_window_set_title(GTK_WINDOW(dialog), "The log level...");
  
  vbox = GTK_DIALOG(dialog)->vbox;
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  
  label = gtk_label_new("Please select the log level:");
  gtk_box_pack_start(GTK_BOX (vbox), label, TRUE, FALSE, 10);
  
  adjustement = gtk_adjustment_new((int) 4, (int) 0, (int) 7, (int)1, (int) 0, (int)0);
  spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustement), 1, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), (gfloat) udata->log_level);
  udata->spin = spin;
  
  gtk_box_pack_start(GTK_BOX(vbox), spin, TRUE, FALSE, 10);
  
 
  hbox = GTK_DIALOG(dialog)->action_area;
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 20);
  
  
  button = gtk_button_new_with_label("OK");
  gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (logOkClb), 
    data);
  
  button = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (logCancelClb), 
    data);
  
  gtk_widget_show_all(dialog);


}

static void encodeFanInClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->state_encoding =FSM_ENCODE_FAN_IN ;
}

static void encodeIcAllClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->state_encoding = FSM_ENCODE_IC_ALL;
}

static void encodeIcPartClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->state_encoding =FSM_ENCODE_IC_PART ;

}

static void encodeSimpleClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->state_encoding = FSM_ENCODE_SIMPLE;

}

static void encodeOkClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
/*
 *   if(fsm_BuildClockedMachine(udata->fsm, udata->state_encoding) == 0)
 *   {
 *      gnome_ok_dialog("Couldn't do the encoding\n, please try another encoding method!\n"); 
 * 
 *   }
 */
  
  udata->state_encoding_old = udata->state_encoding;
  gtk_widget_destroy(GTK_WIDGET(udata->dialEnc));
  udata->dialEnc = NULL;

}

static void encodeCancelClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->state_encoding = udata->state_encoding_old;
  
  gtk_widget_destroy(GTK_WIDGET(udata->dialEnc));
  udata->dialEnc = NULL;

}

static void synthSyncClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->is_async = 0;


}

static void synthAsyncClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->is_async = GNC_HL_OPT_CLOCK;


}


static void synthResetLowClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->reset_type = GNC_HL_OPT_CLR_LOW;
}

static void synthResetHighClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->reset_type = GNC_HL_OPT_CLR_HIGH;
}

static void synthNoResetClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->reset_type = GNC_HL_OPT_NO_RESET;
}

static void synthNecaClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_neca = GNC_SYNTH_OPT_NECA;

}
static void synthNoNecaClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_neca = 0;
}

static void synthGenericClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_generic = GNC_SYNTH_OPT_GENERIC;
}

static void synthNoGenericClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_generic = 0;
}

static void synthTechnClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_library = GNC_SYNTH_OPT_LIBARY;

}

static void synthNoTechnClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_library = 0;

}

static void synthMLevelClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_multi_level = GNC_SYNTH_OPT_LEVELS;

}

static void synthNoMLevelClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_multi_level = 0;

}
static void synthDelayClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_delay = 1;

}

static void synthNoDelayClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_delay = 0;

}

static void synth2LevMinClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_2l_min = GNC_HL_OPT_MINIMIZE;
}

static void synthNo2LevMinClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_2l_min = 0;

}

static void synthDlyPathClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_dly_path = GNC_SYNTH_OPT_DLYPATH;
}

static void synthNoDlyPathClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->t_is_dly_path = 0;

}

static void synthOutFBClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  udata->t_is_fbo = GNC_HL_OPT_FBO;
}

static void synthNoOutFBClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;

  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  udata->t_is_fbo = 0;
}

static void synthesisOkClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->is_async_old = udata->is_async;
  udata->reset_type_old = udata->reset_type;
  udata->t_is_neca_old = udata->t_is_neca;
  udata->t_is_generic_old = udata->t_is_generic;
  udata->t_is_library_old = udata->t_is_library;
  udata->t_is_multi_level_old = udata->t_is_multi_level;
  udata->t_is_2l_min_old = udata->t_is_2l_min;
  udata->t_is_dly_path_old = udata->t_is_dly_path;
  udata->t_is_delay_old = udata->t_is_delay;
  udata->t_is_fbo_old = udata->t_is_fbo;

  
  gtk_widget_destroy(GTK_WIDGET(udata->dialSynth));
  udata->dialSynth = NULL;

}

static void synthesisCancelClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->is_async = udata->is_async_old;
  udata->reset_type = udata->reset_type_old;
  udata->t_is_neca = udata->t_is_neca_old;
  udata->t_is_generic = udata->t_is_generic_old;
  udata->t_is_library = udata->t_is_library_old;
  udata->t_is_multi_level = udata->t_is_multi_level_old;
  udata->t_is_2l_min = udata->t_is_2l_min_old;
  udata->t_is_dly_path_old = udata->t_is_dly_path;
  udata->t_is_delay = udata->t_is_delay_old;
  udata->t_is_fbo = udata->t_is_fbo_old;

  
  gtk_widget_destroy(GTK_WIDGET(udata->dialSynth));
  udata->dialSynth = NULL;

}

static void libOkClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  strcpy(udata->target_library_name, gtk_entry_get_text(GTK_ENTRY(udata->text_tln)));
  strcpy(udata->cell_name, gtk_entry_get_text(GTK_ENTRY(udata->text_cn)));
  strcpy(udata->view_name, gtk_entry_get_text(GTK_ENTRY(udata->text_vn)));
  strcpy(udata->arch_name, gtk_entry_get_text(GTK_ENTRY(udata->text_an)));
  
  gtk_widget_destroy(GTK_WIDGET(udata->dialLib));
  udata->dialLib = NULL;


}

static void libCancelClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gtk_widget_destroy(GTK_WIDGET(udata->dialLib));
  udata->dialLib = NULL;

}

static void logOkClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->log_level = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(udata->spin));
  udata->spin = NULL;
  
  gtk_widget_destroy(GTK_WIDGET(udata->dialLog));
  udata->dialLog = NULL;


}

static void logCancelClb(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  udata->spin = NULL;
  gtk_widget_destroy(GTK_WIDGET(udata->dialLog));
  udata->dialLog = NULL;

}

static void libOkClb1(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  strcpy(udata->cell_library_name, gtk_entry_get_text(GTK_ENTRY(udata->text_cln)));
  

  
  gtk_widget_destroy(GTK_WIDGET(udata->dialLib1));
  udata->dialLib1 = NULL;


}

static void libCancelClb1(GtkWidget *w, gpointer data)
{
  /* data is a pointer to the app */
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));
  
  gtk_widget_destroy(GTK_WIDGET(udata->dialLib1));
  udata->dialLib1 = NULL;

}

static void helpAboutClb(GtkObject *w, gpointer data)
{
  GtkWidget *aboutBox;
  
  const gchar *authors[] =
  {
    "Oliver Kraus",
    "Claudia Spircu",
    "Tobias Dichtl",
    NULL
  };
  
  aboutBox = gnome_about_new("Digital Gate Compiler Application",
    "1.0", 
    "(C) 2001 Computer Aided Circuit Design, Uni Erlangen Nuernberg",
    authors,
    "DGC is free software; you can redistribute it and/or modify it under the terms "
    "of the GNU General Public License as published by the Free Software Foundation",
    NULL);
    
  gtk_widget_show(aboutBox);
}


struct user_data * graphic_NewUData()
{
  struct user_data *udata;
  
  udata = (struct user_data*) malloc(sizeof(struct user_data));
  /*udata->fsm = NULL;
  udata->is_with_output = 1;*/
  strcpy(udata->file_name, "");
  udata->log_level = 4;
  
  udata->nc = NULL;
  udata->fo = NULL;
  udata->lo = NULL;
  udata->fs = NULL;
  udata->state_encoding = FSM_ENCODE_SIMPLE;
  udata->is_async = 0;
  udata->reset_type = GNC_HL_OPT_CLR_LOW;
  udata->t_is_neca = GNC_SYNTH_OPT_NECA;
  udata->t_is_generic = GNC_SYNTH_OPT_GENERIC;
  udata->t_is_library = GNC_SYNTH_OPT_LIBARY;
  udata->t_is_multi_level = GNC_SYNTH_OPT_LEVELS;
  udata->t_is_2l_min = GNC_HL_OPT_MINIMIZE;
  udata->t_is_dly_path = 0;
  udata->t_is_delay = 1;
  udata->t_is_flatten = 1;
  udata->t_is_fbo = GNC_HL_OPT_FBO;
  
  udata->state_encoding_old = udata->state_encoding;
  udata->is_async_old = udata->is_async;
  udata->reset_type_old = udata->reset_type;
  udata->t_is_neca_old = udata->t_is_neca;
  udata->t_is_generic_old = udata->t_is_generic;
  udata->t_is_library_old = udata->t_is_library;
  udata->t_is_multi_level_old = udata->t_is_multi_level;
  udata->t_is_2l_min_old = udata->t_is_2l_min;
  udata->t_is_dly_path_old = udata->t_is_dly_path;
  udata->t_is_delay_old = udata->t_is_delay;
  udata->t_is_fbo_old = udata->t_is_fbo;
  
  strcpy(udata->disable_cells, "");
  strcpy(udata->import_lib_name, "");
  strcpy(udata->cell_library_name, "");
  strcpy(udata->cell_name, "mydesign");
  strcpy(udata->target_library_name, "mylib");
  
  strcpy(udata->view_name, "symbol");
  strcpy(udata->arch_name, "netlist");
 
  udata->cell_ref = -1;
  
  udata->dialEnc = NULL;
  udata->dialSynth = NULL;
  udata->dialLib = NULL;
  udata->dialLib1 = NULL;
  udata->dialLog = NULL;
  udata->text_tln = NULL;
  udata->text_cln = NULL;
  udata->text_cn = NULL;
  udata->text_vn = NULL;
  udata->text_an = NULL;

  udata->text_window = NULL;
  udata->text_text = (char*) malloc(100000 * sizeof(char));
 
  udata->log_window = NULL;
  udata->log_text = (char*) malloc(100000 * sizeof(char));

  udata->err_window = NULL;
  udata->err_text = (char*) malloc(100000 * sizeof(char));
  
  udata->spin = NULL;

  return udata;
}


GtkWidget * makeSynthTable( gpointer data)
{
  GtkWidget *frame, *frameBig;
  GtkWidget *table;
  GtkWidget *vbox;
  
  vbox = gtk_vbox_new(FALSE, 10);
  gtk_widget_show(vbox);
  
  frameBig = gtk_frame_new("General optimization parameters");
  gtk_box_pack_start(GTK_BOX(vbox), frameBig, FALSE, FALSE, 10);
  gtk_container_set_border_width(GTK_CONTAINER(frameBig), 0);
  
  table = gtk_table_new(3,2,TRUE);
  gtk_container_add(GTK_CONTAINER(frameBig), table);
 

  frame = gtk_frame_new("Net cache optimization");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 1, 0, 1, GTK_EXPAND| GTK_FILL, GTK_EXPAND, 10, 10);
 
  
  makeNetMenu(frame, data);
  
  frame = gtk_frame_new("Generic cell optimization");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(table), frame, 1, 2, 0, 1, GTK_EXPAND| GTK_FILL, GTK_EXPAND, 10, 10);

  makeGenericMenu(frame, data);


  frame = gtk_frame_new("Technology optimization");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 1, 1, 2, GTK_EXPAND| GTK_FILL, GTK_EXPAND, 10, 10);


  makeTechnMenu(frame, data);
  
  
  frame = gtk_frame_new("Multi level optimization");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(table), frame, 1, 2, 1, 2, GTK_EXPAND| GTK_FILL, GTK_EXPAND, 10, 10);


  makeMultiMenu(frame, data);


  frame = gtk_frame_new("2 level minimization");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 1, 2, 3, GTK_EXPAND| GTK_FILL, GTK_EXPAND, 10, 10);
  
  make2LevelMenu(frame, data);
  


  frame = gtk_frame_new("Delay path construction");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(table), frame, 1, 2, 2, 3, GTK_EXPAND| GTK_FILL, GTK_EXPAND, 10, 10);
  
  makeDlyPath(frame, data);
 
  frameBig = gtk_frame_new("Sequential circuits only parameters");
  gtk_box_pack_start(GTK_BOX(vbox), frameBig, FALSE, FALSE, 10);
  gtk_container_set_border_width(GTK_CONTAINER(frameBig), 0);

  table = gtk_table_new(2,2,TRUE);
  gtk_container_add(GTK_CONTAINER(frameBig), table);
 
  
    
  frame = gtk_frame_new("State machine type");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 10);
  
  makeStateMenu(frame, data);
  
  
  frame = gtk_frame_new("Reset type");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(table), frame, 1, 2, 0, 1, GTK_EXPAND| GTK_FILL, GTK_EXPAND, 10, 10);

  makeResetMenu(frame, data);

  frame = gtk_frame_new("Delay correction");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(table), frame, 0, 1, 1, 2, GTK_EXPAND| GTK_FILL, GTK_EXPAND, 10, 10);


  makeDelayCorrMenu(frame, data);

  frame = gtk_frame_new("Use output as feedback line");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_table_attach(GTK_TABLE(table), frame, 1, 2, 1, 2, GTK_EXPAND| GTK_FILL, GTK_EXPAND, 10, 10);


  makeOutFBMenu(frame, data);

  
/*
 *   frameBig = gtk_frame_new("DGD files only parameters");
 *   gtk_box_pack_start(GTK_BOX(vbox), frameBig, FALSE, FALSE, 10);
 *   gtk_container_set_border_width(GTK_CONTAINER(frameBig), 15);
 */

  return vbox;
}


void makeStateMenu(GtkWidget *frame, gpointer data)
{
  GtkWidget *menu, *item, *optionmenu;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));


  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("synchronous");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthSyncClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  item = gtk_menu_item_new_with_label("asynchronous");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthAsyncClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  if(udata->is_async == GNC_HL_OPT_CLOCK)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);
  
  
  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_container_add(GTK_CONTAINER(frame), optionmenu);



}


void makeResetMenu(GtkWidget *frame, gpointer data)
{
  GtkWidget *menu, *item, *optionmenu;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("low active");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthResetLowClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  item = gtk_menu_item_new_with_label("high active");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthResetHighClb), data);
  gtk_menu_append(GTK_MENU(menu), item);
  
  if(udata->reset_type == GNC_HL_OPT_CLR_HIGH)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);
 

  item = gtk_menu_item_new_with_label("no reset");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNoResetClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  if(udata->reset_type == GNC_HL_OPT_NO_RESET)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);

  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_container_add(GTK_CONTAINER(frame), optionmenu);

}


void makeNetMenu(GtkWidget *frame, gpointer data)
{
  GtkWidget *menu, *item, *optionmenu;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("enabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNecaClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  item = gtk_menu_item_new_with_label("disabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNoNecaClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  if(udata->t_is_neca == 0)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);
  
  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_container_add(GTK_CONTAINER(frame), optionmenu);

}

void makeGenericMenu(GtkWidget *frame, gpointer data)
{
  GtkWidget *menu, *item, *optionmenu;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("enabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthGenericClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  item = gtk_menu_item_new_with_label("disabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNoGenericClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  if(udata->t_is_generic == 0)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);

  
  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_container_add(GTK_CONTAINER(frame), optionmenu);

}

void makeTechnMenu(GtkWidget *frame, gpointer data)
{
  GtkWidget *menu, *item, *optionmenu;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("enabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthTechnClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  item = gtk_menu_item_new_with_label("disabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNoTechnClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  if(udata->t_is_library == 0)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);
  
  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_container_add(GTK_CONTAINER(frame), optionmenu);

}

void makeMultiMenu(GtkWidget *frame, gpointer data)
{
  GtkWidget *menu, *item, *optionmenu;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("enabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthMLevelClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  item = gtk_menu_item_new_with_label("disabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNoMLevelClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  if(udata->t_is_multi_level == 0)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);
  
  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_container_add(GTK_CONTAINER(frame), optionmenu);

}

void make2LevelMenu(GtkWidget *frame, gpointer data)
{
  GtkWidget *menu, *item, *optionmenu;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("enabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synth2LevMinClb), data);
  /* gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthDelayClb), data); */
  gtk_menu_append(GTK_MENU(menu), item);

  item = gtk_menu_item_new_with_label("disabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNo2LevMinClb), data);
  /* gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNoDelayClb), data); */
  gtk_menu_append(GTK_MENU(menu), item);


  if(udata->t_is_2l_min == 0)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);

  
  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_container_add(GTK_CONTAINER(frame), optionmenu);

}

void makeDlyPath(GtkWidget *frame, gpointer data)
{
  GtkWidget *menu, *item, *optionmenu;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("enabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthDlyPathClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  item = gtk_menu_item_new_with_label("disabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNoDlyPathClb), data);
  gtk_menu_append(GTK_MENU(menu), item);


  if(udata->t_is_dly_path == 0)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);

  
  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_container_add(GTK_CONTAINER(frame), optionmenu);

}

void makeDelayCorrMenu(GtkWidget *frame, gpointer data)
{
  GtkWidget *menu, *item, *optionmenu;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("enabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthDelayClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  item = gtk_menu_item_new_with_label("disabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNoDelayClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  if(udata->t_is_delay == 0)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);
  
  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_container_add(GTK_CONTAINER(frame), optionmenu);

}

void makeOutFBMenu(GtkWidget *frame, gpointer data)
{
  GtkWidget *menu, *item, *optionmenu;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label("enabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthOutFBClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  item = gtk_menu_item_new_with_label("disabled");
  gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(synthNoOutFBClb), data);
  gtk_menu_append(GTK_MENU(menu), item);

  if(udata->t_is_fbo == 0)
    gtk_menu_reorder_child(GTK_MENU(menu), item, 0);
  
  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  gtk_container_add(GTK_CONTAINER(frame), optionmenu);

}

void makeRadioBut(GtkWidget *vbox, gpointer data)
{
  GtkWidget *button;
  GSList * group;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));


  button = gtk_radio_button_new_with_label (NULL, "Fan in");
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
    GTK_SIGNAL_FUNC (encodeFanInClb), data);
  if(udata->state_encoding == FSM_ENCODE_FAN_IN)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (button), TRUE);
 

  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, FALSE, 5);

  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  button = gtk_radio_button_new_with_label(group, "Input constraints (all)");
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
    GTK_SIGNAL_FUNC (encodeIcAllClb), data);
  if(udata->state_encoding == FSM_ENCODE_IC_ALL)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (button), TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, FALSE, 5);

  button = gtk_radio_button_new_with_label(
           gtk_radio_button_group (GTK_RADIO_BUTTON (button)),
           "Input constraints (partially)");
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
    GTK_SIGNAL_FUNC (encodeIcPartClb), data);
  if(udata->state_encoding == FSM_ENCODE_IC_PART)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (button), TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, FALSE, 5);

  button = gtk_radio_button_new_with_label(
           gtk_radio_button_group (GTK_RADIO_BUTTON (button)),
           "Simple");
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
    GTK_SIGNAL_FUNC (encodeSimpleClb), data);
  if(udata->state_encoding == FSM_ENCODE_SIMPLE)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (button), TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), button, TRUE, FALSE, 5);

}

void makeEntries(GtkWidget *vbox, gpointer data)
{
  GtkWidget *text,  *label, *table;
  GtkWidget *frame;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  frame = gtk_frame_new("General parameters: ");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 10);
  

  table = gtk_table_new(3, 2, TRUE);
  gtk_container_add(GTK_CONTAINER(frame), table);

  label = gtk_label_new("Cell name (EDIF): ");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 0);

  
  label = gtk_label_new("Entity name (VHDL): ");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2 , GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 0);

  text = gtk_entry_new();
  udata->text_cn = text;
  gtk_entry_append_text(GTK_ENTRY(text), udata->cell_name);
  gtk_table_attach(GTK_TABLE(table), text, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 0);
  
  label = gtk_label_new("Design name (XNF): ");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3 , GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 0);

  
  frame = gtk_frame_new("EDIF specific parameters: ");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 10);

  table = gtk_table_new(2, 2, TRUE);
  gtk_container_add(GTK_CONTAINER(frame), table);

  label = gtk_label_new("Target library name: ");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 10);

  text = gtk_entry_new();
  udata->text_tln = text;
  gtk_entry_append_text(GTK_ENTRY(text), udata->target_library_name);
  gtk_table_attach(GTK_TABLE(table), text, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 10);

  label = gtk_label_new("View name: ");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 10);

  text = gtk_entry_new();
  udata->text_vn = text;
  gtk_entry_append_text(GTK_ENTRY(text), udata->view_name);
  gtk_table_attach(GTK_TABLE(table), text, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 10);


  frame = gtk_frame_new("VHDL specific parameters: ");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 10);

  table = gtk_table_new(1, 2, TRUE);
  gtk_container_add(GTK_CONTAINER(frame), table);

  label = gtk_label_new("Architecture name: ");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 10);

  text = gtk_entry_new();
  udata->text_an = text;
  gtk_entry_append_text(GTK_ENTRY(text), udata->arch_name);
  gtk_table_attach(GTK_TABLE(table), text, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 10);


}

void makeEntries1(GtkWidget *vbox, gpointer data)
{
  GtkWidget *text,  *label, *table;
  GtkWidget *frame;
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  frame = gtk_frame_new("Rename the library: ");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 10);
  

  table = gtk_table_new(1, 2, TRUE);
  gtk_container_add(GTK_CONTAINER(frame), table);

  label = gtk_label_new("New cell library name: ");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 0);

  
  text = gtk_entry_new();
  udata->text_cln = text;
  gtk_entry_append_text(GTK_ENTRY(text), udata->cell_library_name);
  gtk_table_attach(GTK_TABLE(table), text, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND, 10, 0);
  
 
}

gint eventDeleteClb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  return FALSE;
}

gint eventDestroyClb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_main_quit();
  return 0;
}


/* the synthesis function */
int doAll(gnc nc, gpointer data)
{
  int cell_ref;
  int synth_option = 0;
  int hl_option = 0;
  
  struct user_data *udata;
  
  udata = (struct user_data *) gtk_object_get_user_data(GTK_OBJECT(data));

  if(strcmp(udata->file_name,"")==0)
  {
    gnome_ok_dialog("Load the file first\n");
    return 0;
  }
 
  if(strcmp(udata->import_lib_name, "") == 0)
  {
    gnome_ok_dialog("Load the import gate library first\n"); 
    return 0;
  }
  
  nc->log_level = udata->log_level;

  gnc_DisableStrListBBB(nc, udata->disable_cells);
  
  gnc_Log(nc, 6, "Reading library '%s'.", udata->import_lib_name);
  if ( gnc_ReadLibrary(nc, udata->import_lib_name, udata->cell_library_name) == NULL )
    return 0;

  gnc_Log(nc, 6, "Gate identification.");
  gnc_ApplyBBBs(nc, 0);
  
  if ( udata->t_is_neca )
    synth_option |= GNC_SYNTH_OPT_NECA;
  if ( udata->t_is_generic )
    synth_option |= GNC_SYNTH_OPT_GENERIC;
  if ( udata->t_is_library )
    synth_option |= GNC_SYNTH_OPT_LIBARY;
  if ( udata->t_is_multi_level )
    synth_option |= GNC_SYNTH_OPT_LEVELS;
  if ( udata->t_is_dly_path )
    synth_option |= GNC_SYNTH_OPT_DLYPATH;

  hl_option |= udata->reset_type;
  switch(udata->state_encoding)
  {
    case FSM_ENCODE_FAN_IN:
      hl_option |= GNC_HL_OPT_ENC_FAN_IN;
      break;
  
    case FSM_ENCODE_IC_ALL:
      hl_option |= GNC_HL_OPT_ENC_IC_ALL;
      break;
  
    case FSM_ENCODE_IC_PART:
      hl_option |= GNC_HL_OPT_ENC_IC_PART;
      break;
  
    case FSM_ENCODE_SIMPLE:
      hl_option |= GNC_HL_OPT_ENC_SIMPLE;
      break;
  }
  
  if ( udata->is_async == 0 )
    hl_option |= GNC_HL_OPT_CLOCK;
  if ( udata->t_is_2l_min )
    hl_option |= GNC_HL_OPT_MINIMIZE;
  
  if ( udata->t_is_flatten )
    hl_option |= GNC_HL_OPT_FLATTEN;

  if ( udata->t_is_delay == 0 )
    hl_option |= GNC_HL_OPT_NO_DELAY;

  if ( udata->t_is_fbo )
    hl_option |= GNC_HL_OPT_FBO;

  cell_ref = gnc_SynthByFile(nc, udata->file_name, udata->cell_name, udata->target_library_name, hl_option, synth_option);
  udata->cell_ref = cell_ref;

  if ( cell_ref < 0 )
  {
    gnc_Error(nc, "Synthesis failed.");
  }
  else
  {
    gnc_Log(nc, 6, "Synthesis done.");
  }
  return 1;
}

