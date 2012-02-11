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
#include <KCalCore/Person>

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
#include <QMap>

using namespace KTimeTracker;

//@cond PRIVATE
class timetrackerstorage::Private
{
public:
    Private() : mCalendar( 0 ) {}
    ~Private()
    {
        delete mCalendar;
    }
    KTTCalendar::Ptr mCalendar;
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

QString timetrackerstorage::load(TaskView* view, const QString &fileName)
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

    KCal::ResourceCached* resource;
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
    KCalCore::Person::Ptr owner = resource->owner();
    if ( owner && owner->isEmpty() )
    {
        resource->setOwner( KCalCore::Person::Ptr(
           new KCalCore::Person( settings.getSetting( KEMailSettings::RealName ),
                                 settings.getSetting( KEMailSettings::EmailAddress ) ) ) );
    }

    // Build task view from iCal data
    if (!err.isEmpty())
    {
        KCalCore::Todo::List todoList;
        KCalCore::Todo::List::ConstIterator todo;
        QMultiHash< QString, Task* > map;

        // Build dictionary to look up Task object from Todo uid.  Each task is a
        // QListViewItem, and is initially added with the view as the parent.
        todoList = d->mCalendar->rawTodos();
        kDebug(5970) << "timetrackerstorage::load"
            << "rawTodo count (includes completed todos) ="
            << todoList.count();
        for (todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo)
        {
            Task* task = new Task(*todo, view);
            map.insert( (*todo)->uid(), task );
            view->setRootIsDecorated(true);
            task->setPixmapProgress();
        }

        // Load each task under it's parent task.
        for (todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo)
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

Task* timetrackerstorage::task(const QString& uid, TaskView* view)
// return the tasks with the uid uid out of view.
// If !view, return the todo with the uid uid.
{
    kDebug(5970) << "Entering function";
    KCalCore::Todo::List todoList;
    KCalCore::Todo::List::ConstIterator todo;
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

QString timetrackerstorage::buildTaskView( const KTimeTracker::KTTCalendar::Ptr &calendar,
                                           TaskView *view )
// makes *view contain the tasks out of *rc.
{
    kDebug(5970) << "Entering function";
    QString err;
    KCalCore::Todo::List todoList;
    KCalCore::Todo::List::ConstIterator todo;
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
    todoList = calendar->rawTodos();
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

KCalCore::Event::List timetrackerstorage::rawevents()
{
    kDebug(5970) << "Entering function";
    return d->mCalendar->rawEvents();
}

KCalCore::Todo::List timetrackerstorage::rawtodos()
{
    kDebug(5970) << "Entering function";
    return d->mCalendar->rawTodos();
}

bool timetrackerstorage::allEventsHaveEndTiMe(Task* task)
{
    kDebug(5970) << "Entering function";
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
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
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
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
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    d->mCalendar->deleteAllEvents();
    return err;
}

QString timetrackerstorage::save(TaskView* taskview)
{
    kDebug(5970) << "Entering function";
    QString err;

    QStack<KCalCore::Todo::Ptr> parents;
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

QString timetrackerstorage::setTaskParent(Task* task, Task* parent)
{
    kDebug(5970) << "Entering function";
    QString err;
    KCalCore::Todo::Ptr toDo;
    toDo = d->mCalendar->todo(task->uid());
    if (parent==0) toDo->removeRelation(toDo->relatedTo());
    else toDo->setRelatedTo(d->mCalendar->todo(parent->uid()));
    kDebug(5970) << "Leaving function";
    return err;
}

QString timetrackerstorage::writeTaskAsTodo(Task* task, QStack<KCalCore::Todo::Ptr>& parents)
{
    kDebug(5970) << "Entering function";
    QString err;
    KCalCore::Todo::Ptr todo = d->mCalendar->todo(task->uid());
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
    return d->mCalendar->rawTodos().isEmpty();
}

//----------------------------------------------------------------------------
// Routines that handle logging ktimetracker history


QString timetrackerstorage::addTask(const Task* task, const Task* parent)
{
    kDebug(5970) << "Entering function";
    KCalCore::Todo::Ptr todo;
    QString uid;

    if ( !d->mCalendar )
    {
        kDebug(5970) << "mCalendar is not set";
        return uid;
    }
    todo = KCalCore::Todo::Ptr( new KCalCore::Todo() );
    if ( d->mCalendar->addTodo( todo ) )
    {
        task->asTodo( todo );
        if (parent)
            todo->setRelatedTo(d->mCalendar->todo(parent->uid()));
        uid = todo->uid();
    }
    else
    {
        // Most likely a lock could not be pulled, although there are other
        // possiblities (like a really confused resource manager).
        uid.clear();;
    }
    return uid;
}

QStringList timetrackerstorage::taskidsfromname(QString taskname)
{
    kDebug(5970) << "Entering function";
    QStringList result;
    KCalCore::Todo::List todoList = d->mCalendar->rawTodos();
    for(KCalCore::Todo::List::iterator i = todoList.begin();
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
    KCalCore::Todo::List todoList = d->mCalendar->rawTodos();
    for(KCalCore::Todo::List::iterator i = todoList.begin();
        i != todoList.end(); ++i) result << (*i)->summary();
    return result;
}

QString timetrackerstorage::removeEvent(QString uid)
{
    kDebug(5970) << "Entering function";
    QString err=QString();
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
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
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
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
    KCalCore::Todo::Ptr todo = d->mCalendar->todo(task->uid());
    d->mCalendar->deleteTodo(todo);
    // Save entire file
    saveCalendar();

    return true;
}

bool timetrackerstorage::removeTask(QString taskid)
{
    kDebug(5970) << "Entering function";
    // delete history
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
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
    KCalCore::Todo::Ptr todo = d->mCalendar->todo(taskid);
    d->mCalendar->deleteTodo(todo);

    // Save entire file
    saveCalendar();

    return true;
}

void timetrackerstorage::addComment(const Task* task, const QString& comment)
{
    kDebug(5970) << "Entering function";
    KCalCore::Todo::Ptr todo = d->mCalendar->todo(task->uid());

    // Do this to avoid compiler warnings about comment not being used.  once we
    // transition to using the addComment method, we need this second param.
    QString s = comment;

    // TODO: Use libkcalcore comments
    // todo->addComment(comment);
    // temporary
    todo->setDescription(task->comment());

    saveCalendar();
}

QString timetrackerstorage::report(TaskView *taskview, const ReportCriteria &rc)
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
QString timetrackerstorage::exportcsvFile(TaskView *taskview, const ReportCriteria &rc)
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

int todaySeconds (const QDate &date, const KCalCore::Event::Ptr &event)
{
        kDebug(5970) << "found an event for task, event=" << event.uid();
        KDateTime startTime=event.dtStart();
        KDateTime endTime=event.dtEnd();
        KDateTime NextMidNight=startTime;
        NextMidNight.setTime(QTime ( 0,0 ));
        NextMidNight=NextMidNight.addDays(1);
        // LastMidNight := mdate.setTime(0:00) as it would read in a decent programming language
        KDateTime LastMidNight=KDateTime::currentLocalDateTime();
        LastMidNight.setDate(date);
        LastMidNight.setTime(QTime(0,0));
        int secsstartTillMidNight=startTime.secsTo(NextMidNight);
        int secondsToAdd=0; // seconds that need to be added to the actual cell
        if ( (startTime.date()==date) && (event.dtEnd().date()==date) ) // all the event occurred today
            secondsToAdd=startTime.secsTo(endTime);
        if ( (startTime.date()==date) && (endTime.date()>date) ) // the event started today, but ended later
            secondsToAdd=secsstartTillMidNight;
        if ( (startTime.date()<date) && (endTime.date()==date) ) // the event started before today and ended today
            secondsToAdd=LastMidNight.secsTo(event.dtEnd());
        if ( (startTime.date()<date) && (endTime.date()>date) ) // the event started before today and ended after
            secondsToAdd=86400;

        return secondsToAdd;
}

// export history report as csv, all tasks X all dates in one block
QString timetrackerstorage::exportcsvHistory (TaskView      *taskview,
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
    const int intervalLength = from.daysTo(to)+1;
    QMap< QString, QVector<int> > secsForUid;
    QMap< QString, QString > uidForName;


    // Step 1: Prepare two hashmaps:
    // * "uid -> seconds each day": used while traversing events, as uid is their id
    //                              "seconds each day" are stored in a vector
    // * "name -> uid", ordered by name: used when creating the csv file at the end
    kDebug(5970) << "Taskview Count: " << taskview->count();
    for ( int n=0; n<taskview->count(); n++ )
    {
        task=taskview->itemAt(n);
        kDebug(5970) << "n: " << n << ", Task Name: " << task->name() << ", UID: " << task->uid();
        // uid -> seconds each day
        // * Init each element to zero
        QVector<int> vector(intervalLength, 0);
        secsForUid[task->uid()] = vector;

        // name -> uid
        // * Create task fullname concatenating each parent's name
        QString fullName;
        Task* parentTask;
        parentTask = task;
        fullName += parentTask->name();
        parentTask = parentTask->parent();
        while (parentTask)
        {
            fullName = parentTask->name() + "->" + fullName;
            kDebug(5970) << "Fullname(inside): " << fullName;
            parentTask = parentTask->parent();
            kDebug(5970) << "Parent task: " << parentTask;
        }

        uidForName[fullName] = task->uid();

        kDebug(5970) << "Fullname(end): " << fullName;
    }

    kDebug(5970) << "secsForUid" << secsForUid;
    kDebug(5970) << "uidForName" << uidForName;


    // Step 2: For each date, get the events and calculate the seconds
    // Store the seconds using secsForUid hashmap, so we don't need to translate uids
    // We rely on rawEventsForDate to get the events
    kDebug(5970) << "Let's iterate for each date: ";
    for ( QDate mdate=from; mdate.daysTo(to)>=0; mdate=mdate.addDays(1) )
    {
        kDebug(5970) << mdate.toString();
        KCalCore::Event::List dateEvents = d->mCalendar->rawEventsForDate(mdate);

        for(KCalCore::Event::List::iterator i = dateEvents.begin();i != dateEvents.end(); ++i)
        {
            kDebug(5970) << "Summary: " << (*i)->summary() << ", Related to uid: " << (*i)->relatedToUid();
            kDebug(5970) << "Today's seconds: " << todaySeconds(mdate, **i);
            secsForUid[(*i)->relatedToUid()][from.daysTo(mdate)] += todaySeconds(mdate, **i);
        }
    }


    // Step 3: For each task, generate the matching row for the CSV file
    // We use the two hashmaps to have direct access using the task name

    // First CSV file line
    // FIXME: localize strings and date formats
    retval.append("\"Task name\"");
    for (int i=0; i<intervalLength; i++)
    {
        retval.append(delim);
        retval.append(from.addDays(i).toString());
    }
    retval.append(cr);


    // Rest of the CSV file
    QMapIterator<QString, QString> nameUid(uidForName);
    double time;
    while (nameUid.hasNext())
    {
        nameUid.next();
        retval.append("\"" + nameUid.key() + "\"");
        kDebug(5970) << nameUid.key() << ": " << nameUid.value() << endl;

        for (int day=0; day<intervalLength; day++)
        {
            kDebug(5970) << "Secs for day " << day << ":" << secsForUid[nameUid.value()][day];
            retval.append(delim);
            time = secsForUid[nameUid.value()][day]/60.0;
            retval.append(formatTime(time, rc.decimalMinutes));
        }

        retval.append(cr);
    }

    kDebug() << "Retval is \n" << retval;

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

void timetrackerstorage::startTimer(const Task* task, const KDateTime &when)
{
    kDebug(5970) << "Entering function; when=" << when;
    KCalCore::Event::Ptr e;
    e = baseEvent(task);
    e->setDtStart(when);
    d->mCalendar->addEvent(e);
    task->taskView()->scheduleSave();
}

void timetrackerstorage::startTimer(QString taskID)
{
    kDebug(5970) << "Entering function";
    KCalCore::Todo::List todoList;
    KCalCore::Todo::List::ConstIterator todo;
    todoList = d->mCalendar->rawTodos();
    for( todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo )
    {
        kDebug(5970) << (*todo)->uid();
        kDebug(5970) << taskID;
        if ( (*todo)->uid() == taskID )
        {
            kDebug(5970) << "adding event";
            KCalCore::Event::Ptr e;
            e = baseEvent((*todo));
            e->setDtStart(KDateTime::currentLocalDateTime());
            d->mCalendar->addEvent(e);
        }
    }
    saveCalendar();
}

void timetrackerstorage::stopTimer(const Task* task, const QDateTime &when)
{
    kDebug(5970) << "Entering function; when=" << when;
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
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
    KCalCore::Event::Ptr e;
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


KCalCore::Event::Ptr timetrackerstorage::baseEvent(const Task *task)
{
    kDebug(5970) << "Entering function";
    KCalCore::Event::Ptr e( new KCalCore::Event() );
    QStringList categories;
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

KCalCore::Event::Ptr timetrackerstorage::baseEvent(const KCalCore::Todo::Ptr &todo)
{
    kDebug(5970) << "Entering function";
    KCalCore::Event::Ptr e( new KCalCore::Event() );
    QStringList categories;
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

HistoryEvent::HistoryEvent(const QString &uid, const QString &name,
                            long duration, const KDateTime &start,
                            const KDateTime &stop, const QString &todoUid)
{
    _uid = uid;
    _name = name;
    _duration = duration;
    _start = start;
    _stop = stop;
    _todoUid = todoUid;
}

bool timetrackerstorage::remoteResource(const QString& file) const
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
