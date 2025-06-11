# Installation Guide
## Prerequisites
(Some are already stated in the [README](README.md))
- Ubuntu 22.04 or later
- GTK 4.6+ development libraries
- Make and CMake
- JSON C library (should be already included in [lib/](../lib/))

## Command Sequence

1. Update and install the packages needed for the GUI (+ CMake and build-essential for Make):
    ```bash
    sudo apt-get update
    sudo apt install libgtk-4-dev libglib2.0-dev build-essential cmake
    ```
2. Change to the project root as current working directory, then enter in your terminal:
    ```bash
    mkdir build; cd build
    ```
3. Your working directory should be `build/` at this point. Now enter:
    ```bash
    cmake ..
    ```
   This will create your `Makefile`.
4. You can build your files with
    ```bash
    make
    ```
   You will now have a `gui` executable among the other files. This is the final program.
5. Run the application:
    ```bash
    ./gui
    ```
