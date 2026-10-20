# The dashboard's UI on LVGL. Sources are picked up by folder: a new .cpp in widgets/ or panels/ needs no change here.
file(GLOB SOURCES "*.cpp" "widgets/*.cpp" "panels/*.cpp" "fonts/*.c" "images/*.c")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "."
    REQUIRES lvgl__lvgl x84_can
)
