# SPDX-FileCopyrightText: 2024 Thiago Masato Costa Sueto <thiago.sueto@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

target_sources(ktt-settings
    PRIVATE
    ktimetrackerconfigdialog.cpp ktimetrackerconfigdialog.h
    cfgbehavior.ui
    cfgdisplay.ui
    cfgstorage.ui
)

target_link_libraries(ktt-settings
    PRIVATE
    ktt-general
    ktt-deps
)
