kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES karm.desktop )

kde4_footer()



#original Makefile.am contents follow:

#EXTRA_DIST = karm.desktop newtask.dlg
#
#xdg_apps_DATA = karm.desktop
