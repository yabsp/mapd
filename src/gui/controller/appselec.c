#include "appselec.h"

void populate_app_list(GtkListBox *list_box)
{
    DIR *dir = opendir("/home/schneuzi/Codespace/01_Projects/mapd/build/");
    struct dirent *entry;

    if (!dir)
    {
        g_warning("Failed to open /usr/share/applications/");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (g_strcmp0(entry->d_name, ".") == 0 || g_strcmp0(entry->d_name, "..") == 0)
            continue;

        gchar *filepath = g_build_filename("/usr/share/applications/", entry->d_name, NULL);
        GKeyFile *keyfile = g_key_file_new();

        if (g_key_file_load_from_file(keyfile, filepath, G_KEY_FILE_NONE, NULL))
        {
            gchar *name = g_key_file_get_locale_string(keyfile, "Desktop Entry", "Name", NULL, NULL);
            gchar *exec = g_key_file_get_string(keyfile, "Desktop Entry", "Exec", NULL);

            if (name && exec)
            {
                GtkWidget *row = gtk_list_box_row_new();
                GtkWidget *label = gtk_label_new(name);
                gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
                g_object_set_data(G_OBJECT(row), "exec", g_strdup(exec));
                gtk_list_box_append(GTK_LIST_BOX(list_box), row);
            }

            g_free(name);
            g_free(exec);
        }
        g_key_file_free(keyfile);
        g_free(filepath);

    }
    closedir(dir);
}

void on_app_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    gchar *exec = g_object_get_data(G_OBJECT(row), "exec");
    g_print("Selected application exec: %s\n", exec);
    // TODO insert launcher code here
}
