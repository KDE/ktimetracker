set(ktimetracker_SRCS
    dialogs/edittimedialog.cpp
    dialogs/taskpropertiesdialog.cpp

    export/totalsastext.cpp
    export/csvhistory.cpp
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

qt5_add_dbus_adaptor(ktimetracker_SRCS
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

qt5_add_resources(ktimetracker_SRCS ktimetracker.qrc)

add_library(libktimetracker STATIC ${ktimetracker_SRCS})
target_link_libraries(libktimetracker
    KF5::ConfigWidgets
    KF5::WindowSystem
    KF5::Notifications
    KF5::I18n
    KF5::XmlGui
    KF5::JobWidgets
    KF5::KIOCore
    KF5::IdleTime
    KF5::DBusAddons
    KF5::CalendarCore
    KF5::TextWidgets
)

ecm_add_app_icon(ktimetracker_app_icon_SRCS ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/app/16-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/app/22-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/app/32-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/app/48-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/app/64-apps-ktimetracker.png
    ${CMAKE_CURRENT_SOURCE_DIR}/../icons/app/128-apps-ktimetracker.png
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
