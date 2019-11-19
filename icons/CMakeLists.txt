file(GLOB ktt_icons *.png *.svgz)
ecm_install_icons(
    ICONS       ${ktt_icons}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

add_subdirectory(app)
