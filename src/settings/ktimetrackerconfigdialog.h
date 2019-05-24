/*
 * Copyright (C) 2009 by Laurent Montel <montel@kde.org>
 * Copyright (C) 2019  Alexander Potashev <aspotashev@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the
 *      Free Software Foundation, Inc.
 *      51 Franklin Street, Fifth Floor
 *      Boston, MA  02110-1301  USA.
 *
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
