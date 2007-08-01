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

#include "taskview.h"

#include <cassert>

#include <QClipboard>
#include <QFile>
#include <QLayout>
#include <QHeaderView>
#include <QItemDelegate>
#include <QPainter>
#include <QString>
#include <QTableWidget>
#include <QTextStream>
#include <QTimer>
#include <QtXml/QtXml>
#include <QMouseEvent>
#include <QList>
#include <QListWidget>

#include <KApplication>       // kapp
#include <KConfig>
#include <KDebug>
#include <KFileDialog>
#include <KLocale>            // i18n
#include <KMessageBox>
#include <KUrlRequester>

#include "csvexportdialog.h"
#include "desktoptracker.h"
#include "edittaskdialog.h"
#include "idletimedetector.h"
#include "plannerparser.h"
#include "preferences.h"
#include "printdialog.h"
#include "task.h"
#include "timekard.h"
#include "treeviewheadercontextmenu.h"

#define T_LINESIZE 1023

class DesktopTracker;

class TaskViewDelegate : public QItemDelegate {
public:
  TaskViewDelegate( QObject *parent = 0 ) : QItemDelegate( parent ) {}

  void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const {
    if (index.column () == 5) {
      if (option.state & QStyle::State_Selected) {
        painter->fillRect( option.rect, option.palette.highlight() );
      }
      int rX = option.rect.x() + 2;
      int rY = option.rect.y() + 2;
      int rWidth = option.rect.width() - 4;
      int rHeight = option.rect.height() - 4;

      int value = index.model()->data( index ).toInt();
      int newWidth = (int)(rWidth * (value / 100.));

      int mid = rY + rHeight / 2;
      int width = rWidth / 2;

      QLinearGradient gradient1( rX, mid, rX + width, mid);
      gradient1.setColorAt( 0, Qt::red );
      gradient1.setColorAt( 1, Qt::yellow );
      painter->fillRect( rX, rY, (newWidth < width) ? newWidth : width, rHeight, gradient1 );

      if (newWidth > width) {
        QLinearGradient gradient2( rX + width, mid, rX + 2 * width, mid);
        gradient2.setColorAt( 0, Qt::yellow );
        gradient2.setColorAt( 1, Qt::green );
        painter->fillRect( rX + width, rY, newWidth - width, rHeight, gradient2 );
      }

      painter->setPen( option.state & QStyle::State_Selected ? option.palette.highlight().color() : option.palette.background().color() );
      for (int x = rHeight; x < newWidth; x += rHeight) {
        painter->drawLine( rX + x, rY, rX + x, rY + rHeight - 1 );
      }

      painter->setPen( Qt::black );
      painter->drawText( option.rect, Qt::AlignCenter | Qt::AlignVCenter, QString::number(value) + " %" );
    } else {
      QItemDelegate::paint( painter, option, index );
    }
  }
};

TaskView::TaskView( const QString &icsfile, QWidget *parent ) 
  : QTreeWidget(parent)
{
  assert( !( icsfile.isEmpty() ) );
  _preferences = Preferences::instance();
  _storage = KarmStorage::instance();
  _focusDetector = new FocusDetector(1);
  focustrackingactive = false;

  connect( this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
           this, SLOT(itemStateChanged(QTreeWidgetItem*)) );
  connect( this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
           this, SLOT(itemStateChanged(QTreeWidgetItem*)) );
  connect( this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
           this, SLOT(slotItemDoubleClicked(QTreeWidgetItem*, int)) );
  connect( _focusDetector, SIGNAL( newFocus( const QString & ) ), 
           this, SLOT( newFocusWindowDetected ( const QString & ) ) ); 

  QStringList labels;
  labels << i18n("Task Name") << i18n("Session Time") << i18n("Time") << i18n("Total Session Time") << i18n("Total Time") << i18n("Percent Complete") ;
  setHeaderLabels(labels);
  headerItem()->setWhatsThis(0,"The task name is how you call the task, it can be chose freely.");
  headerItem()->setWhatsThis(1,"The session time is the time since you last chose \"start new session.\"");
  adaptColumns();
  setAllColumnsShowFocus( true );
  setItemDelegateForColumn( 5, new TaskViewDelegate(this) );

  // set up the minuteTimer
  _minuteTimer = new QTimer(this);
  connect( _minuteTimer, SIGNAL( timeout() ), this, SLOT( minuteUpdate() ));
  _minuteTimer->start(1000 * secsPerMinute);

  // resize columns when config is changed
  connect(_preferences, SIGNAL( setupChanged() ), this,SLOT( adaptColumns() ));

  _minuteTimer->start(1000 * secsPerMinute);

  // Set up the idle detection.
  _idleTimeDetector = new IdleTimeDetector( _preferences->idlenessTimeout() );
  connect( _idleTimeDetector, SIGNAL( extractTime(int) ),
           this, SLOT( extractTime(int) ));
  connect( _idleTimeDetector, SIGNAL( stopAllTimers(QDateTime) ),
           this, SLOT( stopAllTimers(QDateTime) ));
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
  connect( _desktopTracker, SIGNAL( reachedActiveDesktop( Task* ) ),
           this, SLOT( startTimerFor(Task*) ));
  connect( _desktopTracker, SIGNAL( leftActiveDesktop( Task* ) ),
           this, SLOT( stopTimerFor(Task*) ));
  dragTask=0;
  setDragEnabled(true);
  setAcceptDrops(true);

  // Header context menu
  TreeViewHeaderContextMenu *headerContextMenu = new TreeViewHeaderContextMenu( this, this, TreeViewHeaderContextMenu::AlwaysCheckBox, QVector<int>() << 0 );
  connect( headerContextMenu, SIGNAL(columnToggled(int)), this, SLOT(slotColumnToggled(int)) );
}

void TaskView::newFocusWindowDetected( const QString &taskName )
{
  QString newTaskName = taskName;
  newTaskName.replace( "\n", "" );

  if ( focustrackingactive ) {
    bool found = false;  // has taskName been found in our tasks
    stopTimerFor( lastTaskWithFocus );
    int i = 0;
    for ( Task* task = item_at_index( i ); task; task = item_at_index( ++i ) ) {
      if ( task->name() == newTaskName ) {
         found = true;
         startTimerFor( task );
         lastTaskWithFocus = task;
      }
    }
    if ( !found ) {
      QString taskuid = addTask( newTaskName );
      if ( taskuid.isNull() ) {
        KMessageBox::error( 0, i18n(
        "Error storing new task. Your changes were not saved. Make sure you can edit your iCalendar file. Also quit all applications using this file and remove any lock file related to its name from ~/.kde/share/apps/kabc/lock/ " ) );
      }
      i = 0;
      for ( Task* task = item_at_index( i ); task; task = item_at_index( ++i ) ) {
        if (task->name() == newTaskName) {
          startTimerFor( task );
          lastTaskWithFocus = task;
        }
      }
    }
    emit updateButtons();
  } // focustrackingactive
}

void TaskView::dropEvent(QDropEvent* qde)
{
  kDebug(5970) << "This is dropEvent" << endl;
  if (this->itemAt(qde->pos())==0) 
  // the task was dropped to an empty place
  {
    // if the task is a sub-task, it shall be freed from its parents, otherwise, nothing happens
    kDebug(5970) << "User drops onto an empty place" << endl;
    if (dragTask->parent())
    {
      _storage->setTaskParent(dragTask,0);
      QTreeWidgetItem* item=dragTask->parent()->takeChild(dragTask->parent()->indexOfChild(dragTask));
      this->insertTopLevelItem(0,item); 
    }
  }
  else
  {
    Task* t = static_cast<Task*>( this->itemAt(qde->pos()) );
    kDebug(5970) << "taking " << dragTask->name() << " dropping onto " << t->name() << endl;
    // is t==dragTask
    if (t != dragTask) 
    {
      // is dragTask an ancestor to t ?
      bool isAncestor=false;
      Task* ancestor=t->parent();
      while (ancestor!=0)
      {
        if (ancestor==dragTask) isAncestor=true;
        kDebug() << "testing " << ancestor->name() << endl;
        kDebug() << "isAncestor=" << isAncestor << endl;
        ancestor=ancestor->parent();
      }
      if (isAncestor) kDebug(5970) << "User dropped a task on its subtask" << endl;
      else  // move the task
      {
        if (dragTask->parent())
        // if the task is a subtask
        {
          dragTask->parent()->takeChild(dragTask->parent()->indexOfChild(dragTask));
          t->addChild( dragTask );
          dragTask->move(t);
          _storage->setTaskParent( dragTask, t );
        }
        else
        // if the task is no subtask
        {
          int indexOfDragTask = indexOfTopLevelItem( dragTask );
          if (indexOfDragTask != -1) 
          {
            takeTopLevelItem( indexOfDragTask );
            t->addChild( dragTask );
          }
        }
        save();
        reFresh();  // setRootIsDecorated may be needed
      }
    }
  }
}

void TaskView::startDrag(Qt::DropActions action)
{
  kDebug(5970) << "Entering TaskView::startDrag" << endl;
  dragTask=(Task*) currentItem();
  kDebug(5970) << "dragTask is " << dragTask->name() << endl;
  QTreeWidget::startDrag(action);  
}

void TaskView::mouseMoveEvent( QMouseEvent *event ) 
{
  QModelIndex index = indexAt( event->pos() );
  
  if (index.isValid() && index.column() == 5) 
  {
    int newValue = (int)((event->pos().x() - visualRect(index).x()) / (double)(visualRect(index).width()) * 100);
    QTreeWidgetItem *item = itemFromIndex( index );
    if (item && item->isSelected()) 
    {
      Task *task = static_cast<Task*>(item);
      if (task) 
      {
        task->setPercentComplete( newValue, _storage );
        
        emit updateButtons();
      }
    }
  } 
  else 
  {
    QTreeWidget::mouseMoveEvent( event );
  }
}

void TaskView::mousePressEvent( QMouseEvent *event ) {
  QModelIndex index = indexAt( event->pos() );
  
  if (   index.isValid() 
      && index.column() == 0
      && visualRect( index ).x() <= event->pos().x()
      && event->pos().x() < visualRect( index ).x() + 19) {
    QTreeWidgetItem *item = itemFromIndex( index );
    if (item) {
      Task *task = static_cast<Task*>(item);
      if (task) {
        if (task->isComplete()) {
          task->setPercentComplete( 0, _storage );
        } else {
          task->setPercentComplete( 100, _storage );
        }
        
        emit updateButtons();
      }
    }
  } else {
    QTreeWidget::mousePressEvent( event );
  }
}

KarmStorage* TaskView::storage()
{
  return _storage;
}

TaskView::~TaskView()
{
  _preferences->save();
}

Task* TaskView::first_child() const
{
  kDebug() << "Entering TaskView::first_child" << endl;
  Task* t = static_cast<Task*>(topLevelItem(0));
  kDebug() << "Leaving TaskView::first_child" << endl;
  return t;
}

Task* TaskView::current_item() const
{
  kDebug() << "Entering TaskView::current_item" << endl;
  return static_cast<Task*>(currentItem());
}

Task* TaskView::item_at_index(int i)
/* This procedure delivers the item at the position i in the KTreeWidget.
Every item is a task. The items are counted linearily. The uppermost item
has the number i=0. */
{
  kDebug(5970) << "Entering TaskView::item_at_index(" << i << ")" << endl;
  if (!first_child()) return 0;

  QTreeWidgetItemIterator item(first_child());
  while( *item && i-- ) ++item;
 
  kDebug(5970) << "Leaving TaskView::item_at_index" << endl;
  if (!*item) return 0;
  else return (Task*) *item;
}

void TaskView::load( const QString &fileName )
{
  assert( !( fileName.isEmpty() ) );

  // if the program is used as an embedded plugin for konqueror, there may be a need
  // to load from a file without touching the preferences.
  kDebug(5970) << "Entering TaskView::load" << endl;
  _isloading = true;
  QString err = _storage->load(this, _preferences, fileName);

  if (!err.isEmpty())
  {
    KMessageBox::error(this, err);
    _isloading = false;
    kDebug(5970) << "Leaving TaskView::load" << endl;
    return;
  }

  // Register tasks with desktop tracker
  int i = 0;
  for ( Task* t = item_at_index(i); t; t = item_at_index(++i) )
    _desktopTracker->registerForDesktops( t, t->getDesktops() );

  if (first_child())
  {
    restoreItemState();
    first_child()->setSelected(true);
    setCurrentItem(first_child());
    _desktopTracker->startTracking();
    _isloading = false;
    kDebug(5970) << "load calls refesh" << endl;
    refresh();
  }
  kDebug(5970) << "Leaving TaskView::load" << endl;
}

void TaskView::restoreItemState()
/* Restores the item state of every item. An item is a task in the list.
Its state is whether it is expanded or not. If a task shall be expanded
is stored in the _preferences object. */
{
  kDebug(5970) << "Entering TaskView::restoreItemState" << endl;
  
  if (first_child()) {
    QTreeWidgetItemIterator item(first_child());
    while( *item ) 
    {
      Task *t = (Task *) *item;
      t->setExpanded( _preferences->readBoolEntry( t->uid() ) );
      ++item;
    }
  }
  kDebug(5970) << "Leaving TaskView::restoreItemState" << endl;
}

void TaskView::itemStateChanged( QTreeWidgetItem *item )
{
  kDebug() << "Entering TaskView::itemStateChanged" << endl;
  if ( !item || _isloading ) return;
  Task *t = (Task *)item;
  kDebug(5970) << "TaskView::itemStateChanged()" << " uid=" << t->uid() << " state=" << t->isExpanded() << endl;
  if( _preferences ) _preferences->writeEntry( t->uid(), t->isExpanded() );
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
    
void TaskView::reFresh()
{
  refresh();
}

QString TaskView::importPlanner( const QString &fileName )
{
  kDebug( 5970 ) << "entering importPlanner" << endl;
  PlannerParser *handler = new PlannerParser( this );
  QString lFileName = fileName;
  if ( lFileName.isEmpty() ) 
    lFileName = KFileDialog::getOpenFileName( QString(), QString(), 0 );
  QFile xmlFile( lFileName );
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
    kDebug(5970) << "Entering TaskView::save(ListView)" << endl;
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

void TaskView::startTimerFor( Task* task, const QDateTime &startTime )
{
  if (task != 0 && activeTasks.indexOf(task) == -1) 
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

void TaskView::stopAllTimers( const QDateTime &when )
{
  kDebug(5970) << "Entering TaskView::stopAllTimers" << endl;
  foreach ( Task *task, activeTasks )
    task->setRunning( false, _storage, when );

  _idleTimeDetector->stopIdleDetection();
  _focusDetector->stopFocusDetection(); 
  activeTasks.clear();
  emit updateButtons();
  emit timersInactive();
  emit tasksChanged( activeTasks);
}

void TaskView::toggleFocusTracking()
{
  focustrackingactive = !focustrackingactive;

  if ( focustrackingactive ) {
    _focusDetector->startFocusDetection();
  } else {
    stopTimerFor( lastTaskWithFocus );
    _focusDetector->stopFocusDetection();
  }
}

void TaskView::startNewSession()
/* This procedure starts a new session. Technically, a session is just an additionally 
stored time that is always contain in the overall time. We speak of session times, 
overalltimes (comprising all sessions) and total times (comprising all subtasks).
That is why there is also a total session time. */
{
  kDebug(5970) << "Entering TaskView::startNewSession" << endl;
  QTreeWidgetItemIterator item( first_child() );
  while ( *item )
  {
    Task * task = (Task *) *item;
    task->startNewSession();
    ++item;
  }
  kDebug(5970) << "Leaving TaskView::startNewSession" << endl;
}

void TaskView::resetTimeForAllTasks()
/* This procedure resets all times (session and overall) for all tasks and subtasks. */
{
  kDebug(5970) << "Entering TaskView::resetTimeForAllTasks" << endl;
  QTreeWidgetItemIterator item( first_child() );
  while ( *item ) 
  {
    Task * task = (Task *) *item;
    task->resetTimes();
    ++item;
  }
  kDebug(5970) << "Leaving TaskView::resetTimeForAllTasks" << endl;
}

void TaskView::stopTimerFor(Task* task)
{
  if ( task != 0 && activeTasks.indexOf(task) != -1 ) {
    activeTasks.removeAll(task);
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
  foreach ( Task *task, activeTasks )
    task->changeTime( minutes, ( save_data ? _storage : 0 ) );
}

void TaskView::newTask()
{
  newTask(i18n("New Task"), 0);
}

void TaskView::newTask( const QString &caption, Task *parent )
{
  EditTaskDialog *dialog = new EditTaskDialog( this, caption, false );
  long total, totalDiff, session, sessionDiff;
  DesktopList desktopList;

  int result = dialog->exec();
  if ( result == QDialog::Accepted ) 
  {
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
  emit updateButtons();
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
    task->setSelected( true );
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
  task->setExpanded(true);
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
  EditTaskDialog *dialog = new EditTaskDialog( this, i18n("Edit Task"), true, &desktopList );
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
          i18n( "Deleting Task"), KStandardGuiItem::del());
    }
    else {
      response = KMessageBox::warningContinueCancel( 0,
          i18n( "Are you sure you want to delete the task named"
          "\n\"%1\" and its entire history?\n"
          "NOTE: all its subtasks and their history will also "
          "be deleted.", task->name()),
          i18n( "Deleting Task"), KStandardGuiItem::del());
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
      task->remove(_storage);
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
  addTimeToActiveTasks(-minutes,false); // subtract time in memory, but do not store it
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
/* This procedure adapts the columns, it can e.g. be called when the user
changes the time format or requests to hide some columns.
The columns are by default:
x - displaycolumn - name
0 - -             - task name
1 - 0             - session time
2 - 1             - time 
3 - 2             - total session time
4 - 3             - total time
5 - 4             - percent complete  */
{
  kDebug(5970) << "Entering TaskView::adaptColumns" << endl;
  for( int x=1; x <= 5; x++) 
  {
    if ( _preferences->displayColumn(x-1) ) setColumnHidden( x, false );
    else setColumnHidden( x, true );
  }
  // maybe this slot is called because the times' format changed, so do a ...
  refresh();
}

void TaskView::deletingTask(Task* deletedTask)
{
  DesktopList desktopList;

  _desktopTracker->registerForDesktops( deletedTask, desktopList );
  activeTasks.removeAll( deletedTask );

  emit tasksChanged( activeTasks);
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

void TaskView::slotItemDoubleClicked( QTreeWidgetItem *item, int )
{
  if (item) {
    Task *task = static_cast<Task*>( item );
    if (task) {
      if (activeTasks.indexOf(task) == -1) { // task is active
        stopAllTimers();
        startCurrentTimer();
      } else {
        stopCurrentTimer();
      }
    }
  }
}

void TaskView::slotColumnToggled( int column )
{
  kDebug() << "column: " << column << endl;
  _preferences->setDisplayColumn( column - 1, !isColumnHidden(column) );
  _preferences->save();
}

#include "taskview.moc"
