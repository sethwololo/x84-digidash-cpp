# The CAN decoding and the dashboard's view of the car. Plain C++: it also builds on the computer, for the tests.
idf_component_register(
    SRCS "view.cpp"
    INCLUDE_DIRS "."
)
