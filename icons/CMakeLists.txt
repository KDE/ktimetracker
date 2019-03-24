file(GLOB ktt_icons *.png *.svgz)
ecm_install_icons(
    ICONS       ${ktt_icons}
    DESTINATION ${ICON_INSTALL_DIR}
    THEME hicolor
)

add_subdirectory(app)
