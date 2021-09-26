qt5_add_resources(icons_SRCS icons.qrc)
add_library(icons OBJECT ${icons_SRCS})

set(APP_ICONS_BREEZE
    ${CMAKE_CURRENT_SOURCE_DIR}/breeze/22-apps-ktimetracker.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/breeze/32-apps-ktimetracker.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/breeze/48-apps-ktimetracker.svg
)
ecm_install_icons(
    ICONS ${APP_ICONS_BREEZE}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME breeze
)

set(APP_ICONS_BREEZE_DARK
    ${CMAKE_CURRENT_SOURCE_DIR}/breeze-dark/22-apps-ktimetracker.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/breeze-dark/32-apps-ktimetracker.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/breeze-dark/48-apps-ktimetracker.svg
)
ecm_install_icons(
    ICONS ${APP_ICONS_BREEZE_DARK}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME breeze-dark
)
