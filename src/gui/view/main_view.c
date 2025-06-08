#include "main_view.h"

/**
 * main_view_get_arguments:
 * @view: a pointer to the MainView instance.
 *
 * Retrieves the current text from the arguments entry field.
 *
 * Returns: a constant string with the arguments text.
 */
const char* main_view_get_arguments(MainView* view)
{
    return gtk_editable_get_text(GTK_EDITABLE(view->args_entry));
}

/**
 * main_view_new:
 * @app: a pointer to the GtkApplication instance.
 *
 * Creates and initializes a new MainView instance. Loads the UI definition from the embedded resource .ui file and
 * retrieves all required widget references. Sets up the main window and associates it with the application instance.
 *
 * Returns: a newly allocated MainView instance.
 */
MainView* main_view_new(GtkApplication *app)
{
    MainView *view = g_malloc(sizeof(MainView));

    GtkBuilder *builder = gtk_builder_new_from_resource("/com/unibas/mapd/data/ui/mapd.ui");
    GtkWidget *image = gtk_picture_new_for_resource("/com/unibas/mapdtest/images/logo.svg");

    view->window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    view->select_app_button = GTK_WIDGET(gtk_builder_get_object(builder, "select_app_button"));
    view->selected_app_label = GTK_WIDGET(gtk_builder_get_object(builder, "selected_app_label"));
    view->args_label = GTK_WIDGET(gtk_builder_get_object(builder, "args_label"));
    view->args_entry = GTK_WIDGET(gtk_builder_get_object(builder, "args_entry"));
    view->launch_button = GTK_WIDGET(gtk_builder_get_object(builder, "launch_button"));
    view->client_grid = GTK_WIDGET(gtk_builder_get_object(builder, "client_grid"));
    view->logo_button = GTK_WIDGET(gtk_builder_get_object(builder, "logo_button"));
    view->client_grid = GTK_WIDGET(gtk_builder_get_object(builder, "client_grid"));
    view->log_text_view = GTK_WIDGET(gtk_builder_get_object(builder, "log_text_view"));

    g_assert(GTK_IS_GRID(view->client_grid));

    gtk_window_set_application(GTK_WINDOW(view->window), app);
    gtk_window_present(GTK_WINDOW(view->window));

    g_object_unref(builder);
    return view;
}

/**
 * main_view_free:
 * @view: a pointer to the MainView instance.
 *
 * Frees the memory allocated for the MainView structure. Does not destroy GTK Widgets.
 */
void main_view_free(MainView *view)
{
    g_free(view);
}
