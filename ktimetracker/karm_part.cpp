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
#include "karmpartadaptor.h"
#include "ktimetracker.h"

karmPart::karmPart( QWidget *parentWidget, QObject *parent )
    : KParts::ReadWritePart(parent),
#ifdef __GNUC__
#warning Port me!
#endif
//    _accel     ( new KAccel( parentWidget ) ),
    _watcher   ( new KAccelMenuWatch( _accel, parentWidget ) )
{
  new KarmPartAdaptor(this);
  QDBusConnection::sessionBus().registerObject("/Karm", this);
    // we need an instance
    setComponentData( karmPartFactory::componentData() );

    // this should be your custom internal widget
    _taskView = new TaskView( parentWidget );

    connect(_taskView, SIGNAL( setStatusBarText(QString)), this, SLOT( setStatusBar(QString) ) );

    // setup PreferenceDialog.
    _preferences = Preferences::instance();

   // notify the part that this is our internal widget
    setWidget(_taskView);

    // create our actions
    KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
    KStandardAction::saveAs(this, SLOT(fileSaveAs()), actionCollection());
    KStandardAction::save(this, SLOT(save()), actionCollection());

    makeMenus();

  _watcher->updateMenus();

  // connections

  connect( _taskView, SIGNAL( totalTimesChanged( long, long ) ),
           this, SLOT( updateTime( long, long ) ) );
  connect( _taskView, SIGNAL( itemSelectionChanged() ),
           this, SLOT( slotSelectionChanged() ) );
  connect( _taskView, SIGNAL( updateButtons() ),
           this, SLOT( slotSelectionChanged() ) );

  // Setup context menu request handling
  _taskView->setContextMenuPolicy( Qt::CustomContextMenu );
  connect( _taskView,
           SIGNAL( customContextMenuRequested( const QPoint& ) ),
           this,
           SLOT( taskViewCustomContextMenuRequested( const QPoint& ) ) );

  _tray = new KarmTray( this );

  connect( _tray, SIGNAL( quitSelected() ), SLOT( quit() ) );

  connect( _taskView, SIGNAL( timersActive() ), _tray, SLOT( startClock() ) );
  connect( _taskView, SIGNAL( timersActive() ), this,  SLOT( enableStopAll() ));
  connect( _taskView, SIGNAL( timersInactive() ), _tray, SLOT( stopClock() ) );
  connect( _taskView, SIGNAL( timersInactive() ),  this,  SLOT( disableStopAll()));
  connect( _taskView, SIGNAL( tasksChanged( QList<Task*> ) ),
                      _tray, SLOT( updateToolTip( QList<Task*> ) ));

  _taskView->load( KStandardDirs::locateLocal( "appdata", 
                   QString::fromLatin1( "karm.ics" ) ) );

  // Everything that uses Preferences has been created now, we can let it
  // emit its signals
  //_preferences->emitSignals(); FIXME relocate effects
  slotSelectionChanged();

    // set our XML-UI resource file
    setXMLFile("karmui.rc");

    // we are read-write by default
    setReadWrite(true);

    // we are not modified since we haven't done anything yet
    setModified(false);
}

karmPart::~karmPart()
{
}

void karmPart::slotSelectionChanged()
{
  Task* item= _taskView->currentItem();
  actionDelete->setEnabled(item);
  actionEdit->setEnabled(item);
  actionStart->setEnabled(item && !item->isRunning() && !item->isComplete());
  actionStop->setEnabled(item && item->isRunning());
  actionMarkAsComplete->setEnabled(item && !item->isComplete());
  actionMarkAsIncomplete->setEnabled(item && item->isComplete());
}

void karmPart::makeMenus()
{
  KAction
    *actionKeyBindings,
    *actionNew,
    *actionNewSub;

  (void) KStandardAction::quit(  this, SLOT( quit() ),  actionCollection());
  (void) KStandardAction::print( this, SLOT( print() ), actionCollection());
  actionKeyBindings = KStandardAction::keyBindings( this, SLOT( keyBindings() ),
      actionCollection() );
  actionPreferences = KStandardAction::preferences(_preferences,
      SLOT(showDialog()), actionCollection() );
  (void) KStandardAction::save( this, SLOT( save() ), actionCollection() );
  QAction *actionStartNewSession  = new KAction(i18n("Start &New Session"), this);
  actionCollection()->addAction("start_new_session", actionStartNewSession );
  connect(actionStartNewSession, SIGNAL(triggered(bool)), SLOT( startNewSession() ));
  QAction *actionResetAll  = new KAction(i18n("&Reset All Times"), this);
  actionCollection()->addAction("reset_all_times", actionResetAll );
  connect(actionResetAll, SIGNAL(triggered(bool)), SLOT( resetAllTimes() ));
  actionStart  = new KAction(KIcon("arrow-right"), i18n("&Start"), this);
  actionCollection()->addAction("start", actionStart );
  connect(actionStart, SIGNAL(triggered(bool) ), _taskView, SLOT( startCurrentTimer() ));
  actionStart->setShortcut(QKeySequence(Qt::Key_S));
  actionStop  = new KAction(KIcon("process-stop"), i18n("S&top"), this);
  actionCollection()->addAction("stop", actionStop );
  connect(actionStop, SIGNAL(triggered(bool) ), _taskView, SLOT( stopCurrentTimer() ));
  actionStopAll  = new KAction(i18n("Stop &All Timers"), this);
  actionCollection()->addAction("stopAll", actionStopAll );
  connect(actionStopAll, SIGNAL(triggered(bool)), _taskView, SLOT( stopAllTimers() ));
  actionStopAll->setShortcut(QKeySequence(Qt::Key_Escape));
  actionStopAll->setEnabled(false);

  actionNew  = new KAction(KIcon("document-new"), i18nc( "creating a new task", "&New..." ), this);
  actionCollection()->addAction("new_task", actionNew );
  connect(actionNew, SIGNAL(triggered(bool) ), _taskView, SLOT( newTask() ));
  actionNew->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_N));
  actionNewSub  = new KAction(KIcon("kmultiple"), i18n("New &Subtask..."), this);
  actionCollection()->addAction("new_sub_task", actionNewSub );
  connect(actionNewSub, SIGNAL(triggered(bool) ), _taskView, SLOT( newSubTask() ));
  actionNewSub->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_N));
  actionDelete  = new KAction(KIcon("edit-delete"), i18n("&Delete"), this);
  actionCollection()->addAction("delete_task", actionDelete );
  connect(actionDelete, SIGNAL(triggered(bool) ), _taskView, SLOT( deleteTask() ));
  actionDelete->setShortcut(QKeySequence(Qt::Key_Delete));
  actionEdit  = new KAction(KIcon("edit"), i18n("&Edit..."), this);
  actionCollection()->addAction("edit_task", actionEdit );
  connect(actionEdit, SIGNAL(triggered(bool) ), _taskView, SLOT( editTask() ));
  actionEdit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
//  actionAddComment = new KAction( i18n("&Add Comment..."),
//      QString::fromLatin1("document"),
//      Qt::CTRL+Qt::ALT+Qt::Key_E,
//      _taskView,
//      SLOT( addCommentToTask() ),
//      actionCollection(),
//      "add_comment_to_task");
  actionMarkAsComplete  = new KAction(KIcon("document"), i18n("&Mark as Complete"), this);
  actionCollection()->addAction("mark_as_complete", actionMarkAsComplete );
  connect(actionMarkAsComplete, SIGNAL(triggered(bool) ), _taskView, SLOT( markTaskAsComplete() ));
  actionMarkAsComplete->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_M));
  actionMarkAsIncomplete  = new KAction(KIcon("document"), i18n("&Mark as Incomplete"), this);
  actionCollection()->addAction("mark_as_incomplete", actionMarkAsIncomplete );
  connect(actionMarkAsIncomplete, SIGNAL(triggered(bool) ), _taskView, SLOT( markTaskAsIncomplete() ));
  actionMarkAsIncomplete->setShortcut(QKeySequence(Qt::CTRL+Qt::Key_M));

  QAction *action  = new KAction(i18n("&Export to CSV File..."), this);
  actionCollection()->addAction("export_csvfile", action );
  connect(action, SIGNAL(triggered(bool) ), _taskView, SLOT(exportcsvFile()));
  action  = new KAction(i18n("Export &History to CSV File..."), this);
  actionCollection()->addAction("export_csvhistory", action );
  connect(action, SIGNAL(triggered(bool) ), SLOT(exportcsvHistory()));
  action  = new KAction(i18n("Import Tasks From &Planner..."), this);
  actionCollection()->addAction("import_planner", action );
  connect(action, SIGNAL(triggered(bool) ), _taskView, SLOT(importPlanner()));
  action  = new KAction(i18n("Configure KArm..."), this);
  actionCollection()->addAction("configure_karm", action );
  connect(action, SIGNAL(triggered(bool) ), _preferences, SLOT(showDialog()));

/*
  new KAction( i18n("Import E&vents"), 0,
                            _taskView,
                            SLOT( loadFromKOrgEvents() ), actionCollection(),
                            "import_korg_events");
  */

  // Tool tops must be set after the createGUI.
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

void karmPart::setReadWrite(bool rw)
{
    // notify your internal widget of the read-write state
    if (rw)
        connect(_taskView, SIGNAL(textChanged()),
                this,     SLOT(setModified()));
    else
    {
        disconnect(_taskView, SIGNAL(textChanged()),
                   this,     SLOT(setModified()));
    }

    ReadWritePart::setReadWrite(rw);
}

void karmPart::setModified(bool modified)
{
    // get a handle on our Save action and make sure it is valid
    QAction *save = actionCollection()->action(KStandardAction::stdName(KStandardAction::Save));
    if (!save)
        return;

    // if so, we either enable or disable it based on the current
    // state
    if (modified)
        save->setEnabled(true);
    else
        save->setEnabled(false);

    // in any event, we want our parent to do it's thing
    ReadWritePart::setModified(modified);
}

bool karmPart::openFile()
{
    // m_file is always local so we can use QFile on it
    _taskView->load(localFilePath());

    // just for fun, set the status bar
    emit setStatusBarText( url().prettyUrl() );

    return true;
}

void karmPart::setStatusBar(const QString & qs)
{
  kDebug(5970) <<"Entering setStatusBar";
  emit setStatusBarText(qs);
}

bool karmPart::saveFile()
{
    // if we aren't read-write, return immediately
    if (isReadWrite() == false)
        return false;

    // m_file is always local, so we use QFile
    QFile file(localFilePath());
    if (file.open(QIODevice::WriteOnly) == false)
        return false;

    // use QTextStream to dump the text to the file
    QTextStream stream(&file);

    file.close();

    return true;
}

void karmPart::fileOpen()
{
    // this slot is called whenever the File->Open menu is selected,
    // the Open shortcut is pressed (usually Qt::CTRL+O) or the Open toolbar
    // button is clicked
    QString file_name = KFileDialog::getOpenFileName();

    if (file_name.isEmpty() == false)
        openUrl(file_name);
}

void karmPart::fileSaveAs()
{
    // this slot is called whenever the File->Save As menu is selected,
    QString file_name = KFileDialog::getSaveFileName();
    if (file_name.isEmpty() == false)
        saveAs(file_name);
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
      pop->popup( _taskView->viewport()->mapToGlobal( point ) );
}

//----------------------------------------------------------------------------
//
//                       D C O P   I N T E R F A C E
//
//----------------------------------------------------------------------------

QString karmPart::version() const
{
  return KTIMETRACKER_VERSION;
}

QString karmPart::deletetodo()
{
  _taskView->deleteTask();
  return "";
}

bool karmPart::getpromptdelete()
{
  return KTimeTrackerSettings::promptDelete();
}

QString karmPart::setpromptdelete( bool prompt )
{
  KTimeTrackerSettings::setPromptDelete( prompt );
  return "";
}

QString karmPart::taskIdFromName( const QString &taskname ) const
{
  QString rval = "";

  for ( int i = 0; i < _taskView->topLevelItemCount(); ++i ) {
    Task *task = static_cast< Task* >( _taskView->topLevelItem( i ) );
    rval = _hasTask( task, taskname );

    if ( !( rval.isEmpty() ) ) {
      break;
    }
  }

  return rval;
}

void karmPart::quit()
{
  // TODO: write something for kapp->quit();
}

bool karmPart::save()
{
  kDebug(5970) <<"Saving time data to disk.";
  QString err=_taskView->save();  // untranslated error msg.

  if (err.isEmpty()) setStatusBar(i18n("Successfully saved tasks and history"));
  else setStatusBar(i18n(err.toUtf8())); // no msgbox since save is called when exiting */
  return true;
}

int karmPart::addTask( const QString& taskname )
{
  DesktopList desktopList;
  QString uid = _taskView->addTask( taskname, 0, 0, desktopList );
  kDebug(5970) <<"MainWindow::addTask(" << taskname <<" ) returns" << uid;
  if ( uid.length() > 0 ) return 0;
  else
  {
    // We can't really tell what happened, b/c the resource framework only
    // returns a boolean.
    return KARM_ERR_GENERIC_SAVE_FAILED;
  }
}

QString karmPart::setPerCentComplete( const QString& taskName, int perCent )
{
  int index = -1;
  QString err="no such task";
  for (int i=0; i<_taskView->count(); i++)
  {
    if ((_taskView->itemAt(i)->name()==taskName))
    {
      index=i;
      if (err.isNull()) err="task name is abigious";
      if (err=="no such task") err=QString();
    }
  }
  if (err.isNull() && index>=0 )
  {
    _taskView->itemAt(index)->setPercentComplete( perCent, _taskView->storage() );
  }
  return err;
}

int karmPart::bookTime
( const QString& taskId, const QString& datetime, long minutes )
{
  int rval = 0;
  QDate startDate;
  QTime startTime;
  QDateTime startDateTime;
  Task *task, *t;

  if ( minutes <= 0 ) rval = KARM_ERR_INVALID_DURATION;

  // Find task
  for ( int i = 0; i < _taskView->topLevelItemCount(); ++i ) {
    task = static_cast< Task* >( _taskView->topLevelItem( i ) );
    t = _hasUid( task, taskId );
    if ( t ) break;
  }
  if ( !t ) rval = KARM_ERR_UID_NOT_FOUND;

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
QString karmPart::getError( int mkb ) const
{
  if ( mkb <= KARM_MAX_ERROR_NO ) return m_error[ mkb ];
  else return i18n( "Invalid error number: %1", mkb );
}

int karmPart::totalMinutesForTaskId( const QString& taskId )
{
  int rval = 0;
  Task *task, *t;

  kDebug(5970) <<"MainWindow::totalTimeForTask(" << taskId <<" )";

  // Find task
  for ( int i = 0; i < _taskView->topLevelItemCount(); ++i ) {
    task = static_cast< Task* >( _taskView->topLevelItem( i ) );
    t = _hasUid( task, taskId );
    if ( t ) break;
  }

  if ( t ) {
    rval = t->totalTime();
    kDebug(5970) <<"MainWindow::totalTimeForTask - task found: rval =" << rval;
  }
  else {
    kDebug(5970) <<"MainWindow::totalTimeForTask - task not found";
    rval = KARM_ERR_UID_NOT_FOUND;
  }

  return rval;
}

QString karmPart::_hasTask( Task* task, const QString &taskname ) const
{
  QString rval = "";
  if ( task->name() == taskname )
  {
    rval = task->uid();
  }
  else
  {
    for ( int i = 0; i < task->childCount(); ++i ) {
      Task *nexttask = static_cast< Task* >( task->child( i ) );
      rval = _hasTask( nexttask, taskname );
      if ( !( rval.isEmpty() ) ) break;
    }
  }
  return rval;
}

Task* karmPart::_hasUid( Task* task, const QString &uid ) const
{
  Task *rval = 0;

  //kDebug(5970) <<"MainWindow::_hasUid(" << task <<"," << uid <<" )";

  if ( task->uid() == uid ) rval = task;
  else
  {
    for ( int i = 0; i < task->childCount(); ++i ) {
      Task *nexttask = static_cast< Task* >( task->child( i ) );
      rval = _hasUid( nexttask, uid );
      if ( rval ) break;
    }
  }
  return rval;
}

QString karmPart::starttimerfor( const QString& taskname )
{
  QString err="no such task";
  for (int i=0; i<_taskView->count(); i++)
  {
    if ((_taskView->itemAt(i)->name()==taskname))
    {
      _taskView->startTimerFor( _taskView->itemAt(i) );
      err="";
    }
  }
  return err;
}

QString karmPart::stoptimerfor( const QString& taskname )
{
  QString err="no such task";
  for (int i=0; i<_taskView->count(); i++)
  {
    if ((_taskView->itemAt(i)->name()==taskname))
    {
      _taskView->stopTimerFor( _taskView->itemAt(i) );
      err="";
    }
  }
  return err;
}

QString karmPart::stopalltimers()
{
  _taskView->stopAllTimers();
  return QString();
}

QString karmPart::exportcsvfile( const QString &filename, const QString &from, 
                                 const QString &to, int type, 
                                 bool decimalMinutes, bool allTasks, 
                                 const QString &delimiter, 
                                 const QString &quote )
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

QString karmPart::importplannerfile( const QString &fileName )
{
  _taskView->importPlanner(fileName);
  return "";
}

QStringList karmPart::getActiveTasks()
{
  QStringList result;

  for ( int i = 0; i < _taskView->count(); ++i ) {
    if ( _taskView->itemAt( i )->isRunning() ) {
      result << _taskView->itemAt( i )->name();
    }
  }

  return result;
}

QStringList karmPart::getTasks()
{
  QStringList result;

  for ( int i = 0; i < _taskView->count(); ++i ) {
    result << _taskView->itemAt( i )->name();
  }

  return result;
}

bool karmPart::isActive( const QString &taskName )
{
  QStringList result;

  Task *task;

  for ( int i = 0; i < _taskView->count(); ++i ) {
    task = _taskView->itemAt( i );
    if ( task->name() == taskName ) {
      return task->isRunning();
    }
  }

  return false;
}

void karmPart::startNewSession()
{
  _taskView->startNewSession();
  _taskView->save();
}


#include "karm_part.moc"
