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
#include <QClipboard>

#include <KDirWatch>
#include <KJobUiDelegate>
#include <KMessageBox>
#include <KLocalizedString>
#include <KJobWidgets>
#include <KIO/StoredTransferJob>

#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "model/task.h"
#include "model/eventsmodel.h"
#include "model/projectmodel.h"
#include "taskview.h"
#include "export/totalsastext.h"
#include "widgets/taskswidget.h"
#include "ktt_debug.h"

const QByteArray eventAppName = QByteArray("ktimetracker");

TimeTrackerStorage::TimeTrackerStorage()
    : m_model(nullptr)
    , m_url()
    , m_fileLock(new QLockFile(QStringLiteral("ktimetrackerics.lock")))
    , m_taskView(nullptr)
{
}

TimeTrackerStorage::~TimeTrackerStorage()
{
    delete m_fileLock;
}

// Loads data from filename into view.
QString TimeTrackerStorage::load(TaskView *view, const QUrl &url)
{
    if (url.isEmpty()) {
        return QStringLiteral("TimeTrackerStorage::load() callled with an empty URL");
    }

    // loading might create the file
    bool removedFromDirWatch = false;
    if (KDirWatch::self()->contains(m_url.path())) {
        KDirWatch::self()->removeFile(m_url.path());
        removedFromDirWatch = true;
    }

    // If same file, don't reload
    if (url == m_url) {
        if (removedFromDirWatch) {
            KDirWatch::self()->addFile(m_url.path());
        }
        return QString();
    }

    const bool fileIsLocal = url.isLocalFile();

    // If file doesn't exist, create a blank one to avoid ResourceLocal load
    // error.  We make it user and group read/write, others read.  This is
    // masked by the users umask.  (See man creat)
    if (fileIsLocal) {
        int handle = open(
            QFile::encodeName(url.path()),
            O_CREAT | O_EXCL | O_WRONLY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (handle != -1) {
            close(handle);
        }
    }

    if (m_model) {
        closeStorage();
    }

    m_model = new ProjectModel();

    if (fileIsLocal) {
        connect(KDirWatch::self(), &KDirWatch::dirty, this, &TimeTrackerStorage::onFileModified);
        if (!KDirWatch::self()->contains(url.path())) {
            KDirWatch::self()->addFile(url.path());
        }
    }

    // Create local file resource and add to resources
    m_url = url;
    FileCalendar m_calendar(m_url);

    m_taskView = view;
//    m_calendar->setTimeSpec( KSystemTimeZones::local() );
    m_calendar.reload();

    // Build task view from iCal data
    QString err;
    eventsModel()->load(m_calendar.rawEvents());
    err = buildTaskView(m_calendar.rawTodos(), view);

    if (removedFromDirWatch) {
        KDirWatch::self()->addFile(m_url.path());
    }
    return err;
}

QUrl TimeTrackerStorage::fileUrl()
{
    return m_url;
}

QString TimeTrackerStorage::buildTaskView(const KCalCore::Todo::List& todos, TaskView* view)
// makes *view contain the tasks out of *rc.
{
    // remember tasks that are running and their start times
    QVector<QString> runningTasks;
    QVector<QDateTime> startTimes;
    for (Task *task : view->getAllTasks()) {
        if (task->isRunning()) {
            runningTasks.append(task->uid());
            startTimes.append(task->startTime());
        }
    }

    tasksModel()->clear();

    QMultiHash<QString, Task*> map;
    for (const auto &todo : todos) {
        Task* task = new Task(todo, view, m_model);
        map.insert(todo->uid(), task);
        task->invalidateCompletedState();
    }

    // 1.1. Load each task under its parent task.
    QString err;
    for (const auto &todo : todos) {
        Task* task = map.value(todo->uid());
        // No relatedTo incident just means this is a top-level task.
        if (!todo->relatedTo().isEmpty()) {
            Task* newParent = map.value(todo->relatedTo());
            // Complete the loading but return a message
            if (!newParent) {
                err = i18n("Error loading \"%1\": could not find parent (uid=%2)",
                           task->name(),
                           todo->relatedTo());
            } else {
                task->move(newParent);
            }
        }
    }

    view->clearActiveTasks();
    // restart tasks that have been running with their start times
    for (Task *task : view->getAllTasks()) {
        for (int n = 0; n < runningTasks.count(); ++n) {
            if (runningTasks[n] == task->uid()) {
                view->startTimerFor(task, startTimes[n]);
            }
        }
    }

    view->refresh();
    return err;
}

void TimeTrackerStorage::closeStorage()
{
    if (m_model) {
        delete m_model;
        m_model = nullptr;
    }
}

EventsModel *TimeTrackerStorage::eventsModel()
{
    if (!m_model) {
        qFatal("TimeTrackerStorage::eventsModel is nullptr");
    }

    return m_model->eventsModel();
}

TasksModel *TimeTrackerStorage::tasksModel()
{
    if (!m_model) {
        qFatal("TimeTrackerStorage::tasksModel is nullptr");
    }

    return m_model->tasksModel();
}

ProjectModel *TimeTrackerStorage::projectModel()
{
    if (!m_model) {
        qFatal("TimeTrackerStorage::projectModel is nullptr");
    }

    return m_model;
}

bool TimeTrackerStorage::allEventsHaveEndTiMe(Task *task)
{
    for (const auto *event : m_model->eventsModel()->eventsForTask(task)) {
        if (!event->hasEndDate()) {
            return false;
        }
    }

    return true;
}

bool TimeTrackerStorage::allEventsHaveEndTiMe()
{
    for (const auto *event : m_model->eventsModel()->events()) {
        if (!event->hasEndDate()) {
            return false;
        }
    }

    return true;
}

QString TimeTrackerStorage::deleteAllEvents()
{
    m_model->eventsModel()->clear();
    return QString();
}

QString TimeTrackerStorage::save()
{
    qCDebug(KTT_LOG) << "Entering function";
    QString errorString;

    errorString = saveCalendar();

    if (errorString.isEmpty()) {
        qCDebug(KTT_LOG) << "TimeTrackerStorage::save : wrote tasks to" << m_url;
    } else {
        qCWarning(KTT_LOG) << "TimeTrackerStorage::save :" << errorString;
    }

    return errorString;
}

QString TimeTrackerStorage::writeTaskAsTodo(Task *task, KCalCore::Todo::Ptr parent)
{
    QString err;
    KCalCore::Todo::Ptr todo(new KCalCore::Todo());
    task->asTodo(todo);
    if (parent) {
        todo->setRelatedTo(parent->uid());
    }

    for (int i = 0; i < task->childCount(); ++i) {
        Task *nextTask = dynamic_cast<Task*>(task->child(i));
        err = writeTaskAsTodo(nextTask, todo);
    }

    return err;
}

//----------------------------------------------------------------------------

int todaySeconds(const QDate &date, const KCalCore::Event::Ptr &event)
{
    if (!event) {
        return 0;
    }

    qCDebug(KTT_LOG) << "found an event for task, event=" << event->uid();
    QDateTime startTime = event->dtStart();
    QDateTime endTime = event->dtEnd();
    QDateTime NextMidNight = startTime;
    NextMidNight.setTime(QTime(0, 0));
    NextMidNight = NextMidNight.addDays(1);
    // LastMidNight := mdate.setTime(0:00) as it would read in a decent programming language
    QDateTime LastMidNight = QDateTime::currentDateTime();
    LastMidNight.setDate(date);
    LastMidNight.setTime(QTime(0, 0));
    int secsstartTillMidNight = startTime.secsTo(NextMidNight);
    int secondsToAdd = 0; // seconds that need to be added to the actual cell

    if (startTime.date() == date && event->dtEnd().date() == date) {
        // all the event occurred today
        secondsToAdd = startTime.secsTo(endTime);
    }

    if (startTime.date() == date && endTime.date()>date) {
        // the event started today, but ended later
        secondsToAdd = secsstartTillMidNight;
    }

    if (startTime.date() < date && endTime.date() == date) {
        // the event started before today and ended today
        secondsToAdd = LastMidNight.secsTo(event->dtEnd());
    }

    if (startTime.date() < date && endTime.date() > date) {
        // the event started before today and ended after
        secondsToAdd = 86400;
    }

    return secondsToAdd;
}

// export history report as csv, all tasks X all dates in one block
QString TimeTrackerStorage::exportCSVHistory(
    TaskView *taskview, const QDate &from, const QDate &to, const ReportCriteria &rc)
{
    qCDebug(KTT_LOG) << "Entering function";

    QString delim = rc.delimiter;
    const QString cr = QStringLiteral("\n");
    QString err = QString::null;
    QString retval;
    const int intervalLength = from.daysTo(to) + 1;
    QMap<QString, QVector<int>> secsForUid;
    QMap<QString, QString > uidForName;

    // Step 1: Prepare two hashmaps:
    // * "uid -> seconds each day": used while traversing events, as uid is their id
    //                              "seconds each day" are stored in a vector
    // * "name -> uid", ordered by name: used when creating the csv file at the end
    qCDebug(KTT_LOG) << "Taskview Count: " << taskview->count();
    for (Task *task : taskview->getAllTasks()) {
        qCDebug(KTT_LOG) << ", Task Name: " << task->name() << ", UID: " << task->uid();
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
        while (parentTask) {
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

    std::unique_ptr<FileCalendar> calendar = m_model->asCalendar(QUrl());

    // Step 2: For each date, get the events and calculate the seconds
    // Store the seconds using secsForUid hashmap, so we don't need to translate uids
    // We rely on rawEventsForDate to get the events
    qCDebug(KTT_LOG) << "Let's iterate for each date: ";
    int dayCount = 0;
    for ( QDate mdate=from; mdate.daysTo(to)>=0; mdate=mdate.addDays(1) )
    {
        if (dayCount++ > 365 * 100) {
            return QStringLiteral("too many days to process");
        }

        qCDebug(KTT_LOG) << mdate.toString();
        KCalCore::Event::List dateEvents = calendar->rawEventsForDate(mdate);

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

    if (rc.bExPortToClipBoard) {
        QApplication::clipboard()->setText(retval);
    } else {
        // store the file locally or remote
        if (rc.url.isLocalFile()) {
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

void TimeTrackerStorage::changeTime(const Task* task, long deltaSeconds)
{
    qCDebug(KTT_LOG) << "Entering function; deltaSeconds=" << deltaSeconds;
    QDateTime end;
    auto kcalEvent = baseEvent(task);
    Event *e = new Event(kcalEvent, eventsModel());

    // Don't use duration, as ICalFormatImpl::writeIncidence never writes a
    // duration, even though it looks like it's used in event.cpp.
    end = task->startTime();
    if ( deltaSeconds > 0 ) end = task->startTime().addSecs(deltaSeconds);
    e->setDtEnd(end);

    // Use a custom property to keep a record of negative durations
    e->setDuration(deltaSeconds);

    eventsModel()->addEvent(e);
    task->taskView()->scheduleSave();
}

bool TimeTrackerStorage::bookTime(const Task* task, const QDateTime& startDateTime, long durationInSeconds)
{
    qCDebug(KTT_LOG) << "Entering function";

    auto kcalEvent = baseEvent(task);
    Event *e = new Event(kcalEvent, eventsModel());

    e->setDtStart(startDateTime);
    e->setDtEnd(startDateTime.addSecs( durationInSeconds));

    // Use a custom property to keep a record of negative durations
    e->setDuration(durationInSeconds);

    eventsModel()->addEvent(e);
    return true;
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

QString TimeTrackerStorage::saveCalendar()
{
    qCDebug(KTT_LOG) << "Entering function";
    bool removedFromDirWatch = false;
    if (KDirWatch::self()->contains(m_url.path())) {
        KDirWatch::self()->removeFile(m_url.path());
        removedFromDirWatch = true;
    }

    if (!m_model) {
        qDebug() << "m_model not set";
        return QString("m_model not set");
    }

    if (!m_fileLock->lock()) {
        return QString("Could not save. Could not lock file.");
    }

    QString errorMessage;
    std::unique_ptr<FileCalendar> calendar = m_model->asCalendar(m_url);
    if (!calendar->save()) {
        errorMessage = QString("Could not save. Could lock file.");
    }
    m_fileLock->unlock();

    if (removedFromDirWatch) {
        KDirWatch::self()->addFile(m_url.path());
    }

    return errorMessage;
}

void TimeTrackerStorage::onFileModified()
{
    if (!m_model) {
        qCWarning(KTT_LOG) << "TaskView::onFileModified(): model is null";
        return;
    }

    qCDebug(KTT_LOG) << "entering function";

    FileCalendar m_calendar(m_url);
//    m_calendar->setTimeSpec( KSystemTimeZones::local() );
    m_calendar.reload();
    buildTaskView(m_calendar.rawTodos(), m_taskView);

    qCDebug(KTT_LOG) << "exiting onFileModified";
}
