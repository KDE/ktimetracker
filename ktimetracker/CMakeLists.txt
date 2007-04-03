project(karm)

macro_bool_to_01(X11_Xss_LIB HAVE_LIBXSS)
configure_file(config-karm.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-karm.h )


add_subdirectory( support ) 
add_subdirectory( pics ) 
add_subdirectory( test ) 

include_directories( ${KDE4_INCLUDE_DIR}/kcal ${CMAKE_SOURCE_DIR}/kresources/remote  )


########### next target ###############
set(karm_shared_SRCS 
	kaccelmenuwatch.cpp desktoptracker.cpp 
	edittaskdialog.cpp idletimedetector.cpp 
	printdialog.cpp timekard.cpp karmutility.cpp ktimewidget.cpp
	karmstorage.cpp mainwindow.cpp preferences.cpp print.cpp 
	task.cpp taskview.cpp tray.cpp  
	csvexportdialog.cpp plannerparser.cpp 
	taskviewwhatsthis.cpp
 )

qt4_add_dbus_adaptor(karm_shared_SRCS org.kde.karm.Karm.xml mainwindow.h MainWindow karmmainadaptor )
kde4_add_ui_files(karm_shared_SRCS csvexportdialog_base.ui )

set(karm_SRCS main.cpp ${karm_shared_SRCS})

kde4_automoc(${karm_SRCS})

kde4_add_executable(karm karm.cpp)
kde4_add_executable(ktimetracker ${karm_SRCS})


target_link_libraries(karm  ${KDE4_KPARTS_LIBS}  ${KDE4_KDEPRINT_LIBS} ${KDE4_KCAL_LIBS} kdepim kcal_resourceremote)
if(X11_Xss_LIB)
	target_link_libraries(karm ${X11_Xss_LIB})
endif(X11_Xss_LIB)

target_link_libraries(ktimetracker  ${KDE4_KPARTS_LIBS}  ${KDE4_KDEPRINT_LIBS} ${KDE4_KCAL_LIBS} kdepim  kcal_resourceremote)
if(X11_Xss_LIB)
	target_link_libraries(ktimetracker  ${X11_Xss_LIB} )
endif(X11_Xss_LIB)

install(TARGETS karm  DESTINATION ${BIN_INSTALL_DIR} )
install(TARGETS ktimetracker  DESTINATION ${BIN_INSTALL_DIR} )


########### next target ###############


########### next target ###############

set(karmpart_PART_SRCS karm_part.cpp ${karm_shared_SRCS})
qt4_add_dbus_adaptor(karmpart_PART_SRCS org.kde.karm.Karm.xml karm_part.h karmPart karmpartadaptor KarmPartAdaptor)

kde4_automoc(${karmpart_PART_SRCS})

kde4_add_plugin(karmpart WITH_PREFIX ${karmpart_PART_SRCS})



target_link_libraries(karmpart  ${KDE4_KPARTS_LIBS} ${KDE4_KCAL_LIBS}  ${KDE4_KDEPRINT_LIBS} kdepim kcal_resourceremote)
if(X11_Xss_LIB)
	target_link_libraries(karmpart  ${X11_Xss_LIB} )
endif(X11_Xss_LIB)


install(TARGETS karmpart  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES karm_part.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES karmui.rc  DESTINATION ${DATA_INSTALL_DIR}/karmpart)
install( FILES karmui.rc  DESTINATION ${DATA_INSTALL_DIR}/karm)

kde4_install_icons( ${ICON_INSTALL_DIR}   )
install( FILES org.kde.karm.Karm.xml DESTINATION ${DBUS_INTERFACES_DIR} )
