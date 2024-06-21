# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
# SPDX-FileCopyrightText: 2021 David Stächele <david@daiwai.de>
# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

add_library(ktt-general STATIC) # Header only + kcfg + logging
add_library(ktt-deps INTERFACE)
add_library(ktt-base STATIC)
add_library(ktt-dialogs STATIC)
add_library(ktt-export STATIC)
add_library(ktt-file STATIC)
add_library(ktt-import STATIC)
add_library(ktt-model STATIC)
add_library(ktt-settings STATIC)
add_library(ktt-widgets STATIC)

target_link_libraries(ktt-deps
    INTERFACE
    Qt6::Widgets
    Qt6::Core5Compat
    KF6::I18n
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::XmlGui
    KF6::CalendarCore
    KF6::ConfigGui
    KF6::IdleTime
    KF6::ConfigWidgets
    KF6::StatusNotifierItem
    KF6::KCMUtils
    KF6::TextWidgets
    KF6::KIOCore
    KF6::IconThemes
)

if(NOT WIN32)
    target_link_libraries(ktt-deps
        INTERFACE
        KF6::WindowSystem
    )
endif()

kconfig_add_kcfg_files(ktt-general settings/ktimetracker.kcfgc)

ecm_qt_declare_logging_category(ktt-general
    HEADER ktt_debug.h
    IDENTIFIER KTT_LOG
    CATEGORY_NAME log_ktt
)

qt_add_dbus_adaptor(ktt-dbus
    # DBus spec
    org.kde.ktimetracker.ktimetracker.xml
    # Header
    base/timetrackerwidget.h
    # Parent class
    TimeTrackerWidget
    # Basename: generates mainadaptor.h/.cpp
    mainadaptor
    # Class name
    MainAdaptor
)

add_executable(ktimetracker)

target_sources(ktimetracker
    PRIVATE
    main.cpp
    ktimetracker.qrc
    ${ktt-dbus}
    $<TARGET_OBJECTS:icons>
)

ki18n_wrap_ui(ktimetracker
    dialogs/exportdialog.ui
    dialogs/historydialog.ui
    settings/cfgbehavior.ui
    settings/cfgdisplay.ui
    settings/cfgstorage.ui
)

# Add icons to the .exe target directly, otherwise
# they are not included on Windows.
ecm_add_app_icon(ktimetracker ICONS
    ${CMAKE_SOURCE_DIR}/icons/breeze-app-icon-png/16-apps-ktimetracker.png
    ${CMAKE_SOURCE_DIR}/icons/breeze-app-icon-png/22-apps-ktimetracker.png
    ${CMAKE_SOURCE_DIR}/icons/breeze-app-icon-png/32-apps-ktimetracker.png
    ${CMAKE_SOURCE_DIR}/icons/breeze-app-icon-png/48-apps-ktimetracker.png
    ${CMAKE_SOURCE_DIR}/icons/breeze-app-icon-png/64-apps-ktimetracker.png
    ${CMAKE_SOURCE_DIR}/icons/breeze-app-icon-png/128-apps-ktimetracker.png
)

target_link_libraries(ktimetracker
    PRIVATE
    ktt-general
    ktt-deps
    ktt-base
    ktt-export
    ktt-import
    ktt-model
)

install(TARGETS ktimetracker ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.ktimetracker.ktimetracker.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})
install(PROGRAMS org.kde.ktimetracker.desktop DESTINATION ${KDE_INSTALL_APPDIR})

add_subdirectory(base)
add_subdirectory(dialogs)
add_subdirectory(export)
add_subdirectory(file)
add_subdirectory(import)
add_subdirectory(model)
add_subdirectory(settings)
add_subdirectory(widgets)
add_subdirectory(pics)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
