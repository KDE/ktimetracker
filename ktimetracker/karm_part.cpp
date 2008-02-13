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

#include "karm_part.h"

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
#include <kparts/genericfactory.h>
#include "mainwindow.h"
#include "karmerrors.h"
#include "task.h"
#include "preferences.h"
#include "tray.h"
#include "version.h"
#include "ktimetracker.h"
#include "timetrackerwidget.h"

typedef KParts::GenericFactory<karmPart> karmPartFactory;

K_EXPORT_COMPONENT_FACTORY( karmPart, karmPartFactory)

karmPart::karmPart( QWidget *parentWidget, QObject *parent, const QStringList& )
    : KParts::ReadWritePart(parent)
{
  // we need an instance
  setComponentData( karmPartFactory::componentData() );

  mMainWidget = new TimetrackerWidget( parentWidget );
  setWidget( mMainWidget );
  makeMenus();
  mMainWidget->openFile( KStandardDirs::locateLocal( "appdata", 
                        QString::fromLatin1( "karm.ics" ) ) );

  // connections
  connect( mMainWidget, SIGNAL( totalTimesChanged( long, long ) ),
           this, SLOT( updateTime( long, long ) ) );
  connect( mMainWidget, SIGNAL( statusBarTextChangeRequested( QString ) ),
                 this, SLOT( setStatusBar( QString ) ) );

  // Setup context menu request handling
  connect( mMainWidget,
           SIGNAL( contextMenuRequested( const QPoint& ) ),
           this,
           SLOT( taskViewCustomContextMenuRequested( const QPoint& ) ) );

  if ( KTimeTrackerSettings::trayIcon() ) mTray = new TrayIcon( this );
  else mTray = new TrayIcon( );

  connect( mTray, SIGNAL( quitSelected() ), SLOT( quit() ) );

  connect( mMainWidget, SIGNAL( timersActive() ), mTray, SLOT( startClock() ) );
  connect( mMainWidget, SIGNAL( timersInactive() ), mTray, SLOT( stopClock() ) );
  connect( mMainWidget, SIGNAL( tasksChanged( const QList<Task*>& ) ),
           mTray, SLOT( updateToolTip( QList<Task*> ) ));
}

karmPart::~karmPart()
{
}

KAboutData *karmPart::createAboutData()
{
  const QByteArray& ba=QByteArray("test");
  const KLocalizedString name=ki18n("myName");
  KAboutData* aboutData=new KAboutData( ba, ba, name, ba, name);
  return aboutData;
  //return KABCore::createAboutData();
  #warning not implemented
}

void karmPart::makeMenus()
{
  mMainWidget->setupActions( actionCollection() );
  KAction *actionKeyBindings;

  actionKeyBindings = KStandardAction::keyBindings( this, SLOT( keyBindings() ),
      actionCollection() );

  setXMLFile( QString::fromLatin1( "karmui.rc" ) );

  // Tool tops must be set after the createGUI.
  actionKeyBindings->setToolTip( i18n("Configure key bindings") );
  actionKeyBindings->setWhatsThis( i18n("This will let you configure key"
                                        "bindings which are specific to ktimetracker") );
}

void karmPart::setStatusBar(const QString & qs)
{
  kDebug(5970) <<"Entering setStatusBar";
  emit setStatusBarText(qs);
}

bool karmPart::openFile()
{
  mMainWidget->openFile();

  return true;
}

bool karmPart::saveFile()
{
  mMainWidget->saveFile();

  return true;
}

extern "C"
{
    KDE_EXPORT void* init_karmpart()
    {
	KGlobal::locale()->insertCatalog("karm");
        return new karmPartFactory;
    }
}

void karmPart::taskViewCustomContextMenuRequested( const QPoint& point )
{
    QMenu* pop = dynamic_cast<QMenu*>(
                          factory()->container( i18n( "task_popup" ), this ) );
    if ( pop )
      pop->popup( point );
}

#include "karm_part.moc"
