#ifndef GUI_CONTROLLER_H
#define GUI_CONTROLLER_H

#include "launcher.h"
#include <glib.h>
#include <dirent.h>
#include <string.h>
#include <gtk/gtk.h>

// Store what row is currently selected
extern GtkListBoxRow *selected_row;

// Logo button click handler
void on_logo_clicked(GtkWidget *widget, gpointer user_data);

// Populate application list dynamically
void populate_app_list(GtkListBox *selector_box);

// Append string handler
void append_analyzer_log(GtkBuilder *builder, const char *text);

// Launch button click handler
void on_launch_button_clicked(GtkWidget *widget, gpointer user_data);

// Kill button click handler
void on_kill_button_clicked(GtkWidget *widget, gpointer user_data);

// Connect all signals and initialize controller logic
void gui_connect_signals(GtkBuilder *builder);

// Signal handler when user selects an application
void on_app_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);

#endif //GUI_CONTROLLER_H
