#include <gtk/gtk.h>

static void
print_hello (GtkWidget *widget,
             gpointer   data)
{
    g_print ("Hello World\n");
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
    GtkBuilder *builder;
    GtkWidget *window;
    GtkWidget *button;

    // Load UI file
    builder = gtk_builder_new_from_file("../src/gui/mapdgui.ui");

    // Get main window by ID
    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));

    // Set the application for the window
    gtk_window_set_application(GTK_WINDOW(window), app);

    // Get button by ID
    button = GTK_WIDGET(gtk_builder_get_object(builder, "hello_button"));

    // Load image
    GtkWidget *image = gtk_image_new_from_file("UniversitaetBaselLogoWeiss.svg");

    // Connect signal to button
    g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);

    // Show window
    gtk_window_present(GTK_WINDOW(window));

    // Cleanup builder after we're done
    g_object_unref(builder);
}

int
main (int    argc,
      char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new ("org.gtk.example", 0);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}