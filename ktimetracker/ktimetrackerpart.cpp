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

#include "ktimetrackerpart.h"

#include <QMenu>

#include <KAboutData>
#include <KAction>
#include <KComponentData>
#include <KGlobal>
#include <KLocale>
#include <KStandardAction>
#include <KStandardDirs>
#include <KXMLGUIFactory>
#include <KActionCollection>

#include <kdemacros.h>
#include <kpluginfactory.h>
#include "mainwindow.h"
#include "ktimetrackerutility.h"
#include "task.h"
#include "preferences.h"
#include "tray.h"
#include "version.h"
#include "ktimetracker.h"
#include "timetrackerwidget.h"

K_PLUGIN_FACTORY(ktimetrackerPartFactory, registerPlugin<ktimetrackerpart>();)
K_EXPORT_PLUGIN( ktimetrackerPartFactory("ktimetracker","ktimetracker") )

ktimetrackerpart::ktimetrackerpart( QWidget *parentWidget, QObject *parent, const QVariantList& )
    : KParts::ReadWritePart(parent)
{
    kDebug(5970) << "Entering function";
    KGlobal::locale()->insertCatalog("ktimetracker");
    KGlobal::locale()->insertCatalog("libkdepim");
    // we need an instance
    mMainWidget = new TimetrackerWidget( parentWidget );
    setWidget( mMainWidget );
    setXMLFile( "ktimetrackerui.rc" );
    makeMenus();
}

ktimetrackerpart::~ktimetrackerpart()
{
}

KAboutData *ktimetrackerpart::createAboutData()
{
    const QByteArray& appname=QByteArray("ktimetracker");
    const QByteArray& catalogname=QByteArray("ktimetracker");
    const KLocalizedString localizedname=ki18n("ktimetracker");
    const QByteArray version=QByteArray(KTIMETRACKER_VERSION);
    KAboutData* aboutData=new KAboutData( appname, catalogname, localizedname, version);
    return aboutData;
}

void ktimetrackerpart::makeMenus()
{
    mMainWidget->setupActions( actionCollection() );
    KAction *actionKeyBindings;
    actionKeyBindings = KStandardAction::keyBindings( this, SLOT( keyBindings() ),
        actionCollection() );
    // Tool tips must be set after the createGUI.
    actionKeyBindings->setToolTip( i18n("Configure key bindings") );
    actionKeyBindings->setWhatsThis( i18n("This will let you configure key"
                                        "bindings which are specific to ktimetracker") );
}

void ktimetrackerpart::setStatusBar(const QString & qs)
{
    kDebug(5970) << "Entering function";
    emit setStatusBarText(qs);
}

bool ktimetrackerpart::openFile(QString icsfile)
{
    mMainWidget->openFile(icsfile);
    emit setWindowCaption(icsfile);
    if ( KTimeTrackerSettings::trayIcon() ) mTray = new TrayIcon( this );
    else mTray = new TrayIcon( );

    // connections
    connect( mMainWidget, SIGNAL( totalTimesChanged( long, long ) ),
           this, SLOT( updateTime( long, long ) ) );
    connect( mMainWidget, SIGNAL( statusBarTextChangeRequested( QString ) ),
           this, SLOT( setStatusBar( QString ) ) );
    connect( mMainWidget, SIGNAL( setCaption( const QString& ) ),
           this, SIGNAL( setWindowCaption( const QString& ) ) );
    connect( mTray, SIGNAL( quitSelected() ), SLOT( quit() ) );
    connect( mMainWidget, SIGNAL( timersActive() ), mTray, SLOT( startClock() ) );
    connect( mMainWidget, SIGNAL( timersInactive() ), mTray, SLOT( stopClock() ) );
    connect( mMainWidget, SIGNAL( tasksChanged( const QList<Task*>& ) ),
           mTray, SLOT( updateToolTip( QList<Task*> ) ));
    return true;
}

bool ktimetrackerpart::openFile()
{
    return openFile(KStandardDirs::locateLocal( "data", QString::fromLatin1( "ktimetracker/ktimetracker.ics" ) ));
}

bool ktimetrackerpart::saveFile()
{
    mMainWidget->saveFile();
    return true;
}

#include "ktimetrackerpart.moc"
