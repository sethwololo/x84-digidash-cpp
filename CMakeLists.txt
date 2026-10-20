cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(x84-digidash)

# esp_lv_decoder (an image decoder the Waveshare BSP pulls in) includes LVGL's private headers, which with
# CONFIG_LV_USE_PPA include the PPA driver's header, without depending on that driver.
target_link_libraries(lvgl_docoder_lib PRIVATE idf::esp_driver_ppa)
