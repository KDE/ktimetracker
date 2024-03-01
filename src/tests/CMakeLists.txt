# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

find_package(Qt6Test CONFIG REQUIRED)
include(ECMAddTests)

add_library(helpers STATIC helpers.cpp)
target_link_libraries(helpers libktimetracker Qt6::Test)

ecm_add_test(formattimetest.cpp LINK_LIBRARIES helpers)
ecm_add_test(exportcsvtest.cpp LINK_LIBRARIES helpers)
ecm_add_test(plannerparsertest.cpp LINK_LIBRARIES helpers)
ecm_add_test(tasktest.cpp LINK_LIBRARIES helpers)
ecm_add_test(storagetest.cpp LINK_LIBRARIES helpers)
