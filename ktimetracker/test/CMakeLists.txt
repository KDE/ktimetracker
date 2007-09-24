set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${KDE4_INCLUDE_DIR}/kcal  )


########### next target ###############

set(runscripts_SRCS script.cpp runscripts.cpp )


kde4_add_executable(runscripts TEST ${runscripts_SRCS})

target_link_libraries(runscripts  ${KDE4_KDECORE_LIBS} )


########### next target ###############
set(locking_SRCS locking.cpp lockerthread.cpp )


kde4_add_executable(locking TEST ${locking_SRCS})

target_link_libraries(locking  ${KDE4_KDECORE_LIBS} ${KDE4_KCAL_LIBS} )

