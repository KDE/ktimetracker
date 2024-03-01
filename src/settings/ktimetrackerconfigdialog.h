/*
    SPDX-FileCopyrightText: 2009 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTIMETRACKERCONFIGDIALOG_H
#define KTIMETRACKERCONFIGDIALOG_H

#include <KCModule>

class KTimeTrackerBehaviorConfig : public KCModule
{
    Q_OBJECT

public:
    explicit KTimeTrackerBehaviorConfig(QWidget *parent);
};

class KTimeTrackerDisplayConfig : public KCModule
{
    Q_OBJECT

public:
    explicit KTimeTrackerDisplayConfig(QWidget *parent);
};

class KTimeTrackerStorageConfig : public KCModule
{
    Q_OBJECT

public:
    explicit KTimeTrackerStorageConfig(QWidget *parent);
};

#endif /* KTIMETRACKERCONFIGDIALOG_H */
