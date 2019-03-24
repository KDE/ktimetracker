install( FILES filedel.xpm clock.xpm clockedit.xpm empty-watch.xpm active-icon-0.xpm active-icon-1.xpm active-icon-2.xpm  	active-icon-3.xpm active-icon-4.xpm active-icon-5.xpm  	active-icon-6.xpm active-icon-7.xpm  	task-complete.xpm task-incomplete.xpm  DESTINATION ${DATA_INSTALL_DIR}/ktimetracker/pics)

qt5_add_resources(pics_SRCS pics.qrc)
add_library(pics OBJECT ${pics_SRCS})
set_property(TARGET pics PROPERTY POSITION_INDEPENDENT_CODE 1)
