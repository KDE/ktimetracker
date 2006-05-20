#include <QClipboard>
#include <QFile>
#include <QLayout>
#include <q3listbox.h>
#include <q3listview.h>
#include <q3ptrlist.h>
#include <q3ptrstack.h>
#include <QString>
#include <QTextStream>
#include <QTimer>
#include <QtXml>
#include <QMouseEvent>
#include <QList>
#include <QListWidget>

#include "kapplication.h"       // kapp
#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>            // i18n
#include <kmessagebox.h>
#include <kurlrequester.h>

#include "csvexportdialog.h"
#include "desktoptracker.h"
#include "edittaskdialog.h"
#include "idletimedetector.h"
#include "karmstorage.h"
#include "plannerparser.h"
#include "preferences.h"
#include "printdialog.h"
#include "reportcriteria.h"
#include "task.h"
#include "taskview.h"
#include "timekard.h"
#include "taskviewwhatsthis.h"

#define T_LINESIZE 1023
#define HIDDEN_COLUMN -10

class DesktopTracker;

TaskView::TaskView(QWidget *parent, const QString &icsfile ):K3ListView(parent)
{
  _preferences = Preferences::instance( icsfile );
  _storage = KarmStorage::instance();

  connect( this, SIGNAL( expanded( Q3ListViewItem * ) ),
           this, SLOT( itemStateChanged( Q3ListViewItem * ) ) );
  connect( this, SIGNAL( collapsed( Q3ListViewItem * ) ),
           this, SLOT( itemStateChanged( Q3ListViewItem * ) ) );

  // setup default values
  previousColumnWidths[0] = previousColumnWidths[1]
  = previousColumnWidths[2] = previousColumnWidths[3] = HIDDEN_COLUMN;

  addColumn( i18n("Task Name") );
  addColumn( i18n("Session Time") );
  addColumn( i18n("Time") );
  addColumn( i18n("Total Session Time") );
  addColumn( i18n("Total Time") );
  setColumnAlignment( 1, Qt::AlignRight );
  setColumnAlignment( 2, Qt::AlignRight );
  setColumnAlignment( 3, Qt::AlignRight );
  setColumnAlignment( 4, Qt::AlignRight );
  adaptColumns();
  setAllColumnsShowFocus( true );

  // set up the minuteTimer
  _minuteTimer = new QTimer(this);
  connect( _minuteTimer, SIGNAL( timeout() ), this, SLOT( minuteUpdate() ));
  _minuteTimer->start(1000 * secsPerMinute);

  // React when user changes iCalFile
  connect(_preferences, SIGNAL(iCalFile(QString)),
      this, SLOT(iCalFileChanged(QString)));

  // resize columns when config is changed
  connect(_preferences, SIGNAL( setupChanged() ), this,SLOT( adaptColumns() ));

  _minuteTimer->start(1000 * secsPerMinute);

  // Set up the idle detection.
  _idleTimeDetector = new IdleTimeDetector( _preferences->idlenessTimeout() );
  connect( _idleTimeDetector, SIGNAL( extractTime(int) ),
           this, SLOT( extractTime(int) ));
  connect( _idleTimeDetector, SIGNAL( stopAllTimers() ),
           this, SLOT( stopAllTimers() ));
  connect( _preferences, SIGNAL( idlenessTimeout(int) ),
           _idleTimeDetector, SLOT( setMaxIdle(int) ));
  connect( _preferences, SIGNAL( detectIdleness(bool) ),
           _idleTimeDetector, SLOT( toggleOverAllIdleDetection(bool) ));
  if (!_idleTimeDetector->isIdleDetectionPossible())
    _preferences->disableIdleDetection();

  // Setup auto save timer
  _autoSaveTimer = new QTimer(this);
  connect( _preferences, SIGNAL( autoSave(bool) ),
           this, SLOT( autoSaveChanged(bool) ));
  connect( _preferences, SIGNAL( autoSavePeriod(int) ),
           this, SLOT( autoSavePeriodChanged(int) ));
  connect( _autoSaveTimer, SIGNAL( timeout() ), this, SLOT( save() ));

  // Setup manual save timer (to save changes a little while after they happen)
  _manualSaveTimer = new QTimer(this);
  _manualSaveTimer->setSingleShot( true );
  connect( _manualSaveTimer, SIGNAL( timeout() ), this, SLOT( save() ));

  // Connect desktop tracker events to task starting/stopping
  _desktopTracker = new DesktopTracker();
  connect( _desktopTracker, SIGNAL( reachedtActiveDesktop( Task* ) ),
           this, SLOT( startTimerFor(Task*) ));
  connect( _desktopTracker, SIGNAL( leftActiveDesktop( Task* ) ),
           this, SLOT( stopTimerFor(Task*) ));
  new TaskViewWhatsThis( this );
  dragTask=0;
  setDragEnabled(true);
  setAcceptDrops(true);
}

void TaskView::contentsDropEvent(QDropEvent* qde)
{
  kDebug() << "This is contentsDropEvent" << endl;
  Task* t=static_cast<Task*>(this->itemAt(qde->pos()));
  TaskView::addTask( dragTask->name(), dragTask->totalTime(), dragTask->sessionTime(), dragTask->getDesktops(), t );

  QString uid=dragTask->uid();
  dragTask->remove(activeTasks, _storage);
  dragTask->removeFromView();
  _preferences->deleteEntry( uid ); // forget if the item was expanded or collapsed
  save();
}

void TaskView::startDrag()
{
  kDebug() << "Entering TaskView::startDrag" << endl;
  K3ListView::startDrag();  
}

Q3DragObject* TaskView::dragObject()
{
  kDebug() << "Entering TaskView::dragObject" << endl;
  dragTask=static_cast<Task *> (currentItem());
  return K3ListView::dragObject();
}

bool TaskView::acceptDrag(QDropEvent* e) const
{
  kDebug() << "Entering TaskView::acceptDrag" << endl;
  return K3ListView::acceptDrag(e);
}

KarmStorage* TaskView::storage()
{
  return _storage;
}

void TaskView::contentsMousePressEvent ( QMouseEvent * e )
{
  kDebug(5970) << "entering contentsMousePressEvent" << endl;
  K3ListView::contentsMousePressEvent(e);
  Task* task = current_item();

  // This checks that there has been a click onto an item,
  // not into an empty part of the K3ListView.
  if ( task != 0 &&  // zero can happen if there is no task
       e->pos().y() >= current_item()->itemPos() && 
       e->pos().y() < current_item()->itemPos()+current_item()->height() ) 

  { 
    // see if the click was on the completed icon
    int leftborder = treeStepSize() * ( task->depth() + ( rootIsDecorated() ? 1 : 0)) + itemMargin();
    if ((leftborder < e->x()) && (e->x() < 19 + leftborder ))
    {
      if ( task->isComplete() ) task->setPercentComplete( 0, _storage );
      else task->setPercentComplete( 100, _storage );
    }
    emit updateButtons();
  }
}

void TaskView::contentsMouseDoubleClickEvent ( QMouseEvent * e )
// if the user double-clicks onto a tasks, he says "I am now working exclusively
// on that task". That means, on a doubleclick, we check if it occurs on an item
// not in the blank space, if yes, stop all other tasks and start the new timer.
{
  kDebug(5970) << "entering contentsMouseDoubleClickEvent" << endl;
  K3ListView::contentsMouseDoubleClickEvent(e);
  
  Task *task = current_item();

  if ( task != 0 )  // current_item() exists
  {
    if ( e->pos().y() >= task->itemPos() &&   // doubleclick was onto current_item()
       e->pos().y() < task->itemPos()+task->height() )
    {
      if ( activeTasks.findRef(task) == -1 )  // task is active
      {
        stopAllTimers();
        startCurrentTimer();
      }
      else stopCurrentTimer();
    }
  }
}

TaskView::~TaskView()
{
  _preferences->save();
}

Task* TaskView::first_child() const
{
  return static_cast<Task*>(firstChild());
}

Task* TaskView::current_item() const
{
  return static_cast<Task*>(currentItem());
}

Task* TaskView::item_at_index(int i)
{
  return static_cast<Task*>(itemAtIndex(i));
}

void TaskView::load( QString fileName )
{
  // if the program is used as an embedded plugin for konqueror, there may be a need
  // to load from a file without touching the preferences.
  _isloading = true;
  QString err = _storage->load(this, _preferences, fileName);

  if (!err.isEmpty())
  {
    KMessageBox::error(this, err);
    _isloading = false;
    return;
  }

  // Register tasks with desktop tracker
  int i = 0;
  for ( Task* t = item_at_index(i); t; t = item_at_index(++i) )
    _desktopTracker->registerForDesktops( t, t->getDesktops() );

  restoreItemState( first_child() );

  setSelected(first_child(), true);
  setCurrentItem(first_child());
  _desktopTracker->startTracking();
  _isloading = false;
  refresh();
}

void TaskView::restoreItemState( Q3ListViewItem *item )
{
  while( item ) 
  {
    Task *t = (Task *)item;
    t->setOpen( _preferences->readBoolEntry( t->uid() ) );
    if( item->childCount() > 0 ) restoreItemState( item->firstChild() );
    item = item->nextSibling();
  }
}

void TaskView::itemStateChanged( Q3ListViewItem *item )
{
  if ( !item || _isloading ) return;
  Task *t = (Task *)item;
  kDebug(5970) << "TaskView::itemStateChanged()" 
    << " uid=" << t->uid() << " state=" << t->isOpen()
    << endl;
  if( _preferences ) _preferences->writeEntry( t->uid(), t->isOpen() );
}

void TaskView::closeStorage() { _storage->closeStorage( this ); }

void TaskView::iCalFileModified(ResourceCalendar *rc)
{
  kDebug(5970) << "entering iCalFileModified" << endl;
  kDebug(5970) << rc->infoText() << endl;
  rc->dump();
  _storage->buildTaskView(rc,this);
  kDebug(5970) << "exiting iCalFileModified" << endl;
}

void TaskView::refresh()
{
  kDebug(5970) << "entering TaskView::refresh()" << endl;
  int i = 0;
  for ( Task* t = item_at_index(i); t; t = item_at_index(++i) )
  {
    t->setPixmapProgress();
    t->update();  // maybe there was a change in the times's format
  }
  
  // remove root decoration if there is no more child.
  i = 0;
  while ( item_at_index( ++i ) && ( item_at_index( i )->depth() == 0 ) );
  setRootIsDecorated( item_at_index( i ) && ( item_at_index( i )->depth() != 0 ) );

  emit updateButtons();
  kDebug(5970) << "exiting TaskView::refresh()" << endl;
}
    
QString TaskView::importPlanner(QString fileName)
{
  kDebug(5970) << "entering importPlanner" << endl;
  PlannerParser* handler=new PlannerParser(this);
  if (fileName.isEmpty()) fileName=KFileDialog::getOpenFileName(QString(), QString(), 0);
  QFile xmlFile( fileName );
  QXmlInputSource source( &xmlFile );
  QXmlSimpleReader reader;
  reader.setContentHandler( handler );
  reader.parse( source );
  refresh();
  return "";
}

QString TaskView::report( const ReportCriteria& rc )
{
  return _storage->report( this, rc );
}

void TaskView::exportcsvFile()
{
  kDebug(5970) << "TaskView::exportcsvFile()" << endl;

  CSVExportDialog dialog( ReportCriteria::CSVTotalsExport, this );
  if ( current_item() && current_item()->isRoot() )
    dialog.enableTasksToExportQuestion();
  dialog.urlExportTo->KUrlRequester::setMode(KFile::File);
  if ( dialog.exec() ) {
    QString err = _storage->report( this, dialog.reportCriteria() );
    if ( !err.isEmpty() ) KMessageBox::error( this, i18n(err.toAscii()) );
  }
}

QString TaskView::exportcsvHistory()
{
  kDebug(5970) << "TaskView::exportcsvHistory()" << endl;
  QString err;
  
  CSVExportDialog dialog( ReportCriteria::CSVHistoryExport, this );
  if ( current_item() && current_item()->isRoot() )
    dialog.enableTasksToExportQuestion();
  dialog.urlExportTo->KUrlRequester::setMode(KFile::File);
  if ( dialog.exec() ) {
    err = _storage->report( this, dialog.reportCriteria() );
  }
  return err;
}

void TaskView::scheduleSave()
{
    _manualSaveTimer->start( 10 );
}

Preferences* TaskView::preferences() { return _preferences; }

QString TaskView::save()
{
    // DF: this code created a new event for the running task(s),
    // at every call (very frequent with autosave) !!!
    // -> if one wants autosave to save the current event, then
    // Task needs to store the "current event" and we need to update
    // it before calling save.
#if 0
  // Stop then start all timers so history entries are written.  This is
  // inefficient if more than one task running, but it is correct.  It is
  // inefficient because the iCalendar file is saved every time a task's
  // setRunning(false, ...) is called.  For a big ics file, this could be a
  // drag.  However, it does ensure that the data will be consistent.  And
  // if the most common use case is that one task is running most of the time,
  // it won't make any difference.
  for (unsigned int i = 0; i < activeTasks.count(); i++)
  {
    activeTasks.at(i)->setRunning(false, _storage);
    activeTasks.at(i)->setRunning(true, _storage);
  }

  // If there was an active task, the iCal file has already been saved.
  if (activeTasks.count() == 0)
#endif
  {
    kDebug(5970) << "Entering TaskView::save()" << endl;
    QString err=_storage->save(this);

    emit setStatusBarText( err.isNull() ? i18n("Saved successfully") : i18n("Error during saving") );
    return err;
  }
}

void TaskView::startCurrentTimer()
{
  startTimerFor( current_item() );
}

long TaskView::count()
{
  long n = 0;
  for (Task* t = item_at_index(n); t; t=item_at_index(++n));
  return n;
}

void TaskView::startTimerFor(Task* task, QDateTime startTime )
{
  if (task != 0 && activeTasks.findRef(task) == -1) 
  {
    if (_preferences->uniTasking()) stopAllTimers();
    _idleTimeDetector->startIdleDetection();
    task->setRunning(true, _storage, startTime);
    activeTasks.append(task);
    emit updateButtons();
    if ( activeTasks.count() == 1 )
        emit timersActive();

    emit tasksChanged( activeTasks);
  }
}

void TaskView::clearActiveTasks()
{
  activeTasks.clear();
}

void TaskView::stopAllTimers()
{
  for ( unsigned int i = 0; i < activeTasks.count(); i++ )
    activeTasks.at(i)->setRunning(false, _storage);

  _idleTimeDetector->stopIdleDetection();
  activeTasks.clear();
  emit updateButtons();
  emit timersInactive();
  emit tasksChanged( activeTasks);
}

void TaskView::startNewSession()
{
  Q3ListViewItemIterator item( first_child());
  for ( ; item.current(); ++item ) {
    Task * task = (Task *) item.current();
    task->startNewSession();
  }
}

void TaskView::resetTimeForAllTasks()
{
  Q3ListViewItemIterator item( first_child());
  for ( ; item.current(); ++item ) {
    Task * task = (Task *) item.current();
    task->resetTimes();
  }
}

void TaskView::stopTimerFor(Task* task)
{
  if ( task != 0 && activeTasks.findRef(task) != -1 ) {
    activeTasks.removeRef(task);
    task->setRunning(false, _storage);
    if ( activeTasks.count() == 0 ) {
      _idleTimeDetector->stopIdleDetection();
      emit timersInactive();
    }
    emit updateButtons();
  }
  emit tasksChanged( activeTasks);
}

void TaskView::stopCurrentTimer()
{
  stopTimerFor( current_item());
}

void TaskView::minuteUpdate()
{
  addTimeToActiveTasks(1, false);
}

void TaskView::addTimeToActiveTasks(int minutes, bool save_data)
{
  for( unsigned int i = 0; i < activeTasks.count(); i++ )
    activeTasks.at(i)->changeTime(minutes, ( save_data ? _storage : 0 ) );
}

void TaskView::newTask()
{
  newTask(i18n("New Task"), 0);
}

void TaskView::newTask(QString caption, Task *parent)
{
  EditTaskDialog *dialog = new EditTaskDialog(caption, false);
  long total, totalDiff, session, sessionDiff;
  DesktopList desktopList;

  int result = dialog->exec();
  if ( result == QDialog::Accepted ) {
    QString taskName = i18n( "Unnamed Task" );
    if ( !dialog->taskName().isEmpty()) taskName = dialog->taskName();

    total = totalDiff = session = sessionDiff = 0;
    dialog->status( &total, &totalDiff, &session, &sessionDiff, &desktopList );

    // If all available desktops are checked, disable auto tracking,
    // since it makes no sense to track for every desktop.
    if ( desktopList.size() ==  _desktopTracker->desktopCount() )
      desktopList.clear();

    QString uid = addTask( taskName, total, session, desktopList, parent );
    if ( uid.isNull() )
    {
      KMessageBox::error( 0, i18n(
            "Error storing new task. Your changes were not saved. Make sure you can edit your iCalendar file. Also quit all applications using this file and remove any lock file related to its name from ~/.kde/share/apps/kabc/lock/ " ) );
    }
  }
}

QString TaskView::addTask
( const QString& taskname, long total, long session, 
  const DesktopList& desktops, Task* parent )
{
  kDebug(5970) << "Entering TaskView::addTask; taskname = " << taskname << endl;
  Task *task;
  if ( parent ) task = new Task( taskname, total, session, desktops, parent );
  else          task = new Task( taskname, total, session, desktops, this );

  task->setUid( _storage->addTask( task, parent ) );
  QString taskuid=task->uid();
  if ( ! taskuid.isNull() )
  {
    _desktopTracker->registerForDesktops( task, desktops );
    setCurrentItem( task );
    setSelected( task, true );
    task->setPixmapProgress();
    save();
  }
  else
  {
    delete task;
  }

  return taskuid;
}

void TaskView::newSubTask()
{
  Task* task = current_item();
  if(!task)
    return;
  newTask(i18n("New Sub Task"), task);
  task->setOpen(true);
  refresh();
}

void TaskView::editTask()
{
  kDebug(5970) << "Entering editTask" << endl;
  Task *task = current_item();
  if (!task)
    return;

  DesktopList desktopList = task->getDesktops();
  DesktopList oldDeskTopList = desktopList;
  EditTaskDialog *dialog = new EditTaskDialog(i18n("Edit Task"), true, &desktopList);
  dialog->setTask( task->name(),
                   task->time(),
                   task->sessionTime() );
  int result = dialog->exec();
  if (result == QDialog::Accepted) 
  {
    QString taskName = i18n("Unnamed Task");
    if (!dialog->taskName().isEmpty()) 
    {
      taskName = dialog->taskName();
    }
    // setName only does something if the new name is different
    task->setName(taskName, _storage);

    // update session time as well if the time was changed
    long total, session, totalDiff, sessionDiff;
    total = totalDiff = session = sessionDiff = 0;
    DesktopList desktopList;
    dialog->status( &total, &totalDiff, &session, &sessionDiff, &desktopList);

    if( totalDiff != 0 || sessionDiff != 0)
      task->changeTimes( sessionDiff, totalDiff, _storage );

    // If all available desktops are checked, disable auto tracking,
    // since it makes no sense to track for every desktop.
    if (desktopList.size() == _desktopTracker->desktopCount())
      desktopList.clear();
    // only do something for autotracking if the new setting is different
    if ( oldDeskTopList != desktopList )
    { 
      task->setDesktopList(desktopList);
      _desktopTracker->registerForDesktops( task, desktopList );
    }
    emit updateButtons();
  }
}

//void TaskView::addCommentToTask()
//{
//  Task *task = current_item();
//  if (!task)
//    return;

//  bool ok;
//  QString comment = KLineEditDlg::getText(i18n("Comment"),
//                       i18n("Log comment for task '%1':").arg(task->name()),
//                       QString(), &ok, this);
//  if ( ok )
//    task->addComment( comment, _storage );
//}

void TaskView::reinstateTask(int completion)
{
  Task* task = current_item();
  if (task == 0) {
    KMessageBox::information(0,i18n("No task selected."));
    return;
  }

  if (completion<0) completion=0;
  if (completion<100)
  {
    task->setPercentComplete(completion, _storage);
    task->setPixmapProgress();
    save();
    emit updateButtons();
  }
}

void TaskView::deleteTask(bool markingascomplete)
{
  kDebug(5970) << "Entering TaskView::deleteTask" << endl;
  Task *task = current_item();
  if (task == 0) {
    KMessageBox::information(0,i18n("No task selected."));
    return;
  }

  int response = KMessageBox::Continue;
  if (!markingascomplete && _preferences->promptDelete()) {
    if (task->childCount() == 0) {
      response = KMessageBox::warningContinueCancel( 0,
          i18n( "Are you sure you want to delete "
          "the task named\n\"%1\" and its entire history?",
           task->name()),
          i18n( "Deleting Task"), KStdGuiItem::del());
    }
    else {
      response = KMessageBox::warningContinueCancel( 0,
          i18n( "Are you sure you want to delete the task named"
          "\n\"%1\" and its entire history?\n"
          "NOTE: all its subtasks and their history will also "
          "be deleted.", task->name()),
          i18n( "Deleting Task"), KStdGuiItem::del());
    }
  }

  if (response == KMessageBox::Continue)
  {
    if (markingascomplete)
    {
      task->setPercentComplete(100, _storage);
      task->setPixmapProgress();
      save();
      emit updateButtons();

      // Have to remove after saving, as the save routine only affects tasks
      // that are in the view.  Otherwise, the new percent complete does not
      // get saved.   (No longer remove when marked as complete.)
      //task->removeFromView();

    }
    else
    {
      QString uid=task->uid();
      task->remove(activeTasks, _storage);
      task->removeFromView();
      _preferences->deleteEntry( uid ); // forget if the item was expanded or collapsed
      save();
    }

    // remove root decoration if there is no more children.
    refresh();

    // Stop idle detection if no more counters are running
    if (activeTasks.count() == 0) {
      _idleTimeDetector->stopIdleDetection();
      emit timersInactive();
    }

    emit tasksChanged( activeTasks );
  }
}

void TaskView::extractTime(int minutes)
{
  addTimeToActiveTasks(-minutes);
}

void TaskView::autoSaveChanged(bool on)
{
  if (on) _autoSaveTimer->start(_preferences->autoSavePeriod()*1000*secsPerMinute);
  else if (_autoSaveTimer->isActive()) _autoSaveTimer->stop();
}

void TaskView::autoSavePeriodChanged(int /*minutes*/)
{
  autoSaveChanged(_preferences->autoSave());
}

void TaskView::adaptColumns()
{
  // to hide a column X we set it's width to 0
  // at that moment we'll remember the original column within
  // previousColumnWidths[X]
  //
  // When unhiding a previously hidden column
  // (previousColumnWidths[X] != HIDDEN_COLUMN !)
  // we restore it's width from the saved value and set
  // previousColumnWidths[X] to HIDDEN_COLUMN

  for( int x=1; x <= 4; x++) {
    // the column was invisible before and were switching it on now
    if(   _preferences->displayColumn(x-1)
       && previousColumnWidths[x-1] != HIDDEN_COLUMN )
    {
      setColumnWidth( x, previousColumnWidths[x-1] );
      previousColumnWidths[x-1] = HIDDEN_COLUMN;
      setColumnWidthMode( x, Q3ListView::Maximum );
    }
    // the column was visible before and were switching it off now
    else
    if( ! _preferences->displayColumn(x-1)
       && previousColumnWidths[x-1] == HIDDEN_COLUMN )
    {
      setColumnWidthMode( x, Q3ListView::Manual ); // we don't want update()
                                                  // to resize/unhide the col
      previousColumnWidths[x-1] = columnWidth( x );
      setColumnWidth( x, 0 );
    }
  }
  // maybe this slot is called because the times' format changed, so do a ...
  refresh();
}

void TaskView::deletingTask(Task* deletedTask)
{
  DesktopList desktopList;

  _desktopTracker->registerForDesktops( deletedTask, desktopList );
  activeTasks.removeRef( deletedTask );

  emit tasksChanged( activeTasks);
}

void TaskView::iCalFileChanged(QString file)
{
  kDebug(5970) << "TaskView:iCalFileChanged: " << file << endl;
  stopAllTimers();
  _storage->save(this);
  load();
}

QList<HistoryEvent> TaskView::getHistory(const QDate& from,
    const QDate& to) const
{
  return _storage->getHistory(from, to);
}

void TaskView::markTaskAsComplete()
{
  if (current_item())
    kDebug(5970) << "TaskView::markTaskAsComplete: "
      << current_item()->uid() << endl;
  else
    kDebug(5970) << "TaskView::markTaskAsComplete: null current_item()" << endl;

  bool markingascomplete = true;
  deleteTask(markingascomplete);
}

void TaskView::markTaskAsIncomplete()
{
  if (current_item())
    kDebug(5970) << "TaskView::markTaskAsComplete: "
      << current_item()->uid() << endl;
  else
    kDebug(5970) << "TaskView::markTaskAsComplete: null current_item()" << endl;

  reinstateTask(50); // if it has been reopened, assume half-done
}


QString TaskView::clipTotals( const ReportCriteria &rc )
// This function stores the user's tasks into the clipboard.
// rc tells how the user wants his report, e.g. all times or session times
{
  kDebug(5970) << "Entering clipTotals" << endl;
  QString err=QString();
  TimeKard t;
  KApplication::clipboard()->setText(t.totalsAsText(this, rc));
  return err;
}

QString TaskView::clipHistory()
{
  QString err=QString();
  PrintDialog dialog;
  if (dialog.exec()== QDialog::Accepted)
  {
    TimeKard t;
    KApplication::clipboard()->
      setText( t.historyAsText(this, dialog.from(), dialog.to(), !dialog.allTasks(), dialog.perWeek(), dialog.totalsOnly() ) );
  }
  return err;
}

#include "taskview.moc"
