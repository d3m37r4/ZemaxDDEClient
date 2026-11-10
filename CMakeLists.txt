cmake_minimum_required(VERSION 3.16)
project(ZemaxDDEClient
    VERSION        "${APP_VERSION_STRING}"
    DESCRIPTION    "Advanced analysis of optical systems using parameters retrieved from Zemax via DDE (Dynamic Data Exchange)"
    HOMEPAGE_URL   "https://github.com/d3m37r4/ZemaxDDEClient"
    LANGUAGES      C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SRC_DIR "src")
set(IMGUI_DIR "${SRC_DIR}/lib/imgui")
set(IMPLOT_DIR "${SRC_DIR}/lib/implot")
set(IMPLOT3D_DIR "${SRC_DIR}/lib/implot3d")
set(NFD_DIR "${SRC_DIR}/lib/nfd")
set(JSON_DIR "${SRC_DIR}/lib/json")
set(TE_DIR "${SRC_DIR}/lib/imgui_test_engine")

# MSYS2 paths (MinGW)
set(MINGW_ROOT "C:/msys64" CACHE STRING "MSYS2 MinGW root directory")
set(MINGW_INCLUDE "${MINGW_ROOT}/mingw64/include")
set(MINGW_LIB "${MINGW_ROOT}/mingw64/lib")

# Generate build number
# Build number = seconds since first commit in repository
# First commit: 4 June 2025 (timestamp 1749018884)
# Obtained via: git rev-list --all --max-parents=0 | xargs git log -1 --format=%ct | sort -n | head -1
if(NOT DEFINED BUILD_TIMESTAMP)
    string(TIMESTAMP BUILD_TIMESTAMP "%s" UTC)
endif()

set(FIRST_COMMIT_TIMESTAMP 1749018884)
math(EXPR APP_BUILD_NUMBER "${BUILD_TIMESTAMP} - ${FIRST_COMMIT_TIMESTAMP}")

# Log the final value
message(STATUS "Using APP_BUILD_NUMBER = ${APP_BUILD_NUMBER} (based on BUILD_TIMESTAMP = ${BUILD_TIMESTAMP})")

# Find Git to get the short commit hash
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE APP_GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

if(NOT DEFINED APP_GIT_COMMIT OR "${APP_GIT_COMMIT}" STREQUAL "")
    set(APP_GIT_COMMIT "unknown")
endif()

# Application version
set(APP_VERSION_MAJOR 0)
set(APP_VERSION_MINOR 8)
set(APP_VERSION_PATCH 0)
set(APP_VERSION_PRERELEASE "b")

# Form version string
set(APP_VERSION_STRING "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")
if(APP_VERSION_PRERELEASE)
    set(APP_VERSION_STRING "${APP_VERSION_STRING}-${APP_VERSION_PRERELEASE}")
endif()

# VERSIONINFO flag for .rc file (VS_FF_PRERELEASE = 0x01, or 0 for stable)
if(APP_VERSION_PRERELEASE)
    set(APP_RC_PRERELEASE_FLAG "0x01L")
else()
    set(APP_RC_PRERELEASE_FLAG "0x00L")
endif()

# Generate version.h from template
configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
    @ONLY
)

# Generate version.rc from template
configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.rc.in
    ${CMAKE_BINARY_DIR}/version.rc
    @ONLY
)

# Source files
set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/app/app_init.cpp
    ${SRC_DIR}/app/config_path.cpp
    ${SRC_DIR}/app/settings.cpp
    ${SRC_DIR}/app/utils.cpp
    ${SRC_DIR}/app/zemax_window_enumerator.cpp
    ${SRC_DIR}/logger/logger.cpp
    ${SRC_DIR}/dde/client_connection.cpp
    ${SRC_DIR}/dde/operation_monitor.cpp
    ${SRC_DIR}/dde/initial_data_load_service.cpp
    ${SRC_DIR}/dde/lens_update.cpp
    ${SRC_DIR}/dde/utils.cpp
    ${SRC_DIR}/dde/dde_connection_manager.cpp
    ${SRC_DIR}/app/compute/surface_profile.cpp
    ${SRC_DIR}/app/compute/surface_map.cpp
    ${SRC_DIR}/app/services/operation_monitor_service.cpp
    ${SRC_DIR}/gui/gui_manager.cpp
    ${SRC_DIR}/gui/shortcuts.cpp
    ${SRC_DIR}/gui/menu_bar_controller.cpp
    ${SRC_DIR}/gui/graphics_backend.cpp
    ${SRC_DIR}/gui/plot_image_exporter.cpp
    ${SRC_DIR}/gui/plot_image_export_util.cpp
    ${SRC_DIR}/gui/log_time_filter.cpp
    ${SRC_DIR}/gui/log_filter_util.cpp
    ${SRC_DIR}/gui/layout_snapshot.cpp
    ${SRC_DIR}/gui/layout_presets.cpp
    ${SRC_DIR}/app/services/surface_profile_service.cpp
    ${SRC_DIR}/app/services/surface_map_service.cpp
    ${SRC_DIR}/gui/surface_profile_service.cpp
    ${SRC_DIR}/gui/surface_irregularity_map_service.cpp
    ${SRC_DIR}/gui/theme_manager.cpp
    ${SRC_DIR}/gui/settings_manager.cpp
    ${SRC_DIR}/gui/ui_operation_monitor.cpp
    ${SRC_DIR}/gui/window_chrome.cpp
    ${SRC_DIR}/gui/utils.cpp
    ${SRC_DIR}/gui/popups/about_dialog.cpp
    ${SRC_DIR}/gui/popups/shortcuts_help_dialog.cpp
    ${SRC_DIR}/gui/popups/connection_lost_dialog.cpp
    ${SRC_DIR}/gui/popups/refresh_failed_dialog.cpp
    ${SRC_DIR}/gui/popups/preferences_dialog.cpp
    ${SRC_DIR}/gui/popups/clean_logs_confirm_dialog.cpp
    ${SRC_DIR}/gui/popups/reset_confirm_dialog.cpp
    ${SRC_DIR}/gui/popups/update_checker.cpp
    ${SRC_DIR}/gui/popups/connect_dde.cpp
    ${SRC_DIR}/gui/dockable_windows_manager.cpp
    ${SRC_DIR}/gui/windows_dockable/dde_status.cpp
    ${SRC_DIR}/gui/windows_dockable/logs.cpp
    ${SRC_DIR}/gui/windows_dockable/optical_system_info.cpp
    ${SRC_DIR}/gui/windows_dockable/surface_profile_inspector.cpp
    ${SRC_DIR}/gui/windows_dockable/surface_irregularity_map.cpp
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

# Add ImPlot3D source files directly
list(APPEND SOURCES
    ${IMPLOT3D_DIR}/implot3d.cpp
    ${IMPLOT3D_DIR}/implot3d_items.cpp
)

# Add NFD-Extended source files directly (Windows backend only)
list(APPEND SOURCES
    ${NFD_DIR}/src/nfd_win.cpp
    ${CMAKE_BINARY_DIR}/version.rc
)

# Dear ImGui Test Engine (GUI automation / automated testing).
# Opt-in: when OFF (the default) the engine sources are not compiled and the
# IMGUI_ENABLE_TEST_ENGINE define is not set, so the shipped release binary is
# byte-for-byte identical to a build without this option.
# Build with: ./build.sh test
option(ZEMAX_ENABLE_TEST_ENGINE "Compile the Dear ImGui Test Engine into the app" OFF)

if(ZEMAX_ENABLE_TEST_ENGINE)
    # The vendored capture tool embeds its own stb_image_write implementation,
    # while the app's plot exporter provides one for the whole binary. Disable
    # the tool's copy so the link has a single definition.
    set_source_files_properties(
        ${TE_DIR}/imgui_test_engine/imgui_capture_tool.cpp
        PROPERTIES COMPILE_DEFINITIONS IMGUI_DISABLE_STB_IMAGE_WRITE_IMPLEMENTATION)
    list(APPEND SOURCES
        ${TE_DIR}/imgui_test_engine/imgui_te_context.cpp
        ${TE_DIR}/imgui_test_engine/imgui_te_coroutine.cpp
        ${TE_DIR}/imgui_test_engine/imgui_te_engine.cpp
        ${TE_DIR}/imgui_test_engine/imgui_te_exporters.cpp
        ${TE_DIR}/imgui_test_engine/imgui_te_perftool.cpp
        ${TE_DIR}/imgui_test_engine/imgui_te_ui.cpp
        ${TE_DIR}/imgui_test_engine/imgui_te_utils.cpp
        ${TE_DIR}/imgui_test_engine/imgui_capture_tool.cpp
        ${SRC_DIR}/test_engine/test_engine.cpp
        ${SRC_DIR}/test_engine/test_cli.cpp
        ${SRC_DIR}/test_engine/test_capture.cpp
        ${SRC_DIR}/test_engine/test_profile.cpp
        ${SRC_DIR}/test_engine/tests/test_registry.cpp
        ${SRC_DIR}/test_engine/tests/test_smoke.cpp
        ${SRC_DIR}/test_engine/tests/test_preferences.cpp
        ${SRC_DIR}/test_engine/tests/test_dialogs.cpp
        ${SRC_DIR}/test_engine/tests/test_logs_dde.cpp
        ${SRC_DIR}/test_engine/tests/test_theme.cpp
        ${SRC_DIR}/test_engine/tests/test_capture.cpp
    )
endif()

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
    ${IMPLOT3D_DIR}
    ${NFD_DIR}/src/include
    ${JSON_DIR}/single_include
    ${MINGW_INCLUDE}
)

if(ZEMAX_ENABLE_TEST_ENGINE)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${TE_DIR}
        ${TE_DIR}/imgui_test_engine
    )
    # IMGUI_ENABLE_TEST_ENGINE must be set for every translation unit that
    # includes imgui_internal.h (it adds fields to ImGuiContext), otherwise
    # the structures would disagree between translation units.
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ZEMAX_ENABLE_TEST_ENGINE
        IMGUI_ENABLE_TEST_ENGINE
        IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL=1
        IMGUI_TEST_ENGINE_ENABLE_CAPTURE=1
    )
endif()

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
    Shell32
    winhttp
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -fdiagnostics-color=always
    -DUNICODE -D_UNICODE
)

target_link_options(${PROJECT_NAME} PRIVATE
    -static-libgcc
    -static-libstdc++
    -static
)

# Convert the input to a lowercase and make it case-insensitive
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)

# Determine whether the build is production (release)
if(BUILD_TYPE_LOWER STREQUAL "release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE APP_PRODUCTION_BUILD=1)
endif()

# Add '-v' and '-DDEBUG_LOG' in debug mode
if(BUILD_TYPE_LOWER STREQUAL "debug")
    message(STATUS "Debug mode: enabling DEBUG_LOG and verbose compiler output (-v)")
    target_compile_options(${PROJECT_NAME} PRIVATE -Og -DDEBUG_LOG -v)
endif()

# Window mode: hide console in release, show in debug mode
if(BUILD_TYPE_LOWER STREQUAL "debug")
    message(STATUS "Debug mode: console enabled (no -mwindows)")
else()
    message(STATUS "Release mode: hiding console with -mwindows")
    target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
endif()

# Strip symbols in release builds
if(BUILD_TYPE_LOWER STREQUAL "release")
    message(STATUS "Release: stripping debug symbols (-s)")
    target_link_options(${PROJECT_NAME} PRIVATE -s)
    message(STATUS "Release: enabling LTO (-flto)")
    target_compile_options(${PROJECT_NAME} PRIVATE -flto)
    target_link_options(${PROJECT_NAME} PRIVATE -flto)
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

# Tests (opt-in)
option(BUILD_TESTING "Build unit tests (GoogleTest)" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
