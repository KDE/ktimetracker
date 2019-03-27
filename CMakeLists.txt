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
	ItemViews
	IconThemes
	JobWidgets
	XmlGui
	KIO
	DBusAddons
)

# Find KdepimLibs Package
find_package(KF5CalendarCore ${KCALENDARCORE_LIB_VERSION} CONFIG REQUIRED)

#We check if X11_Xscreensaver was found
if(X11_Xscreensaver_FOUND)
  message(STATUS "Found the X11 screensaver extension")
  macro_bool_to_01(X11_Xscreensaver_LIB HAVE_LIBXSS)
else()
  message(STATUS "The X11 screensaver extension was NOT found.")
endif()

#add_feature_info("KtimeTracker idle detection" X11_Xscreensaver_FOUND "Measure the screen idle time in KTimeTracker")

# TODO: define HAVE_LIBXSS
#configure_file(config-ktimetracker.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ktimetracker.h )

add_subdirectory(pics)
add_subdirectory(icons)
add_subdirectory(src)


feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
