#include <gtk/gtk.h>
#include "controller/main_controller.h"
#include "analyzer.h"

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
    (void)user_data;
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
    GtkApplication* app = gtk_application_new("com.unibas.mapd", G_APPLICATION_FLAGS_NONE);

    // Initiate the analyzer
    analyzer_init();

    // Create a new GtkApplication instance
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

    // Run the GTK main loop
    int status = g_application_run (G_APPLICATION (app), argc, argv);

    // Cleanup
    g_object_unref (app);
    return status;
}