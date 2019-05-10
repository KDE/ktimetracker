include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(ktimetracker_SRCS
    file/filecalendar.cpp
    file/icalformatkio.cpp
    model/tasksmodel.cpp
    model/tasksmodelitem.cpp
    model/task.cpp
    settings/ktimetrackerconfigdialog.cpp
    desktoptracker.cpp
    edittaskdialog.cpp
    idletimedetector.cpp
    export/totalsastext.cpp
    ktimetrackerutility.cpp
    timetrackerstorage.cpp
    mainwindow.cpp
    taskview.cpp
    tray.cpp
    focusdetector.cpp
    csvexportdialog.cpp
    plannerparser.cpp
    historydialog.cpp
    treeviewheadercontextmenu.cpp
    timetrackerwidget.cpp
    widgets/searchline.cpp
    main.cpp
    $<TARGET_OBJECTS:pics>
    $<TARGET_OBJECTS:ktimetracker_app_icon>
)

ecm_qt_declare_logging_category(ktimetracker_SRCS
    HEADER ktt_debug.h
    IDENTIFIER KTT_LOG
    CATEGORY_NAME log_ktt
)

configure_file(ktimetracker-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/ktimetracker-version.h @ONLY)

qt5_add_dbus_adaptor(ktimetracker_SRCS
    org.kde.ktimetracker.ktimetracker.xml
    timetrackerwidget.h
    TimeTrackerWidget
    mainadaptor
    MainAdaptor
)

ki18n_wrap_ui(ktimetracker_SRCS
    csvexportdialog.ui
    historydialog.ui
    edittaskdialog.ui
    settings/cfgbehavior.ui
    settings/cfgdisplay.ui
    settings/cfgstorage.ui
)

kconfig_add_kcfg_files(ktimetracker_SRCS settings/ktimetracker.kcfgc)

qt5_add_resources(ktimetracker_SRCS ktimetracker.qrc)

add_library(libktimetracker STATIC ${ktimetracker_SRCS})
target_link_libraries(libktimetracker
    KF5::KCMUtils
    KF5::WindowSystem
    KF5::Notifications
    KF5::IconThemes
    KF5::I18n
    KF5::XmlGui
    KF5::JobWidgets
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::IdleTime
    KF5::DBusAddons
    KF5::CalendarCore
    ${X11_X11_LIB}
)
if(X11_Xscreensaver_LIB)
    target_link_libraries(libktimetracker ${X11_Xscreensaver_LIB})
endif()

add_executable(ktimetracker main.cpp)
target_link_libraries(ktimetracker libktimetracker)

install(TARGETS ktimetracker ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.ktimetracker.ktimetracker.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
install(PROGRAMS ktimetracker.desktop DESTINATION ${KDE_INSTALL_APPDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
