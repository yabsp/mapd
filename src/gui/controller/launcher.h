#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <glib.h>
#include <stdio.h>

// Launches the application of a given path with LD_Preload and the memorywrapper
void launch_application(const gchar *exec_path);

#endif //LAUNCHER_H
