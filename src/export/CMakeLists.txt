# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-export
    PRIVATE
    csveventlog.cpp csveventlog.h
    csvhistory.cpp csvhistory.h
    csvtotals.cpp csvtotals.h
    dailyreport.cpp dailyreport.h
    export.cpp export.h
    perioddailyreport.cpp perioddailyreport.h
    periodreport.cpp periodreport.h
    totalsastext.cpp totalsastext.h
)

target_link_libraries(ktt-export
    PRIVATE
    ktt-deps
)

