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

/** timetrackerstorage
  * This class cares for the storage of ktimetracker's data.
  * ktimetracker's data is
  * - tasks like "programming for customer foo"
  * - events like "from 2009-09-07, 8pm till 10pm programming for customer foo"
  * tasks are like the items on your todo list, events are dates when you worked on them.
  * ktimetracker's data is stored in a ResourceCalendar object hold by mCalendar.
  */

#include "timetrackerstorage.h"
#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "storageadaptor.h"
#include "task.h"
#include "taskview.h"
#include "timekard.h"

#include <kemailsettings.h>
#include <kio/netaccess.h>
#include <kcal/resourcecalendar.h>
#include <kcal/resourcelocal.h>
#include <resourceremote.h>
#include <kcal/incidence.h>

#include <KApplication>       // kapp
#include <KDebug>
#include <KLocale>            // i18n
#include <KMessageBox>
#include <KProgressDialog>
#include <KSystemTimeZones>
#include <KTemporaryFile>
#include <KUrl>

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

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#include <fcntl.h>


//@cond PRIVATE
class timetrackerstorage::Private
{
public:
    Private() : mCalendar( 0 ) {}
    ~Private()
    {
        delete mCalendar;
    }
    KCal::ResourceCalendar *mCalendar;
    QString mICalFile;
};
//@endcond

timetrackerstorage::timetrackerstorage() : d( new Private() )
{
}

timetrackerstorage::~timetrackerstorage()
{
    delete d;
}

QString timetrackerstorage::load( TaskView* view, const QString &fileName )
// loads data from filename into view. If no filename is given, filename from preferences is used.
// filename might be of use if this program is run as embedded konqueror plugin.
{
    kDebug(5970) << "Entering function";
    QString err;
    KEMailSettings settings;
    QString lFileName = fileName;

    assert( !( lFileName.isEmpty() ) );

    // If same file, don't reload
    if ( lFileName == d->mICalFile ) return err;


    // If file doesn't exist, create a blank one to avoid ResourceLocal load
    // error.  We make it user and group read/write, others read.  This is
    // masked by the users umask.  (See man creat)
    if ( !( remoteResource( lFileName ) ) )
    {
        int handle;
        handle = open ( QFile::encodeName( lFileName ), O_CREAT | O_EXCL | O_WRONLY,
               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH );
        if ( handle != -1 )
        {
            close( handle );
        }
    }
    if ( d->mCalendar )
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

    QObject::connect (d->mCalendar, SIGNAL(resourceChanged(ResourceCalendar*)),
  	            view, SLOT(iCalFileModified(ResourceCalendar*)));
    d->mCalendar->setTimeSpec( KSystemTimeZones::local() );
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
        kDebug(5970) << "timetrackerstorage::load"
            << "rawTodo count (includes completed todos) ="
            << todoList.count();
        for( todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo )
        {
            Task* task = new Task(*todo, view);
            map.insert( (*todo)->uid(), task );
            view->setRootIsDecorated(true);
            task->setPixmapProgress();
        }

        // Load each task under it's parent task.
        for( todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo )
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

        kDebug(5970) << "timetrackerstorage::load - loaded" << view->count()
            << "tasks from" << d->mICalFile;
    }

    if ( view ) buildTaskView(d->mCalendar, view);
    return err;
}

Task* timetrackerstorage::task( const QString& uid, TaskView* view )
// return the tasks with the uid uid out of view.
// If !view, return the todo with the uid uid.
{
    kDebug(5970) << "Entering function";
    KCal::Todo::List todoList;
    KCal::Todo::List::ConstIterator todo;
    todoList = d->mCalendar->rawTodos();
    todo = todoList.constBegin();
    Task* result=0;
    bool konsolemode=false;
    if ( view == 0 ) konsolemode=true;
    while ( todo != todoList.constEnd() && ( (*todo)->uid() != uid ) ) ++todo;
    if ( todo != todoList.constEnd() ) result = new Task((*todo), view, konsolemode);
    kDebug(5970) << "Leaving function, returning " << result;
    return result;
}

QString timetrackerstorage::icalfile()
{
    kDebug(5970) << "Entering function";
    return d->mICalFile;
}

QString timetrackerstorage::buildTaskView(KCal::ResourceCalendar *rc, TaskView *view)
// makes *view contain the tasks out of *rc.
{
    kDebug(5970) << "Entering function";
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
        if ( task->isRunning() )
        {
            runningTasks.append( task->uid() );
            startTimes.append( task->startTime() );
        }
        ++it;
    }

    view->clear();
    todoList = rc->rawTodos();
    for ( todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo )
    {
        Task* task = new Task(*todo, view);
        task->setWhatsThis(0,i18n("The task name is what you call the task, it can be chosen freely."));
        task->setWhatsThis(1,i18n("The session time is the time since you last chose \"start new session.\""));
        map.insert( (*todo)->uid(), task );
        view->setRootIsDecorated(true);
        task->setPixmapProgress();
    }

    // 1.1. Load each task under it's parent task.
    for( todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo )
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
    for ( int i=0; i<view->count(); i++)
    {
        for ( int n = 0; n < runningTasks.count(); ++n )
        {
            if ( runningTasks[n] == view->itemAt(i)->uid() )
            {
                view->startTimerFor( view->itemAt(i), startTimes[n] );
            }
        }
    }

    view->refresh();
    return err;
}

QString timetrackerstorage::buildTaskView(TaskView *view)
// makes *view contain the tasks out of mCalendar
{
    return buildTaskView(d->mCalendar, view);
}

void timetrackerstorage::closeStorage()
{
    kDebug(5970) << "Entering function";
    if ( d->mCalendar )
    {
        d->mCalendar->close();
        delete d->mCalendar;
        d->mCalendar = 0;
    }
    kDebug(5970) << "Leaving function";
}

KCal::Event::List timetrackerstorage::rawevents()
{
    kDebug(5970) << "Entering function";
    return d->mCalendar->rawEvents();
}

KCal::Todo::List timetrackerstorage::rawtodos()
{
    kDebug(5970) << "Entering function";
    return d->mCalendar->rawTodos();
}

bool timetrackerstorage::allEventsHaveEndTiMe(Task* task)
{
    kDebug(5970) << "Entering function";
    KCal::Event::List eventList = d->mCalendar->rawEvents();
    for(KCal::Event::List::iterator i = eventList.begin();
        i != eventList.end(); ++i)
    {
        if ( (*i)->relatedToUid() == task->uid()
            || ( (*i)->relatedTo()
            && (*i)->relatedTo()->uid() == task->uid() ) )
        {
            if ( !(*i)->hasEndDate() ) return false;
        }
    }
    return true;
}

bool timetrackerstorage::allEventsHaveEndTiMe()
{
    kDebug(5970) << "Entering function";
    KCal::Event::List eventList = d->mCalendar->rawEvents();
    for(KCal::Event::List::iterator i = eventList.begin();
        i != eventList.end(); ++i)
    {
        if ( !(*i)->hasEndDate() ) return false;
    }
    return true;
}

QString timetrackerstorage::deleteAllEvents()
{
    kDebug(5970) << "Entering function";
    QString err;
    KCal::Event::List eventList = d->mCalendar->rawEvents();
    d->mCalendar->deleteAllEvents();
    return err;
}

QString timetrackerstorage::save(TaskView* taskview)
{
    kDebug(5970) << "Entering function";
    QString err;

    QStack<KCal::Todo*> parents;
    if ( taskview ) // we may also be in konsole mode
    {
        for (int i = 0; i < taskview->topLevelItemCount(); ++i )
        {
            Task *task = static_cast< Task* >( taskview->topLevelItem( i ) );
            kDebug( 5970 ) << "write task" << task->name();
            err = writeTaskAsTodo( task, parents );
        }
    }

    err=saveCalendar();

    if ( err.isEmpty() )
    {
        kDebug(5970) << "timetrackerstorage::save : wrote tasks to" << d->mICalFile;
    }
    else
    {
        kWarning(5970) << "timetrackerstorage::save :" << err;
    }
    return err;
}

QString timetrackerstorage::setTaskParent( Task* task, Task* parent )
{
    kDebug(5970) << "Entering function";
    QString err;
    KCal::Todo* toDo;
    toDo = d->mCalendar->todo(task->uid());
    if (parent==0) toDo->removeRelation(toDo->relatedTo());
    else toDo->setRelatedTo(d->mCalendar->todo(parent->uid()));
    kDebug(5970) << "Leaving function";
    return err;
}

QString timetrackerstorage::writeTaskAsTodo(Task* task, QStack<KCal::Todo*>& parents )
{
    kDebug(5970) << "Entering function";
    QString err;
    KCal::Todo* todo;

    todo = d->mCalendar->todo(task->uid());
    if ( !todo )
    {
        kDebug(5970) << "Could not get todo from calendar";
        return "Could not get todo from calendar";
    }
    task->asTodo(todo);
    if ( !parents.isEmpty() ) todo->setRelatedTo( parents.top() );
    parents.push( todo );

    for ( int i = 0; i < task->childCount(); ++i )
    {
        Task *nextTask = static_cast< Task* >( task->child( i ) );
        err = writeTaskAsTodo( nextTask, parents );
    }

    parents.pop();
    return err;
}

bool timetrackerstorage::isEmpty()
{
    kDebug(5970) << "Entering function";
    KCal::Todo::List todoList;
    todoList = d->mCalendar->rawTodos();
    return todoList.empty();
}

//----------------------------------------------------------------------------
// Routines that handle logging ktimetracker history


QString timetrackerstorage::addTask(const Task* task, const Task* parent)
{
    kDebug(5970) << "Entering function";
    KCal::Todo* todo;
    QString uid;

    if ( !d->mCalendar )
    {
        kDebug(5970) << "mCalendar is not set";
        return uid;
    }
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

QStringList timetrackerstorage::taskidsfromname(QString taskname)
{
    kDebug(5970) << "Entering function";
    QStringList result;
    KCal::Todo::List todoList = d->mCalendar->rawTodos();
    for(KCal::Todo::List::iterator i = todoList.begin();
        i != todoList.end(); ++i)
    {
        kDebug(5970) << (*i)->uid();
        if ( (*i)->summary() == taskname ) result << (*i)->uid();
    }
    return result;
}

QStringList timetrackerstorage::taskNames() const
{
    kDebug(5970) << "Entering function";
    QStringList result;
    KCal::Todo::List todoList = d->mCalendar->rawTodos();
    for(KCal::Todo::List::iterator i = todoList.begin();
        i != todoList.end(); ++i) result << (*i)->summary();
    return result;
}

QString timetrackerstorage::removeEvent(QString uid)
{
    kDebug(5970) << "Entering function";
    QString err=QString();
    KCal::Event::List eventList = d->mCalendar->rawEvents();
    for(KCal::Event::List::iterator i = eventList.begin();
        i != eventList.end(); ++i)
    {
        if ( (*i)->uid() == uid )
        {
            d->mCalendar->deleteEvent(*i);
        }
    }
    return err;
}

bool timetrackerstorage::removeTask(Task* task)
{
    kDebug(5970) << "Entering function";
    // delete history
    KCal::Event::List eventList = d->mCalendar->rawEvents();
    for(KCal::Event::List::iterator i = eventList.begin();
        i != eventList.end(); ++i)
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

bool timetrackerstorage::removeTask(QString taskid)
{
    kDebug(5970) << "Entering function";
    // delete history
    KCal::Event::List eventList = d->mCalendar->rawEvents();
    for(KCal::Event::List::iterator i = eventList.begin();
        i != eventList.end();
        ++i)
    {
        if ( (*i)->relatedToUid() == taskid
            || ( (*i)->relatedTo()
            && (*i)->relatedTo()->uid() == taskid))
        {
            d->mCalendar->deleteEvent(*i);
        }
    }

    // delete todo
    KCal::Todo *todo = d->mCalendar->todo(taskid);
    d->mCalendar->deleteTodo(todo);

    // Save entire file
    saveCalendar();

    return true;
}

void timetrackerstorage::addComment(const Task* task, const QString& comment)
{
    kDebug(5970) << "Entering function";
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

QString timetrackerstorage::report( TaskView *taskview, const ReportCriteria &rc )
{
    kDebug(5970) << "Entering function";
    QString err;
    if ( rc.reportType == ReportCriteria::CSVHistoryExport )
    {
        err = exportcsvHistory( taskview, rc.from, rc.to, rc );
    }
    else // if ( rc.reportType == ReportCriteria::CSVTotalsExport )
    {
        if ( !rc.bExPortToClipBoard )
            err = exportcsvFile( taskview, rc );
        else
            err = taskview->clipTotals( rc );
    }
    return err;
}


//----------------------------------------------------------------------------
// Routines that handle Comma-Separated Values export file format.
//
QString timetrackerstorage::exportcsvFile( TaskView *taskview,
                                    const ReportCriteria &rc )
{
    kDebug(5970) << "Entering function";
    QString delim = rc.delimiter;
    QString dquote = rc.quote;
    QString double_dquote = dquote + dquote;
    bool to_quote = true;
    QString err;
    Task* task;
    int maxdepth=0;
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
        QString filename=rc.url.toLocalFile();
        if (filename.isEmpty()) filename=rc.url.url();
        QFile f( filename );
        if( !f.open( QIODevice::WriteOnly ) )
        {
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

// export history report as csv, all tasks X all dates in one block
QString timetrackerstorage::exportcsvHistory ( TaskView      *taskview,
                                            const QDate   &from,
                                            const QDate   &to,
                                            const ReportCriteria &rc)
{
    kDebug(5970) << "Entering function";
    QString delim = rc.delimiter;
    const QString cr = QString::fromLatin1("\n");
    QString err=QString::null;
    QString retval;
    Task* task;

    QTableWidget* itab=new QTableWidget(); // hello, ABAP
    itab->setRowCount(taskview->count()+1);
    itab->setColumnCount(from.daysTo(to)+2);
    // parameter-plausi
    if ( from > to )
    {
        err = QString("'to' has to be a date later than or equal to 'from'.");
    }
    else
    {
        itab->setItem(0,0,new QTableWidgetItem("Task name"));
        for ( QDate mdate=from; mdate.daysTo(to)>=0; mdate=mdate.addDays(1) )
        {
            kDebug(5970) << mdate.toString();
            itab->setItem(0,1+from.daysTo(mdate),new QTableWidgetItem(mdate.toString()));
            for ( int n=0; n<taskview->count(); n++ )
            {
                task=taskview->itemAt(n);
                itab->setItem(n+1,0,new QTableWidgetItem(taskview->itemAt(n)->name()));
                KCal::Event::List eventList = d->mCalendar->rawEvents();
                for(KCal::Event::List::iterator i = eventList.begin();
                    i != eventList.end(); ++i)
                {
                    if ( (*i)->relatedToUid() == task->uid() )
                    {
                        kDebug(5970) << "found an event for task, event=" << (*i)->uid();
                        // dtStart is stored like DTSTART;TZID=Europe/Berlin:20080327T231056
                        // dtEnd is stored like DTEND:20080327T231509Z
                        // we need to subtract the offset from UTC.
                        KDateTime startTime=(*i)->dtStart().addSecs((*i)->dtStart().utcOffset());
                        KDateTime endTime=(*i)->dtEnd().addSecs((*i)->dtEnd().utcOffset());
                        KDateTime NextMidNight=startTime;
                        NextMidNight.setTime(QTime ( 0,0 ));
                        NextMidNight=NextMidNight.addDays(1);
                        // LastMidNight := mdate.setTime(0:00) as it would read in a decent programming language
                        KDateTime LastMidNight=KDateTime::currentLocalDateTime();
                        LastMidNight.setDate(mdate);
                        LastMidNight.setTime(QTime(0,0));
                        int secsstartTillMidNight=startTime.secsTo(NextMidNight);
                        int secondsToAdd=0; // seconds that need to be added to the actual cell
                        if ( (startTime.date()==mdate) && ((*i)->dtEnd().date()==mdate) ) // all the event occurred today
                            secondsToAdd=startTime.secsTo(endTime);
                        if ( (startTime.date()==mdate) && (endTime.date()>mdate) ) // the event started today, but ended later
                            secondsToAdd=secsstartTillMidNight;
                        if ( (startTime.date()<mdate) && (endTime.date()==mdate) ) // the event started before today and ended today
                            secondsToAdd=LastMidNight.secsTo((*i)->dtEnd());
                        if ( (startTime.date()<mdate) && (endTime.date()>mdate) ) // the event started before today and ended after
                            secondsToAdd=86400;
                        int secondsSum=secondsToAdd;
                        if (itab->item(n+1,from.daysTo(mdate)+1))
                            secondsSum=itab->item(n+1,from.daysTo(mdate)+1)->text().toInt()+secondsToAdd;
                        itab->setItem(n+1,from.daysTo(mdate)+1,new QTableWidgetItem(QString::number(secondsSum)));
                    };
                }
            }
        }
        // use the internal table itab to create the return value retval
        for ( int y=0; y<=(taskview->count()); y++ )
        {
            if (itab->item(y,0)) retval.append("\"").append(itab->item(y,0)->text().replace("\"","\"\"")).append("\"");  // task names
            for ( int x=1; x<=from.daysTo(to)+1; x++ )
            {
                retval.append(rc.delimiter);
                if (y>0)
                {
                    if (itab->item(y,x))
                    {
                        kDebug(5970) << "itab->item(y,x)=" << itab->item(y,x)->text();
                        retval.append(formatTime( itab->item(y,x)->text().toInt()/60.0, rc.decimalMinutes ));
                    }
                }
                else // heading
                {
                    retval.append(itab->item(y,x)->text());
                }
            };
            retval.append("\n");
        }
        kDebug() << "Retval is \n" << retval;
    }
    // itab->show(); // GREAT for debugging purposes :)
    if (rc.bExPortToClipBoard)
        taskview->setClipBoardText(retval);
    else
    {
        // store the file locally or remote
        if ((rc.url.isLocalFile()) || (!rc.url.url().contains("/")))
        {
            kDebug(5970) << "storing a local file";
            QString filename=rc.url.toLocalFile();
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
    }
    return err;
}

void timetrackerstorage::startTimer( const Task* task, const KDateTime &when )
{
    kDebug(5970) << "Entering function; when=" << when;
    KCal::Event* e;
    e = baseEvent(task);
    e->setDtStart(when);
    d->mCalendar->addEvent(e);
    task->taskView()->scheduleSave();
}

void timetrackerstorage::startTimer( QString taskID )
{
    kDebug(5970) << "Entering function";
    KCal::Todo::List todoList;
    KCal::Todo::List::ConstIterator todo;
    todoList = d->mCalendar->rawTodos();
    for( todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo )
    {
        kDebug(5970) << (*todo)->uid();
        kDebug(5970) << taskID;
        if ( (*todo)->uid() == taskID )
        {
            kDebug(5970) << "adding event";
            KCal::Event* e;
            e = baseEvent((*todo));
            e->setDtStart(KDateTime::currentLocalDateTime());
            d->mCalendar->addEvent(e);
        }
    }
    saveCalendar();
}

void timetrackerstorage::stopTimer( const Task* task, const QDateTime &when )
{
    kDebug(5970) << "Entering function; when=" << when;
    KCal::Event::List eventList = d->mCalendar->rawEvents();
    for(KCal::Event::List::iterator i = eventList.begin();
        i != eventList.end();
        ++i)
    {
        if ( (*i)->relatedToUid() == task->uid() )
        {
            kDebug(5970) << "found an event for task, event=" << (*i)->uid();
            if (!(*i)->hasEndDate())
            {
                kDebug(5970) << "this event has no enddate";
                (*i)->setDtEnd(KDateTime(when, KDateTime::Spec::LocalZone()));
            }
            else
            {
                kDebug(5970) << "this event has an enddate";
                kDebug(5970) << "end date is " << (*i)->dtEnd();
            }
        };
    }
    saveCalendar();
}

void timetrackerstorage::changeTime(const Task* task, const long deltaSeconds)
{
    kDebug(5970) << "Entering function; deltaSeconds=" << deltaSeconds;
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


KCal::Event* timetrackerstorage::baseEvent(const Task * task)
{
    kDebug(5970) << "Entering function";
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
    categories.append(i18n("KTimeTracker"));
    e->setCategories(categories);

    return e;
}

KCal::Event* timetrackerstorage::baseEvent(const Todo* todo)
{
    kDebug(5970) << "Entering function";
    KCal::Event* e;
    QStringList categories;

    e = new KCal::Event;
    e->setSummary(todo->summary());

    // Can't use setRelatedToUid()--no error, but no RelatedTo written to disk
    e->setRelatedTo(d->mCalendar->todo(todo->uid()));

    // Have to turn this off to get datetimes in date fields.
    e->setAllDay(false);
    e->setDtStart(KDateTime(todo->dtStart()));

    // So someone can filter this mess out of their calendar display
    categories.append(i18n("KTimeTracker"));
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

bool timetrackerstorage::remoteResource( const QString& file ) const
{
    kDebug(5970) << "Entering function";
    QString f = file.toLower();
    bool rval = f.startsWith( QLatin1String("http://") ) || f.startsWith( QLatin1String("ftp://") );
    kDebug(5970) << "timetrackerstorage::remoteResource(" << file <<" ) returns" << rval;
    return rval;
}

QString timetrackerstorage::saveCalendar()
{
    kDebug(5970) << "Entering function";
    QString err;
    KABC::Lock *lock;
    if ( d->mCalendar )
    {
        lock = d->mCalendar->lock();
    }
    else
    {
        kDebug(5970) << "mCalendar not set";
        return err;
    }
    if ( !lock || !lock->lock() ) err=QString("Could not save. Could not lock file.");
    if ( d->mCalendar->save() )
    {
        lock->unlock();
    }
    else err=QString("Could not save. Could lock file.");
    lock->unlock();
    return err;
}
