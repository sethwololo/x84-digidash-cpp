idf_component_register(
    SRCS "main.cpp" "display.cpp"
    INCLUDE_DIRS "."
    REQUIRES ui esp_timer
)

# The C configuration macros of ESP-IDF components set some fields and leave the rest at zero, which C++ warns about.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
