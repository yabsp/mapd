#include "gui_controller.h"

// Callback function for Hello World Button
static void print_hello(GtkWidget *widget, gpointer data)
{
    g_print("Hello World\n");
}

// Signal callback to open website
static void on_logo_clicked(GtkWidget *widget, gpointer user_data)
{
    const gchar *url = "https://www.unibas.ch";
    GError *error = NULL;

    if (!g_app_info_launch_default_for_uri(url, NULL, &error))
    {
        g_warning("Failed to open URL: %s", error->message);
        g_error_free(error);
    }
}

// Connect all signal handlers and initialize dynamic parts of the GUI
void gui_connect_signals(GtkBuilder *builder)
{
    // Connect Hello World button signal
    GtkWidget *button = GTK_WIDGET(gtk_builder_get_object(builder, "hello_button"));
    if (button != NULL)
        g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);

    // Connect list box row activation signal
    GtkWidget *list_box = GTK_WIDGET(gtk_builder_get_object(builder, "app_list"));
    if (list_box != NULL)
        g_signal_connect(list_box, "row-activated", G_CALLBACK(on_app_selected), NULL);

    // Populate the app list
    if (list_box != NULL)
        populate_app_list(GTK_LIST_BOX(list_box));

    // Load the GtkButton
    GtkWidget *logo_button = GTK_WIDGET(gtk_builder_get_object(builder, "logo_button"));
    if (logo_button != NULL)
    {
        // Connect signal for click event
        g_signal_connect(logo_button, "clicked", G_CALLBACK(on_logo_clicked), NULL);
    }

    // Load and insert logo
    GtkWidget *logo_image = GTK_WIDGET(gtk_builder_get_object(builder, "logo_image"));
    if (logo_image != NULL)
    {
        gtk_image_set_from_file(GTK_IMAGE(logo_image), "resources/UniversitaetBaselLogoWeiss.svg");
        gtk_image_set_pixel_size(GTK_IMAGE(logo_image), 300);
    }
}