include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

set(TEST_SOURCES
    test_settings.cpp
    test_operation_monitor.cpp
)

set(TEST_LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/app/settings.cpp
    ${CMAKE_SOURCE_DIR}/src/dde/operation_monitor.cpp
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
