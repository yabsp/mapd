#include <gtk/gtk.h>
#include "view/gui.h"

// Forward declaration of activate function
static void activate(GtkApplication *app, gpointer user_data);

// Entry point of the application
int main (int argc, char **argv)
{
    GtkApplication *app;
    int status;

    // Create a new GtkApplication instance
    app = gtk_application_new ("org.unibas.mapd", 0);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

    // Run the GTK main loop
    status = g_application_run (G_APPLICATION (app), argc, argv);

    // Cleanup
    g_object_unref (app);

    return status;
}

// Initialize the GUI and application state
static void activate(GtkApplication *app, gpointer user_data)
{
    // Build and display the application GUI
    build_gui(app);
}