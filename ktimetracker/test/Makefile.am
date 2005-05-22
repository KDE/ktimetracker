METASOURCES = AUTO

check_PROGRAMS = runscripts locking

runscripts_SOURCES = script.cpp runscripts.cpp
runscripts_LDFLAGS = $(all_libraries) $(KDE_RPATH)
runscripts_LDADD = $(LIB_QT) $(LIB_KDECORE)

locking_SOURCES = locking.cpp lockerthread.cpp
locking_LDFLAGS = $(all_libraries) $(KDE_RPATH)
locking_LDADD = $(LIB_QT) $(top_builddir)/libkcal/libkcal.la

KDE_CXXFLAGS = $(USE_EXCEPTIONS)

INCLUDES = -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/libkcal $(all_includes)

TESTS = runscripts locking
