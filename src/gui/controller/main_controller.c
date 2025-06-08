#include "main_controller.h"

static void on_logo_button_clicked(GtkButton *button, gpointer user_data)
{
    const char *url = "https://www.unibas.ch";
    GError *error = NULL;

    if (!g_app_info_launch_default_for_uri(url, NULL, &error)) {
        g_printerr("Failed to open URL: %s\n", error->message);
        g_clear_error(&error);
    }
}

/**
 * on_select_app_clicked:
 * @button: the select application button.
 * @user_data: pointer to the MainController instance.
 *
 * Handels user clicks on "Select Application" button.
 * Opens a native file chooser dialog to allow the user to select an executable file.
 */
static void on_select_app_clicked(GtkButton *button, gpointer user_data)
{
    MainController *controller = user_data;

    // Create file chooser dynamically
    GtkFileChooserNative *file_chooser = gtk_file_chooser_native_new(
        "Select Application",
        GTK_WINDOW(controller->view->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Open",
        "_Cancel"
    );

    // Connect return signal
    g_signal_connect(file_chooser, "response", G_CALLBACK(on_file_dialog_response), controller);

    // Display dialog
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(file_chooser));
}

/**
 * on_file_dialog_response:
 * @dialog: the file chooser dialog.
 * @response: the response code indicating user's action.
 * @user_data: pointer to the MainController instance.
 *
 * Handles the file selection dialog response. If a file was selected,
 * updates the model and updates the view to display the selected application.
 */
static void on_file_dialog_response(GtkNativeDialog *dialog, int response, gpointer user_data)
{
    MainController *controller = user_data;

    // Saves the selected file if a file was chosen in the dialog
    if (response == GTK_RESPONSE_ACCEPT)
    {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        GFile *file = gtk_file_chooser_get_file(chooser);

        if (file) {
            char *file_path = g_file_get_path(file);
            app_model_set_selected_app(controller->model, file_path);
            gtk_label_set_text(GTK_LABEL(controller->view->selected_app_label),
                               app_model_get_file_name(controller->model));
            g_free(file_path);
            g_object_unref(file);
        }
    }
    // Clean
    g_object_unref(dialog);
}

/**
 * launch_client_with_memwrap:
 * @file_path: path to the executable to launch.
 * @args_text: additional command-line arguments.
 *
 * Launches the client application as a subprocess using memwrap via LD_PRELOAD.
 * Builds the argument vector and spawns the process with appropriate environment variables.
 *
 * Returns: a GSubprocess instance representing the launched client or NULL on failure.
 *
 * TODO use the socket and not standalone
 */
static GSubprocess* launch_client_with_memwrap(const char *file_path, const char *args_text)
{
    GPtrArray *argv_array = g_ptr_array_new();
    g_ptr_array_add(argv_array, g_strdup(file_path));

    // Split additional arguments into argv array
    if (args_text && *args_text) {
        gchar **args_split = g_strsplit(args_text, " ", -1);
        for (gchar **arg = args_split; *arg != NULL; arg++)
            g_ptr_array_add(argv_array, g_strdup(*arg));
        g_strfreev(args_split);
    }
    g_ptr_array_add(argv_array, NULL);

    // Locate memwrap library in the application directory
    char *exe_path = g_file_read_link("/proc/self/exe", NULL);
    gchar *exe_dir = g_path_get_dirname(exe_path);
    gchar *memwrap_path = g_build_filename(exe_dir, "libmem_wrap.so", NULL);

    GError *error = NULL;
    GSubprocessLauncher *launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_NONE);
    g_subprocess_launcher_setenv(launcher, "LD_PRELOAD", memwrap_path, TRUE);

    // Launch the subprocess
    GSubprocess *subprocess = g_subprocess_launcher_spawnv(
        launcher,
        (const gchar * const *) argv_array->pdata,
        &error
    );
    g_object_unref(launcher);

    if (!subprocess) {
        g_printerr("Failed to launch client: %s\n", error->message);
        g_clear_error(&error);
    }

    // Clean
    g_ptr_array_free(argv_array, TRUE);
    g_free(exe_path);
    g_free(exe_dir);
    g_free(memwrap_path);

    return subprocess;
}

/**
 * on_launch_clicked:
 * @button: the launch button.
 * @user_data: pointer to the MainController instance.
 *
 * Handles user clicks on "Launch" button.
 * Launches the selected application as a subprocess and adds it to the client list.
 */
static void on_launch_clicked(GtkButton *button, gpointer user_data)
{
    MainController *controller = user_data;

    const char *file_path = app_model_get_file_path(controller->model);
    const char *args_text = main_view_get_arguments(controller->view);

    // Handles if file path not valid
    if (!file_path || file_path[0] == '\0') {
        g_printerr("No application selected!\n");
        return;
    }

    // Handles if too many clients
    if (controller->clients->len >= MAX_CLIENTS) {
        g_printerr("Maximum number of concurrent clients reached.\n");
        return;
    }

    // Adds client to the grid if subprocess is successful
    GSubprocess *sub = launch_client_with_memwrap(file_path, args_text);
    if (sub) {
        Client *client = g_malloc(sizeof(Client));
        client->file_path = g_strdup(file_path);

        char *basename = g_path_get_basename(file_path);
        client->file_name = g_strdup(basename);
        g_free(basename);

        client->subprocess = sub;
        client->client_id = controller->clients->len + 1;
        client->controller = controller;

        g_ptr_array_add(controller->clients, client);
        add_client_to_grid(controller, client);
    }
}

/**
 * add_client_to_grid:
 * @controller: pointer to the MainController instance.
 * @client: pointer to the Client instance.
 *
 * Adds a newly launched client to the GUI grid, showing its ID, file name,
 * and providing a button to kill the subprocess.
 */
static void add_client_to_grid(MainController *controller, Client *client)
{
    int row = controller->clients->len - 1;

    gchar *client_id_text = g_strdup_printf("Client %d", client->client_id);
    client->client_id_label = gtk_label_new(client_id_text);
    g_free(client_id_text);

    client->file_name_label = gtk_label_new(client->file_name);
    client->kill_button = gtk_button_new_with_label("Kill");

    gtk_grid_attach(GTK_GRID(controller->view->client_grid), client->client_id_label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(controller->view->client_grid), client->file_name_label, 1, row, 1, 1);
    gtk_grid_attach(GTK_GRID(controller->view->client_grid), client->kill_button, 2, row, 1, 1);

    g_signal_connect_swapped(client->kill_button, "clicked", G_CALLBACK(on_kill_client), client);

    gtk_widget_show(client->client_id_label);
    gtk_widget_show(client->file_name_label);
    gtk_widget_show(client->kill_button);
}

/**
 * on_kill_client:
 * @client: pointer to the Client instance to terminate.
 *
 * Terminates the subprocess associated with the client and removes its UI elements from the grid.
 */
static void on_kill_client(Client *client)
{
    if (client->subprocess) {
        g_subprocess_force_exit(client->subprocess);
        g_subprocess_wait_check(client->subprocess, NULL, NULL);
        g_object_unref(client->subprocess);
        client->subprocess = NULL;
    }

    gtk_widget_unparent(client->client_id_label);
    gtk_widget_unparent(client->file_name_label);
    gtk_widget_unparent(client->kill_button);

    // Remove client from array
    g_ptr_array_remove(client->controller->clients, client);
}

static gboolean update_gui_from_message(gpointer user_data)
{
    GuiUpdateData* data = (GuiUpdateData*)user_data;
    Message* msg = data->message;
    MainController* controller = data->controller;

    // Build the log string from Message
    gchar *log_line = g_strdup_printf(
        "Client %d | %s | Addr: %s | Size: %zu | Thread: %lu | Time: %ld\n",
        msg->client_id, msg->type, msg->addr, msg->size, msg->thread, msg->timestamp
    );

    // Append log line to TextView
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(controller->view->log_text_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);

    // Insert text
    gtk_text_buffer_insert(buffer, &end, log_line, -1);
    g_free(log_line);

    // Re-fetch end iter after insert to scroll to end
    gtk_text_buffer_get_end_iter(buffer, &end);
    GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(controller->view->log_text_view), mark);
    gtk_text_buffer_delete_mark(buffer, mark);

    message_free(msg);
    g_free(data);
    return G_SOURCE_REMOVE;
}

static void* analyzer_consumer_thread(void* arg)
{
    MainController *controller = (MainController*)arg;

    while (1) {
        Message msg = dequeue_message();

        GuiUpdateData* data = g_malloc(sizeof(GuiUpdateData));
        data->controller = controller;
        data->message = message_copy(&msg);

        g_idle_add(update_gui_from_message, data);
    }
    return NULL;
}

/**
 * main_controller_new:
 * @app: pointer to the GtkApplication instance.
 *
 * Creates and initializes a new MainController instance.
 * Sets up the model, view, and connects the necessary signal handlers.
 *
 * Returns: pointer to the newly allocated MainController instance.
 */
MainController* main_controller_new(GtkApplication *app)
{
    MainController *controller = g_malloc(sizeof(MainController));
    controller->model = app_model_new();
    controller->view = main_view_new(app);
    controller->clients = g_ptr_array_new_with_free_func(g_free);

    // Connect the signals
    g_signal_connect(controller->view->logo_button, "clicked", G_CALLBACK(on_logo_button_clicked), NULL);
    g_signal_connect(controller->view->select_app_button, "clicked", G_CALLBACK(on_select_app_clicked), controller);
    g_signal_connect(controller->view->launch_button, "clicked", G_CALLBACK(on_launch_clicked), controller);

    // Start consumer thread for messages
    pthread_t consumer_thread;
    pthread_create(&consumer_thread, NULL, analyzer_consumer_thread, controller);
    pthread_detach(consumer_thread);

    return controller;
}

/**
 * main_controller_free:
 * @controller: pointer to the MainController instance.
 *
 * Frees all resources associated with the MainController, including the model, view,
 * client list, and the controller instance itself.
 */
void main_controller_free(MainController *controller)
{
    app_model_free(controller->model);
    main_view_free(controller->view);
    g_ptr_array_free(controller->clients, TRUE);
    g_free(controller);
}
