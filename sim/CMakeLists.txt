# The dashboard on the computer (macOS or Linux), with the same UI code as the board and the simulation's data.
#
#   cmake -S sim -B sim/build && cmake --build sim/build
#   sim/build/dashboard-sim                              a window
#   sim/build/dashboard-sim --screenshot out.png 1       scenario 1 saved as PNG
cmake_minimum_required(VERSION 3.22)
project(x84-digidash-sim C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

# The same LVGL as the firmware (main/idf_component.yml), configured by sim/lv_conf.h.
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE FILEPATH "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(lvgl GIT_REPOSITORY https://github.com/lvgl/lvgl.git GIT_TAG v9.5.0 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(lvgl)

# stb_image_write, to save screenshots as PNG.
FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d)
FetchContent_MakeAvailable(stb)

find_package(SDL2 REQUIRED)
# LVGL's SDL driver (LV_USE_SDL) draws the window.
target_link_libraries(lvgl PUBLIC SDL2::SDL2)

set(COMPONENTS "${CMAKE_CURRENT_SOURCE_DIR}/../components")
file(GLOB UI_SOURCES
    "${COMPONENTS}/ui/*.cpp" "${COMPONENTS}/ui/widgets/*.cpp" "${COMPONENTS}/ui/panels/*.cpp"
    "${COMPONENTS}/ui/fonts/*.c" "${COMPONENTS}/ui/images/*.c")
file(GLOB CAN_SOURCES "${COMPONENTS}/x84_can/*.cpp")

add_executable(dashboard-sim main.cpp ${UI_SOURCES} ${CAN_SOURCES})
target_include_directories(dashboard-sim PRIVATE "${COMPONENTS}/ui" "${COMPONENTS}/x84_can" "${stb_SOURCE_DIR}")
target_link_libraries(dashboard-sim PRIVATE lvgl SDL2::SDL2)
# The generated fonts and images include "lvgl.h".
target_include_directories(dashboard-sim PRIVATE "${lvgl_SOURCE_DIR}")
