# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-model
    PRIVATE
    event.cpp
    eventsmodel.cpp
    projectmodel.cpp
    task.cpp
    tasksmodel.cpp
    tasksmodelitem.cpp
)

target_link_libraries(ktt-model
    PRIVATE
    ktt-import
    ktt-deps
)
