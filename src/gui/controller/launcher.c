#include "launcher.h"

void launch_application(const gchar *exec_path) {

    const gchar *memwrap_path = "/home/schneuzi/Codespace/01_Projects/mapd/build/libmemwrap.so";

    // Build LD_PRELOAD
    gchar *ld_preload = g_strdup_printf("LD_PRELOAD=%s", memwrap_path);

    // Build environment and command arguments
    gchar *envp[] = { ld_preload, NULL };
    gchar *argv[] = { (gchar*)exec_path, NULL };

    // Launch application
    GError *error = NULL;
    gboolean success = g_spawn_async(
        NULL,    // working directory (current directory)
        argv,    // command arguments
        envp,    // environment variables
        G_SPAWN_DEFAULT,  // spawn flags
        NULL, NULL,       // child setup (optional)
        NULL,             // child PID (optional)
        &error            // error
    );

    if (!success) {
        g_warning("Failed to launch: %s", error->message);
        g_error_free(error);
    } else {
        g_print("Launched: %s (with LD_PRELOAD=%s)\n", exec_path, memwrap_path);
    }

    g_free(ld_preload);
}