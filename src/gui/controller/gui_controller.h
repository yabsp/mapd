#ifndef GUI_CONTROLLER_H
#define GUI_CONTROLLER_H

#include <gtk/gtk.h>

// Connect all signals and initialize controller logic
void gui_connect_signals(GtkBuilder *builder);

// Application list logic
void populate_app_list(GtkListBox *list_box);

// Signal handler for app list selection
void on_app_selected(GtkListBox *list_box, GtkListBoxRow *row, gpointer user_data);


#endif //GUI_CONTROLLER_H
