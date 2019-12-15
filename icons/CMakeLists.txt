qt5_add_resources(icons_SRCS icons.qrc)
add_library(icons OBJECT ${icons_SRCS})

add_subdirectory(app)
