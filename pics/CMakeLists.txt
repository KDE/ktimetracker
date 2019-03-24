qt5_add_resources(pics_SRCS pics.qrc)
add_library(pics OBJECT ${pics_SRCS})
set_property(TARGET pics PROPERTY POSITION_INDEPENDENT_CODE 1)
