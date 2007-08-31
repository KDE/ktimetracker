/*
 *     Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
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

#include "mainwindow.h"

#include <numeric>

#include <QMenu>
#include <QString>

#include <KAction>
#include <KActionCollection>
#include <KApplication>       // kapp
#include <KConfig>
#include <KConfigDialog>
#include <KDebug>
#include <KFileDialog>
#include <KGlobal>
#include <KIcon>
#include <KIconLoader>
#include <KLocale>            // i18n
#include <KMessageBox>
#include <KPushButton>
#include <KRecentFilesAction>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KStatusBar>         // statusBar()
#include <KUrl>
#include <KXMLGUIFactory>

#include "edithistorydialog.h"
#include "karmutility.h"
#include "ktimetracker.h"
#include "print.h"
#include "task.h"
#include "taskview.h"
#include "timekard.h"
#include "tray.h"

#include "kaccelmenuwatch.h"
#include "timetrackerwidget.h"
#include "ui_cfgbehavior.h"
#include "ui_cfgdisplay.h"
#include "ui_cfgstorage.h"

MainWindow::MainWindow( const QString &icsfile )
  : KParts::MainWindow( 0, Qt::WindowContextHelpButtonHint ),
#ifdef __GNUC__
#warning Port me!
#endif
//    _accel     ( new KAccel( this ) ),
    _watcher   ( new KAccelMenuWatch( _accel, this ) ),
    _totalSum  ( 0 ),
    _sessionSum( 0 )
{
  mainWidget = new TimetrackerWidget( this );
  //mainWidget->setCornerWidget( new KPushButton( KStandardGuiItem::close(), this ), 
  //                             Qt::TopRightCorner );
  setCentralWidget( mainWidget );
  mainWidget->openFile( icsfile );
  mainWidget->showSearchBar( KTimeTrackerSettings::self()->showSearchBar() );

  // status bar
  startStatusBar();

  // popup menus
  makeMenus();
  _watcher->updateMenus();

  // connections
  connect( mainWidget, SIGNAL( totalTimesChanged( long, long ) ),
           this, SLOT( updateTime( long, long ) ) );
  connect( mainWidget, SIGNAL( currentTaskChanged() ),
           this, SLOT( slotSelectionChanged() ) );
  connect( mainWidget, SIGNAL( currentTaskViewChanged() ),
           this, SLOT( slotSelectionChanged() ) );
  connect( mainWidget, SIGNAL( updateButtons() ),
           this, SLOT( slotSelectionChanged() ) );
  connect( mainWidget, SIGNAL( statusBarTextChangeRequested( QString ) ),
                 this, SLOT( setStatusBar( QString ) ) );
  loadGeometry();
  actionRecentFiles->loadEntries( KGlobal::config()->group( "Recent Files" ) );

  // Setup context menu request handling
  connect( mainWidget,
           SIGNAL( contextMenuRequested( const QPoint& ) ),
           this,
           SLOT( taskViewCustomContextMenuRequested( const QPoint& ) ) );

  if ( KTimeTrackerSettings::trayIcon() ) _tray = new KarmTray( this );
  else _tray = new KarmTray( );

  connect( _tray, SIGNAL( quitSelected() ), SLOT( quit() ) );

  connect( mainWidget, SIGNAL( timersActive() ), _tray, SLOT( startClock() ) );
  connect( mainWidget, SIGNAL( timersActive() ), this,  SLOT( enableStopAll() ));
  connect( mainWidget, SIGNAL( timersInactive() ), _tray, SLOT( stopClock() ) );
  connect( mainWidget, SIGNAL( timersInactive() ),  this,  SLOT( disableStopAll()));
  connect( mainWidget, SIGNAL( tasksChanged( const QList<Task*>& ) ),
                      _tray, SLOT( updateToolTip( QList<Task*> ) ));

  slotSelectionChanged();
}

void MainWindow::slotSelectionChanged()
{
  Task* item = mainWidget->currentTask();
  actionDelete->setEnabled( item );
  actionEdit->setEnabled( item );
  actionStart->setEnabled( item && !item->isRunning() && !item->isComplete() );
  actionStop->setEnabled( item && item->isRunning() );
  actionMarkAsComplete->setEnabled( item && !item->isComplete() );
  actionMarkAsIncomplete->setEnabled( item && item->isComplete() );
  actionFocusTracking->setEnabled( mainWidget->currentTaskView() );
  actionFocusTracking->setChecked( 
    mainWidget->currentTaskView() && 
    mainWidget->currentTaskView()->isFocusTrackingActive() 
  );
  actionNew->setEnabled( mainWidget->currentTaskView() );
  actionNewSub->setEnabled( mainWidget->currentTaskView() );
  actionedithistory->setEnabled( mainWidget->currentTaskView() );
  actionResetAll->setEnabled( mainWidget->currentTaskView() );
  actionStartNewSession->setEnabled( mainWidget->currentTaskView() );
  actionExportTimes->setEnabled( mainWidget->currentTaskView() );
  actionExportHistory->setEnabled( mainWidget->currentTaskView() );
  actionImportPlanner->setEnabled( mainWidget->currentTaskView() );
  actionSave->setEnabled( mainWidget->currentTaskView() );
  actionClose->setEnabled( mainWidget->currentTaskView() );
  actionPrint->setEnabled( mainWidget->currentTaskView() );
}

void MainWindow::slotedithistory()
{
  EditHistoryDialog *dlg = new EditHistoryDialog( mainWidget->currentTaskView() );
  dlg->exec();
}

// This is _old_ code, but shows how to enable/disable add comment menu item.
// We'll need this kind of logic when comments are implemented.
//void MainWindow::timeLoggingChanged(bool on)
//{
//  actionAddComment->setEnabled( on );
//}

void MainWindow::setStatusBar(const QString& qs)
{
  statusBar()->showMessage(i18n(qs.toUtf8()));
}

bool MainWindow::save()
{
  kDebug(5970) <<"Saving time data to disk.";
  QString err = mainWidget->saveFile();  // untranslated error msg.
  if (err.isEmpty()) statusBar()->showMessage(i18n("Successfully saved tasks and history"),1807);
  else statusBar()->showMessage(i18n(err.toAscii()),7707); // no msgbox since save is called when exiting
  saveGeometry();
  return true;
}

void MainWindow::exportcsvHistory()
{
  kDebug(5970) <<"Exporting History to disk.";
  QString err=mainWidget->currentTaskView()->exportcsvHistory();
  if (err.isEmpty()) statusBar()->showMessage(i18n("Successfully exported History to CSV-file"),1807);
  else KMessageBox::error(this, err.toAscii());
  saveGeometry();

}

void MainWindow::quit()
{
  if ( mainWidget->closeAllFiles() ) {
    kapp->quit();
  }
}


MainWindow::~MainWindow()
{
  kDebug(5970) <<"MainWindow::~MainWindows: Quitting karm.";
  actionRecentFiles->saveEntries( KGlobal::config()->group( "Recent Files" ) );
  saveGeometry();
}

void MainWindow::enableStopAll()
{
  actionStopAll->setEnabled(true);
}

void MainWindow::disableStopAll()
{
  actionStopAll->setEnabled(false);
}

void MainWindow::slotFocusTracking()
{
  mainWidget->currentTaskView()->toggleFocusTracking();
  actionFocusTracking->setChecked( mainWidget->currentTaskView()->isFocusTrackingActive() );
}

void MainWindow::openFile()
{
  QString fileName = KFileDialog::getOpenFileName( QString(), QString(), this );
  if ( !fileName.isEmpty() ) {
    actionRecentFiles->addUrl( fileName );
    mainWidget->openFile( fileName );
  }
}

void MainWindow::openFile( const KUrl &url )
{
  mainWidget->openFile( url.path() );
}

void MainWindow::showSettingsDialog()
{
  /* show myself b/c if this method was started from tray icon and the window
     is not visible the applications quits after accepting the settings dialog.
   */
  show();

  KConfigDialog *dialog = new KConfigDialog( 
    this, "settings", KTimeTrackerSettings::self() );

  Ui::BehaviorPage *behaviorUi = new Ui::BehaviorPage;
  QWidget *behaviorPage = new QWidget;
  behaviorUi->setupUi( behaviorPage );
  dialog->addPage( behaviorPage, i18n( "Behavior" ), "gear" );

  Ui::DisplayPage *displayUi = new Ui::DisplayPage;
  QWidget *displayPage = new QWidget;
  displayUi->setupUi( displayPage );
  dialog->addPage( displayPage, i18nc( "settings page for customizing user interface", "Display" ), "zoom-original" );

  Ui::StoragePage *storageUi = new Ui::StoragePage;
  QWidget *storagePage = new QWidget;
  storageUi->setupUi( storagePage );
  dialog->addPage( storagePage, i18n( "Storage" ), "kfm" );

  dialog->exec();
  mainWidget->reconfigureFiles();
}

void MainWindow::slotSearchBar()
{
  bool currentVisible = KTimeTrackerSettings::self()->showSearchBar();
  KTimeTrackerSettings::self()->setShowSearchBar( !currentVisible );
  actionSearchBar->setChecked( !currentVisible );
  mainWidget->showSearchBar( !currentVisible );
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
  // FIXME how to handle this?
  //_taskView->stopAllTimers();
  //_taskView->save();
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

void MainWindow::resetAllTimes()
{
  if ( KMessageBox::warningContinueCancel( this, i18n( "Do you really want to reset the time to zero for all tasks?" ),
       i18n( "Confirmation Required" ), KGuiItem( i18n( "Reset All Times" ) ) ) == KMessageBox::Continue )
    mainWidget->currentTaskView()->resetTimeForAllTasks();
}

void MainWindow::makeMenus()
{

  (KStandardAction::openNew( mainWidget, 
                             SLOT( newFile() ), 
                             actionCollection() ))->setIcon( KIcon( "tab-new" ) );
  (void) KStandardAction::open( this, SLOT( openFile() ), actionCollection() );
  actionClose = KStandardAction::close( mainWidget, SLOT( closeFile() ), actionCollection() );
  (void) KStandardAction::quit(  this, SLOT( quit() ),  actionCollection());
  actionPrint = KStandardAction::print( this, SLOT( print() ), actionCollection());
  actionKeyBindings = KStandardAction::keyBindings( this, SLOT( keyBindings() ),
      actionCollection() );
  actionPreferences = KStandardAction::preferences( this,
    SLOT( showSettingsDialog() ),
    actionCollection() );
  actionSave = KStandardAction::save( this, SLOT( save() ), actionCollection() );
  actionRecentFiles = KStandardAction::openRecent( this, SLOT( openFile( const KUrl &) ), this );
  actionCollection()->addAction( actionRecentFiles->objectName(), actionRecentFiles );

  // Start New Session
  actionStartNewSession  = new KAction(i18n("Start &New Session"), this);
  actionCollection()->addAction("start_new_session", actionStartNewSession );
  connect(actionStartNewSession, SIGNAL(triggered(bool)), SLOT( startNewSession() ));
  actionStartNewSession->setToolTip( i18n("Start a new session") );
  actionStartNewSession->setWhatsThis( i18n("This will reset the session time "
                                            "to 0 for all tasks, to start a "
                                            "new session, without affecting "
                                            "the totals.") );

  // Edit history
  actionedithistory = new KAction( KIcon( "history" ), i18n("Edit History..."), this );
  connect(actionedithistory, SIGNAL(triggered(bool)), SLOT (slotedithistory()));
  actionCollection()->addAction("edit_history", actionedithistory );

  // Reset all times
  actionResetAll  = new KAction(i18n("&Reset All Times"), this);
  actionCollection()->addAction("reset_all_times", actionResetAll );
  connect(actionResetAll, SIGNAL(triggered(bool)), SLOT( resetAllTimes() ));
  actionResetAll->setToolTip( i18n("Reset all times") );
  actionResetAll->setWhatsThis( i18n("This will reset the session and total "
                                     "time to 0 for all tasks, to restart from "
                                     "scratch.") );

  // Start timing
  actionStart  = new KAction(KIcon(QString::fromLatin1("arrow-right")), i18n("&Start"), this);
  actionCollection()->addAction("start", actionStart );
  connect(actionStart, SIGNAL(triggered(bool) ), mainWidget, SLOT( startCurrentTimer() ));
  actionStart->setShortcut(QKeySequence(Qt::Key_S));
  actionStart->setToolTip( i18n("Start timing for selected task") );
  actionStart->setWhatsThis( i18n("This will start timing for the selected "
                                  "task.\n"
                                  "It is even possible to time several tasks "
                                  "simultaneously.\n\n"
                                  "You may also start timing of a tasks by "
                                  "double clicking the left mouse "
                                  "button on a given task. This will, however, "
                                  "stop timing of other tasks."));

  // stop timing
  actionStop  = new KAction(KIcon(QString::fromLatin1("process-stop")), i18n("S&top"), this);
  actionCollection()->addAction("stop", actionStop );
  connect(actionStop, SIGNAL(triggered(bool) ), mainWidget, SLOT( stopCurrentTimer() ));
  actionStop->setShortcut(QKeySequence(Qt::Key_S));
  actionStop->setToolTip( i18n("Stop timing of the selected task") );
  actionStop->setWhatsThis( i18n("Stop timing of the selected task") );

  // Stop all timers
  actionStopAll  = new KAction(i18n("Stop &All Timers"), this);
  actionCollection()->addAction("stopAll", actionStopAll );
  connect(actionStopAll, SIGNAL(triggered(bool)), mainWidget, SLOT( stopAllTimers() ));
  actionStopAll->setShortcut(QKeySequence(Qt::Key_Escape));
  actionStopAll->setEnabled(false);
  actionStopAll->setToolTip( i18n("Stop all of the active timers") );
  actionStopAll->setWhatsThis( i18n("Stop all of the active timers") );

  // Focus tracking
  actionFocusTracking = new KAction(i18n("Track active applications"), this);
  actionFocusTracking->setCheckable( true );
  actionFocusTracking->setChecked( mainWidget->currentTaskView()->isFocusTrackingActive() );
  actionCollection()->addAction("focustracking", actionFocusTracking );
  connect( actionFocusTracking, SIGNAL( triggered( bool ) ),
           this, SLOT( slotFocusTracking() ) );

  // New task
  actionNew  = new KAction( KIcon( QString::fromLatin1( "document-new" ) ), 
                            i18n( "&New Task..." ), this );
  actionCollection()->addAction( "new_task", actionNew );
  connect( actionNew, SIGNAL( triggered( bool ) ), 
           mainWidget, SLOT( newTask() ) );
  actionNew->setShortcut( QKeySequence( Qt::CTRL+Qt::Key_T ) );
  actionNew->setToolTip( i18n( "Create new top level task" ) );
  actionNew->setWhatsThis( i18n( "This will create a new top level task." ) );

  // New subtask
  actionNewSub  = new KAction(KIcon(QString::fromLatin1("new-subtask")), i18n("New &Subtask..."), this);
  actionCollection()->addAction("new_sub_task", actionNewSub );
  connect(actionNewSub, SIGNAL(triggered(bool) ), mainWidget, SLOT( newSubTask() ));
  actionNewSub->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_N));

  // Delete task
  actionDelete  = new KAction(KIcon(QString::fromLatin1("edit-delete")), i18n("&Delete"), this);
  actionCollection()->addAction("delete_task", actionDelete );
  connect(actionDelete, SIGNAL(triggered(bool) ), mainWidget, SLOT( deleteTask() ));
  actionDelete->setShortcut(QKeySequence(Qt::Key_Delete));
  actionDelete->setToolTip( i18n("Delete selected task") );
  actionDelete->setWhatsThis( i18n("This will delete the selected task and "
                                   "all its subtasks.") );

  // Edit task
  actionEdit  = new KAction(KIcon("edit"), i18n("&Edit..."), this);
  actionCollection()->addAction("edit_task", actionEdit );
  connect(actionEdit, SIGNAL(triggered(bool) ), mainWidget, SLOT( editTask() ));
  actionEdit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
  actionEdit->setToolTip( i18n("Edit name or times for selected task") );
  actionEdit->setWhatsThis( i18n("This will bring up a dialog box where you "
                                 "may edit the parameters for the selected "
                                 "task."));

  // Mark as complete
  actionMarkAsComplete  = new KAction(KIcon( UserIcon( "task-complete.xpm" ) ), i18n("&Mark as Complete"), this);
  actionCollection()->addAction("mark_as_complete", actionMarkAsComplete );
  connect(actionMarkAsComplete, SIGNAL(triggered(bool) ), mainWidget, SLOT( markTaskAsComplete() ));
  actionMarkAsComplete->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_M));

  // Mark as incomplete
  actionMarkAsIncomplete  = new KAction(KIcon( UserIcon( "task-incomplete.xpm" ) ), i18n("&Mark as Incomplete"), this);
  actionCollection()->addAction("mark_as_incomplete", actionMarkAsIncomplete );
  connect(actionMarkAsIncomplete, SIGNAL(triggered(bool) ), mainWidget, SLOT( markTaskAsIncomplete() ));
  actionMarkAsIncomplete->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_M));

  // Export times
  actionExportTimes = new KAction(i18n("&Export Times..."), this);
  actionCollection()->addAction("export_times", actionExportTimes );
  connect(actionExportTimes, SIGNAL(triggered(bool) ), mainWidget, SLOT(exportcsvFile()));

  // Export history
  actionExportHistory = new KAction(i18n("Export &History..."), this);
  actionCollection()->addAction("export_history", actionExportHistory );
  connect(actionExportHistory, SIGNAL(triggered(bool) ), SLOT(exportcsvHistory()));

  // Import tasks from Planner
  actionImportPlanner = new KAction(i18n("Import Tasks From &Planner..."), this);
  actionCollection()->addAction("import_planner", actionImportPlanner );
  connect(actionImportPlanner, SIGNAL(triggered(bool) ), mainWidget, SLOT(importPlanner()));

  // Show/Hide SearchBar
  actionSearchBar = new KAction( i18n( "Show Searchbar"), this );
  actionSearchBar->setCheckable( true );
  actionSearchBar->setChecked( KTimeTrackerSettings::self()->showSearchBar() );
  actionCollection()->addAction( "searchbar", actionSearchBar );
  connect( actionSearchBar, SIGNAL( triggered(bool) ), 
           this, SLOT( slotSearchBar() ) );

  setXMLFile( QString::fromLatin1("karmui.rc") );
  createGUI( 0 );

  actionKeyBindings->setToolTip( i18n("Configure key bindings") );
  actionKeyBindings->setWhatsThis( i18n("This will let you configure key"
                                        "bindings which is specific to karm") );

  slotSelectionChanged();
}

void MainWindow::print()
{
  MyPrinter printer(mainWidget->currentTaskView());
  printer.print();
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
