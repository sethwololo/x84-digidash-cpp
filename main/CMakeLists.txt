idf_component_register(
    SRCS "main.cpp" "display.cpp" "can.cpp" "vehicle.cpp" "intro.cpp"
    INCLUDE_DIRS "."
    REQUIRES ui x84_can esp_timer esp_driver_twai nvs_flash
)

# The C configuration macros of ESP-IDF components set some fields and leave the rest at zero, which C++ warns about.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
