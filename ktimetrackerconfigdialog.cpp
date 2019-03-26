/*
 *     Copyright (C) 2009 by Laurent Montel <montel@kde.org>
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

KTimeTrackerConfigDialog::KTimeTrackerConfigDialog( const QString &title,
                                                    QWidget *parent )
    : KCMultiDialog( parent )
{
    setWindowTitle( title );
    setStandardButtons( QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    button(QDialogButtonBox::Ok)->setDefault(true);
    setFaceType( KPageDialog::List );
    addModule( "ktimetracker_config_behavior" );
    addModule( "ktimetracker_config_display" );
    addModule( "ktimetracker_config_storage" );
}

void KTimeTrackerConfigDialog::slotOk()
{
    KTimeTrackerSettings::self()->writeConfig();
}

extern "C"
{
    Q_DECL_EXPORT KCModule *create_ktimetracker_config_behavior(QWidget *parent, const char *)
    {
        auto* page = new KTimeTrackerBehaviorConfig(parent);
        page->setObjectName(QStringLiteral("ktimetracker_config_behavior"));
        return page;
    }
    Q_DECL_EXPORT KCModule *create_ktimetracker_config_storage(QWidget *parent, const char *)
    {
        auto* page = new KTimeTrackerStorageConfig(parent);
        page->setObjectName(QStringLiteral("ktimetracker_config_storage"));
        return page;
    }
    Q_DECL_EXPORT KCModule *create_ktimetracker_config_display(QWidget *parent, const char *)
    {
        auto* page = new KTimeTrackerDisplayConfig(parent);
        page->setObjectName(QStringLiteral("ktimetracker_config_display"));
        return page;
    }
}

KTimeTrackerBehaviorConfig::KTimeTrackerBehaviorConfig(QWidget *parent)
    : KCModule(parent)
{
    QHBoxLayout *lay = new QHBoxLayout( this );
    Ui::BehaviorPage *behaviorUi = new Ui::BehaviorPage;
    QWidget *behaviorPage = new QWidget;
    behaviorUi->setupUi( behaviorPage );
    lay->addWidget( behaviorPage );
    addConfig( KTimeTrackerSettings::self(), behaviorPage );
    load();
}

void KTimeTrackerBehaviorConfig::load()
{
    KCModule::load();
}

void KTimeTrackerBehaviorConfig::save()
{
    KCModule::save();
}

KTimeTrackerStorageConfig::KTimeTrackerStorageConfig(QWidget *parent)
    : KCModule(parent)
{
    QHBoxLayout *lay = new QHBoxLayout( this );
    Ui::StoragePage *storageUi = new Ui::StoragePage;
    QWidget *storagePage = new QWidget;
    storageUi->setupUi( storagePage );
    lay->addWidget( storagePage );
    addConfig( KTimeTrackerSettings::self(), storagePage );
    load();
}

void KTimeTrackerStorageConfig::load()
{
    KCModule::load();
}

void KTimeTrackerStorageConfig::save()
{
    KCModule::save();
}

KTimeTrackerDisplayConfig::KTimeTrackerDisplayConfig( QWidget *parent )
    :KCModule( parent )
{
    QHBoxLayout *lay = new QHBoxLayout( this );
    Ui::DisplayPage *displayUi = new Ui::DisplayPage;
    QWidget *displayPage = new QWidget;
    displayUi->setupUi( displayPage );

    lay->addWidget( displayPage );
    addConfig( KTimeTrackerSettings::self(), displayPage );
    load();
}

void KTimeTrackerDisplayConfig::load()
{
    KCModule::load();
}

void KTimeTrackerDisplayConfig::save()
{
    KCModule::save();
}

