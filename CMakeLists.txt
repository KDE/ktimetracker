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
#find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED DBus Network Test Widgets WebEngine WebEngineWidgets)
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED DBus)

# Find KF5 package
find_package(KF5I18n ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5KCMUtils ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Parts ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5WindowSystem ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Notifications ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5ItemViews ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5IconThemes ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5JobWidgets ${KF5_MIN_VERSION} CONFIG REQUIRED)

# Find KdepimLibs Package
find_package(KF5CalendarCore ${KCALENDARCORE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5KontactInterface ${KONTACTINTERFACE_LIB_VERSION} CONFIG REQUIRED)

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

add_subdirectory(support)
add_subdirectory(pics)
add_subdirectory(icons)

include_directories(
	${CMAKE_SOURCE_DIR}/kresources/remote
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

########### next target ###############
set(ktimetracker_shared_SRCS
    ktimetrackerconfigdialog.cpp
	desktoptracker.cpp
    ktimetrackerpart.cpp
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
	$<TARGET_OBJECTS:pics>
)

ecm_qt_declare_logging_category(ktimetracker_shared_SRCS
	HEADER ktt_debug.h
	IDENTIFIER KTT_LOG
	CATEGORY_NAME log_ktt
)

configure_file(ktimetracker-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/ktimetracker-version.h @ONLY)

qt5_add_dbus_adaptor( ktimetracker_shared_SRCS
        org.kde.ktimetracker.ktimetracker.xml
        timetrackerwidget.h
        TimetrackerWidget
        mainadaptor
        MainAdaptor )

qt5_add_dbus_adaptor( ktimetracker_shared_SRCS
        org.kde.ktimetracker.storage.xml
        taskview.h
        TaskView
        storageadaptor
        StorageAdaptor )

ki18n_wrap_ui(ktimetracker_shared_SRCS
	csvexportdialog.ui
	historydialog.ui
	edittaskdialog.ui
	cfgbehavior.ui
	cfgdisplay.ui
	cfgstorage.ui
)

kconfig_add_kcfg_files( ktimetracker_shared_SRCS ktimetracker.kcfgc )

set(ktimetracker_SRCS main.cpp ${ktimetracker_shared_SRCS})

add_executable(ktimetracker
	${ktimetracker_SRCS}
	$<TARGET_OBJECTS:ktimetracker_app_icon>
)


target_link_libraries(ktimetracker
	KF5::KCMUtils
	KF5::Parts
	KF5::WindowSystem
	KF5::Notifications
	KF5::ItemViews
	KF5::IconThemes
	KF5::JobWidgets
	KF5::CalendarCore
	KF5::KontactInterface
	${X11_X11_LIB}
)
if(X11_Xscreensaver_LIB)
	target_link_libraries(ktimetracker  ${X11_Xscreensaver_LIB} )
endif()

install( TARGETS ktimetracker  ${INSTALL_TARGETS_DEFAULT_ARGS} )


###########

set(kcm_ktimetracker_PART_SRCS ${ktimetracker_shared_SRCS})

add_library(kcm_ktimetracker MODULE ${kcm_ktimetracker_PART_SRCS})

target_link_libraries(kcm_ktimetracker
	KF5::KIOCore
	KF5::KCMUtils
	KF5::Parts
	KF5::Notifications
	KF5::WindowSystem
	KF5::ItemViews
	KF5::IconThemes
	KF5::JobWidgets
	KF5::CalendarCore
	${X11_X11_LIB}
)
if(X11_Xscreensaver_LIB)
    target_link_libraries(kcm_ktimetracker  ${X11_Xscreensaver_LIB} )
endif()

install(TARGETS kcm_ktimetracker DESTINATION ${PLUGIN_INSTALL_DIR})



########### next target ###############

set(ktimetrackerpart_PART_SRCS ${ktimetracker_shared_SRCS})

add_library(ktimetrackerpart MODULE ${ktimetrackerpart_PART_SRCS})

target_link_libraries(ktimetrackerpart
	KF5::KCMUtils
	KF5::CalendarCore
	KF5::KIOCore
	KF5::Parts
	KF5::Notifications
	KF5::ItemViews
	KF5::WindowSystem
	KF5::IconThemes
	KF5::JobWidgets
	${X11_X11_LIB}
)
if(X11_Xscreensaver_LIB)
	target_link_libraries(ktimetrackerpart  ${X11_Xscreensaver_LIB})
endif()


install(TARGETS ktimetrackerpart  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############
install( FILES ktimetracker_config_behavior.desktop  ktimetracker_config_display.desktop  ktimetracker_config_storage.desktop DESTINATION ${SERVICES_INSTALL_DIR})

install( FILES ktimetrackerpart.desktop  DESTINATION ${SERVICES_INSTALL_DIR} )
install(FILES ktimetrackerui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/ktimetracker)

install( FILES org.kde.ktimetracker.ktimetracker.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
