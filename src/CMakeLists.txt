include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(ktimetracker_SRCS
    export/totalsastext.cpp

    file/filecalendar.cpp
    file/icalformatkio.cpp

    model/event.cpp
    model/eventsmodel.cpp
    model/projectmodel.cpp
    model/task.cpp
    model/tasksmodel.cpp
    model/tasksmodelitem.cpp

    settings/ktimetrackerconfigdialog.cpp

    widgets/searchline.cpp
    widgets/taskswidget.cpp

    csvexportdialog.cpp
    desktoptracker.cpp
    edittaskdialog.cpp
    focusdetector.cpp
    historydialog.cpp
    idletimedetector.cpp
    ktimetrackerutility.cpp
    mainwindow.cpp
    plannerparser.cpp
    taskview.cpp
    timetrackerstorage.cpp
    timetrackerwidget.cpp
    tray.cpp
    treeviewheadercontextmenu.cpp
    $<TARGET_OBJECTS:pics>
    $<TARGET_OBJECTS:ktimetracker_app_icon>
)

ecm_qt_declare_logging_category(ktimetracker_SRCS
    HEADER ktt_debug.h
    IDENTIFIER KTT_LOG
    CATEGORY_NAME log_ktt
)

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
