# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause

qt_add_resources(icons_SRCS icons.qrc)
add_library(icons OBJECT ${icons_SRCS})

set(APP_ICONS_BREEZE
    ${CMAKE_CURRENT_SOURCE_DIR}/22-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/32-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/48-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/64-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/128-apps-ktimetracker.png
)
ecm_install_icons(
    ICONS ${APP_ICONS_BREEZE}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

install(FILES org.kde.ktimetracker.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)
