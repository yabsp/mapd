#include "main_controller.h"

/**
 * on_logo_button_clicked:
 *
 * Handles user clicks on Logo. Opens the University of Basel Website
 * @param button Unused
 * @param user_data Unused
 */
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
 * on_file_dialog_response:
 *
 * Handles the file selection dialog response. If a file was selected, updates the model and updates the view to
 * display the selected application.
 *
 * @param dialog file chooser dialog
 * @param response response code indicating user's action
 * @param user_data pointer to the MainController instance
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
 * on_select_app_clicked:
 *
 * Handles user clicks on "Select Application" button.
 * Opens a native file chooser dialog to allow the user to select an executable file.
 *
 * @param button Unused
 * @param user_data Pointer to the MainController
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
 * launch_client_with_memwrap:
 *
 * Launches the client application as a subprocess using memwrap via LD_PRELOAD. Builds the argument vector and
 * spawns the process with appropriate environment variables.
 *
 * @param file_path Path to the executable to launch
 * @param args_text Additional command-line arguments
 * @return GSubprocess instance representing the launched client or NULL on failure
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
 * on_kill_client:
 *
 * Terminates the subprocess associated with the client and removes its UI elements from the grid.
 *
 * @param client Pointer to the Client instance to terminate
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

/**
 * add_client_to_grid:
 *
 * Adds a newly launched client to the GUI, showing its ID, file name, and providing a button to kill the subprocess.
 *
 * @param controller Pointer to the MainController
 * @param client Pointer to the Client
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
 * on_launch_clicked:
 *
 * Handles user clicks on "Launch" button.
 * Launches the selected application as a subprocess and adds it to the client list.
 *
 * @param button Unused
 * @param user_data Pointer to the MainController
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
 * update_gui_from_message:
 *
 * Updates the GUI log window with a new message. Scheduled via g_idle_add() from the analyzer consumer thread,
 * ensuring all GTK operations are performed on the main thread.
 *
 * @param user_data: Pointer to GuiUpdateData containing the controller and message.
 * @return: G_SOURCE_REMOVE to remove the idle function after execution.
 */
static gboolean update_gui_from_message(gpointer user_data)
{
    GuiUpdateData* data = (GuiUpdateData*)user_data;
    Message* msg = data->message;
    MainController* controller = data->controller;

    char time_buf[32];
    struct tm *tm_info = localtime(&msg->timestamp);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    // Build the log string from Message
    gchar *log_line = g_strdup_printf(
        "Client %d | %s | Addr: %s | Size: %zu | Thread: %lu | Time: %s\n",
        msg->client_id, msg->type, msg->addr, msg->size, msg->thread, time_buf);

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

/**
 * analyzer_consumer_thread:
 *
 * Dequeues messages from the analyzer's message queue. Forwards messages to the GTK main thread via g_idle_add() for
 * safe GUI updates.
 *
 * @param arg Pointer to MainController
 * @return NULL when thread exits
 */
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
 * on_options_dialog_response:
 *
 * Callback for the response signal of the options dialog.
 * If user accepts (OK), updates analyzer options based on dialog input fields.
 *
 * @param dialog: Pointer to Options dialog
 * @param response_id: Pointer to dialog response ID (OK, Cancel, etc.)
 * @param user_data: Pointer to OptionsDialogData containing widgets and controller
 */
static void on_options_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    OptionsDialogData *data = user_data;

    if (response_id == GTK_RESPONSE_OK)
    {
        double min_threshold = gtk_spin_button_get_value(data->min_thresh_spin);
        double max_threshold = gtk_spin_button_get_value(data->max_thresh_spin);
        gboolean info_log = gtk_switch_get_active(data->info_log_switch);

        data->controller->options->small_threshold = (int)min_threshold;
        data->controller->options->large_threshold = (int)max_threshold;
        data->controller->options->info_logs_enabled = info_log;

        g_print("Threshold 1: %.2f\n", min_threshold);
        g_print("Threshold 2: %.2f\n", max_threshold);
        g_print("Info Logs: %s\n", info_log ? "enabled" : "disabled");
    }

    g_free(data);
    gtk_window_destroy(GTK_WINDOW(dialog));
}


/**
 * on_options_button_clicked:
 *
 * Handles user clicks on "Options" button. Launches the options dialog and adds buttons in it.
 *
 * @param button Unused
 * @param user_data Pointer to MainController
 */
static void on_options_button_clicked(GtkButton *button, gpointer user_data)
{
    MainController *controller = user_data;

    // Dynamically create the dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Options",
        GTK_WINDOW(controller->view->window),
        GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_top(grid, 12);
    gtk_widget_set_margin_bottom(grid, 12);
    gtk_widget_set_margin_start(grid, 12);
    gtk_widget_set_margin_end(grid, 12);
    gtk_box_append(GTK_BOX(content_area), grid);

    // Minimum threshold
    GtkWidget *max_label = gtk_label_new("Maximum Threshold");
    gtk_widget_set_halign(max_label, GTK_ALIGN_START);
    GtkWidget *max_spin = gtk_spin_button_new_with_range(0, 1000000, 50);
    gtk_grid_attach(GTK_GRID(grid), max_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), max_spin, 1, 0, 1, 1);

    // Maximum threshold
    GtkWidget *min_label = gtk_label_new("Minimum Threshold");
    gtk_widget_set_halign(min_label, GTK_ALIGN_START);
    GtkWidget *min_spin = gtk_spin_button_new_with_range(0, 1000000, 50);
    gtk_grid_attach(GTK_GRID(grid), min_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), min_spin, 1, 1, 1, 1);

    // Logs
    GtkWidget *log_label = gtk_label_new("System Logs");
    gtk_widget_set_halign(log_label, GTK_ALIGN_START);
    GtkWidget *log_switch = gtk_switch_new();
    gtk_grid_attach(GTK_GRID(grid), log_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), log_switch, 1, 2, 1, 1);
    gtk_widget_set_halign(log_switch, GTK_ALIGN_END);

    // Set initial values from controller options
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(min_spin), controller->options->small_threshold);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(max_spin), controller->options->large_threshold);
    gtk_switch_set_active(GTK_SWITCH(log_switch), controller->options->info_logs_enabled);

    OptionsDialogData *data = g_malloc(sizeof(OptionsDialogData));
    data->min_thresh_spin = GTK_SPIN_BUTTON(min_spin);
    data->max_thresh_spin = GTK_SPIN_BUTTON(max_spin);
    data->info_log_switch = GTK_SWITCH(log_switch);
    data->controller = controller;

    g_signal_connect(dialog, "response", G_CALLBACK(on_options_dialog_response), data);

    gtk_widget_show(dialog);
}

/**
 * main_controller_new:
 *
 * Creates and initializes a new MainController instance. Sets up the model, view, and connects the signal handlers.
 *
 * @param app Pointer to the GtkApplication
 * @return Pointer to the newly allocated MainController
 */
MainController* main_controller_new(GtkApplication *app)
{
    // Allocates MainController and stars model and view
    MainController *controller = g_malloc(sizeof(MainController));
    controller->model = app_model_new();
    controller->view = main_view_new(app);
    controller->clients = g_ptr_array_new_with_free_func(g_free);

    // Sets default for options
    controller->options = g_malloc(sizeof(AnalyzerOptions));
    controller->options->small_threshold = 50000;
    controller->options->large_threshold = 5000;
    controller->options->info_logs_enabled = TRUE;

    // Start analyzer with options
    analyzer_init(controller->options);

    // Connects the signals
    g_signal_connect(controller->view->logo_button, "clicked", G_CALLBACK(on_logo_button_clicked), NULL);
    g_signal_connect(controller->view->select_app_button, "clicked", G_CALLBACK(on_select_app_clicked), controller);
    g_signal_connect(controller->view->launch_button, "clicked", G_CALLBACK(on_launch_clicked), controller);
    g_signal_connect(controller->view->options_button, "clicked", G_CALLBACK(on_options_button_clicked), controller);

    // Starts consumer thread for messages
    pthread_t consumer_thread;
    pthread_create(&consumer_thread, NULL, analyzer_consumer_thread, controller);
    pthread_detach(consumer_thread);

    return controller;
}

/**
 * main_controller_free:
 *
 * Frees all resources associated with the MainController, including the model, view, client list, and the controller.
 *
 * @param controller Pointer to the MainController
 */
void main_controller_free(MainController *controller)
{
    app_model_free(controller->model);
    main_view_free(controller->view);
    g_ptr_array_free(controller->clients, TRUE);
    g_free(controller);
}