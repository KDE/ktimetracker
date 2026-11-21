# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

find_package(Qt6Test CONFIG REQUIRED)
include(ECMAddTests)

add_library(helpers STATIC helpers.cpp)
target_link_libraries(helpers
    PUBLIC
    ktt-model
    ktt-widgets
    ktt-export
    ktt-dialogs
    ktt-deps
    Qt6::Test
)

ecm_add_test(formattimetest.cpp LINK_LIBRARIES helpers)
ecm_add_test(exportcsvtest.cpp LINK_LIBRARIES helpers)
# ecm_add_test(plannerparsertest.cpp LINK_LIBRARIES helpers)
ecm_add_test(tasktest.cpp LINK_LIBRARIES helpers)
ecm_add_test(storagetest.cpp LINK_LIBRARIES helpers)

# The main window needs its GUI description, which the application reads out of
# the Qt resource file, and ktt-base, which is where MainWindow lives.
ecm_add_test(mainwindowtest.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../ktimetracker.qrc TEST_NAME mainwindowtest LINK_LIBRARIES helpers ktt-base)
