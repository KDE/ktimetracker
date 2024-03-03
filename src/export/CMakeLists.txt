# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-export
    PRIVATE
    csveventlog.cpp
    csvhistory.cpp
    csvtotals.cpp
    export.cpp
    totalsastext.cpp
)

target_link_libraries(ktt-export
    PRIVATE
    ktt-deps
)
