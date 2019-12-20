qt5_add_resources(icons_SRCS icons.qrc)
add_library(icons OBJECT ${icons_SRCS})

set(APP_ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/app/16-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/app/22-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/app/32-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/app/48-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/app/64-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/app/128-apps-ktimetracker.png
)
ecm_install_icons(
    ICONS ${APP_ICONS}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
