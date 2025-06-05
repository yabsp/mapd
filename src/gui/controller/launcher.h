#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <glib.h>
#include <stdio.h>

// Launches the application of given path and arguments with LD_Preload via the memorywrapper and returns if successful
gboolean launch_application(const gchar *exec_path, const gchar *arguments);

#endif //LAUNCHER_H
