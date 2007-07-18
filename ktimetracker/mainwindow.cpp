/*
* Top Level window for ktimetracker.
* Distributed under the GPL.
*/

#include <numeric>

#include "kaccelmenuwatch.h"
#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>       // kapp
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kshortcutsdialog.h>
#include <klocale.h>            // i18n
#include <kmessagebox.h>
#include <kstatusbar.h>         // statusBar()
#include <kstandardaction.h>
#include <qnamespace.h>
#include <QMenu>
#include <QString>
#include <kicon.h>
#include "karmerrors.h"
#include "karmutility.h"
#include "mainwindow.h"
#include "preferences.h"
#include "print.h"
#include "task.h"
#include "taskview.h"
#include "timekard.h"
#include "tray.h"
#include "version.h"
#include <kxmlguifactory.h>

#include "karmmainadaptor.h"
#include <QtDBus>

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
  new KarmAdaptor(this);
  QDBusConnection::sessionBus().registerObject("/Karm", this);

  _taskView  = new TaskView( this, icsfile );

  setCentralWidget( _taskView );
  // status bar
  startStatusBar();

  // setup PreferenceDialog.
  _preferences = Preferences::instance();

  // popup menus
  makeMenus();
  _watcher->updateMenus();

  // connections
  connect( _taskView, SIGNAL( totalTimesChanged( long, long ) ),
           this, SLOT( updateTime( long, long ) ) );
  connect( _taskView, SIGNAL( itemSelectionChanged () ),
           this, SLOT( slotSelectionChanged() ) );
  connect( _taskView, SIGNAL( updateButtons() ),
           this, SLOT( slotSelectionChanged() ) );
  connect( _taskView, SIGNAL( setStatusBarText( QString ) ),
                 this, SLOT( setStatusBar( QString ) ) );
  loadGeometry();

  // Setup context menu request handling
  _taskView->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( _taskView,
           SIGNAL( customContextMenuRequested( const QPoint& ) ),
           this,
           SLOT( taskViewCustomContextMenuRequested( const QPoint& ) ) );

  if ( _preferences->trayIcon() ) _tray = new KarmTray( this );
  else _tray = new KarmTray( );

  connect( _tray, SIGNAL( quitSelected() ), SLOT( quit() ) );

  connect( _taskView, SIGNAL( timersActive() ), _tray, SLOT( startClock() ) );
  connect( _taskView, SIGNAL( timersActive() ), this,  SLOT( enableStopAll() ));
  connect( _taskView, SIGNAL( timersInactive() ), _tray, SLOT( stopClock() ) );
  connect( _taskView, SIGNAL( timersInactive() ),  this,  SLOT( disableStopAll()));
  connect( _taskView, SIGNAL( tasksChanged( QList<Task*> ) ),
                      _tray, SLOT( updateToolTip( QList<Task*> ) ));

  _taskView->load();

  // Everything that uses Preferences has been created now, we can let it
  // emit its signals
  _preferences->emitSignals();
  slotSelectionChanged();

  // ToDo: Register with DCOP

  // Set up DCOP error messages
  m_error[ KARM_ERR_GENERIC_SAVE_FAILED ] =
    i18n( "Save failed, most likely because the file could not be locked." );
  m_error[ KARM_ERR_COULD_NOT_MODIFY_RESOURCE ] =
    i18n( "Could not modify calendar resource." );
  m_error[ KARM_ERR_MEMORY_EXHAUSTED ] =
    i18n( "Out of memory--could not create object." );
  m_error[ KARM_ERR_UID_NOT_FOUND ] =
    i18n( "UID not found." );
  m_error[ KARM_ERR_INVALID_DATE ] =
    i18n( "Invalidate date--format is YYYY-MM-DD." );
  m_error[ KARM_ERR_INVALID_TIME ] =
    i18n( "Invalid time--format is YYYY-MM-DDTHH:MM:SS." );
  m_error[ KARM_ERR_INVALID_DURATION ] =
    i18n( "Invalid task duration--must be greater than zero." );
}

void MainWindow::slotSelectionChanged()
{
  Task* item= _taskView->current_item();
  actionDelete->setEnabled(item);
  actionEdit->setEnabled(item);
  actionStart->setEnabled(item && !item->isRunning() && !item->isComplete());
  actionStop->setEnabled(item && item->isRunning());
  actionMarkAsComplete->setEnabled(item && !item->isComplete());
  actionMarkAsIncomplete->setEnabled(item && item->isComplete());
}

void MainWindow::slotedithistory()
{
  kDebug(5970) << "This is slotedithistory" << endl;
  _taskView->listallevents();
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
  kDebug(5970) << "Saving time data to disk." << endl;
  QString err=_taskView->save();  // untranslated error msg.
  if (err.isEmpty()) statusBar()->showMessage(i18n("Successfully saved tasks and history"),1807);
  else statusBar()->showMessage(i18n(err.toAscii()),7707); // no msgbox since save is called when exiting
  saveGeometry();
  return true;
}

void MainWindow::exportcsvHistory()
{
  kDebug(5970) << "Exporting History to disk." << endl;
  QString err=_taskView->exportcsvHistory();
  if (err.isEmpty()) statusBar()->showMessage(i18n("Successfully exported History to CSV-file"),1807);
  else KMessageBox::error(this, err.toAscii());
  saveGeometry();

}

void MainWindow::quit()
{
  kapp->quit();
}


MainWindow::~MainWindow()
{
  kDebug(5970) << "MainWindow::~MainWindows: Quitting karm." << endl;
  _taskView->stopAllTimers();
  save();
  _taskView->closeStorage();
}

void MainWindow::enableStopAll()
{
  actionStopAll->setEnabled(true);
}

void MainWindow::disableStopAll()
{
  actionStopAll->setEnabled(false);
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
  statusBar()->changeItem( i18n("Total: %1" , time), 1);
}

void MainWindow::startStatusBar()
{
  statusBar()->insertPermanentItem( i18n("Session"), 0, 0 );
  statusBar()->insertPermanentItem( i18n("Total" ), 1, 0);
}

void MainWindow::saveProperties( KConfigGroup &cfg )
{
  _taskView->stopAllTimers();
  _taskView->save();
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
  _taskView->startNewSession();
}

void MainWindow::resetAllTimes()
{
  if ( KMessageBox::warningContinueCancel( this, i18n( "Do you really want to reset the time to zero for all tasks?" ),
       i18n( "Confirmation Required" ), KGuiItem( i18n( "Reset All Times" ) ) ) == KMessageBox::Continue )
    _taskView->resetTimeForAllTasks();
}

void MainWindow::makeMenus()
{

  (void) KStandardAction::quit(  this, SLOT( quit() ),  actionCollection());
  (void) KStandardAction::print( this, SLOT( print() ), actionCollection());
  actionKeyBindings = KStandardAction::keyBindings( this, SLOT( keyBindings() ),
      actionCollection() );
  actionPreferences = KStandardAction::preferences(_preferences,
      SLOT(showDialog()),
      actionCollection() );
  (void) KStandardAction::save( this, SLOT( save() ), actionCollection() );

  // Start New Session
  QAction *actionStartNewSession  = new KAction(i18n("Start &New Session"), this);
  actionCollection()->addAction("start_new_session", actionStartNewSession );
  connect(actionStartNewSession, SIGNAL(triggered(bool)), SLOT( startNewSession() ));
  actionStartNewSession->setToolTip( i18n("Start a new session") );
  actionStartNewSession->setWhatsThis( i18n("This will reset the session time "
                                            "to 0 for all tasks, to start a "
                                            "new session, without affecting "
                                            "the totals.") );

  // Edit history
  actionedithistory = new KAction(i18n("Edit history"), this);
  connect(actionedithistory, SIGNAL(triggered(bool)), SLOT (slotedithistory()));
  actionCollection()->addAction("edit_history", actionedithistory );

  // Reset all times
  QAction *actionResetAll  = new KAction(i18n("&Reset All Times"), this);
  actionCollection()->addAction("reset_all_times", actionResetAll );
  connect(actionResetAll, SIGNAL(triggered(bool)), SLOT( resetAllTimes() ));
  actionResetAll->setToolTip( i18n("Reset all times") );
  actionResetAll->setWhatsThis( i18n("This will reset the session and total "
                                     "time to 0 for all tasks, to restart from "
                                     "scratch.") );

  // Start timing
  actionStart  = new KAction(KIcon(QString::fromLatin1("arrow-right")), i18n("&Start"), this);
  actionCollection()->addAction("start", actionStart );
  connect(actionStart, SIGNAL(triggered(bool) ), _taskView, SLOT( startCurrentTimer() ));
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
  connect(actionStop, SIGNAL(triggered(bool) ), _taskView, SLOT( stopCurrentTimer() ));
  actionStop->setShortcut(QKeySequence(Qt::Key_S));
  actionStop->setToolTip( i18n("Stop timing of the selected task") );
  actionStop->setWhatsThis( i18n("Stop timing of the selected task") );
  
  // Stop all timers
  actionStopAll  = new KAction(i18n("Stop &All Timers"), this);
  actionCollection()->addAction("stopAll", actionStopAll );
  connect(actionStopAll, SIGNAL(triggered(bool)), _taskView, SLOT( stopAllTimers() ));
  actionStopAll->setShortcut(QKeySequence(Qt::Key_Escape));
  actionStopAll->setEnabled(false);
  actionStopAll->setToolTip( i18n("Stop all of the active timers") );
  actionStopAll->setWhatsThis( i18n("Stop all of the active timers") );

  // New task
  actionNew  = new KAction(KIcon(QString::fromLatin1("document-new")), i18n("&New..."), this);
  actionCollection()->addAction("new_task", actionNew );
  connect(actionNew, SIGNAL(triggered(bool) ), _taskView, SLOT( newTask() ));
  actionNew->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_N));

  // New subtask
  actionNewSub  = new KAction(KIcon(QString::fromLatin1("kmultiple")), i18n("New &Subtask..."), this);
  actionCollection()->addAction("new_sub_task", actionNewSub );
  connect(actionNewSub, SIGNAL(triggered(bool) ), _taskView, SLOT( newSubTask() ));
  actionNewSub->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_N));
  actionNew->setToolTip( i18n("Create new top level task") );
  actionNew->setWhatsThis( i18n("This will create a new top level task.") );

  // Delete task
  actionDelete  = new KAction(KIcon(QString::fromLatin1("edit-delete")), i18n("&Delete"), this);
  actionCollection()->addAction("delete_task", actionDelete );
  connect(actionDelete, SIGNAL(triggered(bool) ), _taskView, SLOT( deleteTask() ));
  actionDelete->setShortcut(QKeySequence(Qt::Key_Delete));
  actionDelete->setToolTip( i18n("Delete selected task") );
  actionDelete->setWhatsThis( i18n("This will delete the selected task and "
                                   "all its subtasks.") );

  // Edit task
  actionEdit  = new KAction(KIcon("edit"), i18n("&Edit..."), this);
  actionCollection()->addAction("edit_task", actionEdit );
  connect(actionEdit, SIGNAL(triggered(bool) ), _taskView, SLOT( editTask() ));
  actionEdit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
  actionEdit->setToolTip( i18n("Edit name or times for selected task") );
  actionEdit->setWhatsThis( i18n("This will bring up a dialog box where you "
                                 "may edit the parameters for the selected "
                                 "task."));

  // Mark as complete
  actionMarkAsComplete  = new KAction(KIcon("document"), i18n("&Mark as Complete"), this);
  actionCollection()->addAction("mark_as_complete", actionMarkAsComplete );
  connect(actionMarkAsComplete, SIGNAL(triggered(bool) ), _taskView, SLOT( markTaskAsComplete() ));
  actionMarkAsComplete->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_M));

  // Mark as incomplete
  actionMarkAsIncomplete  = new KAction(KIcon("document"), i18n("&Mark as Incomplete"), this);
  actionCollection()->addAction("mark_as_incomplete", actionMarkAsIncomplete );
  connect(actionMarkAsIncomplete, SIGNAL(triggered(bool) ), _taskView, SLOT( markTaskAsIncomplete() ));
  actionMarkAsIncomplete->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_M));

  // Export times
  QAction *action  = new KAction(i18n("&Export Times..."), this);
  actionCollection()->addAction("export_times", action );
  connect(action, SIGNAL(triggered(bool) ), _taskView, SLOT(exportcsvFile()));

  // Export history
  action  = new KAction(i18n("Export &History..."), this);
  actionCollection()->addAction("export_history", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(exportcsvHistory()));

  // Import tasks from Planner
  action  = new KAction(i18n("Import Tasks From &Planner..."), this);
  actionCollection()->addAction("import_planner", action );
  connect(action, SIGNAL(triggered(bool) ), _taskView, SLOT(importPlanner()));

  setXMLFile( QString::fromLatin1("karmui.rc") );
  createGUI( 0 );

  actionKeyBindings->setToolTip( i18n("Configure key bindings") );
  actionKeyBindings->setWhatsThis( i18n("This will let you configure key"
                                        "bindings which is specific to karm") );

  slotSelectionChanged();
}

void MainWindow::print()
{
  MyPrinter printer(_taskView);
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
      pop->popup( _taskView->viewport()->mapToGlobal( point ) );
}

//----------------------------------------------------------------------------
//
//                       D C O P   I N T E R F A C E
//
//----------------------------------------------------------------------------

QString MainWindow::version() const
{
  return KTIMETRACKER_VERSION;
}

QString MainWindow::deletetodo()
{
  _taskView->deleteTask();
  return "";
}

bool MainWindow::getpromptdelete()
{
  return _preferences->promptDelete();
}

QString MainWindow::setpromptdelete( bool prompt )
{
  _preferences->setPromptDelete( prompt );
  return "";
}

QString MainWindow::taskIdFromName( const QString &taskname ) const
{
  QString rval = "";

  Task* task = _taskView->first_child();
  while ( rval.isEmpty() && task )
  {
    rval = _hasTask( task, taskname );
    task = task->nextSibling();
  }

  return rval;
}

int MainWindow::addTask( const QString& taskname )
{
  DesktopList desktopList;
  QString uid = _taskView->addTask( taskname, 0, 0, desktopList );
  kDebug(5970) << "MainWindow::addTask( " << taskname << " ) returns " << uid << endl;
  if ( uid.length() > 0 ) return 0;
  else
  {
    // We can't really tell what happened, b/c the resource framework only
    // returns a boolean.
    return KARM_ERR_GENERIC_SAVE_FAILED;
  }
}

QString MainWindow::setPerCentComplete( const QString& taskName, int perCent )
{
  int index = -1;
  QString err="no such task";
  for (int i=0; i<_taskView->count(); i++)
  {
    if ((_taskView->item_at_index(i)->name()==taskName))
    {
      index=i;
      if (err.isNull()) err="task name is abigious";
      if (err=="no such task") err=QString();
    }
  }
  if (err.isNull() && index>=0)
  {
    _taskView->item_at_index(index)->setPercentComplete( perCent, _taskView->storage() );
  }
  return err;
}

int MainWindow::bookTime
( const QString& taskId, const QString& datetime, long minutes )
{
  int rval = 0;
  QDate startDate;
  QTime startTime;
  QDateTime startDateTime;
  Task *task, *t;

  if ( minutes <= 0 ) rval = KARM_ERR_INVALID_DURATION;

  // Find task
  task = _taskView->first_child();
  t = NULL;
  while ( !t && task )
  {
    t = _hasUid( task, taskId );
    task = task->nextSibling();
  }
  if ( t == NULL ) rval = KARM_ERR_UID_NOT_FOUND;

  // Parse datetime
  if ( !rval )
  {
    startDate = QDate::fromString( datetime, Qt::ISODate );
    if ( datetime.length() > 10 )  // "YYYY-MM-DD".length() = 10
    {
      startTime = QTime::fromString( datetime, Qt::ISODate );
    }
    else startTime = QTime( 12, 0 );
    if ( startDate.isValid() && startTime.isValid() )
    {
      startDateTime = QDateTime( startDate, startTime );
    }
    else rval = KARM_ERR_INVALID_DATE;

  }

  // Update task totals (session and total) and save to disk
  if ( !rval )
  {
    t->changeTotalTimes( t->sessionTime() + minutes, t->totalTime() + minutes );
    if ( ! _taskView->storage()->bookTime( t, startDateTime, minutes * 60 ) )
    {
      rval = KARM_ERR_GENERIC_SAVE_FAILED;
    }
  }

  return rval;
}

// There was something really bad going on with DCOP when I used a particular
// argument name; if I recall correctly, the argument name was errno.
QString MainWindow::getError( int mkb ) const
{
  if ( mkb <= KARM_MAX_ERROR_NO ) return m_error[ mkb ];
  else return i18n( "Invalid error number: %1", mkb );
}

int MainWindow::totalMinutesForTaskId( const QString& taskId )
{
  int rval = 0;
  Task *task, *t;

  kDebug(5970) << "MainWindow::totalTimeForTask( " << taskId << " )" << endl;

  // Find task
  task = _taskView->first_child();
  t = NULL;
  while ( !t && task )
  {
    t = _hasUid( task, taskId );
    task = task->nextSibling();
  }
  if ( t != NULL )
  {
    rval = t->totalTime();
    kDebug(5970) << "MainWindow::totalTimeForTask - task found: rval = " << rval << endl;
  }
  else
  {
    kDebug(5970) << "MainWindow::totalTimeForTask - task not found" << endl;
    rval = KARM_ERR_UID_NOT_FOUND;
  }

  return rval;
}

QString MainWindow::_hasTask( Task* task, const QString &taskname ) const
{
  QString rval = "";
  if ( task->name() == taskname )
  {
    rval = task->uid();
  }
  else
  {
    Task* nexttask = task->firstChild();
    while ( rval.isEmpty() && nexttask )
    {
      rval = _hasTask( nexttask, taskname );
      nexttask = nexttask->nextSibling();
    }
  }
  return rval;
}

Task* MainWindow::_hasUid( Task* task, const QString &uid ) const
{
  Task *rval = NULL;

  //kDebug(5970) << "MainWindow::_hasUid( " << task << ", " << uid << " )" << endl;

  if ( task->uid() == uid ) rval = task;
  else
  {
    Task* nexttask = task->firstChild();
    while ( !rval && nexttask )
    {
      rval = _hasUid( nexttask, uid );
      nexttask = nexttask->nextSibling();
    }
  }
  return rval;
}
QString MainWindow::starttimerfor( const QString& taskname )
{
  int index = -1;
  QString err="no such task";
  for (int i=0; i<_taskView->count(); i++)
  {
    if ((_taskView->item_at_index(i)->name()==taskname))
    {
      index=i;
      if (err.isNull() ) err="task name is abigious";
      if (err=="no such task") err=QString();
    }
  }
  if (err.isNull() && index>=0) _taskView->startTimerFor( _taskView->item_at_index(index) );
  return err;
}

QString MainWindow::stoptimerfor( const QString& taskname )
{
  int index=-1;
  QString err="no such task";
  for (int i=0; i<_taskView->count(); i++)
  {
    if ((_taskView->item_at_index(i)->name()==taskname))
    {
      index=i;
      if (err.isNull()) err="task name is abigious";
      if (err=="no such task") err=QString();
    }
  }
  if (err.isNull() && index>=0) _taskView->stopTimerFor( _taskView->item_at_index(index) );
  return err;
}

QString MainWindow::stopalltimers()
{
  _taskView->stopAllTimers();
  return QString();
}

QString MainWindow::exportcsvfile( QString filename, QString from, QString to, int type, bool decimalMinutes, bool allTasks, QString delimiter, QString quote )
{
  ReportCriteria rc;
  rc.url=filename;
  rc.from=QDate::fromString( from );
  if ( rc.from.isNull() ) rc.from=QDate::fromString( from, Qt::ISODate );
  kDebug(5970) << "rc.from " << rc.from << endl;
  rc.to=QDate::fromString( to );
  if ( rc.to.isNull() ) rc.to=QDate::fromString( to, Qt::ISODate );
  kDebug(5970) << "rc.to " << rc.to << endl;
  rc.reportType=(ReportCriteria::REPORTTYPE) type;  // history report or totals report
  rc.decimalMinutes=decimalMinutes;
  rc.allTasks=allTasks;
  rc.delimiter=delimiter;
  rc.quote=quote;
  return _taskView->report( rc );
}

QString MainWindow::importplannerfile( QString fileName )
{
  return _taskView->importPlanner(fileName);
}


#include "mainwindow.moc"
