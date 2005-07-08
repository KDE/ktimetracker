/*
* Top Level window for KArm.
* Distributed under the GPL.
*/

#include <numeric>

#include "kaccelmenuwatch.h"
#include <dcopclient.h>
#include <kaccel.h>
#include <kaction.h>
#include <kapplication.h>       // kapp
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kkeydialog.h>
#include <klocale.h>            // i18n
#include <kmessagebox.h>
#include <kstatusbar.h>         // statusBar()
#include <kstdaction.h>
#include <qkeycode.h>
#include <qpopupmenu.h>
#include <qptrlist.h>
#include <qstring.h>

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

MainWindow::MainWindow( const QString &icsfile )
  : DCOPObject ( "KarmDCOPIface" ),
    KParts::MainWindow(), 
    _accel     ( new KAccel( this ) ),
    _watcher   ( new KAccelMenuWatch( _accel, this ) ),
    _totalSum  ( 0 ),
    _sessionSum( 0 )
{

  _taskView  = new TaskView( this, 0, icsfile );

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
  connect( _taskView, SIGNAL( selectionChanged ( QListViewItem * )),
           this, SLOT(slotSelectionChanged()));
  connect( _taskView, SIGNAL( updateButtons() ),
           this, SLOT(slotSelectionChanged()));

  loadGeometry();

  // Setup context menu request handling
  connect( _taskView,
           SIGNAL( contextMenuRequested( QListViewItem*, const QPoint&, int )),
           this,
           SLOT( contextMenuRequest( QListViewItem*, const QPoint&, int )));

  _tray = new KarmTray( this );

  connect( _tray, SIGNAL( quitSelected() ), SLOT( quit() ) );

  connect( _taskView, SIGNAL( timersActive() ), _tray, SLOT( startClock() ) );
  connect( _taskView, SIGNAL( timersActive() ), this,  SLOT( enableStopAll() ));
  connect( _taskView, SIGNAL( timersInactive() ), _tray, SLOT( stopClock() ) );
  connect( _taskView, SIGNAL( timersInactive() ),  this,  SLOT( disableStopAll()));
  connect( _taskView, SIGNAL( tasksChanged( QPtrList<Task> ) ),
                      _tray, SLOT( updateToolTip( QPtrList<Task> ) ));

  _taskView->load();

  // Everything that uses Preferences has been created now, we can let it
  // emit its signals
  _preferences->emitSignals();
  slotSelectionChanged();

  // Register with DCOP
  if ( !kapp->dcopClient()->isRegistered() ) 
  {
    kapp->dcopClient()->registerAs( "karm" );
    kapp->dcopClient()->setDefaultObject( objId() );
  }

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

// This is _old_ code, but shows how to enable/disable add comment menu item.
// We'll need this kind of logic when comments are implemented.
//void MainWindow::timeLoggingChanged(bool on)
//{
//  actionAddComment->setEnabled( on );
//}

bool MainWindow::save()
{
  kdDebug(5970) << "Saving time data to disk." << endl;
  QString err=_taskView->save();  // untranslated error msg.
  if (err.isEmpty()) statusBar()->message(i18n("Successfully saved tasks and history"),1807);
  else statusBar()->message(i18n(err.ascii()),7707); // no msgbox since save is called when exiting
  saveGeometry();
  return true;
}

void MainWindow::exportcsvHistory()
{
  kdDebug(5970) << "Exporting History to disk." << endl;
  QString err=_taskView->exportcsvHistory();
  if (err.isEmpty()) statusBar()->message(i18n("Successfully exported History to CSV-file"),1807);
  else KMessageBox::error(this, err.ascii());
  saveGeometry();
  
}

void MainWindow::quit()
{
  kapp->quit();
}


MainWindow::~MainWindow()
{
  kdDebug(5970) << "MainWindow::~MainWindows: Quitting karm." << endl;
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
  statusBar()->changeItem( i18n("Session: %1").arg(time), 0 );

  time = formatTime( _totalSum );
  statusBar()->changeItem( i18n("Total: %1" ).arg(time), 1);
}

void MainWindow::startStatusBar()
{
  statusBar()->insertItem( i18n("Session"), 0, 0, true );
  statusBar()->insertItem( i18n("Total" ), 1, 0, true );
}

void MainWindow::saveProperties( KConfig* cfg )
{
  _taskView->stopAllTimers();
  _taskView->save();
  cfg->writeEntry( "WindowShown", isVisible());
}

void MainWindow::readProperties( KConfig* cfg )
{
  if( cfg->readBoolEntry( "WindowShown", true ))
    show();
}

void MainWindow::keyBindings()
{
  KKeyDialog::configure( actionCollection(), this );
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
  KAction
    *actionKeyBindings,
    *actionNew,
    *actionNewSub;

  (void) KStdAction::quit(  this, SLOT( quit() ),  actionCollection());
  (void) KStdAction::print( this, SLOT( print() ), actionCollection());
  actionKeyBindings = KStdAction::keyBindings( this, SLOT( keyBindings() ),
      actionCollection() );
  actionPreferences = KStdAction::preferences(_preferences,
      SLOT(showDialog()),
      actionCollection() );
  (void) KStdAction::save( this, SLOT( save() ), actionCollection() );
  KAction* actionStartNewSession = new KAction( i18n("Start &New Session"),
      0,
      this,
      SLOT( startNewSession() ),
      actionCollection(),
      "start_new_session");
  KAction* actionResetAll = new KAction( i18n("&Reset All Times"),
      0,
      this,
      SLOT( resetAllTimes() ),
      actionCollection(),
      "reset_all_times");
  actionStart = new KAction( i18n("&Start"),
      QString::fromLatin1("1rightarrow"), Key_S,
      _taskView,
      SLOT( startCurrentTimer() ), actionCollection(),
      "start");
  actionStop = new KAction( i18n("S&top"),
      QString::fromLatin1("stop"), 0,
      _taskView,
      SLOT( stopCurrentTimer() ), actionCollection(),
      "stop");
  actionStopAll = new KAction( i18n("Stop &All Timers"),
      Key_Escape,
      _taskView,
      SLOT( stopAllTimers() ), actionCollection(),
      "stopAll");
  actionStopAll->setEnabled(false);

  actionNew = new KAction( i18n("&New..."),
      QString::fromLatin1("filenew"), CTRL+Key_N,
      _taskView,
      SLOT( newTask() ), actionCollection(),
      "new_task");
  actionNewSub = new KAction( i18n("New &Subtask..."),
      QString::fromLatin1("kmultiple"), CTRL+ALT+Key_N,
      _taskView,
      SLOT( newSubTask() ), actionCollection(),
      "new_sub_task");
  actionDelete = new KAction( i18n("&Delete"),
      QString::fromLatin1("editdelete"), Key_Delete,
      _taskView,
      SLOT( deleteTask() ), actionCollection(),
      "delete_task");
  actionEdit = new KAction( i18n("&Edit..."),
      QString::fromLatin1("edit"), CTRL + Key_E,
      _taskView,
      SLOT( editTask() ), actionCollection(),
      "edit_task");
//  actionAddComment = new KAction( i18n("&Add Comment..."),
//      QString::fromLatin1("document"),
//      CTRL+ALT+Key_E,
//      _taskView,
//      SLOT( addCommentToTask() ),
//      actionCollection(),
//      "add_comment_to_task");
  actionMarkAsComplete = new KAction( i18n("&Mark as Complete"),
      QString::fromLatin1("document"),
      CTRL+Key_M,
      _taskView,
      SLOT( markTaskAsComplete() ),
      actionCollection(),
      "mark_as_complete");
  actionMarkAsIncomplete = new KAction( i18n("&Mark as Incomplete"),
      QString::fromLatin1("document"),
      CTRL+Key_M,
      _taskView,
      SLOT( markTaskAsIncomplete() ),
      actionCollection(),
      "mark_as_incomplete");
  actionClipTotals = new KAction( i18n("&Copy Totals to Clipboard"),
      QString::fromLatin1("klipper"),
      CTRL+Key_C,
      _taskView,
      SLOT( clipTotals() ),
      actionCollection(),
      "clip_totals");
  actionClipHistory = new KAction( i18n("Copy &History to Clipboard"),
      QString::fromLatin1("klipper"),
      CTRL+ALT+Key_C,
      _taskView,
      SLOT( clipHistory() ),
      actionCollection(),
      "clip_history");

  new KAction( i18n("Import &Legacy Flat File..."), 0,
      _taskView, SLOT(loadFromFlatFile()), actionCollection(),
      "import_flatfile");
  new KAction( i18n("&Export to CSV File..."), 0,
      _taskView, SLOT(exportcsvFile()), actionCollection(),
      "export_csvfile");
  new KAction( i18n("Export &History to CSV File..."), 0,
      this, SLOT(exportcsvHistory()), actionCollection(),
      "export_csvhistory");
  new KAction( i18n("Import Tasks From &Planner..."), 0,
      _taskView, SLOT(importPlanner()), actionCollection(),
      "import_planner");  

/*
  new KAction( i18n("Import E&vents"), 0,
                            _taskView,
                            SLOT( loadFromKOrgEvents() ), actionCollection(),
                            "import_korg_events");
  */

  setXMLFile( QString::fromLatin1("karmui.rc") );
  createGUI( 0 );

  // Tool tips must be set after the createGUI.
  actionKeyBindings->setToolTip( i18n("Configure key bindings") );
  actionKeyBindings->setWhatsThis( i18n("This will let you configure key"
                                        "bindings which is specific to karm") );

  actionStartNewSession->setToolTip( i18n("Start a new session") );
  actionStartNewSession->setWhatsThis( i18n("This will reset the session time "
                                            "to 0 for all tasks, to start a "
                                            "new session, without affecting "
                                            "the totals.") );
  actionResetAll->setToolTip( i18n("Reset all times") );
  actionResetAll->setWhatsThis( i18n("This will reset the session and total "
                                     "time to 0 for all tasks, to restart from "
                                     "scratch.") );

  actionStart->setToolTip( i18n("Start timing for selected task") );
  actionStart->setWhatsThis( i18n("This will start timing for the selected "
                                  "task.\n"
                                  "It is even possible to time several tasks "
                                  "simultaneously.\n\n"
                                  "You may also start timing of a tasks by "
                                  "double clicking the left mouse "
                                  "button on a given task. This will, however, "
                                  "stop timing of other tasks."));

  actionStop->setToolTip( i18n("Stop timing of the selected task") );
  actionStop->setWhatsThis( i18n("Stop timing of the selected task") );

  actionStopAll->setToolTip( i18n("Stop all of the active timers") );
  actionStopAll->setWhatsThis( i18n("Stop all of the active timers") );

  actionNew->setToolTip( i18n("Create new top level task") );
  actionNew->setWhatsThis( i18n("This will create a new top level task.") );

  actionDelete->setToolTip( i18n("Delete selected task") );
  actionDelete->setWhatsThis( i18n("This will delete the selected task and "
                                   "all its subtasks.") );

  actionEdit->setToolTip( i18n("Edit name or times for selected task") );
  actionEdit->setWhatsThis( i18n("This will bring up a dialog box where you "
                                 "may edit the parameters for the selected "
                                 "task."));
  //actionAddComment->setToolTip( i18n("Add a comment to a task") );
  //actionAddComment->setWhatsThis( i18n("This will bring up a dialog box where "
  //                                     "you can add a comment to a task. The "
  //                                     "comment can for instance add information on what you "
  //                                     "are currently doing. The comment will "
  //                                     "be logged in the log file."));
  actionClipTotals->setToolTip(i18n("Copy task totals to clipboard"));
  actionClipHistory->setToolTip(i18n("Copy time card history to clipboard."));

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
    KConfig &config = *kapp->config();

    config.setGroup( QString::fromLatin1("Main Window Geometry") );
    int w = config.readNumEntry( QString::fromLatin1("Width"), 100 );
    int h = config.readNumEntry( QString::fromLatin1("Height"), 100 );
    w = QMAX( w, sizeHint().width() );
    h = QMAX( h, sizeHint().height() );
    resize(w, h);
  }
}


void MainWindow::saveGeometry()
{
  KConfig &config = *KGlobal::config();
  config.setGroup( QString::fromLatin1("Main Window Geometry"));
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

void MainWindow::contextMenuRequest( QListViewItem*, const QPoint& point, int )
{
    QPopupMenu* pop = dynamic_cast<QPopupMenu*>(
                          factory()->container( i18n( "task_popup" ), this ) );
    if ( pop )
      pop->popup( point );
}

//----------------------------------------------------------------------------
//
//                       D C O P   I N T E R F A C E
//
//----------------------------------------------------------------------------

QString MainWindow::version() const
{
  return KARM_VERSION;
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
  kdDebug(5970) << "MainWindow::addTask( " << taskname << " ) returns " << uid << endl;
  if ( uid.length() > 0 ) return 0;
  else
  {
    // We can't really tell what happened, b/c the resource framework only
    // returns a boolean.
    return KARM_ERR_GENERIC_SAVE_FAILED;
  }
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
  if ( mkb < KARM_MAX_ERROR_NO + 1 ) return m_error[ mkb ];
  else return i18n( "Invalid error number: %1" ).arg( mkb );
}

int MainWindow::totalMinutesForTaskId( const QString& taskId )
{
  int rval = 0;
  Task *task, *t;
  
  kdDebug(5970) << "MainWindow::totalTimeForTask( " << taskId << " )" << endl;

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
    kdDebug(5970) << "MainWindow::totalTimeForTask - task found: rval = " << rval << endl;
  }
  else 
  {
    kdDebug(5970) << "MainWindow::totalTimeForTask - task not found" << endl;
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

  //kdDebug(5970) << "MainWindow::_hasUid( " << task << ", " << uid << " )" << endl;

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
  int index;
  QString err="no such task";
  for (int i=0; i<_taskView->count(); i++)
  {
    if ((_taskView->item_at_index(i)->name()==taskname))
    {
      index=i;
      if (err==QString::null) err="task name is abigious";
      if (err=="no such task") err=QString::null;
    }
  }
  if (err==QString::null) _taskView->startTimerFor( _taskView->item_at_index(index) );
  return err;
}

QString MainWindow::stoptimerfor( const QString& taskname )
{
  int index;
  QString err="no such task";
  for (int i=0; i<_taskView->count(); i++)
  {
    if ((_taskView->item_at_index(i)->name()==taskname))
    {
      index=i;
      if (err==QString::null) err="task name is abigious";
      if (err=="no such task") err=QString::null;
    }
  }
  if (err==QString::null) _taskView->stopTimerFor( _taskView->item_at_index(index) );
  return err;
}

QString MainWindow::exportcsvfile( QString filename, QString from, QString to, int type, bool decimalMinutes, bool allTasks, QString delimiter, QString quote )
{
  ReportCriteria rc;
  rc.allTasks=allTasks;
  rc.decimalMinutes=decimalMinutes;
  rc.delimiter=delimiter;
  rc.from=QDate::fromString( from );
  rc.quote=quote;
  rc.reportType=(ReportCriteria::REPORTTYPE) type;
  rc.to=QDate::fromString( to );
  rc.url=filename;
  return _taskView->report( rc );
}

QString MainWindow::importplannerfile( QString fileName )
{
  return _taskView->importPlanner(fileName);
}


#include "mainwindow.moc"
