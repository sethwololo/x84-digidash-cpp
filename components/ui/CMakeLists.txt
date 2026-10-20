file(GLOB FONTS "fonts/*.c")

idf_component_register(
    SRCS ${FONTS}
    INCLUDE_DIRS "."
    REQUIRES lvgl__lvgl
)
