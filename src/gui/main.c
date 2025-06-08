#include <gtk/gtk.h>
#include "controller/main_controller.h"

/**
 * activate:
 * @app: a pointer to the GtkApplication instance.
 * @user_data: user data passed to the callback (unused here).
 *
 * Called when the application is activated.
 * Initializes the main controller, which sets up components
 * and connects the signals.
 */
static void activate(GtkApplication *app, gpointer user_data)
{
    MainController *controller = main_controller_new(app);
}

/**
 * main:
 * @argc: number of command-line arguments.
 * @argv: array of command-line arguments.
 *
 * Entry point of the application. Creates a new GtkApplication
 * instance, connects the "activate" signal, runs the GTK main loop, and
 * performs cleanup upon exit.
 *
 * Returns: exit status code of the application.
 */
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