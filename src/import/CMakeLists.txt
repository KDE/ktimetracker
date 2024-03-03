# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-import
    PRIVATE
    plannerparser.cpp
)

target_link_libraries(ktt-import
    PRIVATE
    ktt-base
    ktt-deps
)
