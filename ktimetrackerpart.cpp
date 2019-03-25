/*
 *     Copyright (C) 2005 by Thorsten Staerk <kde@staerk.de>
 *                   2007 the ktimetracker developers
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

// TODO: what does totalTimesChanged()?

#include "ktimetrackerpart.h"

#include <QMenu>

#include <KAboutData>
#include <QAction>
#include <KLocalizedString>
#include <KStandardAction>

#include <KXMLGUIFactory>
#include <KActionCollection>
#include <KShortcutsDialog>

#include <kpluginfactory.h>
#include <QStandardPaths>
#include "ktimetrackerutility.h"
#include "task.h"
#include "preferences.h"
#include "tray.h"
#include "ktimetracker-version.h"
#include "ktimetracker.h"
#include "timetrackerwidget.h"

K_PLUGIN_FACTORY(ktimetrackerPartFactory, registerPlugin<ktimetrackerpart>();)
K_EXPORT_PLUGIN( ktimetrackerPartFactory("ktimetracker","ktimetracker") )

ktimetrackerpart::ktimetrackerpart( QWidget *parentWidget, QObject *parent, const QVariantList& )
    : KParts::ReadWritePart(parent)
{
    qCDebug(KTT_LOG) << "Entering function";
    // we need an instance
    mMainWidget = new TimetrackerWidget( parentWidget );
    setWidget( mMainWidget );
    setXMLFile( "ktimetrackerui.rc" );
    makeMenus();
}

ktimetrackerpart::~ktimetrackerpart()
{
}

//KAboutData *ktimetrackerpart::createAboutData()
//{
//    const QByteArray& appname=QByteArray("ktimetracker");
//    const QByteArray& catalogname=QByteArray("ktimetracker");
//    const KLocalizedString localizedname=ki18n("ktimetracker");
//    const QByteArray version=QByteArray(KTIMETRACKER_VERSION);
//    KAboutData* aboutData=new KAboutData( appname, catalogname, localizedname, version);
//    return aboutData;
//}

void ktimetrackerpart::makeMenus()
{
    mMainWidget->setupActions( actionCollection() );
    QAction *actionKeyBindings;
    actionKeyBindings = KStandardAction::keyBindings( this, SLOT(keyBindings()),
        actionCollection() );
    // Tool tips must be set after the createGUI.
    actionKeyBindings->setToolTip( i18n("Configure key bindings") );
    actionKeyBindings->setWhatsThis( i18n("This will let you configure key"
                                        "bindings which are specific to ktimetracker") );
}

void ktimetrackerpart::keyBindings()
{
  KShortcutsDialog::configure( actionCollection(),
                               KShortcutsEditor::LetterShortcutsAllowed );
}

void ktimetrackerpart::setStatusBar(const QString & qs)
{
    qCDebug(KTT_LOG) << "Entering function";
    emit setStatusBarText(qs);
}

bool ktimetrackerpart::openFile(QString icsfile)
{
    mMainWidget->openFile(icsfile);
    emit setWindowCaption(icsfile);

    // connections
    connect( mMainWidget, SIGNAL(statusBarTextChangeRequested(QString)),
           this, SLOT(setStatusBar(QString)) );
    connect( mMainWidget, SIGNAL(setCaption(QString)),
           this, SIGNAL(setWindowCaption(QString)) );
    return true;
}

bool ktimetrackerpart::openFile()
{
    return openFile(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + QString::fromLatin1( "ktimetracker/ktimetracker.ics" ) );
}

bool ktimetrackerpart::saveFile()
{
    mMainWidget->saveFile();
    return true;
}

#include "ktimetrackerpart.moc"
