
############ Autoconf-generated variables  ###################
INCLUDES = $(all_includes)
LDFLAGS	= $(all_libraries) $(KDE_RPATH)
LDADD =  $(LIB_KDEUI)


bin_PROGRAMS = karm
SUBDIRS = doc support

BUILT_SOURCES =  moc_adddata.cpp moc_adddlg.cpp moc_kaccelmenuwatch.cpp moc_karm.cpp moc_task.cpp moc_top.cpp

karm_SOURCES =  version.h adddata.cpp adddlg.cpp karm.cpp main.cpp \
	task.cpp top.cpp kaccelmenuwatch.h kaccelmenuwatch.cpp \
	adddata.h adddlg.h karm.h task.h toolicons.h top.h \
	$(BUILT_SOURCES) 

DISTCLEANFILES = moc_adddata.cpp moc_adddlg.cpp moc_karm.cpp \
	moc_task.cpp moc_top.cpp


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
	$(XGETTEXT) -C -ktranslate -ki18n -x $(includedir)/kde.pot *.cpp && mv messages.po ../po/karm.pot


srcdoc:
	kdoc -a -p -H -d $(HOME)/web/src/karm karm *.h -lkdecore -lkdeui -lqt

###KMAKE-start (don't edit or delete this block)
moc_adddata.cpp: adddata.h
	$(MOC) adddata.h -o moc_adddata.cpp

moc_adddlg.cpp: adddlg.h
	$(MOC) adddlg.h -o moc_adddlg.cpp

moc_kaccelmenuwatch.cpp: kaccelmenuwatch.h
	$(MOC) kaccelmenuwatch.h -o moc_kaccelmenuwatch.cpp

moc_karm.cpp: karm.h
	$(MOC) karm.h -o moc_karm.cpp

moc_task.cpp: task.h
	$(MOC) task.h -o moc_task.cpp

moc_top.cpp: top.h
	$(MOC) top.h -o moc_top.cpp

###KMAKE-end
