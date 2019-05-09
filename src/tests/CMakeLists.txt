find_package(Qt5Test CONFIG REQUIRED)
include(ECMAddTests)

ecm_add_test(formattimetest.cpp LINK_LIBRARIES libktimetracker Qt5::Test)
