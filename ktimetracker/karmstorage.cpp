/*
 *     Copyright (C) 2003, 2004 by Mark Bucciarelli <mark@hubcapconsutling.com>
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

#include "karmstorage.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cassert>

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QHeaderView>
#include <QList>
#include <QMultiHash>
#include <QProgressBar>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QTextStream>

#include <KApplication>       // kapp
#include <KDebug>
#include <KLocale>            // i18n
#include <KMessageBox>
#include <KPasswordDialog>
#include <KProgressDialog>
#include <KUrl>
#include <KTemporaryFile>

#include <kemailsettings.h>
#include <kcal/incidence.h>
#include <kcal/resourcecalendar.h>
#include <kcal/resourcelocal.h>
#include <resourceremote.h>
#include <kpimprefs.h>
#include <kio/netaccess.h>

#include "taskview.h"
#include "timekard.h"
#include "karmutility.h"
#include "preferences.h"
#include "task.h"

KarmStorage *KarmStorage::_instance = 0;
static long linenr;  // how many lines written by printTaskHistory so far


KarmStorage *KarmStorage::instance()
{
  if (_instance == 0) _instance = new KarmStorage();
  return _instance;
}

KarmStorage::KarmStorage()
{
  _calendar = 0;
}

QString KarmStorage::load( TaskView* view, const Preferences* preferences, 
                           const QString &fileName )
// loads data from filename into view. If no filename is given, filename from preferences is used.
// filename might be of use if this program is run as embedded konqueror plugin.
{
  kDebug(5970) << "Entering KarmStorage::load" << endl;
  QString err;
  KEMailSettings settings;
  QString lFileName = fileName;

  if ( lFileName.isEmpty() ) lFileName = preferences->iCalFile();

  // If same file, don't reload
  if ( lFileName == _icalfile ) return err;


  // If file doesn't exist, create a blank one to avoid ResourceLocal load
  // error.  We make it user and group read/write, others read.  This is
  // masked by the users umask.  (See man creat)
  if ( ! remoteResource( _icalfile ) )
  {
    int handle;
    handle = open (
        QFile::encodeName( lFileName ),
        O_CREAT|O_EXCL|O_WRONLY,
        S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH
        );
    if (handle != -1) close(handle);
  }

  if ( _calendar)
    closeStorage(view);

  // Create local file resource and add to resources
  _icalfile = lFileName;

  KCal::ResourceCached *resource;
  if ( remoteResource( _icalfile ) )
  {
    KUrl url( _icalfile );
    resource = new KCal::ResourceRemote( url, url ); // same url for upload and download
  }
  else
  {
    resource = new KCal::ResourceLocal( _icalfile );
  }
  _calendar = resource;

  QObject::connect (_calendar, SIGNAL(resourceChanged(ResourceCalendar *)),
  	            view, SLOT(iCalFileModified(ResourceCalendar *)));
  _calendar->setTimeSpec( KPimPrefs::timeSpec() );
  _calendar->setResourceName( QString::fromLatin1("KTimeTracker") );
  _calendar->open();
  _calendar->load();

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
    todoList = _calendar->rawTodos();
    kDebug(5970) << "KarmStorage::load "
      << "rawTodo count (includes completed todos) ="
      << todoList.count() << endl;
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

    kDebug(5970) << "KarmStorage::load - loaded " << view->count()
      << " tasks from " << _icalfile << endl;
  }

  buildTaskView(_calendar, view);
  return err;
}

QString KarmStorage::icalfile()
{
  kDebug(5970) << "Entering KarmStorage::icalfile" << endl;
  return _icalfile;
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
  for ( int i=0; i<view->count(); i++)
  {
    if ( view->item_at_index(i)->isRunning() )
    {
      runningTasks.append( view->item_at_index(i)->uid() );
      startTimes.append( view->item_at_index(i)->lastStart() );
    }
  }

  // delete old tasks

  while (view->item_at_index(0)) view->item_at_index(0)->cut();

  todoList = rc->rawTodos();
  for( todo = todoList.begin(); todo != todoList.end(); ++todo )
  {
    Task* task = new Task(*todo, view);
    task->setWhatsThis(0,"The task name is how you call the task, it can be chose freely.");
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
      if ( runningTasks[n] == view->item_at_index(i)->uid() ) {
        view->startTimerFor( view->item_at_index(i), startTimes[n] );
      }
    }
  }

  view->refresh();

  return err;
}

void KarmStorage::closeStorage(TaskView* view)
{
  if ( _calendar )
  {
    _calendar->close();
    delete _calendar;
    _calendar = 0;

    view->clear();
  }
}

KCal::Event::List KarmStorage::rawevents()
{
  kDebug(5970) << "Entering KarmStorage::listallevents" << endl;
  return _calendar->rawEvents();
}

QString KarmStorage::save(TaskView* taskview)
{
  kDebug(5970) << "entering KarmStorage::save" << endl;
  QString err;

  QStack<KCal::Todo*> parents;

  for (Task* task=taskview->first_child(); task; task = task->nextSibling())
  {
    err=writeTaskAsTodo(task, 1, parents );
  }

  if ( !saveCalendar() )
  {
    err="Could not save";
  }

  if ( err.isEmpty() )
  {
    kDebug(5970)
      << "KarmStorage::save : wrote "
      << taskview->count() << " tasks to " << _icalfile << endl;
  }
  else
  {
    kWarning(5970) << "KarmStorage::save : " << err << endl;
  }

  return err;
}

QString KarmStorage::setTaskParent( Task* task, Task* parent )
{
  kDebug(5970) << "Entering KarmStorage::setTaskParent" << endl;
  QString err=QString();
  KCal::Todo* toDo;
  toDo = _calendar->todo(task->uid());
  if (parent==0) toDo->removeRelation(toDo->relatedTo());
  else toDo->setRelatedTo(_calendar->todo(parent->uid()));
 // buildTaskView(_calendar, _view);
  kDebug(5970) << "Leaving KarmStorage::setTaskParent" << endl;
  return err;
}

QString KarmStorage::writeTaskAsTodo(Task* task, const int level,
    QStack<KCal::Todo*>& parents )
{
  QString err;
  KCal::Todo* todo;

  todo = _calendar->todo(task->uid());
  if ( !todo )
  {
    kDebug(5970) << "Could not get todo from calendar" << endl;
    return "Could not get todo from calendar";
  }
  task->asTodo(todo);
  if ( !parents.isEmpty() ) todo->setRelatedTo( parents.top() );
  parents.push( todo );

  for ( Task* nextTask = task->firstChild(); nextTask;
        nextTask = nextTask->nextSibling() )
  {
    err = writeTaskAsTodo(nextTask, level+1, parents );
  }

  parents.pop();
  return err;
}

bool KarmStorage::isEmpty()
{
  KCal::Todo::List todoList;

  todoList = _calendar->rawTodos();
  return todoList.empty();
}

bool KarmStorage::isNewStorage(const Preferences* preferences) const
{
  if ( !_icalfile.isNull() ) return preferences->iCalFile() != _icalfile;
  else return false;
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
    << "KarmStorage::exportcsvFile: " << rc.url << endl;

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
    if ( taskview->item_at_index(tasknr)->depth() > maxdepth )
      maxdepth = taskview->item_at_index(tasknr)->depth();
    tasknr++;
  }

  // Export to file
  tasknr = 0;
  while ( tasknr < taskview->count() && !dialog.wasCancelled() )
  {
    task = taskview->item_at_index( tasknr );
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
  if ( _calendar->addTodo( todo ) )
  {
    task->asTodo( todo  );
    if (parent)
      todo->setRelatedTo(_calendar->todo(parent->uid()));
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
  KCal::Event::List eventList = _calendar->rawEvents();
  for(KCal::Event::List::iterator i = eventList.begin();
      i != eventList.end();
      ++i)
  {
    if ( (*i)->relatedToUid() == task->uid()
        || ( (*i)->relatedTo()
            && (*i)->relatedTo()->uid() == task->uid()))
    {
      _calendar->deleteEvent(*i);
    }
  }

  // delete todo
  KCal::Todo *todo = _calendar->todo(task->uid());
  _calendar->deleteTodo(todo);

  // Save entire file
  saveCalendar();

  return true;
}

void KarmStorage::addComment(const Task* task, const QString& comment)
{

  KCal::Todo* todo;

  todo = _calendar->todo(task->uid());

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
  for (Task* subTask = task->firstChild();
      subTask;
      subTask = subTask->nextSibling())
  {
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
  QString delim = rc.delimiter;
  const QString cr = QString::fromLatin1("\n");
  QString err;

  // below taken from timekard.cpp
  QString retval;
  QString taskhdr, totalhdr;
  QString line, buf;
  long sum;

  QList<HistoryEvent> events;
  QList<HistoryEvent>::iterator event;
  QMap<QString, long> taskdaytotals;
  QMap<QString, long> daytotals;
  QString daytaskkey, daykey;
  QDate day;
  QDate dayheading;

  // parameter-plausi
  if ( from > to )
  {
    err = QString::fromLatin1 (
            "'to' has to be a date later than or equal to 'from'.");
  }

  // header
  retval += i18n("Task History\n");
  retval += i18n("From %1 to %2",
     KGlobal::locale()->formatDate(from),
     KGlobal::locale()->formatDate(to));
  retval += cr;
  retval += i18n("Printed on: %1",
     KGlobal::locale()->formatDateTime(QDateTime::currentDateTime()));
  retval += cr;

  day=from;
  events = taskview->getHistory(from, to);
  taskdaytotals.clear();
  daytotals.clear();

  // Build lookup dictionary used to output data in table cells.  keys are
  // in this format: YYYYMMDD_NNNNNN, where Y = year, M = month, d = day and
  // NNNNN = the VTODO uid.  The value is the total seconds logged against
  // that task on that day.  Note the UID is the todo id, not the event id,
  // so times are accumulated for each task.
  for (event = events.begin(); event != events.end(); ++event)
  {
    daykey = (*event).start().date().toString(QString::fromLatin1("yyyyMMdd"));
    daytaskkey = QString(QString::fromLatin1("%1_%2"))
        .arg(daykey)
        .arg((*event).todoUid());

    if (taskdaytotals.contains(daytaskkey))
        taskdaytotals.insert(daytaskkey,
                taskdaytotals[daytaskkey] + (*event).duration());
    else
        taskdaytotals.insert(daytaskkey, (*event).duration());
  }

  // day headings
  dayheading = from;
  while ( dayheading <= to )
  {
    // Use ISO 8601 format for date.
    retval += dayheading.toString(QString::fromLatin1("yyyy-MM-dd"));
    retval += delim;
    dayheading=dayheading.addDays(1);
  }
  retval += i18n("Sum") + delim + i18n("Total Sum") + delim + i18n("Task Hierarchy");
  retval += cr;
  retval += line;

  // the tasks
  QVector<QString> matrix;
  linenr = 0;
  for ( int i = 0; i <= taskview->count() + 1; ++i) {
    matrix.append( "" );
  }

  if (events.empty())
  {
    retval += i18n("  No hours logged.");
  }
  else
  {
    if ( rc.allTasks )
    {
      for ( Task* task= taskview->item_at_index(0);
            task; task= task->nextSibling() )
      {
        printTaskHistory( task, taskdaytotals, daytotals, from, to, 0,
                          matrix, rc );
      }
    }
    else
    {
      printTaskHistory( taskview->current_item(), taskdaytotals, daytotals,
                        from, to, 0, matrix, rc );
    }
    for (int i=0; i<matrix.count(); i++) retval+=matrix[i];
    retval += line;

    // totals
    sum = 0;
    day = from;
    while (day<=to)
    {
      daykey = day.toString(QString::fromLatin1("yyyyMMdd"));

      if (daytotals.contains(daykey))
      {
        retval += QString::fromLatin1("%1")
            .arg(formatTime(daytotals[daykey]/60, rc.decimalMinutes));
        sum += daytotals[daykey];  // in seconds
      }
      retval += delim;
      day = day.addDays(1);
    }

    retval += QString::fromLatin1("%1%2%3%4")
        .arg( formatTime( sum/60, rc.decimalMinutes ) )
        .arg( delim ).arg( delim )
        .arg( i18n( "Total" ) );
  }

  // above taken from timekard.cpp

  // save, either locally or remote

  if ((rc.url.isLocalFile()) || (!rc.url.url().contains("/")))
  {
    QString filename=rc.url.path();
    if (filename.isEmpty()) filename=rc.url.url();
    QFile f( filename );
    if( !f.open( QIODevice::WriteOnly ) ) {
        err = i18n( "Could not open \"%1\".", filename );
    }
    if (!err.isEmpty())
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

void KarmStorage::stopTimer( const Task* task, const QDateTime &when )
{
  kDebug(5970) << "Entering stopTimer when=" << when << endl;
  kDebug(5970) << "task->startTime=" << task->startTime() << endl;
  long delta = task->startTime().secsTo(when);
  changeTime(task, delta);
}

bool KarmStorage::bookTime(const Task* task,
                           const QDateTime& startDateTime,
                           const long durationInSeconds)
{
  kDebug(5970) << "Entering KarmStorage::bookTime" << endl;
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

  return _calendar->addEvent(e);
}

void KarmStorage::changeTime(const Task* task, const long deltaSeconds)
{
  kDebug(5970) << "Entering KarmStorage::changeTime deltaSeconds=" <<deltaSeconds << endl;
  KCal::Event* e;
  QDateTime end;

  // Don't write events (with timer start/stop duration) if user has turned
  // this off in the settings dialog.
  if ( ! task->taskView()->preferences()->logging() ) return;

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

  _calendar->addEvent(e);

  // This saves the entire iCal file each time, which isn't efficient but
  // ensures no data loss.  A faster implementation would be to append events
  // to a file, and then when KArm closes, append the data in this file to the
  // iCal file.
  //
  // Meanwhile, we simply use a timer to delay the full-saving until the GUI
  // has updated, for better user feedback. Feel free to get rid of this
  // if/when implementing the faster saving (DF).
  task->taskView()->scheduleSave();
}


KCal::Event* KarmStorage::baseEvent(const Task * task)
{
  KCal::Event* e;
  QStringList categories;

  e = new KCal::Event;
  e->setSummary(task->name());

  // Can't use setRelatedToUid()--no error, but no RelatedTo written to disk
  e->setRelatedTo(_calendar->todo(task->uid()));

  // Debugging: some events where not getting a related-to field written.
  assert(e->relatedTo()->uid() == task->uid());

  // Have to turn this off to get datetimes in date fields.
  e->setFloats(false);
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

  for(QDate d = from; d <= to; d = d.addDays(1))
  {
    events = _calendar->rawEventsForDate( d, KPimPrefs::timeSpec() );
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
            kDebug(5970) << "KarmStorage::getHistory(): "
              << "The event " << (*event)->uid()
              << " is not related to a todo.  Dropped." << endl;
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

  kDebug(5970) << "KarmStorage::remoteResource( " << file << " ) returns " << rval  << endl;
  return rval;
}

bool KarmStorage::saveCalendar()
{
  kDebug(5970) << "KarmStorage::saveCalendar" << endl;

  KABC::Lock *lock = _calendar->lock();
  if ( !lock || !lock->lock() )
    return false;

  if ( _calendar->save() ) {
    lock->unlock();
    return true;
  }

  lock->unlock();
  return false;
}
