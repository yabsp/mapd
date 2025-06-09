#include <gtk/gtk.h>
#include "controller/main_controller.h"

MainController* controller = NULL;

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
    (void)user_data;

    // Creates a new MainController
    controller = main_controller_new(app);
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
    // Creates a new application
    GtkApplication* app = gtk_application_new("com.unibas.mapd", G_APPLICATION_FLAGS_NONE);

    // Create a new GtkApplication instance
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

    // Runs the GTK main loop
    int status = g_application_run (G_APPLICATION (app), argc, argv);

    // Cleanup after controller has ended
    if (controller)
        main_controller_free(controller);

    g_object_unref (app);
    return status;
}