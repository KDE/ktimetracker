# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-model
    PRIVATE
    event.cpp event.h
    eventsmodel.cpp eventsmodel.h
    projectmodel.cpp projectmodel.h
    task.cpp task.h
    tasksmodel.cpp tasksmodel.h
    tasksmodelitem.cpp tasksmodelitem.h
)

target_link_libraries(ktt-model
    PRIVATE
    ktt-import
    ktt-deps
)
