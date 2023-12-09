set(ktimetracker_SRCS
    dialogs/edittimedialog.cpp
    dialogs/taskpropertiesdialog.cpp

    export/totalsastext.cpp
    export/csvhistory.cpp
    export/csveventlog.cpp
    export/csvtotals.cpp
    export/export.cpp

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

    dialogs/exportdialog.cpp
    desktoptracker.cpp
    focusdetector.cpp
    dialogs/historydialog.cpp
    idletimedetector.cpp
    ktimetrackerutility.cpp
    mainwindow.cpp
    import/plannerparser.cpp
    taskview.cpp
    timetrackerstorage.cpp
    timetrackerwidget.cpp
    tray.cpp
    treeviewheadercontextmenu.cpp
    $<TARGET_OBJECTS:pics>
    $<TARGET_OBJECTS:icons>
)

ecm_qt_declare_logging_category(ktimetracker_SRCS
    HEADER ktt_debug.h
    IDENTIFIER KTT_LOG
    CATEGORY_NAME log_ktt
)

qt_add_dbus_adaptor(ktimetracker_SRCS
    org.kde.ktimetracker.ktimetracker.xml
    timetrackerwidget.h
    TimeTrackerWidget
    mainadaptor
    MainAdaptor
)

ki18n_wrap_ui(ktimetracker_SRCS
    dialogs/exportdialog.ui
    dialogs/historydialog.ui
    settings/cfgbehavior.ui
    settings/cfgdisplay.ui
    settings/cfgstorage.ui
)

kconfig_add_kcfg_files(ktimetracker_SRCS settings/ktimetracker.kcfgc)

qt_add_resources(ktimetracker_SRCS ktimetracker.qrc)

add_library(libktimetracker STATIC ${ktimetracker_SRCS})
target_link_libraries(libktimetracker
    Qt6::Core5Compat
    KF6::ConfigWidgets
    KF6::WindowSystem
    KF6::Notifications
    KF6::StatusNotifierItem
    KF6::I18n
    KF6::XmlGui
    KF6::JobWidgets
    KF6::KIOCore
    KF6::IdleTime
    KF6::DBusAddons
    KF6::CalendarCore
    KF6::TextWidgets
    KF6::KCMUtils
)

ecm_add_app_icon(ktimetracker_app_icon_SRCS ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/breeze-app-icon-png/16-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/breeze-app-icon-png/22-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/breeze-app-icon-png/32-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/breeze-app-icon-png/48-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/breeze-app-icon-png/64-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/breeze-app-icon-png/128-apps-ktimetracker.png
)

# Add ktimetracker_app_icon_SRCS to the .exe target directly, otherwise
# the icon is not included on Windows.
add_executable(ktimetracker
    main.cpp
    ${ktimetracker_app_icon_SRCS}
)
target_link_libraries(ktimetracker libktimetracker)

install(TARGETS ktimetracker ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.ktimetracker.ktimetracker.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})
install(PROGRAMS org.kde.ktimetracker.desktop DESTINATION ${KDE_INSTALL_APPDIR})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
