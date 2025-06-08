#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H

#include <gtk/gtk.h>

/**
 * MainView:
 *
 * Represents the main app view and its UI components. Holds references to all the relevant widgets in the main
 * window, allowing the controller to interact with the user interface.
 */
typedef struct {
    GtkWidget *window;
    GtkWidget *select_app_label;
    GtkWidget *select_app_button;
    GtkWidget *selected_app_label;
    GtkWidget *chooser_dialog;
    GtkWidget *args_entry;
    GtkWidget *args_label;
    GtkWidget *launch_button;
    GtkWidget *client_grid;
    GtkWidget *logo_button;
    GtkWidget *log_text_view;
} MainView;

/**
 * main_view_get_arguments:
 *
 * Retrieves arguments entered by the user in the arguments entry field.
 *
 * @param view: Pointer to the MainView instance.
 * @return A constant string containing the entered arguments (owned by GTK, do not free).
 */
const char* main_view_get_arguments(MainView* view);

/**
 * main_view_new:
 *
 * Creates and initializes the main application view by loading the UI definition from the embedded resource file.
 *
 * @param app: Pointer to the GtkApplication instance.
 * @return A pointer to a newly allocated MainView instance.
 */
MainView* main_view_new(GtkApplication *app);

/**
 * main_view_free:
 *
 * Frees the memory allocated for the MainView structure.
 *
 * @param view: Pointer to the MainView instance to free.
 */
void main_view_free(MainView *view);

#endif //MAIN_VIEW_H
