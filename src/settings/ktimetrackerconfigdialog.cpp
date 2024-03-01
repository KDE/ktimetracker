/*
    SPDX-FileCopyrightText: 2009 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ktimetrackerconfigdialog.h"

#include <QHBoxLayout>
#include <QPushButton>

#include "ktimetracker.h"
#include "ui_cfgbehavior.h"
#include "ui_cfgdisplay.h"
#include "ui_cfgstorage.h"

KTimeTrackerBehaviorConfig::KTimeTrackerBehaviorConfig(QWidget *parent)
    : KCModule(parent)
{
    auto *lay = new QHBoxLayout(widget());
    auto *behaviorUi = new Ui::BehaviorPage;
    auto *behaviorPage = new QWidget;
    behaviorUi->setupUi(behaviorPage);
    lay->addWidget(behaviorPage);
    addConfig(KTimeTrackerSettings::self(), behaviorPage);
    load();
}

KTimeTrackerStorageConfig::KTimeTrackerStorageConfig(QWidget *parent)
    : KCModule(parent)
{
    auto *lay = new QHBoxLayout(widget());
    auto *storageUi = new Ui::StoragePage;
    auto *storagePage = new QWidget;
    storageUi->setupUi(storagePage);
    lay->addWidget(storagePage);
    addConfig(KTimeTrackerSettings::self(), storagePage);
    load();
}

KTimeTrackerDisplayConfig::KTimeTrackerDisplayConfig(QWidget *parent)
    : KCModule(parent)
{
    auto *lay = new QHBoxLayout(widget());
    auto *displayUi = new Ui::DisplayPage;
    auto *displayPage = new QWidget;
    displayUi->setupUi(displayPage);
    lay->addWidget(displayPage);
    addConfig(KTimeTrackerSettings::self(), displayPage);
    load();
}

#include "moc_ktimetrackerconfigdialog.cpp"
