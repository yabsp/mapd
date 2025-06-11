#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include "model/app_model.h"
#include "view/main_view.h"
#include "message.h"
#include "fragmentation.h"
#include "analyzer/analyzer.h"


gboolean update_fragmentation_label(gpointer data);

/**
 * MainController:
 *
 * Represents the main controller in the MVC architecture. Coordinates the application model, view and the options
 */
typedef struct {
    AppModel *model;
    MainView *view;
    AnalyzerOptions* options;
} MainController;

extern MainController* global_main_controller;

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
    GtkSpinButton *small_thresh_spin;
    GtkSpinButton *large_thresh_spin;
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
