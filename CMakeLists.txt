cmake_minimum_required(VERSION 3.10)
project(MemAnalyzer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall -Wextra -fPIC -g -pthread")

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/gui
    ${PROJECT_SOURCE_DIR}/lib/gui/model
    ${PROJECT_SOURCE_DIR}/lib/gui/view
    ${PROJECT_SOURCE_DIR}/src/gui/controller
    ${PROJECT_SOURCE_DIR}/src/gui/resources
)

# Source files for analyzer
set(ANALYZER_SRC
    src/analyzer.c
    src/message.c
    src/fragmentation.c
    lib/cJSON.c
)

add_executable(analyzer ${ANALYZER_SRC})

# Memwrap
set(WRAPPER_SRC src/memwrap.c)

add_library(memwrap SHARED ${WRAPPER_SRC})
target_link_libraries(memwrap dl)

# Test_alloc and test_valgrind
set(TEST_ALLOC_SRC tests/test_alloc.c)
set(TEST_VALGRIND_SRC tests/test_valgrind.c)

add_executable(test_alloc ${TEST_ALLOC_SRC})
target_link_libraries(test_alloc memwrap)

add_executable(test_valgrind ${TEST_VALGRIND_SRC})

# Find GTK for gui
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk4)

if (GTK_FOUND)
    message(STATUS "GTK found: ${GTK_INCLUDE_DIRS}")
    include_directories(${GTK_INCLUDE_DIRS})
    link_directories(${GTK_LIBRARY_DIRS})
    add_definitions(${GTK_CFLAGS_OTHER})

    file(GLOB_RECURSE GUI_SRC
            src/gui/model/*.c
            src/gui/view/*.c
            src/gui/controller/*.c
            src/gui/main.c
    )

    # Build gui executable only if GTK is found
    add_executable(gui ${GUI_SRC})

    target_include_directories(gui PRIVATE
            ${PROJECT_SOURCE_DIR}/src/gui
            ${PROJECT_SOURCE_DIR}/src/gui/controller
            ${PROJECT_SOURCE_DIR}/src/gui/view
    )

    target_link_libraries(gui ${GTK_LIBRARIES})
endif()

# Copy resources
file(GLOB GUI_RESOURCE_FILES
        src/gui/resources/*.ui
        src/gui/resources/*.cmb
        src/gui/resources/*.svg
)

# Copy to build directory
file(COPY ${GUI_RESOURCE_FILES} DESTINATION ${CMAKE_BINARY_DIR}/resources)
message(STATUS "Copied resources to ${CMAKE_BINARY_DIR}/resources")

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
