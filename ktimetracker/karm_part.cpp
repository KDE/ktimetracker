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

#include <QByteArray>
#include <QFile>
#include <QMenu>
#include <QTextStream>
#include <QtDBus>

#include <KAboutData>
#include <KAction>
#include <KActionCollection>
#include <KComponentData>
#include <KFileDialog>
#include <KGlobal>
#include <KIcon>
#include <KLocale>
#include <KStandardAction>
#include <KStandardDirs>
#include <KXMLGUIFactory>

#include <kdemacros.h>

#include "mainwindow.h"
#include "kaccelmenuwatch.h"
#include "karmerrors.h"
#include "task.h"
#include "preferences.h"
#include "tray.h"
#include "version.h"
#include "ktimetracker.h"
#include "timetrackerwidget.h"

karmPart::karmPart( QWidget *parentWidget, QObject *parent )
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
                                        "bindings which is specific to ktimetracer") );
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

// It's usually safe to leave the factory code alone.. with the
// notable exception of the KAboutData data
KComponentData *karmPartFactory::s_instance = 0L;
KAboutData* karmPartFactory::s_about = 0L;

karmPartFactory::karmPartFactory()
    : KParts::Factory()
{
}

karmPartFactory::~karmPartFactory()
{
    delete s_instance;
    delete s_about;

    s_instance = 0L;
}

KParts::Part* karmPartFactory::createPartObject( QWidget *parentWidget, QObject *parent,
                                                 const char* classname, const QStringList &args )
{
  Q_UNUSED( args );

  // Create an instance of our Part
  karmPart* obj = new karmPart( parentWidget, parent );

  // See if we are to be read-write or not
  if ( QLatin1String( classname ) == QLatin1String( "KParts::ReadOnlyPart" ) )
      obj->setReadWrite(false);

  return obj;
}

const KComponentData &karmPartFactory::componentData()
{
    if( !s_instance )
    {
        s_about = new KAboutData("karmpart", 0, ki18n("karmPart"), "0.1");
        s_about->addAuthor(ki18n("Thorsten Staerk"), KLocalizedString(), "thorsten@staerk.de");
        s_instance = new KComponentData(s_about);
    }
    return *s_instance;
}

extern "C"
{
    KDE_EXPORT void* init_libkarmpart()
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
