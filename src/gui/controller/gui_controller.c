#include "gui_controller.h"

GtkListBoxRow *selected_row = NULL;
GtkBuilder *global_builder = NULL;

// Logo: open website of Univesity Basel
// TODO DOES NOT WORK
void on_logo_clicked(GtkWidget *widget, gpointer user_data)
{
    const gchar *url = "https://www.unibas.ch";
    GError *error = NULL;

    if (!g_app_info_launch_default_for_uri(url, NULL, &error))
    {
        g_warning("Failed to open URL: %s", error->message);
        g_error_free(error);
    }
}

// Populate application list dynamically
void populate_app_list(GtkListBox *selector_box)
{
    const gchar *directory = "/home/schneuzi/Codespace/01_Projects/mapd/build/";
    DIR *dir = opendir(directory);
    struct dirent *entry;

    if (!dir)
    {
        g_warning("Failed to open %s", directory);
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (g_strcmp0(entry->d_name, ".") == 0 || g_strcmp0(entry->d_name, "..") == 0)
            continue;

        // Build full file path
        gchar *filepath = g_build_filename(directory, entry->d_name, NULL);

        // Check if path is an executable file
        if (g_file_test(filepath, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_EXECUTABLE))
        {
            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *label = gtk_label_new(entry->d_name);
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
            g_object_set_data(G_OBJECT(row), "exec_path", g_strdup(filepath));
            gtk_list_box_append(GTK_LIST_BOX(selector_box), row);
        }

        g_free(filepath);
    }
    closedir(dir);
}

// Append string to the end of the log
void append_analyzer_log(GtkBuilder *builder, const char *text)
{
    GtkWidget *textview = GTK_WIDGET(gtk_builder_get_object(builder, "analyzer_log"));
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    // Insert new text at the end
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    gtk_text_buffer_insert(buffer, &end_iter, text, -1);

    // Autoscroll to bottom
    GtkAdjustment *adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(textview));
    gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
}

// Store the execution path if a application is selected
void on_app_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    selected_row = row;
    gchar *exec_path = g_object_get_data(G_OBJECT(row), "exec_path");
    g_print("Selected application: %s\n", exec_path);
}


// launch_button: initiates launching of application and changes view if successful
void on_launch_button_clicked(GtkWidget *widget, gpointer user_data)
{
    GtkBuilder *builder = (GtkBuilder *)user_data;

    // Check if process is selected
    if (!selected_row)
    {
        g_warning("No process selected!");
        return;
    }

    // Parse command
    const gchar *exec_path = g_object_get_data(G_OBJECT(selected_row), "exec_path");
    GtkWidget *entry = GTK_WIDGET(gtk_builder_get_object(builder, "arg_entry"));
    const gchar *arguments = gtk_editable_get_text(GTK_EDITABLE(entry));

    // If launch sucessful change to process view
    if (launch_application(exec_path, arguments))
    {
        GtkWidget *stack = GTK_WIDGET(gtk_builder_get_object(builder, "main_stack"));
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "process_box");
    }
}


// Connect all signal handlers and initialize dynamic parts of the GUI
void gui_connect_signals(GtkBuilder *builder)
{
    // Connect selector_box row
    GtkWidget *selector_box = GTK_WIDGET(gtk_builder_get_object(builder, "selector_list"));
    if (selector_box != NULL)
    {
        g_signal_connect(selector_box, "row-activated", G_CALLBACK(on_app_selected), NULL);
        // Populate the app list
        populate_app_list(GTK_LIST_BOX(selector_box));
    }

    // Connect the logo_button
    // TODO match the button size with the logo
    GtkWidget *logo_button = GTK_WIDGET(gtk_builder_get_object(builder, "logo_button"));
    if (logo_button != NULL)
        g_signal_connect(logo_button, "clicked", G_CALLBACK(on_logo_clicked), NULL);

    // Conect the launch_button
    GtkWidget *launch_button = GTK_WIDGET(gtk_builder_get_object(builder, "launch_button"));
    if (launch_button != NULL)
        g_signal_connect(launch_button, "clicked", G_CALLBACK(on_launch_button_clicked), builder);

    // Load and insert logo_image
    GtkWidget *logo_image = GTK_WIDGET(gtk_builder_get_object(builder, "logo_image"));
    if (logo_image != NULL)
    {
        gtk_image_set_from_file(GTK_IMAGE(logo_image), "resources/UniversitaetBaselLogoWeiss.svg");
        gtk_image_set_pixel_size(GTK_IMAGE(logo_image), 300);
    }
}