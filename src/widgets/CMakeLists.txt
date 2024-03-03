# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-widgets
    PRIVATE
    searchline.cpp
    taskswidget.cpp
)

target_link_libraries(ktt-widgets
    PRIVATE
    ktt-deps
)
