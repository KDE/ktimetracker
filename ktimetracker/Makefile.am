
############ Autoconf-generated variables  ###################
INCLUDES = -I$(top_srcdir)/libkcal -I$(top_srcdir)/kresources/remote -I$(top_srcdir) $(all_includes)

bin_PROGRAMS = karm
SUBDIRS = support pics test

METASOURCES = AUTO

COMPILE_BEFORE_karm = libkdepim kresources

noinst_LTLIBRARIES = libkarm_shared.la

libkarm_shared_la_SOURCES = kaccelmenuwatch.cpp desktoptracker.cpp \
                edittaskdialog.cpp idletimedetector.cpp \
                printdialog.cpp timekard.cpp karmutility.cpp ktimewidget.cpp \
                karmstorage.cpp mainwindow.cpp preferences.cpp print.cpp \
                task.cpp taskview.cpp tray.cpp \
		csvexportdialog_base.ui csvexportdialog.cpp plannerparser.cpp \
		karmdcopiface.skel

karm_SOURCES = main.cpp
karm_LDADD = libkarm_shared.la -lkdeprint $(top_builddir)/libkcal/libkcal.la \
	     $(top_builddir)/kresources/remote/libkcal_resourceremote.la \
             $(top_builddir)/libkdepim/libkdepim.la $(LIBXSS)
karm_LDFLAGS = $(all_libraries) $(KDE_RPATH)
noinst_HEADERS = desktoptracker.h edittaskdialog.h printdialog.h \
                 idletimedetector.h kaccelmenuwatch.h timekard.h \
		 karmutility.h ktimewidget.h karmstorage.h mainwindow.h \
		 preferences.h print.h task.h taskview.h toolicons.h \
		 tray.h version.h csvexportdialog.h plannerparser.h

KDE_ICON = karm

rcdir = $(kde_datadir)/karm
rc_DATA = karmui.rc

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Utilities
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Utilities/karm.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/karm.pot

srcdoc:
	kdoc -a -p -H -d $(HOME)/web/src/karm karm *.h -lkdecore -lkdeui -lqt

DOXYGEN_REFERENCES = kdeui
include $(top_srcdir)/admin/Doxyfile.am

#########################################################################
# KPART SECTION
#########################################################################
kde_module_LTLIBRARIES = libkarmpart.la

# the Part's source, library search path, and link libraries
libkarmpart_la_SOURCES = karm_part.cpp 
libkarmpart_la_LDFLAGS = -module -avoid-version -no-undefined $(KDE_PLUGIN) $(all_libraries)
libkarmpart_la_LIBADD  = libkarm_shared.la $(LIB_KPARTS) $(LIB_KFILE) \
	     -lkdeprint $(top_builddir)/libkcal/libkcal.la \
	     $(top_builddir)/kresources/remote/libkcal_resourceremote.la \
             $(top_builddir)/libkdepim/libkdepim.la $(LIBXSS)

# this is where the desktop file will go 
partdesktopdir   = $(kde_servicesdir)
partdesktop_DATA = karm_part.desktop

# this is where the part's XML-GUI resource file goes
partrcdir   = $(kde_datadir)/karmpart
partrc_DATA = karmui.rc
