cmake_minimum_required(VERSION 3.5)
set(KDEPIM_VERSION_NUMBER "5.10.80")
project(ktimetracker VERSION ${KDEPIM_VERSION_NUMBER})

set(KF5_MIN_VERSION "5.55.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(ECMInstallIcons)
include(KDEInstallDirs)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(FeatureSummary)
include(KDECMakeSettings)

set(QT_REQUIRED_VERSION "5.10.0")
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED DBus)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
	I18n
	Config
	KCMUtils
	WindowSystem
	Notifications
	IconThemes
	JobWidgets
	XmlGui
	KIO
	DBusAddons
	IdleTime
)

# Find KdepimLibs Package
find_package(KF5CalendarCore ${KCALENDARCORE_LIB_VERSION} CONFIG REQUIRED)

add_subdirectory(pics)
add_subdirectory(icons)
add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
