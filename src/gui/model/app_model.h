#ifndef APP_MODEL_H
#define APP_MODEL_H

/**
 * AppModel:
 *
 * Holds information to a client application running in the memwrap.
 */
typedef struct {
    char *file_path;
    char *file_name;
} AppModel;

AppModel* app_model_new(void);
void app_model_set_selected_app(AppModel* model, const char* path);
const char* app_model_get_file_path(AppModel* model);
const char* app_model_get_file_name(AppModel* model);
void app_model_free(AppModel* model);

#endif //APP_MODEL_H
