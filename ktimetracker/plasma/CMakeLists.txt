set(ktimetracker_engine_SRCS
    ktimetrackerengine.cpp
)

kde4_add_plugin(plasma_engine_ktimetracker ${ktimetracker_engine_SRCS})
target_link_libraries(plasma_engine_ktimetracker ${KDE4_KDECORE_LIBS} ${PLASMA_LIBS})

install(TARGETS plasma_engine_ktimetracker DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-engine-ktimetracker.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

set(ktimetracker_applet_SRCS
    ktimetrackerapplet.cpp)

kde4_add_plugin(plasma_applet_timetrackerapplet ${ktimetracker_applet_SRCS})
target_link_libraries(plasma_applet_timetrackerapplet ${KDE4_KIO_LIBS} ${PLASMA_LIBS})

install(TARGETS plasma_applet_timetrackerapplet DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-timetrackerapplet-default.desktop DESTINATION ${SERVICES_INSTALL_DIR})
