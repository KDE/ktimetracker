kde4_header()

add_subdirectory( support ) 
add_subdirectory( pics ) 
add_subdirectory( test ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkcal ${CMAKE_SOURCE_DIR}/kresources/remote ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############
set(karm_shared_SRCS 
	kaccelmenuwatch.cpp desktoptracker.cpp 
	edittaskdialog.cpp idletimedetector.cpp 
	printdialog.cpp timekard.cpp karmutility.cpp ktimewidget.cpp
	karmstorage.cpp mainwindow.cpp preferences.cpp print.cpp 
	task.cpp taskview.cpp tray.cpp  
	csvexportdialog.cpp plannerparser.cpp 
	taskviewwhatsthis.cpp
 )
kde4_add_ui3_files(karm_shared_SRCS csvexportdialog_base.ui )
kde4_add_dcop_skels(karm_shared_SRCS karmdcopiface.h )

set(karm_SRCS main.cpp ${karm_shared_SRCS})

kde4_automoc(${karm_SRCS})

kde4_add_executable(karm ${karm_SRCS})

target_link_libraries(karm  ${KDE4_KPARTS_LIBS}  kdeprint kcal kdepim ${X11_Xss_LIB} kcal_resourceremote)

install_targets(/bin karm )


########### next target ###############


########### next target ###############

set(karmpart_PART_SRCS karm_part.cpp ${karm_shared_SRCS})

kde4_automoc(${karmpart_PART_SRCS})

kde4_add_plugin(karmpart WITH_PREFIX ${karmpart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} karmpart )

target_link_libraries(karmpart  ${KDE4_KPARTS_LIBS} kcal ${X11_Xss_LIB} kdeprint kdepim kcal_resourceremote)

install_targets(${PLUGIN_INSTALL_DIR} karmpart )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES karm_part.desktop )
install_files( ${DATA_INSTALL_DIR}/karmpart FILES karmui.rc )
install_files( ${DATA_INSTALL_DIR}/karm FILES karmui.rc )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )

install(FILES uninstall.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}${APPLNK_INSTALL_DIR}/Utilities/ RENAME karm.desktop)


kde4_create_doxygen_docs( REFERENCES kdeui  )

kde4_footer()



#original Makefile.am contents follow:

#
############# Autoconf-generated variables  ###################
#INCLUDES = -I$(top_srcdir)/libkcal -I$(top_srcdir)/kresources/remote -I$(top_srcdir) $(all_includes)
#
#bin_PROGRAMS = karm
#SUBDIRS = support pics test
#
#METASOURCES = AUTO
#
#COMPILE_BEFORE_karm = libkdepim kresources
#
#noinst_LTLIBRARIES = libkarm_shared.la
#
#libkarm_shared_la_SOURCES = kaccelmenuwatch.cpp desktoptracker.cpp \
#                edittaskdialog.cpp idletimedetector.cpp \
#                printdialog.cpp timekard.cpp karmutility.cpp ktimewidget.cpp \
#                karmstorage.cpp mainwindow.cpp preferences.cpp print.cpp \
#                task.cpp taskview.cpp tray.cpp \
#		csvexportdialog_base.ui csvexportdialog.cpp plannerparser.cpp \
#		karmdcopiface.skel taskviewwhatsthis.cpp
#libkarm_shared_la_LIBADD = $(top_builddir)/libkcal/libkcal.la \
#	     $(top_builddir)/kresources/remote/libkcal_resourceremote.la \
#             $(top_builddir)/libkdepim/libkdepim.la $(LIBXSS) -lkdeprint 
#karm_SOURCES = main.cpp
#karm_LDADD = libkarm_shared.la 
#karm_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#noinst_HEADERS = desktoptracker.h edittaskdialog.h printdialog.h \
#                 idletimedetector.h kaccelmenuwatch.h timekard.h \
#		 karmutility.h ktimewidget.h karmstorage.h mainwindow.h \
#		 preferences.h print.h task.h taskview.h toolicons.h \
#		 tray.h version.h csvexportdialog.h plannerparser.h taskviewwhatsthis.h
#
#KDE_ICON = karm
#
#rcdir = $(kde_datadir)/karm
#rc_DATA = karmui.rc
#
#install-data-local: uninstall.desktop
#	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Utilities
#	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Utilities/karm.desktop
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/karm.pot
#
#srcdoc:
#	kdoc -a -p -H -d $(HOME)/web/src/karm karm *.h -lkdecore -lkdeui -lqt
#
#DOXYGEN_REFERENCES = kdeui
#include $(top_srcdir)/admin/Doxyfile.am
#
##########################################################################
## KPART SECTION
##########################################################################
#kde_module_LTLIBRARIES = libkarmpart.la
#
## the Part's source, library search path, and link libraries
#libkarmpart_la_SOURCES = karm_part.cpp 
#libkarmpart_la_LDFLAGS = -module -avoid-version -no-undefined $(KDE_PLUGIN) $(all_libraries)
#libkarmpart_la_LIBADD  = libkarm_shared.la $(LIB_KPARTS)
#
## this is where the desktop file will go 
#partdesktopdir   = $(kde_servicesdir)
#partdesktop_DATA = karm_part.desktop
#
## this is where the part's XML-GUI resource file goes
#partrcdir   = $(kde_datadir)/karmpart
#partrc_DATA = karmui.rc
