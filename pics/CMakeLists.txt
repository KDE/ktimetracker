install( FILES empty-watch.xpm task-complete.xpm task-incomplete.xpm  DESTINATION ${DATA_INSTALL_DIR}/ktimetracker/pics)

qt5_add_resources(pics_SRCS pics.qrc)
add_library(pics OBJECT ${pics_SRCS})
set_property(TARGET pics PROPERTY POSITION_INDEPENDENT_CODE 1)
