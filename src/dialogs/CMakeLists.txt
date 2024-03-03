# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-dialogs
    PRIVATE
    edittimedialog.cpp edittimedialog.h
    exportdialog.cpp exportdialog.h
    historydialog.cpp historydialog.h
    taskpropertiesdialog.cpp taskpropertiesdialog.h
    exportdialog.ui
    historydialog.ui
)

target_link_libraries(ktt-dialogs
    PRIVATE
    ktt-deps
)
