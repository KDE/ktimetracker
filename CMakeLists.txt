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

# Find KF5 package
find_package(KF5I18n ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5KCMUtils ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5WindowSystem ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Notifications ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5ItemViews ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5IconThemes ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5JobWidgets ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5XmlGui ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5KIO ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5DBusAddons ${KF5_MIN_VERSION} CONFIG REQUIRED)

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

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

########### next target ###############
set(ktimetracker_SRCS
	settings/ktimetrackerconfigdialog.cpp
	desktoptracker.cpp
	edittaskdialog.cpp
	idletimedetector.cpp
	timekard.cpp
	ktimetrackerutility.cpp
	timetrackerstorage.cpp
	mainwindow.cpp
	preferences.cpp
	task.cpp
	taskview.cpp
	tray.cpp
	focusdetector.cpp
	csvexportdialog.cpp
	plannerparser.cpp
	historydialog.cpp
	treeviewheadercontextmenu.cpp
	timetrackerwidget.cpp
	focusdetectornotifier.cpp
	kttcalendar.cpp
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

qt5_add_dbus_adaptor(ktimetracker_SRCS
	org.kde.ktimetracker.storage.xml
	taskview.h
	TaskView
	storageadaptor
	StorageAdaptor
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

add_executable(ktimetracker
	${ktimetracker_SRCS}
)

target_link_libraries(ktimetracker
	KF5::KCMUtils
	KF5::WindowSystem
	KF5::Notifications
	KF5::ItemViews
	KF5::IconThemes
	KF5::I18n
	KF5::XmlGui
	KF5::JobWidgets
	KF5::KIOCore
	KF5::KIOWidgets
	KF5::DBusAddons
	KF5::CalendarCore
	${X11_X11_LIB}
)
if(X11_Xscreensaver_LIB)
	target_link_libraries(ktimetracker ${X11_Xscreensaver_LIB})
endif()

install(TARGETS ktimetracker ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############
install(FILES org.kde.ktimetracker.ktimetracker.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
install(PROGRAMS ktimetracker.desktop DESTINATION ${KDE_INSTALL_APPDIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
