# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-dialogs
    PRIVATE
    edittimedialog.cpp
    exportdialog.cpp
    historydialog.cpp
    taskpropertiesdialog.cpp
    exportdialog.ui
    historydialog.ui
)

target_link_libraries(ktt-dialogs
    PRIVATE
    ktt-deps
)
