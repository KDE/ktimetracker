/*
    SPDX-FileCopyrightText: 2003, 2004 Mark Bucciarelli <mark@hubcapconsutling.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
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

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QMultiHash>
#include <QStandardPaths>

#include <KDirWatch>
#include <KLocalizedString>

#include "ktt_debug.h"
#include "model/eventsmodel.h"
#include "model/projectmodel.h"
#include "model/task.h"
#include "model/tasksmodel.h"
#include "taskview.h"
#include "widgets/taskswidget.h"

const QByteArray eventAppName = QByteArray("ktimetracker");

TimeTrackerStorage::TimeTrackerStorage()
    : m_model(nullptr)
    , m_taskView(nullptr)
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
    if (KDirWatch::self()->contains(m_url.toLocalFile())) {
        KDirWatch::self()->removeFile(m_url.toLocalFile());
        removedFromDirWatch = true;
    }

    // If same file, don't reload
    if (url == m_url) {
        if (removedFromDirWatch) {
            KDirWatch::self()->addFile(m_url.toLocalFile());
        }
        return QString();
    }

    if (m_model) {
        closeStorage();
    }

    m_model = new ProjectModel();

    if (url.isLocalFile()) {
        connect(KDirWatch::self(), &KDirWatch::created, this, &TimeTrackerStorage::onFileModified);
        connect(KDirWatch::self(), &KDirWatch::dirty, this, &TimeTrackerStorage::onFileModified);
        if (!KDirWatch::self()->contains(url.toLocalFile())) {
            KDirWatch::self()->addFile(url.toLocalFile());
        }
    }

    // Create local file resource and add to resources
    m_url = url;
    FileCalendar m_calendar(m_url);

    m_taskView = view;
    m_calendar.reload();

    // Build task view from iCal data
    QString err;
    eventsModel()->load(m_calendar.rawEvents());
    err = loadTasksFromCalendar(m_calendar.rawTodos());

    if (removedFromDirWatch) {
        KDirWatch::self()->addFile(m_url.toLocalFile());
    }
    return err;
}

QUrl TimeTrackerStorage::fileUrl()
{
    return m_url;
}

QString TimeTrackerStorage::loadTasksFromCalendar(const KCalendarCore::Todo::List &todos)
{
    tasksModel()->clear();

    QMultiHash<QString, Task *> map;
    for (const auto &todo : todos) {
        Task *task = new Task(todo, m_model);
        map.insert(todo->uid(), task);
        task->invalidateCompletedState();
    }

    // 1.1. Load each task under its parent task.
    QString err{};
    for (const auto &todo : todos) {
        Task *task = map.value(todo->uid());
        // No relatedTo incident just means this is a top-level task.
        if (!todo->relatedTo().isEmpty()) {
            Task *newParent = map.value(todo->relatedTo());
            // Complete the loading but return a message
            if (!newParent) {
                err = i18n("Error loading \"%1\": could not find parent (uid=%2)", task->name(), todo->relatedTo());
            } else {
                task->move(newParent);
            }
        }
    }

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

bool TimeTrackerStorage::allEventsHaveEndTime(Task *task)
{
    for (const auto *event : m_model->eventsModel()->eventsForTask(task)) {
        if (!event->hasEndDate()) {
            return false;
        }
    }

    return true;
}

// static
QString TimeTrackerStorage::createLockFileName(const QUrl &url)
{
    QString canonicalSeedString;
    QString baseName;
    if (url.isLocalFile()) {
        QFileInfo fileInfo(url.toLocalFile());
        canonicalSeedString = fileInfo.absoluteFilePath();
        baseName = fileInfo.fileName();
    } else {
        canonicalSeedString = url.url();
        baseName = QFileInfo(url.path()).completeBaseName();
    }

    // Report failure if canonicalSeedString is empty.
    if (canonicalSeedString.isEmpty()) {
        return QString();
    }

    // Remove characters disallowed by operating systems
    baseName.replace(QRegularExpression(QStringLiteral("[") + QRegularExpression::escape(QStringLiteral("\\/:*?\"<>|")) + QStringLiteral("]")), QString());

    const QString &hash = QString::fromLatin1(QCryptographicHash::hash(canonicalSeedString.toUtf8(), QCryptographicHash::Sha1).toHex());
    const QString &lockBaseName = QStringLiteral("ktimetracker_%1_%2.lock").arg(hash).arg(baseName);

    // Put the lock file in a directory for temporary files
    return QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath(lockBaseName);
}

QString TimeTrackerStorage::save()
{
    bool removedFromDirWatch = false;
    if (KDirWatch::self()->contains(m_url.toLocalFile())) {
        KDirWatch::self()->removeFile(m_url.toLocalFile());
        removedFromDirWatch = true;
    }

    if (!m_model) {
        qCWarning(KTT_LOG) << "TimeTrackerStorage::save: m_model is nullptr";
        // No i18n() here because it's too technical and unlikely to happen
        return QStringLiteral("m_model is nullptr");
    }

    const QString &fileLockPath = createLockFileName(m_url);
    QLockFile fileLock(fileLockPath);
    if (!fileLock.lock()) {
        qCWarning(KTT_LOG) << "TimeTrackerStorage::save: m_fileLock->lock() failed";
        return i18nc("%1=lock file path", "Could not write lock file \"%1\". Disk full?", fileLockPath);
    }

    QString errorMessage{};
    std::unique_ptr<FileCalendar> calendar = m_model->asCalendar(m_url);
    if (!calendar->save()) {
        qCWarning(KTT_LOG) << "TimeTrackerStorage::save: calendar->save() failed";
        errorMessage = i18nc("%1=destination file path/URL", "Failed to save iCalendar file as \"%1\".", m_url.toString());
    } else {
        qCDebug(KTT_LOG) << "TimeTrackerStorage::save: wrote tasks to" << m_url;
    }
    fileLock.unlock();

    if (removedFromDirWatch) {
        KDirWatch::self()->addFile(m_url.toLocalFile());
    }

    return errorMessage;
}

//----------------------------------------------------------------------------

bool TimeTrackerStorage::bookTime(const Task *task, const QDateTime &startDateTime, int64_t durationInSeconds)
{
    return eventsModel()->bookTime(task, startDateTime, durationInSeconds);
}

void TimeTrackerStorage::onFileModified()
{
    if (!m_model) {
        qCWarning(KTT_LOG) << "TaskView::onFileModified(): model is null";
        return;
    }

    // TODO resolve conflicts if KTimeTracker has unsaved changes in its data structures

    qCDebug(KTT_LOG) << "entering function";

    FileCalendar m_calendar(m_url);
    m_calendar.reload();

    // Remember tasks that are running and their start times.
    // Maps task UID to task's startTime.
    QHash<QString, QDateTime> startTimeForUid{};
    for (Task *task : tasksModel()->getActiveTasks()) {
        startTimeForUid[task->uid()] = task->startTime();
    }

    loadTasksFromCalendar(m_calendar.rawTodos());

    // Restart tasks that have been running, with their start times.
    for (Task *task : tasksModel()->getAllTasks()) {
        if (startTimeForUid.contains(task->uid())) {
            m_taskView->startTimerFor(task, startTimeForUid[task->uid()]);
        }
    }

    projectModel()->refresh();
    m_taskView->tasksWidget()->refresh();

    qCDebug(KTT_LOG) << "exiting onFileModified";
}

#include "moc_timetrackerstorage.cpp"
