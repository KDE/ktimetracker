
include_directories( ${KDE4_INCLUDE_DIR}/kcal  )


########### next target ###############

set(runscripts_SRCS script.cpp runscripts.cpp )


kde4_add_test_executable(runscripts ${runscripts_SRCS})

target_link_libraries(runscripts  ${KDE4_KDECORE_LIBS} )


########### next target ###############
set(locking_SRCS locking.cpp lockerthread.cpp )


kde4_add_test_executable(locking ${locking_SRCS})

target_link_libraries(locking  ${KDE4_KDECORE_LIBS} ${KDE4_KCAL_LIBS} )

