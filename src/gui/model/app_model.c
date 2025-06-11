#include "app_model.h"
#include <stdlib.h>
#include <glib.h>

AppModel* app_model_new(void)
{
    AppModel *model = malloc(sizeof(AppModel));
    model->file_name = NULL;
    model->file_path = NULL;
    return model;
}

void app_model_set_selected_app(AppModel* model, const char* path) {
    if (model->file_path)
        free(model->file_path);
    if (model->file_name)
        free(model->file_name);

    model->file_path = strdup(path);
    model->file_name = g_path_get_basename(path);
}

const char* app_model_get_file_path(AppModel* model)
{
    return model->file_path;
}

const char* app_model_get_file_name(AppModel* model)
{
    return model->file_name;
}

void app_model_free(AppModel* model)
{
    if (model) {
        free(model->file_path);
        free(model->file_name);
        free(model);
    }
}