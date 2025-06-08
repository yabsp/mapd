#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include "model/app_model.h"
#include "view/main_view.h"
#include "message.h"

// Maximum number of concurrent clients supported
#define MAX_CLIENTS 5

/**
 * MainController:
 *
 * Represents the main controller in the MVC architecture.
 * Coordinates the application model, view, and the list of running clients.
 */
typedef struct {
    AppModel *model;
    MainView *view;
    GPtrArray *clients;
} MainController;

/**
 * Client:
 *
 * Represents a launched client subprocess.
 * Stores information about the subprocess, its file path, ID,
 * and the associated UI elements for displaying and controlling it.
 */
typedef struct {
    char *file_path;
    char *file_name;
    GSubprocess *subprocess;
    int client_id;

    GtkWidget *client_id_label;
    GtkWidget *file_name_label;
    GtkWidget *kill_button;

    MainController *controller;
} Client;

typedef struct {
    MainController* controller;
    Message* message;
} GuiUpdateData;

/**
 * main_controller_new:
 *
 * Creates and initializes a new MainController instance.
 * Sets up the model, view, and connects signal handlers.
 *
 * @param app: The GtkApplication instance.
 * @return Pointer to the newly allocated MainController.
 */
MainController* main_controller_new(GtkApplication *app);

/**
 * main_controller_free:
 *
 * Frees all resources associated with the MainController instance,
 * including the model, view, client list, and the controller itself.
 *
 * @param controller: The MainController instance to free.
 */
void main_controller_free(MainController *controller);

// Internal function declarations
static void on_select_app_clicked(GtkButton *button, gpointer user_data);
static void on_file_dialog_response(GtkNativeDialog *dialog, int response, gpointer user_data);
static GSubprocess* launch_client_with_memwrap(const char *file_path, const char *args_text);
static void on_launch_clicked(GtkButton *button, gpointer user_data);
static void add_client_to_grid(MainController *controller, Client *client);
static void on_kill_client(Client *client);

#endif //MAIN_CONTROLLER_H
