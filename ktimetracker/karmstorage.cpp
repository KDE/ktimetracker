/*
 *     Copyright (C) 2003, 2004 by Mark Bucciarelli <mark@hubcapconsutling.com>
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

#include "karmstorage.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cassert>

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QList>
#include <QMultiHash>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QTextStream>

#include <KApplication>       // kapp
#include <KDebug>
#include <KLocale>            // i18n
#include <KProgressDialog>
#include <KUrl>
#include <KTemporaryFile>

#include <kemailsettings.h>
#include <kcal/incidence.h>
#include <kcal/resourcecalendar.h>
#include <kcal/resourcelocal.h>
#include <resourceremote.h>
#include <kpimprefs.h>  // for timezone
#include <kio/netaccess.h>

#include "edithistorydialog.h"
#include "taskview.h"
#include "timekard.h"
#include "karmutility.h"
#include "ktimetracker.h"
#include "task.h"
#include "storageadaptor.h"

static long linenr;  // how many lines written by printTaskHistory so far

//@cond PRIVATE
class KarmStorage::Private {
  public:
    Private() : mCalendar( 0 ) {}

    KCal::ResourceCalendar *mCalendar;
    QString mICalFile;
};
//@endcond

KarmStorage::KarmStorage() : d( new Private() )
{
}

QString KarmStorage::load( TaskView* view, const QString &fileName )
// loads data from filename into view. If no filename is given, filename from preferences is used.
// filename might be of use if this program is run as embedded konqueror plugin.
{
  kDebug(5970) <<"Entering KarmStorage::load";
  QString err;
  KEMailSettings settings;
  QString lFileName = fileName;

  assert( !( lFileName.isEmpty() ) );

  // If same file, don't reload
  if ( lFileName == d->mICalFile ) return err;


  // If file doesn't exist, create a blank one to avoid ResourceLocal load
  // error.  We make it user and group read/write, others read.  This is
  // masked by the users umask.  (See man creat)
  if ( !( remoteResource( lFileName ) ) ) {
    int handle;
    handle = open (
               QFile::encodeName( lFileName ),
               O_CREAT | O_EXCL | O_WRONLY,
               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
             );
    if ( handle != -1 ) {
      close( handle );
    }
  }

  if ( d->mCalendar)
    closeStorage();

  // Create local file resource and add to resources
  d->mICalFile = lFileName;

  KCal::ResourceCached *resource;
  if ( remoteResource( d->mICalFile ) )
  {
    KUrl url( d->mICalFile );
    resource = new KCal::ResourceRemote( url, url ); // same url for upload and download
  }
  else
  {
    resource = new KCal::ResourceLocal( d->mICalFile );
  }
  d->mCalendar = resource;

  QObject::connect (d->mCalendar, SIGNAL(resourceChanged(ResourceCalendar *)),
  	            view, SLOT(iCalFileModified(ResourceCalendar *)));
  d->mCalendar->setTimeSpec( KPIM::KPimPrefs::timeSpec() );
  d->mCalendar->setResourceName( QString::fromLatin1("KTimeTracker") );
  d->mCalendar->open();
  d->mCalendar->load();

  // Claim ownership of iCalendar file if no one else has.
  KCal::Person owner = resource->owner();
  if ( owner.isEmpty() )
  {
    resource->setOwner( KCal::Person(
          settings.getSetting( KEMailSettings::RealName ),
          settings.getSetting( KEMailSettings::EmailAddress ) ) );
  }

  // Build task view from iCal data
  if (!err.isEmpty())
  {
    KCal::Todo::List todoList;
    KCal::Todo::List::ConstIterator todo;
    QMultiHash< QString, Task* > map;

    // Build dictionary to look up Task object from Todo uid.  Each task is a
    // QListViewItem, and is initially added with the view as the parent.
    todoList = d->mCalendar->rawTodos();
    kDebug(5970) <<"KarmStorage::load"
      << "rawTodo count (includes completed todos) ="
      << todoList.count();
    for( todo = todoList.begin(); todo != todoList.end(); ++todo )
    {
      // Initially, if a task was complete, it was removed from the view.
      // However, this increased the complexity of reporting on task history.
      //
      // For example, if a task is complete yet has time logged to it during
      // the date range specified on the history report, we have to figure out
      // how that task fits into the task hierarchy.  Currently, this
      // structure is held in memory by the structure in the list view.
      //
      // I considered creating a second tree that held the full structure of
      // all complete and incomplete tasks.  But this seemed to much of a
      // change with an impending beta release and a full todo list.
      //
      // Hence this "solution".  Include completed tasks, but mark them as
      // inactive in the view.
      //
      //if ((*todo)->isCompleted()) continue;

      Task* task = new Task(*todo, view);
      map.insert( (*todo)->uid(), task );
      view->setRootIsDecorated(true);
      task->setPixmapProgress();
    }

    // Load each task under it's parent task.
    for( todo = todoList.begin(); todo != todoList.end(); ++todo )
    {
      Task* task = map.value( (*todo)->uid() );

      // No relatedTo incident just means this is a top-level task.
      if ( (*todo)->relatedTo() )
      {
        Task* newParent = map.value( (*todo)->relatedToUid() );

        // Complete the loading but return a message
        if ( !newParent )
          err = i18n("Error loading \"%1\": could not find parent (uid=%2)",
             task->name(),
             (*todo)->relatedToUid());

        if (!err.isEmpty()) task->move( newParent );
      }
    }

    kDebug(5970) << "KarmStorage::load - loaded" << view->count()
      << "tasks from" << d->mICalFile;
  }

  buildTaskView(d->mCalendar, view);
  return err;
}

QString KarmStorage::icalfile()
{
  kDebug(5970) <<"Entering KarmStorage::icalfile";
  return d->mICalFile;
}

QString KarmStorage::buildTaskView(KCal::ResourceCalendar *rc, TaskView *view)
// makes *view contain the tasks out of *rc.
{
  QString err;
  KCal::Todo::List todoList;
  KCal::Todo::List::ConstIterator todo;
  QMultiHash< QString, Task* > map;
  QVector<QString> runningTasks;
  QVector<QDateTime> startTimes;

  // remember tasks that are running and their start times
  QTreeWidgetItemIterator it( view );
  while ( *it ) 
  {
    Task *task = static_cast< Task* >( *it );
    if ( task->isRunning() ) {
      runningTasks.append( task->uid() );
      startTimes.append( task->startTime() );
    }
    ++it;
  }

  // delete old tasks

  while (view->itemAt(0)) view->itemAt(0)->cut();

  todoList = rc->rawTodos();
  for( todo = todoList.begin(); todo != todoList.end(); ++todo )
  {
    Task* task = new Task(*todo, view);
    task->setWhatsThis(0,"The task name is how you call the task, it can be chosen freely.");
    task->setWhatsThis(1,"The session time is the time since you last chose \"start new session.\"");
    map.insert( (*todo)->uid(), task );
    view->setRootIsDecorated(true);
    task->setPixmapProgress();
  }

  // 1.1. Load each task under it's parent task.
  for( todo = todoList.begin(); todo != todoList.end(); ++todo )
  {
    Task* task = map.value( (*todo)->uid() );
    // No relatedTo incident just means this is a top-level task.
    if ( (*todo)->relatedTo() )
    {
      Task* newParent = map.value( (*todo)->relatedToUid() );

      // Complete the loading but return a message
      if ( !newParent )
        err = i18n("Error loading \"%1\": could not find parent (uid=%2)",
           task->name(),
           (*todo)->relatedToUid());
      else task->move( newParent );
    }
  }

  view->clearActiveTasks();
  // restart tasks that have been running with their start times
  for ( int i=0; i<view->count(); i++) {
    for ( int n = 0; n < runningTasks.count(); ++n ) {
      if ( runningTasks[n] == view->itemAt(i)->uid() ) {
        view->startTimerFor( view->itemAt(i), startTimes[n] );
      }
    }
  }

  view->refresh();

  return err;
}

void KarmStorage::closeStorage()
{
  kDebug(5970) << "Entering KarmStorage::closeStorage";
  if ( d->mCalendar )
  {
    d->mCalendar->close();
    delete d->mCalendar;
    d->mCalendar = 0;
  }
  kDebug(5970) << "Exiting KarmStorage::closeStorage";
}

KCal::Event::List KarmStorage::rawevents()
{
  kDebug(5970) <<"Entering KarmStorage::listallevents";
  return d->mCalendar->rawEvents();
}

QString KarmStorage::save(TaskView* taskview)
{
  kDebug(5970) <<"entering KarmStorage::save";
  QString err;

  QStack<KCal::Todo*> parents;

  for (int i = 0; i < taskview->topLevelItemCount(); ++i ) 
  {
    Task *task = static_cast< Task* >( taskview->topLevelItem( i ) );
    kDebug( 5970 ) << "write task" << task->name();
    err = writeTaskAsTodo( task, parents );
  }

  err=saveCalendar();

  if ( err.isEmpty() )
  {
    kDebug(5970)
      << "KarmStorage::save : wrote"
      << taskview->count() << "tasks to" << d->mICalFile;
  }
  else
  {
    kWarning(5970) <<"KarmStorage::save :" << err;
  }

  return err;
}

QString KarmStorage::setTaskParent( Task* task, Task* parent )
{
  kDebug(5970) <<"Entering KarmStorage::setTaskParent";
  QString err=QString();
  KCal::Todo* toDo;
  toDo = d->mCalendar->todo(task->uid());
  if (parent==0) toDo->removeRelation(toDo->relatedTo());
  else toDo->setRelatedTo(d->mCalendar->todo(parent->uid()));
 // buildTaskView(d->mCalendar, _view);
  kDebug(5970) <<"Leaving KarmStorage::setTaskParent";
  return err;
}

QString KarmStorage::writeTaskAsTodo(Task* task, QStack<KCal::Todo*>& parents )
{
  QString err;
  KCal::Todo* todo;

  todo = d->mCalendar->todo(task->uid());
  if ( !todo )
  {
    kDebug(5970) <<"Could not get todo from calendar";
    return "Could not get todo from calendar";
  }
  task->asTodo(todo);
  if ( !parents.isEmpty() ) todo->setRelatedTo( parents.top() );
  parents.push( todo );

  for ( int i = 0; i < task->childCount(); ++i ) {
    Task *nextTask = static_cast< Task* >( task->child( i ) );
    err = writeTaskAsTodo( nextTask, parents );
  }

  parents.pop();
  return err;
}

bool KarmStorage::isEmpty()
{
  KCal::Todo::List todoList;

  todoList = d->mCalendar->rawTodos();
  return todoList.empty();
}

//----------------------------------------------------------------------------
// Routines that handle Comma-Separated Values export file format.
//
QString KarmStorage::exportcsvFile( TaskView *taskview,
                                    const ReportCriteria &rc )
{
  QString delim = rc.delimiter;
  QString dquote = rc.quote;
  QString double_dquote = dquote + dquote;
  bool to_quote = true;

  QString err=QString();
  Task* task;
  int maxdepth=0;

  kDebug(5970)
    << "KarmStorage::exportcsvFile:" << rc.url;

  QString title = i18n("Export Progress");
  KProgressDialog dialog( taskview, 0, title );
  dialog.setAutoClose( true );
  dialog.setAllowCancel( true );
  dialog.progressBar()->setMaximum( 2 * taskview->count() );

  // The default dialog was not displaying all the text in the title bar.
  int width = taskview->fontMetrics().width(title) * 3;
  QSize dialogsize;
  dialogsize.setWidth(width);
  dialog.setInitialSize( dialogsize );

  if ( taskview->count() > 1 ) dialog.show();

  QString retval;

  // Find max task depth
  int tasknr = 0;
  while ( tasknr < taskview->count() && !dialog.wasCancelled() )
  {
    dialog.progressBar()->setValue( dialog.progressBar()->value() + 1 );
    if ( tasknr % 15 == 0 ) kapp->processEvents(); // repainting is slow
    if ( taskview->itemAt(tasknr)->depth() > maxdepth )
      maxdepth = taskview->itemAt(tasknr)->depth();
    tasknr++;
  }

  // Export to file
  tasknr = 0;
  while ( tasknr < taskview->count() && !dialog.wasCancelled() )
  {
    task = taskview->itemAt( tasknr );
    dialog.progressBar()->setValue( dialog.progressBar()->value() + 1 );
    if ( tasknr % 15 == 0 ) kapp->processEvents();

    // indent the task in the csv-file:
    for ( int i=0; i < task->depth(); ++i ) retval += delim;

    /*
    // CSV compliance
    // Surround the field with quotes if the field contains
    // a comma (delim) or a double quote
    if (task->name().contains(delim) || task->name().contains(dquote))
      to_quote = true;
    else
      to_quote = false;
    */
    to_quote = true;

    if (to_quote)
      retval += dquote;

    // Double quotes replaced by a pair of consecutive double quotes
    retval += task->name().replace( dquote, double_dquote );

    if (to_quote)
      retval += dquote;

    // maybe other tasks are more indented, so to align the columns:
    for ( int i = 0; i < maxdepth - task->depth(); ++i ) retval += delim;

    retval += delim + formatTime( task->sessionTime(),
                                   rc.decimalMinutes )
           + delim + formatTime( task->time(),
                                   rc.decimalMinutes )
           + delim + formatTime( task->totalSessionTime(),
                                   rc.decimalMinutes )
           + delim + formatTime( task->totalTime(),
                                   rc.decimalMinutes )
           + '\n';
    tasknr++;
  }

  // save, either locally or remote
  if ((rc.url.isLocalFile()) || (!rc.url.url().contains("/")))
  {
    QString filename=rc.url.path();
    if (filename.isEmpty()) filename=rc.url.url();
    QFile f( filename );
    if( !f.open( QIODevice::WriteOnly ) ) {
        err = i18n( "Could not open \"%1\".", filename );
    }
    if (err.length()==0)
    {
      QTextStream stream(&f);
      // Export to file
      stream << retval;
      f.close();
    }
  }
  else // use remote file
  {
    KTemporaryFile tmpFile;
    if ( !tmpFile.open() ) err = QString::fromLatin1( "Unable to get temporary file" );
    else
    {
      QTextStream stream ( &tmpFile );
      stream << retval;
      stream.flush();
      if (!KIO::NetAccess::upload( tmpFile.fileName(), rc.url, 0 )) err=QString::fromLatin1("Could not upload");
    }
  }

  return err;
}

//----------------------------------------------------------------------------
// Routines that handle logging KArm history
//

//
// public routines:
//

QString KarmStorage::addTask(const Task* task, const Task* parent)
{
  KCal::Todo* todo;
  QString uid;

  todo = new KCal::Todo();
  if ( d->mCalendar->addTodo( todo ) )
  {
    task->asTodo( todo  );
    if (parent)
      todo->setRelatedTo(d->mCalendar->todo(parent->uid()));
    uid = todo->uid();
  }
  else
  {
    // Most likely a lock could not be pulled, although there are other
    // possiblities (like a really confused resource manager).
    uid = "";
  }

  return uid;
}

bool KarmStorage::removeTask(Task* task)
{

  // delete history
  KCal::Event::List eventList = d->mCalendar->rawEvents();
  for(KCal::Event::List::iterator i = eventList.begin();
      i != eventList.end();
      ++i)
  {
    if ( (*i)->relatedToUid() == task->uid()
        || ( (*i)->relatedTo()
            && (*i)->relatedTo()->uid() == task->uid()))
    {
      d->mCalendar->deleteEvent(*i);
    }
  }

  // delete todo
  KCal::Todo *todo = d->mCalendar->todo(task->uid());
  d->mCalendar->deleteTodo(todo);

  // Save entire file
  saveCalendar();

  return true;
}

void KarmStorage::addComment(const Task* task, const QString& comment)
{

  KCal::Todo* todo;

  todo = d->mCalendar->todo(task->uid());

  // Do this to avoid compiler warnings about comment not being used.  once we
  // transition to using the addComment method, we need this second param.
  QString s = comment;

  // TODO: Use libkcal comments
  // todo->addComment(comment);
  // temporary
  todo->setDescription(task->comment());

  saveCalendar();
}

long KarmStorage::printTaskHistory (
        const Task               *task,
        const QMap<QString,long> &taskdaytotals,
        QMap<QString,long>       &daytotals,
        const QDate              &from,
        const QDate              &to,
        const int                level,
        QVector<QString>         &matrix,
        const ReportCriteria     &rc)
// to>=from is precondition
{
  long ownline=linenr++; // the how many-th instance of this function is this
  long colrectot=0;      // column where to write the task's total recursive time
  QVector<QString> cell; // each line of the matrix is stored in an array of cells, one containing the recursive total
  long add;              // total recursive time of all subtasks
  QString delim = rc.delimiter;
  QString dquote = rc.quote;
  QString double_dquote = dquote + dquote;
  bool to_quote = true;

  const QString cr = QString::fromLatin1("\n");
  QString buf;
  QString daytaskkey, daykey;
  QDate day;
  long sum;

  if ( !task ) return 0;

  day = from;
  sum = 0;
  while (day <= to)
  {
    // write the time in seconds for the given task for the given day to s
    daykey = day.toString(QString::fromLatin1("yyyyMMdd"));
    daytaskkey = QString::fromLatin1("%1_%2")
      .arg(daykey)
      .arg(task->uid());

    if (taskdaytotals.contains(daytaskkey))
    {
      cell.append(
        QString::fromLatin1( "%1" ).arg(
          formatTime( taskdaytotals[ daytaskkey ] / 60, rc.decimalMinutes )
        )
      );
      sum += taskdaytotals[daytaskkey];  // in seconds

      if (daytotals.contains(daykey))
        daytotals.insert(daykey, daytotals[daykey]+taskdaytotals[daytaskkey]);
      else
        daytotals.insert(daykey, taskdaytotals[daytaskkey]);
    }
    cell.append( delim );

    day = day.addDays(1);
  }

  // Total for task
  cell.append(
    QString::fromLatin1( "%1" ).arg(
      formatTime( sum / 60, rc.decimalMinutes )
    )
  );

  // room for the recursive total time (that cannot be calculated now)
  cell.append( delim );
  colrectot = cell.size();
  cell.append( "???" );
  cell.append( delim );

  // Task name
  for ( int i = level + 1; i > 0; i-- ) cell.append( delim );

  /*
  // CSV compliance
  // Surround the field with quotes if the field contains
  // a comma (delim) or a double quote
  to_quote = task->name().contains(delim) || task->name().contains(dquote);
  */
  to_quote = true;
  if ( to_quote) cell.append( dquote );


  // Double quotes replaced by a pair of consecutive double quotes
  cell.append( task->name().replace( dquote, double_dquote ) );

  if ( to_quote) cell.append( dquote );

  cell.append( cr );

  add=0;
  for ( int i = 0; i < task->childCount(); ++i ) {
    Task *subTask = static_cast< Task* >( task->child( i ) );
    add += printTaskHistory( subTask, taskdaytotals, daytotals, from, to , level+1, matrix,
                      rc );
  }
  cell[colrectot]=(QString::fromLatin1("%1").arg(formatTime((add+sum)/60, rc.decimalMinutes )));
  for (int i=0; i < cell.count(); i++) matrix[ownline]+=cell[i];
  return add+sum;
}

QString KarmStorage::report( TaskView *taskview, const ReportCriteria &rc )
{
  QString err;
  if ( rc.reportType == ReportCriteria::CSVHistoryExport )
  {
      if ( !rc.bExPortToClipBoard )
        err = exportcsvHistory( taskview, rc.from, rc.to, rc );
      else
        err=taskview->clipHistory();
  }
  else if ( rc.reportType == ReportCriteria::CSVTotalsExport )
  {
    if ( !rc.bExPortToClipBoard )
      err = exportcsvFile( taskview, rc );
    else
      err = taskview->clipTotals( rc );
  }
  else {
      // hmmmm ... assert(0)?
  }
  return err;
}

// export history report as csv, all tasks X all dates in one block
QString KarmStorage::exportcsvHistory ( TaskView      *taskview,
                                            const QDate   &from,
                                            const QDate   &to,
                                            const ReportCriteria &rc)
{
  // build up an edithistorydialog, sort its start date column, and write a csv formated output into retval
  // ToDo: what if a task spans over several days ?
  // ToDo: what if a user gets the dates displayed differently ?
  kDebug(5970) << "Entering KarmStorage::exportcsvHistory";
  QString delim = rc.delimiter;
  const QString cr = QString::fromLatin1("\n");
  QString err=QString::null;
  QString retval;

  EditHistoryDialog* historyWidget=new EditHistoryDialog(taskview);
  QTableWidget* table = historyWidget->tableWidget();
  table->sortItems(2,Qt::AscendingOrder);  // sort by StartDate
  
  // parameter-plausi
  if ( from > to )
  {
    err = QString("'to' has to be a date later than or equal to 'from'.");
  }
  else
  {
    retval=QString("Task Name").append(delim);
    for (QDate date=from; date<to; date=date.addDays(1) )
    {
      retval.append(KGlobal::locale()->formatDate(date).append(delim));
      kDebug() << date.toString();
    } 
    retval.append(KGlobal::locale()->formatDate(to)).append("\n");

    int taskstart, endtask;
    if ( rc.allTasks )
    {
      taskstart=0;
      endtask=taskview->count()-1;
    }
    else 
    {
      for ( int i=0; i<taskview->count(); ++i)
      {
        if (taskview->itemAt(i) == taskview->currentItem())
        {
          taskstart=i;
          endtask=i;
        }
      }
    }
    for ( int i = taskstart; i <= endtask; ++i ) 
    {
      Task *task = static_cast< Task* >( taskview->itemAt( i ) );
      retval.append(task->name());
      kDebug(5970) << "appending task " << taskview->itemAt(i)->name();
      QDate date=from;
      int row=0; // row in table. The table is sorted by StartTime.
      while (date<=to)
      {
        kDebug(5970) << "date is now " << date;
        row=0;
        QDate dateintable=QDateTime::fromString(table->item(row,1)->text(),"yyyy-MM-dd HH:mm:ss").date();
        kDebug(5970) << "date in the history table is now " << dateintable;

        while ((dateintable<date) && (row<(table->rowCount())))
        {
          dateintable=QDateTime::fromString(table->item(row,1)->text(),"yyyy-MM-dd HH:mm:ss").date();
          if (dateintable<date) row++;
        }
        // now, we have a task, a date and a dateintable.
        // dateintable is now date, row is at the uppermost line with date
        int durationsecs=0; // duration of a task spent in one or more events on one day.
        kDebug() << "Calculating duration for task " << task->name() << " on date " << dateintable;
        if (row<(table->rowCount())) dateintable=QDateTime::fromString(table->item(row,1)->text(),"yyyy-MM-dd HH:mm:ss").date();
        kDebug() << "dateintable is now " << dateintable << "rowcount is " << table->rowCount() << " row is " << row;
        while (dateintable==date && row<table->rowCount()) 
        {
          kDebug() << "dateintable is " << dateintable;
          kDebug() << "table item is " << table->item(row,0)->text() << "task is " << task->name();
          if (table->item(row,0)->text()==task->name())
          {
            kDebug(5970) << "found " << task->name() << "in historytable";
            QTime startTime=QDateTime::fromString(table->item(row,1)->text(),"yyyy-MM-dd HH:mm:ss").time();
            QTime endTime=QDateTime::fromString(table->item(row,2)->text(),"yyyy-MM-dd HH:mm:ss").time();
            durationsecs+=startTime.secsTo(endTime);
            kDebug(5970) << "the duration was " << durationsecs;
          }
          row++;
          if (row<table->rowCount()) dateintable=QDateTime::fromString(table->item(row,1)->text(),"yyyy-MM-dd HH:mm:ss").date();
        }
        retval+=delim+formatTime( durationsecs/60.0, rc.decimalMinutes );
        date=date.addDays(1);
      }
      retval.append("\n");
    }
    kDebug() << retval;
  }

  // store the file locally or remote
  if ((rc.url.isLocalFile()) || (!rc.url.url().contains("/")))
  {
    kDebug(5970) << "storing a local file";
    QString filename=rc.url.path();
    if (filename.isEmpty()) filename=rc.url.url();
    QFile f( filename );
    if( !f.open( QIODevice::WriteOnly ) ) 
    {
      err = i18n( "Could not open \"%1\".", filename );
      kDebug(5970) << "Could not open file";
    }
    kDebug() << "Err is " << err;
    if (err.length()==0)
    {
      QTextStream stream(&f);
      kDebug(5970) << "Writing to file: " << retval;
      // Export to file
      stream << retval;
      f.close();
    }
  } 
  else // use remote file
  {
    KTemporaryFile tmpFile;
    if ( !tmpFile.open() )
    {
      err = QString::fromLatin1( "Unable to get temporary file" );
    }
    else
    {
      QTextStream stream ( &tmpFile );
      stream << retval;
      stream.flush();
      if (!KIO::NetAccess::upload( tmpFile.fileName(), rc.url, 0 )) err=QString::fromLatin1("Could not upload");
    }
  }
  return err;
}

void KarmStorage::startTimer( const Task* task, const KDateTime &when )
{
  KCal::Event* e;
  e = baseEvent(task);
  e->setDtStart(when);
  d->mCalendar->addEvent(e);
  task->taskView()->scheduleSave();
}

void KarmStorage::stopTimer( const Task* task, const QDateTime &when )
{
  kDebug(5970) <<"Entering stopTimer when=" << when;
  KCal::Event::List eventList = d->mCalendar->rawEvents();
  for(KCal::Event::List::iterator i = eventList.begin();
      i != eventList.end();
      ++i)
  {
    if ( (*i)->relatedToUid() == task->uid() )
    {
      if (!(*i)->hasEndDate())
      {
	QString s=when.toString("yyyy-MM-ddThh:mm:ss.zzzZ"); // need the KDE standard from the ISO standard, not the QT one
	KDateTime kwhen=KDateTime::fromString(s);
	kDebug() << "kwhen ==" <<  kwhen;
        (*i)->setDtEnd(kwhen);
      }
    };
  }
}

bool KarmStorage::bookTime(const Task* task,
                           const QDateTime& startDateTime,
                           const long durationInSeconds)
{
  kDebug(5970) <<"Entering KarmStorage::bookTime";
  // Ignores preferences setting re: logging history.
  KCal::Event* e;
  QDateTime end;
  KDateTime start( startDateTime, KDateTime::Spec::LocalZone() ); //??? is LocalZone correct ???

  e = baseEvent( task );
  e->setDtStart( start );
  e->setDtEnd( start.addSecs( durationInSeconds ) );

  // Use a custom property to keep a record of negative durations
  e->setCustomProperty( KGlobal::mainComponent().componentName().toUtf8(),
      QByteArray("duration"),
      QString::number(durationInSeconds));

  return d->mCalendar->addEvent(e);
}

void KarmStorage::changeTime(const Task* task, const long deltaSeconds)
{
  kDebug(5970) <<"Entering KarmStorage::changeTime deltaSeconds=" <<deltaSeconds;
  KCal::Event* e;
  QDateTime end;

  e = baseEvent(task);

  // Don't use duration, as ICalFormatImpl::writeIncidence never writes a
  // duration, even though it looks like it's used in event.cpp.
  end = task->startTime();
  if ( deltaSeconds > 0 ) end = task->startTime().addSecs(deltaSeconds);
  e->setDtEnd(KDateTime(end, KDateTime::Spec::LocalZone()));

  // Use a custom property to keep a record of negative durations
  e->setCustomProperty( KGlobal::mainComponent().componentName().toUtf8(),
      QByteArray("duration"),
      QString::number(deltaSeconds));

  d->mCalendar->addEvent(e);

  task->taskView()->scheduleSave();
}


KCal::Event* KarmStorage::baseEvent(const Task * task)
{
  KCal::Event* e;
  QStringList categories;

  e = new KCal::Event;
  e->setSummary(task->name());

  // Can't use setRelatedToUid()--no error, but no RelatedTo written to disk
  e->setRelatedTo(d->mCalendar->todo(task->uid()));

  // Debugging: some events where not getting a related-to field written.
  assert(e->relatedTo()->uid() == task->uid());

  // Have to turn this off to get datetimes in date fields.
  e->setAllDay(false);
  e->setDtStart(KDateTime(task->startTime(), KDateTime::Spec::LocalZone()));

  // So someone can filter this mess out of their calendar display
  categories.append(i18n("KArm"));
  e->setCategories(categories);

  return e;
}

HistoryEvent::HistoryEvent( const QString &uid, const QString &name,
                            long duration, const KDateTime &start,
                            const KDateTime &stop, const QString &todoUid )
{
  _uid = uid;
  _name = name;
  _duration = duration;
  _start = start;
  _stop = stop;
  _todoUid = todoUid;
}


QList<HistoryEvent> KarmStorage::getHistory(const QDate& from,
    const QDate& to)
{
  QList<HistoryEvent> retval;
  QStringList processed;
  KCal::Event::List events;
  KCal::Event::List::iterator event;
  QString duration;

  for( QDate date = from; date <= to; date = date.addDays( 1 ) ) {
    events = d->mCalendar->rawEventsForDate( date, KPIM::KPimPrefs::timeSpec() );
    for (event = events.begin(); event != events.end(); ++event) 
    {

      // KArm events have the custom property X-KDE-Karm-duration
      if (! processed.contains( (*event)->uid())) 
      {
        // If an event spans multiple days, CalendarLocal::rawEventsForDate
        // will return the same event on both days.  To avoid double-counting
        // such events, we (arbitrarily) attribute the hours from both days on
        // the first day.  This mis-reports the actual time spent, but it is
        // an easy fix for a (hopefully) rare situation.
        processed.append( (*event)->uid());

        // ktimetracker was name karm till KDE 3 incl.
        // if a KDE-karm-duRation exists and no KDE-ktimetracker-Duration, change this
        if (
          (*event)->customProperty( KGlobal::mainComponent().componentName().toUtf8(),
          QByteArray( "duration" )) == QString::null && (*event)->customProperty( "karm",
          QByteArray( "duration" )) != QString::null )

          (*event)->setCustomProperty(  
            KGlobal::mainComponent().componentName().toUtf8(),
            QByteArray( "duration" ), (*event)->customProperty( "karm",
            QByteArray( "duration" )));

        duration = (*event)->customProperty(KGlobal::mainComponent().componentName().toUtf8(),
            QByteArray("duration"));
        if ( ! duration.isNull() )
        {
          if ( (*event)->relatedTo()
              &&  ! (*event)->relatedTo()->uid().isEmpty() )
          {
            retval.append(HistoryEvent(
                (*event)->uid(),
                (*event)->summary(),
                duration.toLong(),
                (*event)->dtStart(),
                (*event)->dtEnd(),
                (*event)->relatedTo()->uid()
                ));
          }
          else
            // Something is screwy with the ics file, as this KArm history event
            // does not have a todo related to it.  Could have been deleted
            // manually?  We'll continue with report on with report ...
            kDebug(5970) <<"KarmStorage::getHistory():"
              << "The event" << (*event)->uid()
              << "is not related to a todo.  Dropped.";
        }
      }
    }
  }

  return retval;
}

bool KarmStorage::remoteResource( const QString& file ) const
{
  QString f = file.toLower();
  bool rval = f.startsWith( "http://" ) || f.startsWith( "ftp://" );

  kDebug(5970) <<"KarmStorage::remoteResource(" << file <<" ) returns" << rval;
  return rval;
}

QString KarmStorage::saveCalendar()
{
  kDebug(5970) << "Entering KarmStorage::saveCalendar";

  QString err=QString();
  KABC::Lock *lock = d->mCalendar->lock();
  if ( !lock || !lock->lock() ) err=QString("Could not save. Could not lock file.");

  if ( d->mCalendar->save() ) 
  {
    lock->unlock();
  }
  else err=QString("Could not save. Could lock file.");

  lock->unlock();
  return err;
}
