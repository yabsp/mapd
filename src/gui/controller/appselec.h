#ifndef APPSELEC_H
#define APPSELEC_H

#include <gtk/gtk.h>
#include <dirent.h>
#include <string.h>

typedef struct {
    gchar *name;
    gchar *exec;
} AppEntry;

// This function will populate your GtkListBox with installed apps
void populate_app_list(GtkListBox *list_box);

// This will be called when a user selects an application
void on_app_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data);

#endif //APPSELEC_H
