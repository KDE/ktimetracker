
#include "kaccelmenuwatch.h"
#include "karm_part.h"
#include "karmerrors.h"
#include "task.h"
#include "preferences.h"
#include "tray.h"
#include "version.h"

#include <kinstance.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstdaction.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <klocale.h>

#include <QFile>
#include <qtextstream.h>
#include <q3multilineedit.h>
#include <QByteArray>
#include <Q3PtrList>
#include <q3popupmenu.h>
#include <kxmlguifactory.h>
#include "mainwindow.h"

#include "karmpartadaptor.h"
#include <QtDBus>

karmPart::karmPart( QWidget *parentWidget, QObject *parent )
    : KParts::ReadWritePart(parent),
#warning Port me!
//    _accel     ( new KAccel( parentWidget ) ),
    _watcher   ( new KAccelMenuWatch( _accel, parentWidget ) )
{
  new KarmPartAdaptor(this);
  QDBus::sessionBus().registerObject("/Karm", this);
    // we need an instance
    setInstance( karmPartFactory::instance() );

    // this should be your custom internal widget
    _taskView = new TaskView( parentWidget );

    connect(_taskView, SIGNAL( setStatusBarText(QString)), this, SLOT( setStatusBar(QString) ) );

    // setup PreferenceDialog.
    _preferences = Preferences::instance();

   // notify the part that this is our internal widget
    setWidget(_taskView);

    // create our actions
    KStdAction::open(this, SLOT(fileOpen()), actionCollection());
    KStdAction::saveAs(this, SLOT(fileSaveAs()), actionCollection());
    KStdAction::save(this, SLOT(save()), actionCollection());

    makeMenus();

  _watcher->updateMenus();

  // connections

  connect( _taskView, SIGNAL( totalTimesChanged( long, long ) ),
           this, SLOT( updateTime( long, long ) ) );
  connect( _taskView, SIGNAL( selectionChanged ( Q3ListViewItem * )),
           this, SLOT(slotSelectionChanged()));
  connect( _taskView, SIGNAL( updateButtons() ),
           this, SLOT(slotSelectionChanged()));

  // Setup context menu request handling
  connect( _taskView,
           SIGNAL( contextMenuRequested( Q3ListViewItem*, const QPoint&, int )),
           this,
           SLOT( contextMenuRequest( Q3ListViewItem*, const QPoint&, int )));

  _tray = new KarmTray( this );

  connect( _tray, SIGNAL( quitSelected() ), SLOT( quit() ) );

  connect( _taskView, SIGNAL( timersActive() ), _tray, SLOT( startClock() ) );
  connect( _taskView, SIGNAL( timersActive() ), this,  SLOT( enableStopAll() ));
  connect( _taskView, SIGNAL( timersInactive() ), _tray, SLOT( stopClock() ) );
  connect( _taskView, SIGNAL( timersInactive() ),  this,  SLOT( disableStopAll()));
  connect( _taskView, SIGNAL( tasksChanged( Q3PtrList<Task> ) ),
                      _tray, SLOT( updateToolTip( Q3PtrList<Task> ) ));

  _taskView->load();

  // Everything that uses Preferences has been created now, we can let it
  // emit its signals
  _preferences->emitSignals();
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
  Task* item= _taskView->current_item();
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

  (void) KStdAction::quit(  this, SLOT( quit() ),  actionCollection());
  (void) KStdAction::print( this, SLOT( print() ), actionCollection());
  actionKeyBindings = KStdAction::keyBindings( this, SLOT( keyBindings() ),
      actionCollection() );
  actionPreferences = KStdAction::preferences(_preferences,
      SLOT(showDialog()), actionCollection() );
  (void) KStdAction::save( this, SLOT( save() ), actionCollection() );
  KAction* actionStartNewSession = new KAction( i18n("Start &New Session"),
      0, this, SLOT( startNewSession() ), actionCollection(), "start_new_session");
  KAction* actionResetAll = new KAction( i18n("&Reset All Times"),
      0, this, SLOT( resetAllTimes() ), actionCollection(), "reset_all_times");
  actionStart = new KAction(KIcon("1rightarrow"),  i18n("&Start"), actionCollection(), "start");
  connect(actionStart, SIGNAL(triggered(bool) ), _taskView, SLOT( startCurrentTimer() ));
  actionStart->setShortcut(Qt::Key_S);
  actionStop = new KAction(KIcon("stop"),  i18n("S&top"), actionCollection(), "stop");
  connect(actionStop, SIGNAL(triggered(bool) ), _taskView, SLOT( stopCurrentTimer() ));
  actionStopAll = new KAction( i18n("Stop &All Timers"),
      Qt::Key_Escape, _taskView, SLOT( stopAllTimers() ), actionCollection(), "stopAll");
  actionStopAll->setEnabled(false);

  actionNew = new KAction(KIcon("filenew"),  i18n("&New..."), actionCollection(), "new_task");
  connect(actionNew, SIGNAL(triggered(bool) ), _taskView, SLOT( newTask() ));
  actionNew->setShortcut(Qt::CTRL+Qt::Key_N);
  actionNewSub = new KAction(KIcon("kmultiple"),  i18n("New &Subtask..."), actionCollection(), "new_sub_task");
  connect(actionNewSub, SIGNAL(triggered(bool) ), _taskView, SLOT( newSubTask() ));
  actionNewSub->setShortcut(Qt::CTRL+Qt::ALT+Qt::Key_N);
  actionDelete = new KAction(KIcon("editdelete"),  i18n("&Delete"), actionCollection(), "delete_task");
  connect(actionDelete, SIGNAL(triggered(bool) ), _taskView, SLOT( deleteTask() ));
  actionDelete->setShortcut(Qt::Key_Delete);
  actionEdit = new KAction(KIcon("edit"),  i18n("&Edit..."), actionCollection(), "edit_task");
  connect(actionEdit, SIGNAL(triggered(bool) ), _taskView, SLOT( editTask() ));
  actionEdit->setShortcut(Qt::CTRL + Qt::Key_E);
//  actionAddComment = new KAction( i18n("&Add Comment..."),
//      QString::fromLatin1("document"),
//      Qt::CTRL+Qt::ALT+Qt::Key_E,
//      _taskView,
//      SLOT( addCommentToTask() ),
//      actionCollection(),
//      "add_comment_to_task");
  actionMarkAsComplete = new KAction(KIcon("document"),  i18n("&Mark as Complete"), actionCollection(), "mark_as_complete");
  connect(actionMarkAsComplete, SIGNAL(triggered(bool) ), _taskView, SLOT( markTaskAsComplete() ));
  actionMarkAsComplete->setShortcut(Qt::CTRL+Qt::Key_M);
  actionMarkAsIncomplete = new KAction(KIcon("document"),  i18n("&Mark as Incomplete"), actionCollection(), "mark_as_incomplete");
  connect(actionMarkAsIncomplete, SIGNAL(triggered(bool) ), _taskView, SLOT( markTaskAsIncomplete() ));
  actionMarkAsIncomplete->setShortcut(Qt::CTRL+Qt::Key_M);
  actionClipTotals = new KAction(KIcon("klipper"),  i18n("&Copy Totals to Clipboard"), actionCollection(), "clip_totals");
  connect(actionClipTotals, SIGNAL(triggered(bool) ), _taskView, SLOT( clipTotals() ));
  actionClipTotals->setShortcut(Qt::CTRL+Qt::Key_C);
  actionClipHistory = new KAction(KIcon("klipper"),  i18n("Copy &History to Clipboard"), actionCollection(), "clip_history");
  connect(actionClipHistory, SIGNAL(triggered(bool) ), _taskView, SLOT( clipHistory() ));
  actionClipHistory->setShortcut(Qt::CTRL+Qt::ALT+Qt::Key_C);

  KAction *action = new KAction( i18n("Import &Legacy Flat File..."), actionCollection(), "import_flatfile");
  connect(action, SIGNAL(triggered(bool) ), _taskView, SLOT(loadFromFlatFile()));
  action = new KAction( i18n("&Export to CSV File..."), actionCollection(), "export_csvfile");
  connect(action, SIGNAL(triggered(bool) ), _taskView, SLOT(exportcsvFile()));
  action = new KAction( i18n("Export &History to CSV File..."), actionCollection(), "export_csvhistory");
  connect(action, SIGNAL(triggered(bool) ), SLOT(exportcsvHistory()));
  action = new KAction( i18n("Import Tasks From &Planner..."), actionCollection(), "import_planner");
  connect(action, SIGNAL(triggered(bool) ), _taskView, SLOT(importPlanner()));
  action = new KAction( i18n("Configure KArm..."), actionCollection(), "configure_karm");
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
    KAction *save = actionCollection()->action(KStdAction::stdName(KStdAction::Save));
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
    _taskView->load(m_file);

    // just for fun, set the status bar
    emit setStatusBarText( m_url.prettyUrl() );

    return true;
}

void karmPart::setStatusBar(const QString & qs)
{
  kDebug(5970) << "Entering setStatusBar" << endl;
  emit setStatusBarText(qs);
}

bool karmPart::saveFile()
{
    // if we aren't read-write, return immediately
    if (isReadWrite() == false)
        return false;

    // m_file is always local, so we use QFile
    QFile file(m_file);
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
        openURL(file_name);
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
#include <kaboutdata.h>
#include <klocale.h>

KInstance*  karmPartFactory::s_instance = 0L;
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
    // Create an instance of our Part
    karmPart* obj = new karmPart( parentWidget, parent );

    // See if we are to be read-write or not
    if (QByteArray(classname) == "KParts::ReadOnlyPart")
        obj->setReadWrite(false);

    return obj;
}

KInstance* karmPartFactory::instance()
{
    if( !s_instance )
    {
        s_about = new KAboutData("karmpart", I18N_NOOP("karmPart"), "0.1");
        s_about->addAuthor("Thorsten Staerk", 0, "thorsten@staerk.de");
        s_instance = new KInstance(s_about);
    }
    return s_instance;
}

extern "C"
{
    KDE_EXPORT void* init_libkarmpart()
    {
	KGlobal::locale()->insertCatalog("karm");
        return new karmPartFactory;
    }
}

void karmPart::contextMenuRequest( Q3ListViewItem*, const QPoint& point, int )
{
    QMenu* pop = dynamic_cast<QMenu*>(
                          factory()->container( i18n( "task_popup" ), this ) );
    if ( pop )
      pop->popup( point );
}

//----------------------------------------------------------------------------
//
//                       D C O P   I N T E R F A C E
//
//----------------------------------------------------------------------------

QString karmPart::version() const
{
  return KARM_VERSION;
}

QString karmPart::deletetodo()
{
  _taskView->deleteTask();
  return "";
}

bool karmPart::getpromptdelete()
{
  return _preferences->promptDelete();
}

QString karmPart::setpromptdelete( bool prompt )
{
  _preferences->setPromptDelete( prompt );
  return "";
}

QString karmPart::taskIdFromName( const QString &taskname ) const
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

void karmPart::quit()
{
  // TODO: write something for kapp->quit();
}

bool karmPart::save()
{
  kDebug(5970) << "Saving time data to disk." << endl;
  QString err=_taskView->save();  // untranslated error msg.

  if (err.isEmpty()) setStatusBar(i18n("Successfully saved tasks and history"));
  else setStatusBar(i18n(err.toAscii())); // no msgbox since save is called when exiting */
  return true;
}

int karmPart::addTask( const QString& taskname )
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

QString karmPart::setPerCentComplete( const QString& taskName, int perCent )
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
  if (err.isNull() && index>=0 )
  {
    _taskView->item_at_index(index)->setPercentComplete( perCent, _taskView->storage() );
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
QString karmPart::getError( int mkb ) const
{
  if ( mkb <= KARM_MAX_ERROR_NO ) return m_error[ mkb ];
  else return i18n( "Invalid error number: %1", mkb );
}

int karmPart::totalMinutesForTaskId( const QString& taskId )
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

QString karmPart::_hasTask( Task* task, const QString &taskname ) const
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

Task* karmPart::_hasUid( Task* task, const QString &uid ) const
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

QString karmPart::starttimerfor( const QString& taskname )
{
  QString err="no such task";
  for (int i=0; i<_taskView->count(); i++)
  {
    if ((_taskView->item_at_index(i)->name()==taskname))
    {
      _taskView->startTimerFor( _taskView->item_at_index(i) );
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
    if ((_taskView->item_at_index(i)->name()==taskname))
    {
      _taskView->stopTimerFor( _taskView->item_at_index(i) );
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

QString karmPart::exportcsvfile( QString filename, QString from, QString to, int type, bool decimalMinutes, bool allTasks, QString delimiter, QString quote )
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

QString karmPart::importplannerfile( QString fileName )
{
  return _taskView->importPlanner(fileName);
}



#include <q3popupmenu.h>
#include "karm_part.moc"
