cmake_minimum_required(VERSION 3.5)
set(KDEPIM_VERSION_NUMBER "5.10.80")
project(ktimetracker VERSION ${KDEPIM_VERSION_NUMBER})

set(KF5_MIN_VERSION "5.54.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

ecm_setup_version(${KDEPIM_VERSION_NUMBER}
    VARIABLE_PREFIX KTIMETRACKER
    VERSION_HEADER src/ktimetracker-version.h
)

set(QT_REQUIRED_VERSION "5.10.0")
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED DBus Gui Widgets Xml)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Config
    ConfigWidgets
    DBusAddons
    DocTools
    I18n
    IdleTime
    JobWidgets
    KIO
    Notifications
    WindowSystem
    XmlGui
    TextWidgets
)
find_package(KF5 5.63.0 REQUIRED COMPONENTS
    CalendarCore
)

add_subdirectory(pics)
add_subdirectory(icons)
add_subdirectory(doc)
add_subdirectory(src)

install(FILES org.kde.ktimetracker.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
