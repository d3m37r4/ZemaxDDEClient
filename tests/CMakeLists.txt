include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

set(TEST_SOURCES
    test_settings.cpp
    test_operation_monitor.cpp
    test_surface_map.cpp
    test_shortcuts.cpp
    test_input_validation.cpp
    test_gui_utils.cpp
    test_plot_image_export.cpp
    test_log_time_filter.cpp
)

set(TEST_LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/app/settings.cpp
    ${CMAKE_SOURCE_DIR}/src/dde/operation_monitor.cpp
    ${CMAKE_SOURCE_DIR}/src/app/services/operation_monitor_service.cpp
    ${CMAKE_SOURCE_DIR}/src/app/compute/surface_map.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/plot_image_export_util.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/log_time_filter.cpp
)

add_executable(tests ${TEST_SOURCES} ${TEST_LIB_SOURCES})

target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/lib/json/single_include
)

target_link_libraries(tests PRIVATE GTest::gtest_main)

target_compile_options(tests PRIVATE
    -Wall -Wextra
    -DUNICODE -D_UNICODE
)

gtest_discover_tests(tests)
