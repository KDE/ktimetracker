file(GLOB app_icons "*-apps-ktimetracker.png")
ecm_add_app_icon(ktimetracker_app_icon_SRCS ICONS ${app_icons})

add_library(ktimetracker_app_icon OBJECT ${ktimetracker_app_icon_SRCS})

ecm_install_icons(
    ICONS ${app_icons}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
