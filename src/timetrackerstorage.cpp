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

/** TimeTrackerStorage
  * This class cares for the storage of ktimetracker's data.
  * ktimetracker's data is
  * - tasks like "programming for customer foo"
  * - events like "from 2009-09-07, 8pm till 10pm programming for customer foo"
  * tasks are like the items on your todo list, events are dates when you worked on them.
  * ktimetracker's data is stored in a ResourceCalendar object hold by mCalendar.
  */

#include "timetrackerstorage.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <QApplication>
#include <QLockFile>
#include <QProgressDialog>
#include <QUrl>
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
#include <QMap>

#include <KCalCore/Person>
#include <KDirWatch>
#include <KJobUiDelegate>
#include <KMessageBox>
#include <KLocalizedString>
#include <KJobWidgets>
#include <KEMailSettings>
#include <KIO/StoredTransferJob>

#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "task.h"
#include "taskview.h"
#include "timekard.h"
#include "ktt_debug.h"

const QByteArray eventAppName = QByteArray("ktimetracker");

//@cond PRIVATE
class TimeTrackerStorage::Private
{
public:
    Private()
    {
        m_fileLock = new QLockFile(QStringLiteral("ktimetrackerics.lock"));
    }

    ~Private()
    {
      delete m_fileLock;
    }

    KTTCalendar::Ptr mCalendar;
    QString mICalFile;
    QLockFile *m_fileLock;
};
//@endcond

TimeTrackerStorage::TimeTrackerStorage()
    : d(new Private())
{
}

TimeTrackerStorage::~TimeTrackerStorage()
{
    delete d;
}

QString TimeTrackerStorage::load(TaskView* view, const QString &fileName)
// loads data from filename into view. If no filename is given, filename from preferences is used.
// filename might be of use if this program is run as embedded konqueror plugin.
{
    // loading might create the file
    bool removedFromDirWatch = false;
    if ( KDirWatch::self()->contains( d->mICalFile ) ) {
      KDirWatch::self()->removeFile( d->mICalFile );
      removedFromDirWatch = true;
    }
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    KEMailSettings settings;
    QString lFileName = fileName;

    Q_ASSERT( !( lFileName.isEmpty() ) );

    // If same file, don't reload
    if ( lFileName == d->mICalFile ) {
        if ( removedFromDirWatch ) {
          KDirWatch::self()->addFile( d->mICalFile );
        }
        return err;
    }
    // If file doesn't exist, create a blank one to avoid ResourceLocal load
    // error.  We make it user and group read/write, others read.  This is
    // masked by the users umask.  (See man creat)
    const bool fileIsLocal = !isRemoteFile( lFileName );
    if (fileIsLocal) {
        int handle;
        handle = open(QFile::encodeName( lFileName ), O_CREAT | O_EXCL | O_WRONLY,
               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH );
        if (handle != -1) {
            close(handle);
        }
    }
    if (d->mCalendar) {
        closeStorage();
    }
    // Create local file resource and add to resources
    d->mICalFile = lFileName;
    d->mCalendar = KTTCalendar::createInstance(d->mICalFile, /*monitorFile=*/ fileIsLocal);

    if (view) {
        connect(d->mCalendar.data(), &KTTCalendar::calendarChanged, view, &TaskView::iCalFileModified);
    }
//    d->mCalendar->setTimeSpec( KSystemTimeZones::local() );
    d->mCalendar->reload();

    // Claim ownership of iCalendar file if no one else has.
    KCalCore::Person::Ptr owner = d->mCalendar->owner();
    if (owner && owner->isEmpty()) {
        d->mCalendar->setOwner( KCalCore::Person::Ptr(
           new KCalCore::Person( settings.getSetting( KEMailSettings::RealName ),
                                 settings.getSetting( KEMailSettings::EmailAddress ) ) ) );
    }

    // Build task view from iCal data
    if (!err.isEmpty()) {
        KCalCore::Todo::List todoList;
        KCalCore::Todo::List::ConstIterator todo;
        QMultiHash< QString, Task* > map;

        // Build dictionary to look up Task object from Todo uid.  Each task is a
        // QListViewItem, and is initially added with the view as the parent.
        todoList = d->mCalendar->rawTodos();
        qCDebug(KTT_LOG) << "TimeTrackerStorage::load"
            << "rawTodo count (includes completed todos) ="
            << todoList.count();
        for (todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo) {
            Task* task = new Task(*todo, view);
            map.insert( (*todo)->uid(), task );
            view->setRootIsDecorated(true);
            task->setPixmapProgress();
        }

        // Load each task under its parent task.
        for (todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo)
        {
            Task* task = map.value( (*todo)->uid() );
            // No relatedTo incident just means this is a top-level task.
            if ( !(*todo)->relatedTo().isEmpty() )
            {
                Task *newParent = map.value( (*todo)->relatedTo() );

                // Complete the loading but return a message
                if ( !newParent )
                    err = i18n("Error loading \"%1\": could not find parent (uid=%2)",
                        task->name(), (*todo)->relatedTo()  );

                if (!err.isEmpty()) task->move( newParent );
            }
        }

        qCDebug(KTT_LOG) << "TimeTrackerStorage::load - loaded" << view->count()
            << "tasks from" << d->mICalFile;
    }

    if (view) {
        buildTaskView(d->mCalendar->weakPointer(), view);
    }

    if (removedFromDirWatch) {
        KDirWatch::self()->addFile(d->mICalFile);
    }
    return err;
}

Task* TimeTrackerStorage::task(const QString& uid, TaskView* view)
// return the tasks with the uid uid out of view.
// If !view, return the todo with the uid uid.
{
    qCDebug(KTT_LOG) << "Entering function";
    KCalCore::Todo::List todoList;
    KCalCore::Todo::List::ConstIterator todo;
    todoList = d->mCalendar->rawTodos();
    todo = todoList.constBegin();
    Task* result=0;

    while (todo != todoList.constEnd() && ((*todo)->uid() != uid)) {
        ++todo;
    }

    if (todo != todoList.constEnd()) {
        result = new Task((*todo), view);
    }

    qCDebug(KTT_LOG) << "Leaving function, returning " << result;
    return result;
}

QString TimeTrackerStorage::icalfile()
{
    qCDebug(KTT_LOG) << "Entering function";
    return d->mICalFile;
}

QString TimeTrackerStorage::buildTaskView(const KTTCalendar::Ptr& calendar, TaskView* view)
// makes *view contain the tasks out of *rc.
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    KCalCore::Todo::List todoList;
    KCalCore::Todo::List::ConstIterator todo;
    QMultiHash< QString, Task* > map;
    QVector<QString> runningTasks;
    QVector<QDateTime> startTimes;

    // remember tasks that are running and their start times
    QTreeWidgetItemIterator it(view);
    while (*it) {
        Task *task = static_cast<Task*>(*it);
        if (task->isRunning()) {
            runningTasks.append(task->uid());
            startTimes.append(task->startTime());
        }
        ++it;
    }

    view->clear();
    todoList = calendar->rawTodos();
    for (todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo) {
        Task* task = new Task(*todo, view);
        task->setWhatsThis(0, i18n("The task name is what you call the task, it can be chosen freely."));
        task->setWhatsThis(1, i18n("The session time is the time since you last chose \"start new session.\""));
        map.insert((*todo)->uid(), task);
        view->setRootIsDecorated(true);
        task->setPixmapProgress();
    }

    // 1.1. Load each task under its parent task.
    for (todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo) {
        Task* task = map.value((*todo)->uid());
        // No relatedTo incident just means this is a top-level task.
        if (!(*todo)->relatedTo().isEmpty()) {
            Task* newParent = map.value((*todo)->relatedTo());
            // Complete the loading but return a message
            if (!newParent) {
                err = i18n("Error loading \"%1\": could not find parent (uid=%2)",
                           task->name(),
                           (*todo)->relatedTo());
            } else {
                task->move(newParent);
            }
        }
    }

    view->clearActiveTasks();
    // restart tasks that have been running with their start times
    for (int i = 0; i<view->count(); ++i) {
        for (int n = 0; n < runningTasks.count(); ++n) {
            if (runningTasks[n] == view->itemAt(i)->uid()) {
                view->startTimerFor(view->itemAt(i), startTimes[n]);
            }
        }
    }

    view->refresh();
    return err;
}

QString TimeTrackerStorage::buildTaskView(TaskView *view)
// makes *view contain the tasks out of mCalendar
{
    return buildTaskView(d->mCalendar, view);
}

void TimeTrackerStorage::closeStorage()
{
    qCDebug(KTT_LOG) << "Entering function";
    if (d->mCalendar) {
        d->mCalendar->close();
        d->mCalendar = KTTCalendar::Ptr();
    }
    qCDebug(KTT_LOG) << "Leaving function";
}

KCalCore::Event::List TimeTrackerStorage::rawevents()
{
    qCDebug(KTT_LOG) << "Entering function";
    return d->mCalendar->rawEvents();
}

KCalCore::Todo::List TimeTrackerStorage::rawtodos()
{
    qCDebug(KTT_LOG) << "Entering function";
    return d->mCalendar->rawTodos();
}

bool TimeTrackerStorage::allEventsHaveEndTiMe(Task* task)
{
    qCDebug(KTT_LOG) << "Entering function";
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
        i != eventList.end(); ++i)
    {
        if ( (*i)->relatedTo() == task->uid() && !(*i)->hasEndDate() ) {
          return false;
        }
    }
    return true;
}

bool TimeTrackerStorage::allEventsHaveEndTiMe()
{
    qCDebug(KTT_LOG) << "Entering function";
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
        i != eventList.end(); ++i)
    {
        if ( !(*i)->hasEndDate() ) return false;
    }
    return true;
}

QString TimeTrackerStorage::deleteAllEvents()
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
//    d->mCalendar->deleteAllEvents(); // FIXME
    return err;
}

QString TimeTrackerStorage::save(TaskView* taskview)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString errorString;

    QStack<KCalCore::Todo::Ptr> parents;
    for (int i = 0; i < taskview->topLevelItemCount(); ++i) {
        Task *task = static_cast< Task* >( taskview->topLevelItem( i ) );
        qCDebug(KTT_LOG) << "write task" << task->name();
        errorString = writeTaskAsTodo(task, parents);
    }

    errorString = saveCalendar();

    if (errorString.isEmpty()) {
        qCDebug(KTT_LOG) << "TimeTrackerStorage::save : wrote tasks to" << d->mICalFile;
    } else {
        qCWarning(KTT_LOG) << "TimeTrackerStorage::save :" << errorString;
    }

    return errorString;
}

QString TimeTrackerStorage::writeTaskAsTodo(Task* task, QStack<KCalCore::Todo::Ptr>& parents)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    KCalCore::Todo::Ptr todo = d->mCalendar->todo(task->uid());
    if (!todo) {
        qCDebug(KTT_LOG) << "Could not get todo from calendar";
        return "Could not get todo from calendar";
    }
    task->asTodo(todo);
    if (!parents.isEmpty()) {
        todo->setRelatedTo(parents.top() ? parents.top()->uid() : QString());
    }
    parents.push( todo );

    for (int i = 0; i < task->childCount(); ++i) {
        Task *nextTask = static_cast<Task*>(task->child(i));
        err = writeTaskAsTodo( nextTask, parents );
    }

    parents.pop();
    return err;
}

bool TimeTrackerStorage::isEmpty()
{
    qCDebug(KTT_LOG) << "Entering function";
    return d->mCalendar->rawTodos().isEmpty();
}

//----------------------------------------------------------------------------
// Routines that handle logging ktimetracker history


QString TimeTrackerStorage::addTask(const Task* task, const Task* parent)
{
    qCDebug(KTT_LOG) << "Entering function";
    KCalCore::Todo::Ptr todo;
    QString uid;

    if (!d->mCalendar) {
        qCDebug(KTT_LOG) << "mCalendar is not set";
        return uid;
    }
    todo = KCalCore::Todo::Ptr(new KCalCore::Todo());
    if (d->mCalendar->addTodo(todo)) {
        task->asTodo(todo);
        if (parent) {
            todo->setRelatedTo( parent->uid() );
        }
        uid = todo->uid();
    } else {
        // Most likely a lock could not be pulled, although there are other
        // possiblities (like a really confused resource manager).
        uid.clear();
    }
    return uid;
}

QStringList TimeTrackerStorage::taskNames() const
{
    qCDebug(KTT_LOG) << "Entering function";
    QStringList result;
    KCalCore::Todo::List todoList = d->mCalendar->rawTodos();
    for(KCalCore::Todo::List::iterator i = todoList.begin();
        i != todoList.end(); ++i) {
        result << (*i)->summary();
    }
    return result;
}

QString TimeTrackerStorage::removeEvent(QString uid)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err=QString();
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
        i != eventList.end(); ++i) {
        if ((*i)->uid() == uid) {
            d->mCalendar->deleteEvent(*i);
        }
    }
    return err;
}

bool TimeTrackerStorage::removeTask(Task* task)
{
    qCDebug(KTT_LOG) << "Entering function";
    // delete history
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
        i != eventList.end(); ++i) {
        if ((*i)->relatedTo() == task->uid()) {
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

bool TimeTrackerStorage::removeTask(QString taskid)
{
    qCDebug(KTT_LOG) << "Entering function";
    // delete history
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
        i != eventList.end();
        ++i) {
        if ((*i)->relatedTo() == taskid) {
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

void TimeTrackerStorage::addComment(const Task* task, const QString& comment)
{
    qCDebug(KTT_LOG) << "Entering function";
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

QString TimeTrackerStorage::report(TaskView *taskview, const ReportCriteria &rc)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    if (rc.reportType == ReportCriteria::CSVHistoryExport) {
        err = exportcsvHistory(taskview, rc.from, rc.to, rc);
    } else { // rc.reportType == ReportCriteria::CSVTotalsExport
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
QString TimeTrackerStorage::exportcsvFile(TaskView *taskview, const ReportCriteria &rc)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString delim = rc.delimiter;
    QString dquote = rc.quote;
    QString double_dquote = dquote + dquote;
    bool to_quote = true;
    QString err;
    Task* task;
    int maxdepth=0;
    QProgressDialog dialog(i18n("Exporting to CSV..."), i18n("Cancel"), 0, static_cast<int>(2 * taskview->count()), taskview, 0);
    dialog.setAutoClose(true);
    dialog.setWindowTitle(i18nc("@title:window", "Export Progress"));

    if (taskview->count() > 1) {
        dialog.show();
    }

    QString retval;

    // Find max task depth
    int tasknr = 0;
    while (tasknr < taskview->count() && !dialog.wasCanceled()) {
        dialog.setValue(dialog.value() + 1);
        if (tasknr % 15 == 0) {
            QApplication::processEvents(); // repainting is slow
        }
        if (taskview->itemAt(tasknr)->depth() > maxdepth) {
            maxdepth = taskview->itemAt(tasknr)->depth();
        }
        tasknr++;
    }

    // Export to file
    tasknr = 0;
    while (tasknr < taskview->count() && !dialog.wasCanceled())
    {
        task = taskview->itemAt( tasknr );
        dialog.setValue(dialog.value() + 1);
        if (tasknr % 15 == 0) {
            QApplication::processEvents();
        }

        // indent the task in the csv-file:
        for (int i = 0; i < task->depth(); ++i) {
            retval += delim;
        }

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

        if (to_quote) {
            retval += dquote;
        }

        // Double quotes replaced by a pair of consecutive double quotes
        retval += task->name().replace(dquote, double_dquote);

        if (to_quote) {
            retval += dquote;
        }

        // maybe other tasks are more indented, so to align the columns:
        for (int i = 0; i < maxdepth - task->depth(); ++i) {
            retval += delim;
        }

        retval += delim + formatTime(task->sessionTime(), rc.decimalMinutes)
                + delim + formatTime(task->time(), rc.decimalMinutes)
                + delim + formatTime(task->totalSessionTime(), rc.decimalMinutes)
                + delim + formatTime(task->totalTime(), rc.decimalMinutes)
                + '\n';
        tasknr++;
    }

    // save, either locally or remote
    if (rc.url.isLocalFile() || !rc.url.url().contains("/")) {
        QString filename = rc.url.toLocalFile();
        if (filename.isEmpty()) {
            filename = rc.url.url();
        }
        QFile f(filename);
        if (!f.open(QIODevice::WriteOnly)) {
            err = i18n("Could not open \"%1\".", filename);
        }
        if (err.length() == 0) {
            QTextStream stream(&f);
            // Export to file
            stream << retval;
            f.close();
        }
    } else {
        // use remote file
        auto* const job = KIO::storedPut(retval.toUtf8(), rc.url, -1);
        KJobWidgets::setWindow(job, &dialog);
        if (!job->exec()) {
            err=QString::fromLatin1("Could not upload");
        }
    }

    return err;
}

int todaySeconds(const QDate &date, const KCalCore::Event::Ptr &event)
{
        if ( !event )
          return 0;

        qCDebug(KTT_LOG) << "found an event for task, event=" << event->uid();
        QDateTime startTime=event->dtStart();
        QDateTime endTime=event->dtEnd();
        QDateTime NextMidNight=startTime;
        NextMidNight.setTime(QTime ( 0,0 ));
        NextMidNight=NextMidNight.addDays(1);
        // LastMidNight := mdate.setTime(0:00) as it would read in a decent programming language
        QDateTime LastMidNight=QDateTime::currentDateTime();
        LastMidNight.setDate(date);
        LastMidNight.setTime(QTime(0,0));
        int secsstartTillMidNight=startTime.secsTo(NextMidNight);
        int secondsToAdd=0; // seconds that need to be added to the actual cell
        if ( (startTime.date()==date) && (event->dtEnd().date()==date) ) // all the event occurred today
            secondsToAdd=startTime.secsTo(endTime);
        if ( (startTime.date()==date) && (endTime.date()>date) ) // the event started today, but ended later
            secondsToAdd=secsstartTillMidNight;
        if ( (startTime.date()<date) && (endTime.date()==date) ) // the event started before today and ended today
            secondsToAdd=LastMidNight.secsTo(event->dtEnd());
        if ( (startTime.date()<date) && (endTime.date()>date) ) // the event started before today and ended after
            secondsToAdd=86400;

        return secondsToAdd;
}

// export history report as csv, all tasks X all dates in one block
QString TimeTrackerStorage::exportcsvHistory(
    TaskView* taskview, const QDate& from, const QDate& to, const ReportCriteria& rc)
{
    qCDebug(KTT_LOG) << "Entering function";

    QString delim = rc.delimiter;
    const QString cr = QStringLiteral("\n");
    QString err = QString::null;
    QString retval;
    Task* task;
    const int intervalLength = from.daysTo(to) + 1;
    QMap<QString, QVector<int>> secsForUid;
    QMap<QString, QString > uidForName;

    // Step 1: Prepare two hashmaps:
    // * "uid -> seconds each day": used while traversing events, as uid is their id
    //                              "seconds each day" are stored in a vector
    // * "name -> uid", ordered by name: used when creating the csv file at the end
    qCDebug(KTT_LOG) << "Taskview Count: " << taskview->count();
    for ( int n=0; n<taskview->count(); n++ )
    {
        task=taskview->itemAt(n);
        qCDebug(KTT_LOG) << "n: " << n << ", Task Name: " << task->name() << ", UID: " << task->uid();
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
        parentTask = parentTask->parentTask();
        while (parentTask)
        {
            fullName = parentTask->name() + "->" + fullName;
            qCDebug(KTT_LOG) << "Fullname(inside): " << fullName;
            parentTask = parentTask->parentTask();
            qCDebug(KTT_LOG) << "Parent task: " << parentTask;
        }

        uidForName[fullName] = task->uid();

        qCDebug(KTT_LOG) << "Fullname(end): " << fullName;
    }

    qCDebug(KTT_LOG) << "secsForUid" << secsForUid;
    qCDebug(KTT_LOG) << "uidForName" << uidForName;


    // Step 2: For each date, get the events and calculate the seconds
    // Store the seconds using secsForUid hashmap, so we don't need to translate uids
    // We rely on rawEventsForDate to get the events
    qCDebug(KTT_LOG) << "Let's iterate for each date: ";
    for ( QDate mdate=from; mdate.daysTo(to)>=0; mdate=mdate.addDays(1) )
    {
        qCDebug(KTT_LOG) << mdate.toString();
        KCalCore::Event::List dateEvents = d->mCalendar->rawEventsForDate(mdate);

        for(KCalCore::Event::List::iterator i = dateEvents.begin();i != dateEvents.end(); ++i)
        {
            qCDebug(KTT_LOG) << "Summary: " << (*i)->summary() << ", Related to uid: " << (*i)->relatedTo();
            qCDebug(KTT_LOG) << "Today's seconds: " << todaySeconds(mdate, *i);
            secsForUid[(*i)->relatedTo()][from.daysTo(mdate)] += todaySeconds(mdate, *i);
        }
    }


    // Step 3: For each task, generate the matching row for the CSV file
    // We use the two hashmaps to have direct access using the task name

    // First CSV file line
    // FIXME: localize strings and date formats
    retval.append("\"Task name\"");
    for (int i=0; i<intervalLength; ++i)
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
        qCDebug(KTT_LOG) << nameUid.key() << ": " << nameUid.value() << endl;

        for (int day=0; day<intervalLength; day++)
        {
            qCDebug(KTT_LOG) << "Secs for day " << day << ":" << secsForUid[nameUid.value()][day];
            retval.append(delim);
            time = secsForUid[nameUid.value()][day]/60.0;
            retval.append(formatTime(time, rc.decimalMinutes));
        }

        retval.append(cr);
    }

    qDebug() << "Retval is \n" << retval;

    if (rc.bExPortToClipBoard)
        taskview->setClipBoardText(retval);
    else
    {
        // store the file locally or remote
        if ((rc.url.isLocalFile()) || (!rc.url.url().contains("/")))
        {
            qCDebug(KTT_LOG) << "storing a local file";
            QString filename=rc.url.toLocalFile();
            if (filename.isEmpty()) filename=rc.url.url();
            QFile f( filename );
            if( !f.open( QIODevice::WriteOnly ) )
            {
                err = i18n( "Could not open \"%1\".", filename );
                qCDebug(KTT_LOG) << "Could not open file";
            }
            qDebug() << "Err is " << err;
            if (err.length()==0)
            {
                QTextStream stream(&f);
                qCDebug(KTT_LOG) << "Writing to file: " << retval;
                // Export to file
                stream << retval;
                f.close();
            }
        }
        else // use remote file
        {
            auto* const job = KIO::storedPut(retval.toUtf8(), rc.url, -1);
            //KJobWidgets::setWindow(job, &dialog); // TODO: add progress dialog
            if (!job->exec()) {
                err=QString::fromLatin1("Could not upload");
            }
        }
    }
    return err;
}

void TimeTrackerStorage::startTimer(const Task* task, const QDateTime &when)
{
    qCDebug(KTT_LOG) << "Entering function; when=" << when;
    KCalCore::Event::Ptr e;
    e = baseEvent(task);
    e->setDtStart(when);
    d->mCalendar->addEvent(e);
    task->taskView()->scheduleSave();
}

void TimeTrackerStorage::startTimer(QString taskID)
{
    qCDebug(KTT_LOG) << "Entering function";
    KCalCore::Todo::List todoList;
    KCalCore::Todo::List::ConstIterator todo;
    todoList = d->mCalendar->rawTodos();
    for( todo = todoList.constBegin(); todo != todoList.constEnd(); ++todo )
    {
        qCDebug(KTT_LOG) << (*todo)->uid();
        qCDebug(KTT_LOG) << taskID;
        if ( (*todo)->uid() == taskID )
        {
            qCDebug(KTT_LOG) << "adding event";
            KCalCore::Event::Ptr e;
            e = baseEvent((*todo));
            e->setDtStart(QDateTime::currentDateTime());
            d->mCalendar->addEvent(e);
        }
    }
    saveCalendar();
}

void TimeTrackerStorage::stopTimer(const Task* task, const QDateTime &when)
{
    qCDebug(KTT_LOG) << "Entering function; when=" << when;
    KCalCore::Event::List eventList = d->mCalendar->rawEvents();
    for(KCalCore::Event::List::iterator i = eventList.begin();
        i != eventList.end();
        ++i)
    {
        if ( (*i)->relatedTo() == task->uid() )
        {
            qCDebug(KTT_LOG) << "found an event for task, event=" << (*i)->uid();
            if (!(*i)->hasEndDate())
            {
                qCDebug(KTT_LOG) << "this event has no enddate";
//                (*i)->setDtEnd(KDateTime(when, KDateTime::Spec::LocalZone()));
                (*i)->setDtEnd(when);
            }
            else
            {
                qCDebug(KTT_LOG) << "this event has an enddate";
                qCDebug(KTT_LOG) << "end date is " << (*i)->dtEnd();
            }
        };
    }
    saveCalendar();
}

void TimeTrackerStorage::changeTime(const Task* task, long deltaSeconds)
{
    qCDebug(KTT_LOG) << "Entering function; deltaSeconds=" << deltaSeconds;
    KCalCore::Event::Ptr e;
    QDateTime end;
    e = baseEvent(task);

    // Don't use duration, as ICalFormatImpl::writeIncidence never writes a
    // duration, even though it looks like it's used in event.cpp.
    end = task->startTime();
    if ( deltaSeconds > 0 ) end = task->startTime().addSecs(deltaSeconds);
    e->setDtEnd(end);

    // Use a custom property to keep a record of negative durations
    e->setCustomProperty(eventAppName, QByteArray("duration"), QString::number(deltaSeconds));

    d->mCalendar->addEvent(e);
    task->taskView()->scheduleSave();
}

bool TimeTrackerStorage::bookTime(const Task* task, const QDateTime& startDateTime, long durationInSeconds)
{
    qCDebug(KTT_LOG) << "Entering function";

    auto e = baseEvent(task);
    e->setDtStart(startDateTime);
    e->setDtEnd(startDateTime.addSecs( durationInSeconds));

    // Use a custom property to keep a record of negative durations
    e->setCustomProperty(eventAppName, QByteArray("duration"), QString::number(durationInSeconds));
    return d->mCalendar->addEvent(e);
}

KCalCore::Event::Ptr TimeTrackerStorage::baseEvent(const Task *task)
{
    qCDebug(KTT_LOG) << "Entering function";
    KCalCore::Event::Ptr e( new KCalCore::Event() );
    QStringList categories;
    e->setSummary(task->name());

    // Can't use setRelatedToUid()--no error, but no RelatedTo written to disk
    e->setRelatedTo( task->uid() );

    // Debugging: some events where not getting a related-to field written.
    Q_ASSERT(e->relatedTo() == task->uid());

    // Have to turn this off to get datetimes in date fields.
    e->setAllDay(false);
//    e->setDtStart(KDateTime(task->startTime(), KDateTime::Spec::LocalZone()));
    e->setDtStart(task->startTime());

    // So someone can filter this mess out of their calendar display
    categories.append(i18n("KTimeTracker"));
    e->setCategories(categories);

    return e;
}

KCalCore::Event::Ptr TimeTrackerStorage::baseEvent(const KCalCore::Todo::Ptr &todo)
{
    qCDebug(KTT_LOG) << "Entering function";
    KCalCore::Event::Ptr e( new KCalCore::Event() );
    QStringList categories;
    e->setSummary(todo->summary());

    // Can't use setRelatedToUid()--no error, but no RelatedTo written to disk
    e->setRelatedTo( todo->uid() );

    // Have to turn this off to get datetimes in date fields.
    e->setAllDay(false);
//    e->setDtStart(KDateTime(todo->dtStart()));
    e->setDtStart(todo->dtStart());

    // So someone can filter this mess out of their calendar display
    categories.append(i18n("KTimeTracker"));
    e->setCategories(categories);

    return e;
}

bool TimeTrackerStorage::isRemoteFile(const QString& file) const
{
    qCDebug(KTT_LOG) << "Entering function";
    QString f = file.toLower();
    bool rval = f.startsWith(QStringLiteral("http://")) || f.startsWith(QStringLiteral("ftp://"));
    qCDebug(KTT_LOG) << "TimeTrackerStorage::isRemoteFile(" << file << " ) returns" << rval;
    return rval;
}

QString TimeTrackerStorage::saveCalendar()
{
    qCDebug(KTT_LOG) << "Entering function";
    bool removedFromDirWatch = false;
    if (KDirWatch::self()->contains(d->mICalFile)) {
        KDirWatch::self()->removeFile(d->mICalFile);
        removedFromDirWatch = true;
    }

    QString errorMessage;
    if (d->mCalendar) {
        d->m_fileLock->lock();
    } else {
        qDebug() << "mCalendar not set";
        return errorMessage;
    }

    if (!d->mCalendar->save()) {
        errorMessage = QString("Could not save. Could lock file.");
    }
    d->m_fileLock->unlock();

    if (removedFromDirWatch) {
        KDirWatch::self()->addFile(d->mICalFile);
    }

    return errorMessage;
}

KTTCalendar::Ptr TimeTrackerStorage::calendar() const
{
    return _calendar;
}
