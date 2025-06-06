#include "launcher.h"

// Launches the application of given path and arguments with LD_Preload via the memorywrapper and returns if successful
gboolean launch_application(const gchar *exec_path, const gchar *arguments)
{
    const gchar *memwrap_path = "/home/schneuzi/Codespace/01_Projects/mapd/build/libmemwrap.so";

    // Build LD_PRELOAD, environment and command arguments
    gchar *ld_preload = g_strdup_printf("LD_PRELOAD=%s", memwrap_path);
    gchar *envp[] = { ld_preload, NULL };
    gchar **argv = NULL;
    GError *parse_error = NULL;

    if (arguments && strlen(arguments) > 0)
	{
		gchar *command_line = g_strdup_printf("%s %s", exec_path, arguments);
		if (!g_shell_parse_argv(command_line, NULL, &argv, &parse_error))
        {
            g_warning("Argument parsing failed: %s", parse_error->message);
            g_error_free(parse_error);
            g_free(command_line);
            g_free(ld_preload);
            return FALSE;
        }
		g_free(command_line);
    }
	else
	{
        argv = g_new0(gchar *, 2);
        argv[0] = g_strdup(exec_path);
        argv[1] = NULL;
	}

    // Launch application
    GError *error = NULL;
    gboolean success = g_spawn_async
    (
        NULL,				// working directory
        argv,				// command arguments
        envp,				// environment variables
        G_SPAWN_DEFAULT,	// spawn flags
        NULL, NULL,			// child setup
        NULL,             	// child PID
        &error            	// error
    );

    // Check if application launch failed
    if (!success)
	{
        g_warning("Failed to launch: %s", error->message);
        g_error_free(error);
        g_free(ld_preload);
        return FALSE;
    }
 	else
        g_print("Launched: %s (with LD_PRELOAD=%s)\n", exec_path, memwrap_path);

	g_strfreev(argv);
    g_free(ld_preload);
    return TRUE;
}