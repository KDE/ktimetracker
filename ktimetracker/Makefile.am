
############ Autoconf-generated variables  ###################
AM_CPPFLAGS = -DQT_NO_CAST_ASCII
INCLUDES = $(all_includes)
LDFLAGS	= $(all_libraries) $(KDE_RPATH)
LDADD =  $(LIB_KDEUI) $(LIBXSS)


bin_PROGRAMS = karm
SUBDIRS = support pics

METASOURCES = AUTO

karm_SOURCES =  adddlg.cpp karm.cpp main.cpp idle.cpp \
	task.cpp top.cpp kaccelmenuwatch.cpp print.cpp docking.cpp\
	preferences.cpp ktimewidget.cpp
noinst_HEADERS = adddlg.h karm.h task.h toolicons.h top.h kaccelmenuwatch.h version.h print.h idle.h docking.h preferences.h ktimewidget.h

KDE_ICON = karm

appsdir = $(kde_appsdir)/Utilities

rcdir = $(kde_datadir)/karm
rc_DATA = karmui.rc

install-data-local:
	$(mkinstalldirs) $(appsdir)
	$(INSTALL_DATA) $(srcdir)/support/karm.desktop $(appsdir)

uninstall-local:
	rm -f $(kde_appsdir)/karm.kdesktop

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/karm.pot


srcdoc:
	kdoc -a -p -H -d $(HOME)/web/src/karm karm *.h -lkdecore -lkdeui -lqt


