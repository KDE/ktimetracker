project(ktimetracker)

macro_bool_to_01(X11_Xscreensaver_LIB HAVE_LIBXSS)
configure_file(config-karm.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-karm.h )


add_subdirectory( support ) 
add_subdirectory( pics ) 
add_subdirectory( test ) 

include_directories( ${KDE4_INCLUDE_DIR}/kcal ${CMAKE_SOURCE_DIR}/kresources/remote  )


########### next target ###############
set(karm_shared_SRCS 
	desktoptracker.cpp 
	edittaskdialog.cpp idletimedetector.cpp 
	timekard.cpp karmutility.cpp ktimewidget.cpp
	karmstorage.cpp mainwindow.cpp preferences.cpp 
	task.cpp taskview.cpp tray.cpp  focusdetector.cpp
	csvexportdialog.cpp plannerparser.cpp 
	treeviewheadercontextmenu.cpp edithistorydialog.cpp timetrackerwidget.cpp 
  focusdetectornotifier.cpp
 )

qt4_add_dbus_adaptor( karm_shared_SRCS 
                      org.kde.ktimetracker.ktimetracker.xml 
                      timetrackerwidget.h 
                      TimetrackerWidget 
                      mainadaptor
                      MainAdaptor )
qt4_add_dbus_adaptor( karm_shared_SRCS 
                      org.kde.ktimetracker.storage.xml 
                      taskview.h 
                      TaskView
                      storageadaptor
                      StorageAdaptor )
kde4_add_ui_files( karm_shared_SRCS 
  csvexportdialog_base.ui 
  cfgbehavior.ui 
  cfgdisplay.ui
  cfgstorage.ui )
kde4_add_kcfg_files( karm_shared_SRCS ktimetracker.kcfgc )

set(karm_SRCS main.cpp ${karm_shared_SRCS})


kde4_add_executable(karm karm.cpp)

# todo: move to Oxygen icon
kde4_add_app_icon(karm_SRCS "hi*-app-ktimetracker.png")

kde4_add_executable(ktimetracker ${karm_SRCS})


target_link_libraries(karm   ${KDE4_KIO_LIBS} ${KDE4_KCAL_LIBS} kdepim kcal_resourceremote)
if(X11_Xscreensaver_LIB)
	target_link_libraries(karm ${X11_Xscreensaver_LIB})
endif(X11_Xscreensaver_LIB)

target_link_libraries(ktimetracker   ${KDE4_KIO_LIBS} ${KDE4_KCAL_LIBS} kdepim  kcal_resourceremote)
if(X11_Xscreensaver_LIB)
	target_link_libraries(ktimetracker  ${X11_Xscreensaver_LIB} )
endif(X11_Xscreensaver_LIB)

install(TARGETS karm  ${INSTALL_TARGETS_DEFAULT_ARGS} )
install(TARGETS ktimetracker  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### next target ###############

set(karmpart_PART_SRCS karm_part.cpp ${karm_shared_SRCS})

kde4_add_plugin(karmpart ${karmpart_PART_SRCS})

target_link_libraries(karmpart  ${KDE4_KCAL_LIBS}  ${KDE4_KIO_LIBS} kdepim kcal_resourceremote)
if(X11_Xscreensaver_LIB)
	target_link_libraries(karmpart  ${X11_Xscreensaver_LIB} )
endif(X11_Xscreensaver_LIB)


install(TARGETS karmpart  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES karm_part.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES karmui.rc  DESTINATION ${DATA_INSTALL_DIR}/karmpart)
install( FILES karmui.rc  DESTINATION ${DATA_INSTALL_DIR}/ktimetracker)

kde4_install_icons( ${ICON_INSTALL_DIR}   )
install( FILES org.kde.ktimetracker.ktimetracker.xml DESTINATION ${KDE4_DBUS_INTERFACES_DIR} )
