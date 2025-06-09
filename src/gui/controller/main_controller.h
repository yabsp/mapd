#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include "model/app_model.h"
#include "view/main_view.h"
#include "message.h"
#include "fragmentation.h"
#include "analyzer/analyzer.h"

// Maximum number of concurrent clients supported
#define MAX_CLIENTS 5

/**
 * MainController:
 *
 * Represents the main controller in the MVC architecture. Coordinates the application model, view, the options and
 * the list of running clients.
 */
typedef struct {
    AppModel *model;
    MainView *view;
    AnalyzerOptions* options;
    GPtrArray *clients;
} MainController;

/**
 * Client:
 *
 * Represents a launched client subprocess. Stores information about the subprocess, its file path, ID, and the
 * associated UI elements for displaying and controlling it.
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

/**
 * GuiUpdateData:
 *
 * Represents the data needed to update the GUI
 */
typedef struct {
    MainController* controller;
    Message* message;
} GuiUpdateData;

/**
 * OptionsDialogData:
 *
 * Holds widget pointers and controller to pass to the options dialog response callback.
 */
typedef struct {
    GtkSpinButton *min_thresh_spin;
    GtkSpinButton *max_thresh_spin;
    GtkSwitch *info_log_switch;
    MainController *controller;
} OptionsDialogData;

/**
 * main_controller_new:
 *
 * Creates and initializes a new MainController instance. Sets up the model, view, and connects signal handlers.
 *
 * @param app The GtkApplication instance
 * @return Pointer to the newly allocated MainController
 */
MainController* main_controller_new(GtkApplication *app);

/**
 * main_controller_free:
 *
 * Frees all resources associated with the MainController instance, model, view and client list.
 *
 * @param controller The MainController instance to free
 */
void main_controller_free(MainController *controller);

#endif //MAIN_CONTROLLER_H
