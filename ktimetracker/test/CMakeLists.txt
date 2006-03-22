kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/libkcal ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(runscripts_SRCS script.cpp runscripts.cpp )

kde4_automoc(${runscripts_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(runscripts ${runscripts_SRCS})

target_link_libraries(runscripts  ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(locking_SRCS locking.cpp lockerthread.cpp )

kde4_automoc(${locking_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(locking ${locking_SRCS})

target_link_libraries(locking  ${KDE4_KDECORE_LIBS} kcal )

endif(KDE4_BUILD_TESTS)

########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#METASOURCES = AUTO
#
#check_PROGRAMS = runscripts locking
#
#runscripts_SOURCES = script.cpp runscripts.cpp
#runscripts_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#runscripts_LDADD = $(LIB_QT) $(LIB_KDECORE)
#
#locking_SOURCES = locking.cpp lockerthread.cpp
#locking_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#locking_LDADD = $(LIB_QT) $(top_builddir)/libkcal/libkcal.la
#
#KDE_CXXFLAGS = $(USE_EXCEPTIONS)
#
#INCLUDES = -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/libkcal $(all_includes)
#
#TESTS = runscripts locking
