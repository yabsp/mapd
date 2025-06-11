cmake_minimum_required(VERSION 3.16)
project(mapdtest C)

set(CMAKE_C_STANDARD 11)

# --- Find GTK4 ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
find_program(GLIB_COMPILE_RESOURCES glib-compile-resources REQUIRED)

# --- Build cJSON ---
add_library(cjson STATIC lib/cJSON.c lib/cJSON.h)
target_include_directories(cjson PUBLIC ${CMAKE_SOURCE_DIR}/lib)

# --- Build message module ---
add_library(message STATIC src/message/message.c)
target_include_directories(message PUBLIC
        ${CMAKE_SOURCE_DIR}/src/analyzer
        ${CMAKE_SOURCE_DIR}/src/message)
target_link_libraries(message PUBLIC cjson)

# --- Build memwrap shared library (LD_PRELOAD wrapper) ---
add_library(memwrap SHARED src/memwrap/memwrap.c)
target_include_directories(memwrap PRIVATE src/memwrap src/message)
target_link_libraries(memwrap PRIVATE message)
set_target_properties(memwrap PROPERTIES OUTPUT_NAME "mem_wrap")

# --- Build analyzer executable ---
add_library(analyzer STATIC
    src/analyzer/analyzer.c
    src/analyzer/fragmentation.c
)

target_include_directories(analyzer PRIVATE
    ${CMAKE_SOURCE_DIR}/src/message
    ${CMAKE_SOURCE_DIR}/src/gui/controller
    ${CMAKE_SOURCE_DIR}/src/gui/view
    ${CMAKE_SOURCE_DIR}/src/gui/model
)
target_link_libraries(analyzer PRIVATE PkgConfig::GTK4)

# --- Build GUI executable ---

# Compile GResource
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/resources.c
    COMMAND ${GLIB_COMPILE_RESOURCES}
    --target=${CMAKE_BINARY_DIR}/resources.c
    --generate-source
    --sourcedir=${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/resources/resources.gresource.xml
    DEPENDS ${CMAKE_SOURCE_DIR}/src/gui/resources/resources.gresource.xml
)

add_executable(gui
    src/gui/main.c
    src/gui/controller/main_controller.c
    src/gui/view/main_view.c
    src/gui/model/app_model.c
    ${CMAKE_BINARY_DIR}/resources.c
)

target_include_directories(gui PRIVATE
    src
    src/analyzer
    src/gui
    src/gui/controller
    src/gui/view
    src/gui/model
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(gui PRIVATE PkgConfig::GTK4 analyzer message memwrap)

# --- Build test_alloc executable ---
add_executable(test_alloc tests/test_alloc.c)
target_include_directories(test_alloc PRIVATE src/message src/memwrap)
target_link_libraries(test_alloc PRIVATE message pthread)

# --- Build test_valgrind executable ---
add_executable(test_valgrind tests/test_valgrind.c)
target_include_directories(test_valgrind PRIVATE src/message)
target_link_libraries(test_valgrind PRIVATE message pthread)

# --- Additional for github workflow ---

add_custom_target(valgrind-analyzer
        COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/scripts/valgrind-analyzer.sh
        DEPENDS gui
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(valgrind-test
        COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/scripts/valgrind-test.sh
        DEPENDS test_valgrind
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(clean-all
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)