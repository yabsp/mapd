#include "launcher.h"

// Launches the application of given path and arguments with LD_Preload via the memorywrapper and returns if successful
gboolean launch_application(const gchar *exec_path, const gchar *arguments)
{
    const gchar *memwrap_path = "/home/schneuzi/Codespace/01_Projects/mapd/build/libmemwrap.so";

    // Build LD_PRELOAD, environment and command arguments
    gchar *ld_preload = g_strdup_printf("LD_PRELOAD=%s", memwrap_path);
    gchar *envp[] = { ld_preload, NULL };

    // Prepare full command string: binary + arguments (single string)
    gchar *full_command = NULL;
    if (arguments && strlen(arguments) > 0)
        full_command = g_strdup_printf("%s %s", exec_path, arguments);
    else
        full_command = g_strdup(exec_path);

    // Use g_shell_parse_argv to safely split into argv[]
    gchar **argv = NULL;
    GError *parse_error = NULL;
    if (!g_shell_parse_argv(full_command, NULL, &argv, &parse_error))
    {
        g_warning("Argument parsing failed: %s", parse_error->message);
        g_error_free(parse_error);
        g_free(full_command);
        g_free(ld_preload);
        return FALSE;
    }

    // Launch application
    GError *error = NULL;
    gboolean success = g_spawn_async
    (
        NULL,    // working directory
        argv,    // command arguments
        envp,    // environment variables
        G_SPAWN_DEFAULT,  // spawn flags
        NULL, NULL,       // child setup
        NULL,             // child PID
        &error            // error
    );

    // Check if application launch failed
    if (!success) {
        g_warning("Failed to launch: %s", error->message);
        g_error_free(error);
        g_free(ld_preload);
        return FALSE;
    } else {
        g_print("Launched: %s (with LD_PRELOAD=%s)\n", exec_path, memwrap_path);
    }

    g_free(ld_preload);
    return TRUE;
}