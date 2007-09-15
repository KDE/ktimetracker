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
#include <KActionCollection>
#include <KApplication>       // kapp
#include <KDebug>
#include <KFileDialog>
#include <KGlobal>
#include <KIcon>
#include <KIconLoader>
#include <KLocale>            // i18n
#include <KMessageBox>
#include <KPushButton>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KStatusBar>         // statusBar()
#include <KUrl>
#include <KXMLGUIFactory>

#include "karmutility.h"
#include "ktimetracker.h"
#include "task.h"
#include "taskview.h"
#include "timekard.h"
#include "tray.h"

#include "timetrackerwidget.h"

MainWindow::MainWindow( const QString &icsfile )
  : KParts::MainWindow( 0, Qt::WindowContextHelpButtonHint ),
    _totalSum  ( 0 ),
    _sessionSum( 0 )
{
  mainWidget = new TimetrackerWidget( this );
  setCentralWidget( mainWidget );
  makeMenus();
  mainWidget->openFile( icsfile );

  // status bar
  startStatusBar();

  // connections
  connect( mainWidget, SIGNAL( totalTimesChanged( long, long ) ),
           this, SLOT( updateTime( long, long ) ) );
  connect( mainWidget, SIGNAL( statusBarTextChangeRequested( QString ) ),
                 this, SLOT( setStatusBar( QString ) ) );
  loadGeometry();

  // Setup context menu request handling
  connect( mainWidget,
           SIGNAL( contextMenuRequested( const QPoint& ) ),
           this,
           SLOT( taskViewCustomContextMenuRequested( const QPoint& ) ) );

  if ( KTimeTrackerSettings::trayIcon() ) _tray = new TrayIcon( this );
  else _tray = new TrayIcon( );

  connect( _tray, SIGNAL( quitSelected() ), SLOT( quit() ) );

  connect( mainWidget, SIGNAL( timersActive() ), _tray, SLOT( startClock() ) );
  connect( mainWidget, SIGNAL( timersInactive() ), _tray, SLOT( stopClock() ) );
  connect( mainWidget, SIGNAL( tasksChanged( const QList<Task*>& ) ),
                      _tray, SLOT( updateToolTip( QList<Task*> ) ));
}

void MainWindow::setStatusBar(const QString& qs)
{
  statusBar()->showMessage(i18n(qs.toUtf8()));
}

// FIXME send saving slot
/*bool MainWindow::save()
{
  kDebug(5970) <<"Saving time data to disk.";
  QString err = mainWidget->saveFile();  // untranslated error msg.
  if (err.isEmpty()) statusBar()->showMessage(i18n("Successfully saved tasks and history"),1807);
  else statusBar()->showMessage(i18n(err.toAscii()),7707); // no msgbox since save is called when exiting
  saveGeometry();
  return true;
}*/

// FIXME
/*void MainWindow::exportcsvHistory()
{
  kDebug(5970) <<"Exporting History to disk.";
  QString err=mainWidget->currentTaskView()->exportcsvHistory();
  if (err.isEmpty()) statusBar()->showMessage(i18n("Successfully exported History to CSV-file"),1807);
  else KMessageBox::error(this, err.toAscii());
  saveGeometry();

}*/

void MainWindow::quit()
{
  if ( mainWidget->closeAllFiles() ) {
    kapp->quit();
  }
}


MainWindow::~MainWindow()
{
  kDebug(5970) <<"MainWindow::~MainWindows: Quitting ktimetracker.";
  saveGeometry();
}

/**
 * Calculate the sum of the session time and the total time for all
 * toplevel tasks and put it in the statusbar.
 */

void MainWindow::updateTime( long sessionDiff, long totalDiff )
{
  _sessionSum += sessionDiff;
  _totalSum   += totalDiff;

  updateStatusBar();
}

void MainWindow::updateStatusBar( )
{
  QString time;

  time = formatTime( _sessionSum );
  statusBar()->changeItem( i18n("Session: %1", time), 0 );

  time = formatTime( _totalSum );
  statusBar()->changeItem( i18nc( "total time of all tasks", "Total: %1", time ), 1);
}

void MainWindow::startStatusBar()
{
  statusBar()->insertPermanentItem( i18n("Session"), 0, 0 );
  statusBar()->insertPermanentItem( i18nc( "total time of all tasks", "Total" ), 1, 0);
}

void MainWindow::saveProperties( KConfigGroup &cfg )
{
  cfg.writeEntry( "WindowShown", isVisible());
}

void MainWindow::readProperties( const KConfigGroup &cfg )
{
  if( cfg.readEntry( "WindowShown", true ))
    show();
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

  setXMLFile( QString::fromLatin1( "karmui.rc" ) );
  createGUI( 0 );

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
