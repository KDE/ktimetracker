
############ Autoconf-generated variables  ###################
INCLUDES = $(all_includes)
LDFLAGS	= $(all_libraries) $(KDE_RPATH)
LDADD =  -lkdeui -lkdecore -lqt -lXext -lX11 $(LIBSOCKET)


bin_PROGRAMS = karm

karm_SOURCES =  adddata.cpp adddlg.cpp karm.cpp main.cpp task.cpp top.cpp \
	 moc_adddata.cpp moc_adddlg.cpp moc_karm.cpp moc_task.cpp \
	 moc_top.cpp 

DISTCLEANFILES = moc_adddata.cpp moc_adddlg.cpp moc_karm.cpp \
	moc_task.cpp moc_top.cpp

noinst_HEADERS = adddata.h adddlg.h karm.h task.h toolicons.h top.h

SUBDIRS = doc support

icon_DATA = karm.xpm
icondir = $(kde_icondir)

EXTRA_DIST = $(icon_DATA) mini-karm.xpm

appsdir = $(kde_appsdir)/Utilities

install-data-local:
	$(mkinstalldirs) $(appsdir)
	$(INSTALL_DATA) support/KArm.kdelnk $(appsdir)
	$(mkinstalldirs) $(kde_toolbardir)
	$(INSTALL_DATA) support/filedel.xpm $(kde_toolbardir)
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) mini-karm.xpm $(kde_minidir)/karm.xpm

uninstall-local:
	rm -f $(kde_appsdir)/Utilities/KArm.kdelnk
	rm -f $(kde_minidir)/karm.xpm
	rm -f $(kde_toolbardir)/filedel.xpm
	-rmdir -p $(kde_toolbardir)
	-rmdir -p $(kde_minidir)
	-rmdir -p $(kde_appsdir)

messages:
	@XGETTEXT@ -C --keyword=translate -x ../../kdelibs/po/kde.pot $(karm_SOURCES) && mv messages.po ../po/karm.pot


### Run a-Moc -- kmake-generated moc rules ####

moc_adddata.cpp: adddata.h
	$(MOC) adddata.h -o moc_adddata.cpp
moc_adddlg.cpp: adddlg.h
	$(MOC) adddlg.h -o moc_adddlg.cpp
moc_karm.cpp: karm.h
	$(MOC) karm.h -o moc_karm.cpp
moc_task.cpp: task.h
	$(MOC) task.h -o moc_task.cpp
moc_top.cpp: top.h
	$(MOC) top.h -o moc_top.cpp

