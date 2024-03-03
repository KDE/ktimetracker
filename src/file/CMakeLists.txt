# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-file
    PRIVATE
    filecalendar.cpp filecalendar.h
    icalformatkio.cpp icalformatkio.h
)

target_link_libraries(ktt-file
    PRIVATE
    ktt-deps
)
