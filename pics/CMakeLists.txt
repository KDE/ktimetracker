# SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

qt_add_resources(pics_SRCS pics.qrc)
add_library(pics OBJECT ${pics_SRCS})
