cmake_minimum_required(VERSION 3.10)
project(MemAnalyzer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall -Wextra -fPIC -g -pthread")


# Include directories
include_directories(lib src src/gui)

# Source files for analyzer
set(ANALYZER_SRC
        src/analyzer.c
        src/message.c
        lib/cJSON.c
        src/fragmentation.c
)

# Source files for GUI
set(GUI_SRC
        src/gui/gui.c
)

# Source files for memwrap
set(WRAPPER_SRC
        src/memwrap.c
)

# Source files for tests
set(TEST_ALLOC_SRC
        tests/test_alloc.c
)
set(TEST_VALGRIND_SRC
        tests/test_valgrind.c
)

# Analyzer executable
add_executable(analyzer ${ANALYZER_SRC})

# Shared library for memwrap
add_library(memwrap SHARED ${WRAPPER_SRC})
target_link_libraries(memwrap dl)

# test_alloc executable
add_executable(test_alloc ${TEST_ALLOC_SRC})
target_link_libraries(test_alloc memwrap)

# test_valgrind executable
add_executable(test_valgrind ${TEST_VALGRIND_SRC})

# === GTK SUPPORT ===
# Find GTK for gui
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk4)

if (GTK_FOUND)
    message(STATUS "GTK found: ${GTK_INCLUDE_DIRS}")
    include_directories(${GTK_INCLUDE_DIRS})
    link_directories(${GTK_LIBRARY_DIRS})
    add_definitions(${GTK_CFLAGS_OTHER})

    # Build gui executable only if GTK is found
    add_executable(gui ${GUI_SRC})
    target_link_libraries(gui ${GTK_LIBRARIES})
endif()

# Custom targets for CI or test automation

add_custom_target(valgrind-analyzer
        COMMAND timeout 10s valgrind --leak-check=full --show-leak-kinds=all --log-file=${OUTDIR}/valgrind-analyzer.log ${OUTDIR}/analyzer
        COMMAND sleep 12
        COMMAND cat ${OUTDIR}/valgrind-analyzer.log
        COMMAND bash -c "if grep \"definitely lost: [1-9][0-9]* bytes\" ${OUTDIR}/valgrind-analyzer.log; then echo \"Memory leaks found in analyzer\"; exit 1; else echo \"No leaks detected in analyzer\"; fi"
        DEPENDS analyzer
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(valgrind-test
        COMMAND bash -c "if valgrind --leak-check=full --error-exitcode=1 ${OUTDIR}/test_valgrind; then echo \"Leak not detected — test failed\"; exit 1; else echo \"Leak detected — Valgrind is working as expected\"; fi"
        DEPENDS test_valgrind
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(clean-all
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${OUTDIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
