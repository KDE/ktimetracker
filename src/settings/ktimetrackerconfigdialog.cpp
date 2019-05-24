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

#include "ktimetrackerconfigdialog.h"
#include <QHBoxLayout>
#include <QPushButton>
#include "ui_cfgbehavior.h"
#include "ui_cfgdisplay.h"
#include "ui_cfgstorage.h"
#include "ktimetracker.h"

KTimeTrackerBehaviorConfig::KTimeTrackerBehaviorConfig(QWidget *parent)
    : KCModule(parent)
{
    auto *lay = new QHBoxLayout(this);
    auto *behaviorUi = new Ui::BehaviorPage;
    QWidget *behaviorPage = new QWidget;
    behaviorUi->setupUi(behaviorPage);
    lay->addWidget(behaviorPage);
    addConfig(KTimeTrackerSettings::self(), behaviorPage);
    load();
}

KTimeTrackerStorageConfig::KTimeTrackerStorageConfig(QWidget *parent)
    : KCModule(parent)
{
    auto *lay = new QHBoxLayout(this);
    auto *storageUi = new Ui::StoragePage;
    QWidget *storagePage = new QWidget;
    storageUi->setupUi(storagePage);
    lay->addWidget(storagePage);
    addConfig(KTimeTrackerSettings::self(), storagePage);
    load();
}

KTimeTrackerDisplayConfig::KTimeTrackerDisplayConfig(QWidget *parent)
    : KCModule(parent)
{
    auto *lay = new QHBoxLayout(this);
    auto *displayUi = new Ui::DisplayPage;
    QWidget *displayPage = new QWidget;
    displayUi->setupUi(displayPage);
    lay->addWidget(displayPage);
    addConfig(KTimeTrackerSettings::self(), displayPage);
    load();
}
