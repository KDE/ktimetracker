
include_directories(
	${CMAKE_SOURCE_DIR}/kaddressbook
	${CMAKE_BINARY_DIR}/kaddressbook
	${CMAKE_SOURCE_DIR}/kaddressbook/common
	${CMAKE_BINARY_DIR}/kaddressbook/common
	${CMAKE_SOURCE_DIR}/kaddressbook/interfaces
	
)


########### next target ###############

set(kcm_ktimetrackerconfig_PART_SRCS 
   kcmktimetrackerconfig.cpp ktimetrackerconfigwidget.cpp )


kde4_add_plugin(kcm_ktimetrackerconfig ${kcm_ktimetrackerconfig_PART_SRCS})



target_link_libraries(kcm_ktimetrackerconfig ${KDE4_KIO_LIBS})

install(TARGETS kcm_ktimetrackerconfig  DESTINATION ${PLUGIN_INSTALL_DIR})



########### install files ###############

#install( FILES ktimetrackerconfig.desktop DESTINATION ${SERVICES_INSTALL_DIR})


