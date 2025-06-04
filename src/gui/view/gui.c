#include "gui.h"

// Build GUI from interface definition .ui in resources
void build_gui(GtkApplication *app)
{
    GtkBuilder *builder;
    GtkWidget *window;

    // Load UI file
    builder = gtk_builder_new_from_file("resources/mapdgui.ui");

    // Get main window and set application for the window
    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    gtk_window_set_application(GTK_WINDOW(window), app);

    // Pass to Controller for logic
    gui_connect_signals(builder);

    // Present window
    gtk_window_present(GTK_WINDOW(window));

    // Cleanup
    g_object_unref(builder);
}