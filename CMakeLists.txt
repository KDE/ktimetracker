# SPDX-FileCopyrightText: 2019, 2022, 2023 Alexander Potashev <aspotashev@gmail.com>
# SPDX-FileCopyrightText: 2019 Andreas Sturmlechner <asturm@gentoo.org>
# SPDX-FileCopyrightText: 2019 Friedrich W. H. Kossebau <kossebau@kde.org>
# SPDX-FileCopyrightText: 2019 Pino Toscano <pino@kde.org>
# SPDX-FileCopyrightText: 2022 Christophe Giboudeaux <christophe@krop.fr>
# SPDX-FileCopyrightText: 2023 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-FileCopyrightText: 2024 Konstantin Avadov <konstantin.avadov@yahoo.com>

# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.20)
set(APP_VERSION_NUMBER "6.0.0")
project(ktimetracker VERSION ${APP_VERSION_NUMBER})

set(KF6_MIN_VERSION "5.240.0")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(ECM ${KF6_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)
include(FeatureSummary)

ecm_setup_version(${APP_VERSION_NUMBER}
    VARIABLE_PREFIX KTIMETRACKER
    VERSION_HEADER src/ktimetracker-version.h
)

set(QT_REQUIRED_VERSION "6.4.0")
find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED
    DBus
    Gui
    Widgets
    Xml
    Core5Compat
)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    Config
    ConfigWidgets
    CoreAddons
    DBusAddons
    DocTools
    I18n
    IdleTime
    JobWidgets
    KIO
    Notifications
    StatusNotifierItem
    XmlGui
    TextWidgets
    CalendarCore
    KCMUtils
    IconThemes
)

if(NOT WIN32)
    find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
        WindowSystem
    )
endif()

add_subdirectory(icons)
add_subdirectory(doc)
add_subdirectory(src)

install(FILES org.kde.ktimetracker.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

ki18n_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
