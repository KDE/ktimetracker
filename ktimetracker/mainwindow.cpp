/*
 *     Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
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

#include "mainwindow.h"

#include <numeric>

#include <QMenu>
#include <QString>

#include <KAction>
#include <KApplication>       // kapp
#include <KDebug>
#include <KGlobal>
#include <KIcon>
#include <KLocale>            // i18n
#include <KMessageBox>
#include <KPushButton>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KStatusBar>         // statusBar()
#include <KXMLGUIFactory>
#include <KActionCollection>

#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "task.h"
#include "taskview.h"
#include "timekard.h"
#include "tray.h"

#include "timetrackerwidget.h"

MainWindow::MainWindow( const QString &icsfile )
  :  KParts::MainWindow( )
{
  kDebug(5970) << "Entering function";
  // Setup our actions
  setupActions();

  // this routine will find and load our Part.
  KLibFactory *factory = KLibLoader::self()->factory("ktimetrackerpart");
  if (factory)
  {
    // now that the Part is loaded, we cast it to a Part to get
    // our hands on it
    m_part = static_cast<KParts::ReadWritePart *>
       (factory->create(this, "ktimetrackerpart" ));

    if (m_part)
    {
      // tell the KParts::MainWindow that this is indeed
      // the main widget
      setCentralWidget(m_part->widget());

      setupGUI(ToolBar | Keys | StatusBar | Save);
      connect(configureAction, SIGNAL(triggered(bool)),
        m_part->widget(), SLOT(showSettingsDialog()));
      // and integrate the part's GUI with the shell's
      setXMLFile( QString::fromLatin1( "ktimetrackerui.rc" ) );
      createGUI(m_part);
    }
  }
  else
  {
    // if we couldn't find our Part, we exit since the Shell by
    // itself can't do anything useful
    KMessageBox::error(this, "Could not find our Part!");
    qApp->quit();
    // we return here, cause qApp->quit() only means "exit the
    // next time we enter the event loop...
    return;
  }
  setWindowFlags( windowFlags() | Qt::WindowContextHelpButtonHint );

  slotSetCaption( icsfile );  // set the window title to our iCal file

  // connections
  connect( mainWidget, SIGNAL( statusBarTextChangeRequested( QString ) ),
                 this, SLOT( setStatusBar( QString ) ) );
  connect( mainWidget, SIGNAL( setCaption( const QString& ) ),
                 this, SLOT( slotSetCaption( const QString& ) ) );
  loadGeometry();

  // Setup context menu request handling
  connect( mainWidget,
           SIGNAL( contextMenuRequested( const QPoint& ) ),
           this,
           SLOT( taskViewCustomContextMenuRequested( const QPoint& ) ) );

  _tray = new TrayIcon( this );

  connect( _tray, SIGNAL( quitSelected() ), SLOT( quit() ) );

  connect( m_part->widget(), SIGNAL( timersActive() ), _tray, SLOT( startClock() ) );
  connect( m_part->widget(), SIGNAL( timersInactive() ), _tray, SLOT( stopClock() ) );
  connect( m_part->widget(), SIGNAL( tasksChanged( const QList<Task*>& ) ),
                      _tray, SLOT( updateToolTip( QList<Task*> ) ));
}

void MainWindow::setupActions()
{
    KStandardAction::open(this, SLOT(fileOpen()),
        actionCollection());
    KStandardAction::quit(qApp, SLOT(closeAllWindows()),
        actionCollection());
    configureAction = new KAction(this);
    configureAction->setText(i18n("Configure Ktimetracker"));
    actionCollection()->addAction("configure_ktimetracker", configureAction);
}

void MainWindow::readProperties( const KConfigGroup &cfg )
{
  if( cfg.readEntry( "WindowShown", true ))   
    show();
}

void MainWindow::saveProperties( KConfigGroup &cfg )
{
  cfg.writeEntry( "WindowShown", isVisible());   
}

void MainWindow::slotSetCaption( const QString& qs )
{
  setCaption( qs );
}

void MainWindow::setStatusBar(const QString& qs)
{
  statusBar()->showMessage(i18n(qs.toUtf8()));
}

void MainWindow::quit()
{
  kDebug() << "Entering function";
  if ( mainWidget->closeAllFiles() ) 
  {
    kapp->quit();
  }
}


MainWindow::~MainWindow()
{
  kDebug(5970) << "MainWindow::~MainWindows: Quitting ktimetracker.";
  saveGeometry();
}

void MainWindow::keyBindings()
{
  KShortcutsDialog::configure( actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, this );
}

void MainWindow::startNewSession()
{
  mainWidget->currentTaskView()->startNewSession();
}

void MainWindow::makeMenus()
{
  mainWidget->setupActions( actionCollection() );

  actionKeyBindings = KStandardAction::keyBindings( this, SLOT( keyBindings() ),
      actionCollection() );

  setupGUI();

  actionKeyBindings->setToolTip( i18n( "Configure key bindings" ) );
  actionKeyBindings->setWhatsThis( i18n( "This will let you configure key"
                                         "bindings which are specific to ktimetracker" ) );
}

void MainWindow::loadGeometry()
{
  if (initialGeometrySet()) setAutoSaveSettings();
  else
  {
    KConfigGroup config = KGlobal::config()->group( QString::fromLatin1("Main Window Geometry") );
    int w = config.readEntry( QString::fromLatin1("Width"), 100 );
    int h = config.readEntry( QString::fromLatin1("Height"), 100 );
    w = qMax( w, sizeHint().width() );
    h = qMax( h, sizeHint().height() );
    resize(w, h);
  }
}


void MainWindow::saveGeometry()
{
  KConfigGroup config = KGlobal::config()->group( QString::fromLatin1("Main Window Geometry") );
  config.writeEntry( QString::fromLatin1("Width"), width());
  config.writeEntry( QString::fromLatin1("Height"), height());
  config.sync();
}

bool MainWindow::queryClose()
{
  if ( !kapp->sessionSaving() ) {
    hide();
    return false;
  }
  return KMainWindow::queryClose();
}

void MainWindow::taskViewCustomContextMenuRequested( const QPoint& point )
{
    QMenu* pop = dynamic_cast<QMenu*>(
                          factory()->container( i18n( "task_popup" ), this ) );
    if ( pop )
      pop->popup( point );
}

#include "mainwindow.moc"
