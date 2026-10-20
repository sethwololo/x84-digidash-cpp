# Tests of the CAN decoding, on the computer:
#
#   cmake -S components/x84_can/test -B components/x84_can/test/build
#   cmake --build components/x84_can/test/build && components/x84_can/test/build/x84_can_test
cmake_minimum_required(VERSION 3.22)
project(x84_can_test CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
# doctest is a single header: its own CMake (too old for CMake 4) is skipped by pointing SOURCE_SUBDIR nowhere.
FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/doctest/doctest.git GIT_TAG v2.4.12 GIT_SHALLOW TRUE
    SOURCE_SUBDIR none)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(doctest json)

set(COMPONENT "${CMAKE_CURRENT_SOURCE_DIR}/..")
add_executable(x84_can_test
    main.cpp signal_test.cpp frames_test.cpp view_test.cpp trip_test.cpp
    "${COMPONENT}/frames.cpp" "${COMPONENT}/view.cpp" "${COMPONENT}/trip.cpp")
target_include_directories(x84_can_test PRIVATE "${COMPONENT}" "${doctest_SOURCE_DIR}")
target_link_libraries(x84_can_test PRIVATE nlohmann_json::nlohmann_json)
target_compile_definitions(x84_can_test PRIVATE SIGNALS_JSON="${CMAKE_CURRENT_SOURCE_DIR}/../../../can/signals-x84.json")
# doctest recognizes macOS by a macro recent SDKs no longer define up front, takes it for Linux and calls signal
# functions that are not declared: turn off its signal handling and its break into the debugger, which the tests
# do not need. (A function-like macro has to go through the compile options: CMake drops it from definitions.)
target_compile_definitions(x84_can_test PRIVATE DOCTEST_CONFIG_NO_POSIX_SIGNALS)
target_compile_options(x84_can_test PRIVATE "-DDOCTEST_BREAK_INTO_DEBUGGER()=")
