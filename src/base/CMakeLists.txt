# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-general
    PRIVATE
    desktoplist.h
    reportcriteria.h
)

target_link_libraries(ktt-general
    PRIVATE
    ktt-deps
)

target_sources(ktt-base
    PRIVATE
    ${ktt-dbus}
    desktoptracker.cpp desktoptracker.h
    focusdetector.cpp focusdetector.h
    idletimedetector.cpp idletimedetector.h
    ktimetrackerutility.cpp ktimetrackerutility.h
    mainwindow.cpp mainwindow.h
    taskview.cpp taskview.h
    timetrackerstorage.cpp timetrackerstorage.h
    timetrackerwidget.cpp timetrackerwidget.h
    tray.cpp tray.h
    treeviewheadercontextmenu.cpp treeviewheadercontextmenu.h
)

target_link_libraries(ktt-base
    PRIVATE
    ktt-deps
    ktt-widgets
    ktt-settings
    ktt-dialogs
    ktt-file
)
