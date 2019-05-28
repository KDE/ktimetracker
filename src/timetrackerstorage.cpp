/*
 * Copyright (C) 2003, 2004 by Mark Bucciarelli <mark@hubcapconsutling.com>
 * Copyright (C) 2019  Alexander Potashev <aspotashev@gmail.com>
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
#include <QUrl>
#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QMultiHash>
#include <QStringList>
#include <QTableWidget>
#include <QTextStream>
#include <QMap>
#include <QClipboard>

#include <KDirWatch>
#include <KLocalizedString>

#include "export/export.h"
#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "model/task.h"
#include "model/eventsmodel.h"
#include "model/projectmodel.h"
#include "model/tasksmodel.h"
#include "export/csvhistory.h"
#include "taskview.h"
#include "widgets/taskswidget.h"
#include "ktt_debug.h"

const QByteArray eventAppName = QByteArray("ktimetracker");

TimeTrackerStorage::TimeTrackerStorage()
    : m_model(nullptr)
    , m_url()
    , m_taskView(nullptr)
{
}

TimeTrackerStorage::~TimeTrackerStorage()
{
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

    if (m_model) {
        closeStorage();
    }

    m_model = new ProjectModel();

    if (url.isLocalFile()) {
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
    for (Task *task : tasksModel()->getAllTasks()) {
        if (task->isRunning()) {
            runningTasks.append(task->uid());
            startTimes.append(task->startTime());
        }
    }

    tasksModel()->clear();

    QMultiHash<QString, Task*> map;
    for (const auto &todo : todos) {
        Task* task = new Task(todo, m_model);
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
    for (Task *task : tasksModel()->getAllTasks()) {
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
    bool removedFromDirWatch = false;
    if (KDirWatch::self()->contains(m_url.path())) {
        KDirWatch::self()->removeFile(m_url.path());
        removedFromDirWatch = true;
    }

    if (!m_model) {
        qCWarning(KTT_LOG) << "TimeTrackerStorage::save: m_model is nullptr";
        return QString("m_model not set");
    }

    QLockFile fileLock(QStringLiteral("ktimetrackerics.lock"));
    if (!fileLock.lock()) {
        qCWarning(KTT_LOG) << "TimeTrackerStorage::save: m_fileLock->lock() failed";
        return QString("Could not save. Could not lock file.");
    }

    QString errorMessage;
    std::unique_ptr<FileCalendar> calendar = m_model->asCalendar(m_url);
    if (!calendar->save()) {
        qCWarning(KTT_LOG) << "TimeTrackerStorage::save: calendar->save() failed";
        errorMessage = QString("Could not save. Could lock file.");
    }
    fileLock.unlock();

    if (removedFromDirWatch) {
        KDirWatch::self()->addFile(m_url.path());
    }

    qCDebug(KTT_LOG) << "TimeTrackerStorage::save: wrote tasks to" << m_url;
    return errorMessage;
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

bool TimeTrackerStorage::bookTime(const Task* task, const QDateTime& startDateTime, long durationInSeconds)
{
    return eventsModel()->bookTime(task, startDateTime, durationInSeconds);
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
