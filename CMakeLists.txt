cmake_minimum_required(VERSION 3.16)
project(ZemaxDDEClient
    VERSION        "${APP_VERSION_STRING}"
    DESCRIPTION    "A C++ GUI application for interacting with Zemax via DDE"
    HOMEPAGE_URL   "https://github.com/d3m37r4/ZemaxDDEClient"
    LANGUAGES      C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SRC_DIR "src")
set(IMGUI_DIR "${SRC_DIR}/lib/imgui")
set(IMPLOT_DIR "${SRC_DIR}/lib/implot")
set(NFD_DIR "${SRC_DIR}/lib/nfd")

# MSYS2 paths (MinGW)
set(MINGW_ROOT "C:/msys64" CACHE STRING "MSYS2 MinGW root directory")
set(MINGW_INCLUDE "${MINGW_ROOT}/mingw64/include")
set(MINGW_LIB "${MINGW_ROOT}/mingw64/lib")

# Find Git to get the short commit hash
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE APP_GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(APP_GIT_COMMIT "unknown")
endif()

# Generate build number
# Build number = seconds since first commit in repository
# First commit timestamp (UTC): 1749018884 (4 June 2025)
# Obtained via: git rev-list --all --max-parents=0 | xargs git log -1 --format=%ct | sort -n | head -1
set(FIRST_COMMIT_TIMESTAMP 1749018884)

string(TIMESTAMP CURRENT_TIME "%s" UTC)
math(EXPR APP_BUILD_NUMBER "${CURRENT_TIME} - ${FIRST_COMMIT_TIMESTAMP}")

# Application version
set(APP_VERSION_MAJOR 0)
set(APP_VERSION_MINOR 1)
set(APP_VERSION_PATCH 0)
set(APP_VERSION_PRERELEASE "a")

# Form version string
set(APP_VERSION_STRING "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")
if(APP_VERSION_PRERELEASE)
    set(APP_VERSION_STRING "${APP_VERSION_STRING}-${APP_VERSION_PRERELEASE}")
endif()

# Generate version.h from template
configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
    @ONLY
)

# Source files
set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/app/app_init.cpp
    ${SRC_DIR}/app/app_actions.cpp
    ${SRC_DIR}/logger/logger.cpp
    ${SRC_DIR}/dde/dde_zemax_client_core.cpp
    ${SRC_DIR}/dde/dde_zemax_client_data.cpp
    ${SRC_DIR}/dde/dde_zemax_utils.cpp
    ${SRC_DIR}/gui/gui_init.cpp
    ${SRC_DIR}/gui/gui_container.cpp
    ${SRC_DIR}/gui/gui_surface_analysis.cpp
    ${SRC_DIR}/gui/gui_utils.cpp
    ${SRC_DIR}/gui/components/gui_navbar.cpp
    ${SRC_DIR}/gui/components/gui_sidebar.cpp
    ${SRC_DIR}/gui/components/gui_dde_status.cpp
    ${SRC_DIR}/gui/components/gui_content.cpp
    ${SRC_DIR}/gui/components/gui_debug_log.cpp
    ${SRC_DIR}/gui/components/gui_popups.cpp
    ${SRC_DIR}/gui/content_pages/gui_page_optical_system_info.cpp
    ${SRC_DIR}/gui/content_pages/gui_page_surface_sag_analysis.cpp
)

# Add ImGui source files directly
list(APPEND SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Add ImPlot source files directly
list(APPEND SOURCES
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp
)

# Add NFD source files directly
list(APPEND SOURCES
    ${NFD_DIR}/src/nfd_common.c
    ${NFD_DIR}/src/nfd_win.cpp
)

# Compile 'nfd_common.c' as C-code
set_source_files_properties(${NFD_DIR}/src/nfd_common.c PROPERTIES LANGUAGE C)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_BINARY_DIR}
    ${SRC_DIR}
    ${SRC_DIR}/app
    ${SRC_DIR}/gui
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMPLOT_DIR}
    ${NFD_DIR}/src/include
    ${MINGW_INCLUDE}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw3
    OpenGL32
    Gdi32
    User32
    Imm32
    Ole32
    Comdlg32
    Uuid
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra
    -DUNICODE -D_UNICODE
    -static-libgcc -static-libstdc++
)

# Convert the input to a lowercase and make it case-insensitive
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)

# Add '-v' and '-DDEBUG_LOG' in debug mode
if(BUILD_TYPE_LOWER STREQUAL "debug")
    message(STATUS "Debug mode: enabling DEBUG_LOG and verbose compiler output (-v)")
    target_compile_options(${PROJECT_NAME} PRIVATE -DDEBUG_LOG -v)
endif()

# Window mode: hide console in release, show in debug mode
if(BUILD_TYPE_LOWER STREQUAL "debug")
    message(STATUS "Debug mode: console enabled (no -mwindows)")
else()
    message(STATUS "Release mode: hiding console with -mwindows")
    target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
endif()

# Optimization flags
if(BUILD_OPTIMIZE)
    if(BUILD_OPTIMIZE EQUAL 1)
        message(STATUS "Enabling -O2 optimization")
        target_compile_options(${PROJECT_NAME} PRIVATE -O2)
    elseif(BUILD_OPTIMIZE EQUAL 2)
        message(STATUS "Enabling -O3 optimization")
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    else()
        message(WARNING "BUILD_OPTIMIZE=${BUILD_OPTIMIZE} is not supported. Use 1 (-O2) or 2 (-O3)")
    endif()
endif()

# Output executable name
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}_${APP_VERSION_STRING}_${APP_BUILD_NUMBER}"
)
