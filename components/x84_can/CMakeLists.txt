# The CAN frames of the car and the dashboard's view of them. Plain C++, without ESP-IDF: it also builds on the
# computer, for the tests in test/.
idf_component_register(
    SRCS "frames.cpp" "view.cpp" "trip.cpp"
    INCLUDE_DIRS "."
)
