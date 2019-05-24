/*
 *     Copyright (C) 1997 by Stephan Kulow <coolo@kde.org>
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

#include "task.h"

#include <QDateTime>

#include <KCalCore/CalFormat>

#include "model/projectmodel.h"
#include "model/eventsmodel.h"
#include "model/tasksmodel.h"
#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "timetrackerstorage.h"
#include "taskview.h"
#include "ktt_debug.h"

static const QByteArray eventAppName = QByteArray("ktimetracker");

Task::Task( const QString& taskName, const QString& taskDescription, long minutes, long sessionTime,
            DesktopList desktops, TaskView* taskView, ProjectModel *projectModel, Task *parentTask)
    : TasksModelItem(projectModel->tasksModel(), parentTask)
    , m_taskView(taskView)
    , m_projectModel(projectModel)
{
    if (parentTask) {
        parentTask->addChild(this);
    } else {
        m_projectModel->tasksModel()->addChild(this);
    }

    init(taskName, taskDescription, minutes, sessionTime, nullptr, desktops, 0, 0);

    mUid = KCalCore::CalFormat::createUniqueId();
}

Task::Task(const KCalCore::Todo::Ptr &todo, TaskView* taskView, ProjectModel *projectModel)
    : TasksModelItem(projectModel->tasksModel(), nullptr)
    , m_taskView(taskView)
    , m_projectModel(projectModel)
{
    projectModel->tasksModel()->addChild(this);

    long minutes = 0;
    QString name;
    QString description;
    long sessionTime = 0;
    QString sessionStartTiMe;
    int percent_complete = 0;
    int priority = 0;
    DesktopList desktops;

    parseIncidence( todo, minutes, sessionTime, sessionStartTiMe, name, description, desktops, percent_complete,
                  priority );
    init( name, description, minutes, sessionTime, sessionStartTiMe, desktops, percent_complete, priority);
}

int Task::depth()
// Deliver the depth of a task, i.e. how many tasks are supertasks to it.
// A toplevel task has the depth 0.
{
    qCDebug(KTT_LOG) << "Entering function";
    int res = 0;
    Task* t = this;
    while (t = t->parentTask()) {
        res++;
    }
    qCDebug(KTT_LOG) << "Leaving function. depth is:" << res;
    return res;
}

void Task::init(
    const QString& taskName, const QString& taskDescription, long minutes, long sessionTime, QString sessionStartTiMe,
    const DesktopList& desktops, int percent_complete, int priority)
{
    m_isRunning = false;
    mName = taskName.trimmed();
    mDescription = taskDescription.trimmed();
    mLastStart = QDateTime::currentDateTime();
    mTotalTime = mTime = minutes;
    mTotalSessionTime = mSessionTime = sessionTime;
    mDesktops = desktops;

    mPercentComplete = percent_complete;
    mPriority = priority;
    mSessionStartTiMe = QDateTime::fromString(sessionStartTiMe);

    update();
    changeParentTotalTimes(mSessionTime, mTime);
}

void Task::delete_recursive()
{
    while (this->child(0)) {
        Task* t = (Task*) this->child(0);
        t->delete_recursive();
    }
    delete this;
}

// This is the back-end, the front-end is StartTimerFor()
void Task::setRunning(bool on, TimeTrackerStorage* storage, const QDateTime& when)
{
    if (on != m_isRunning) {
        m_isRunning = on;
        invalidateRunningState();

        if (on) {
            mLastStart = when;
            qCDebug(KTT_LOG) << "task has been started for " << when;
        }
    }
}

// setRunning is the back-end, the front-end is StartTimerFor().
// resumeRunning does the same as setRunning, but not add a new
// start date to the storage.
void Task::resumeRunning()
{
    qCDebug(KTT_LOG) << "Entering function";
    if (!isRunning()) {
        m_isRunning = true;
        invalidateRunningState();
    }
}

bool Task::isRunning() const
{
    return m_isRunning;
}

void Task::setName(const QString& name, TimeTrackerStorage* storage)
{
    qCDebug(KTT_LOG) << "Entering function, name=" << name;

    QString oldname = mName;
    if (oldname != name) {
        mName = name;
        storage->setName(this, oldname);
        update();
    }
}

void Task::setDescription(const QString& description)
{
    qCDebug(KTT_LOG) << "Entering function, description=" << description;

    QString olddescription = mDescription;
    if (olddescription != description) {
        mDescription = description;
        update();
    }
}

void Task::setPercentComplete(int percent)
{
    qCDebug(KTT_LOG) << "Entering function(" << percent << "):" << mUid;

    if (!percent) {
        mPercentComplete = 0;
    } else if (percent > 100) {
        mPercentComplete = 100;
    } else if (percent < 0) {
        mPercentComplete = 0;
    } else {
        mPercentComplete = percent;
    }

    if (isRunning() && mPercentComplete == 100) {
        emit m_projectModel->tasksModel()->taskCompleted(this);
    }

    invalidateCompletedState();

    // When parent marked as complete, mark all children as complete as well.
    // This behavior is consistent with KOrganizer (as of 2003-09-24).
    if (mPercentComplete == 100) {
        for (int i = 0; i < childCount(); ++i) {
            Task *task = dynamic_cast<Task*>(child(i));
            task->setPercentComplete(mPercentComplete);
        }
    }
    // maybe there is a column "percent completed", so do a ...
    update();
}

void Task::setPriority(int priority)
{
    if (priority < 0) {
        priority = 0;
    } else if (priority > 9) {
        priority = 9;
    }

    mPriority = priority;
    update();
}

bool Task::isComplete()
{
    return mPercentComplete == 100;
}

void Task::setDesktopList(const DesktopList& desktopList)
{
    mDesktops = desktopList;
}

QString Task::addTime(long minutes)
{
    mTime += minutes;
    this->addTotalTime(minutes);
    return QString();
}

QString Task::addTotalTime(long minutes)
{
    mTotalTime += minutes;
    if (parentTask()) {
        parentTask()->addTotalTime(minutes);
    }
    return QString();
}

QString Task::addSessionTime(long minutes)
{
    mSessionTime += minutes;
    this->addTotalSessionTime(minutes);
    return QString();
}

QString Task::addTotalSessionTime(long minutes)
{
    mTotalSessionTime += minutes;
    if (parentTask()) {
        parentTask()->addTotalSessionTime(minutes);
    }
    return QString();
}

QString Task::setTime(long minutes)
{
    mTime = minutes;
    mTotalTime += minutes;
    return QString();
}

QString Task::recalculatetotaltime()
{
    QString result;
    setTotalTime(0);
    // FIXME: this code is obviously broken
    Task* child;
    for (int i = 0; i < this->childCount(); ++i) {
        child = (Task*)this->child(i);
    }
    addTotalTime(time());
    return result;
}

QString Task::recalculatetotalsessiontime()
{
    QString result;
    setTotalSessionTime(0);
    // FIXME: this code is obviously broken
    Task* child;
    for (int i = 0; i < this->childCount(); ++i) {
        child = (Task*)this->child(i);
    }
    addTotalSessionTime(time());
    return result;
}

QString Task::setSessionTime(long minutes)
{
    mSessionTime = minutes;
    mTotalSessionTime += minutes;
    return QString();
}

void Task::changeTimes(long minutesSession, long minutes, EventsModel *eventsModel)
{
    qDebug() << "Task's sessionStartTiMe is " << mSessionStartTiMe;
    if (minutesSession != 0 || minutes != 0) {
        mSessionTime += minutesSession;
        mTime += minutes;
        if (eventsModel) {
            eventsModel->changeTime(this, minutes * secsPerMinute);
        }
        changeTotalTimes(minutesSession, minutes);
    }
}

void Task::changeTime(long minutes, EventsModel *eventsModel)
{
    changeTimes(minutes, minutes, eventsModel);
}

void Task::changeTotalTimes(long minutesSession, long minutes)
{
    qCDebug(KTT_LOG)
        << "Task::changeTotalTimes(" << minutesSession << ","
        << minutes << ") for" << name();
    mTotalSessionTime += minutesSession;
    mTotalTime += minutes;
    update();
    changeParentTotalTimes(minutesSession, minutes);
}

void Task::resetTimes()
{
    mTotalSessionTime -= mSessionTime;
    mTotalTime -= mTime;
    changeParentTotalTimes(-mSessionTime, -mTime);
    mSessionTime = 0;
    mTime = 0;
    update();
}

void Task::changeParentTotalTimes(long minutesSession, long minutes)
{
    if (parentTask()) {
        parentTask()->changeTotalTimes(minutesSession, minutes);
    }
}

bool Task::remove(TimeTrackerStorage* storage)
{
    qCDebug(KTT_LOG) << "entering function" << mName;
    bool ok = true;

    for (int i = 0; i < childCount(); ++i) {
        Task* task = static_cast<Task*>(child(i));
        task->remove(storage);
    }

    if (isRunning()) {
        setRunning(false, storage);
    }

    m_projectModel->eventsModel()->removeAllForTask(this);

    changeParentTotalTimes(-mSessionTime, -mTime);

    storage->save();

    return ok;
}

QString Task::fullName() const
{
    if (isRoot()) {
        return name();
    } else {
        return parentTask()->fullName() + QString::fromLatin1("/") + name();
    }
}

KCalCore::Todo::Ptr Task::asTodo(const KCalCore::Todo::Ptr& todo) const
{
    Q_ASSERT(todo != nullptr);

    qCDebug(KTT_LOG) <<"Task::asTodo: name() = '" << name() << "'";
    todo->setUid(uid());
    todo->setSummary(name());
    todo->setDescription(description());

    // Note: if the date start is empty, the KOrganizer GUI will have the
    // checkbox blank, but will prefill the todo's starting datetime to the
    // time the file is opened.
    // todo->setDtStart( current );

    todo->setCustomProperty(eventAppName, QByteArray("totalTaskTime"), QString::number(mTime));
    todo->setCustomProperty(eventAppName, QByteArray("totalSessionTime"), QString::number(mSessionTime));
    todo->setCustomProperty(eventAppName, QByteArray("sessionStartTiMe"), mSessionStartTiMe.toString());
    qDebug() << "mSessionStartTiMe=" << mSessionStartTiMe.toString();

    if (getDesktopStr().isEmpty()) {
        todo->removeCustomProperty(eventAppName, QByteArray("desktopList"));
    } else {
        todo->setCustomProperty(eventAppName, QByteArray("desktopList"), getDesktopStr());
    }

    todo->setPercentComplete(mPercentComplete);
    todo->setPriority( mPriority );

    if (parentTask()) {
        todo->setRelatedTo(parentTask()->uid());
    }

    return todo;
}

bool Task::parseIncidence( const KCalCore::Incidence::Ptr &incident, long& minutes,
    long& sessionMinutes, QString& sessionStartTiMe, QString& name, QString& description, DesktopList& desktops,
    int& percent_complete, int& priority )
{
    qCDebug(KTT_LOG) << "Entering function";
    bool ok;
    name = incident->summary();
    description = incident->description();
    mUid = incident->uid();
    mComment = incident->description();

    ok = false;
    minutes = getCustomProperty(incident, QStringLiteral("totalTaskTime")).toInt(&ok);
    if (!ok) {
        minutes = 0;
    }

    ok = false;
    sessionMinutes = getCustomProperty(incident, QStringLiteral("totalSessionTime")).toInt(&ok);
    if (!ok) {
        sessionMinutes = 0;
    }

    sessionStartTiMe = getCustomProperty(incident, QStringLiteral("sessionStartTiMe"));

    QString desktopList = getCustomProperty(incident, QStringLiteral("desktopList"));
    QStringList desktopStrList = desktopList.split(QStringLiteral(","), QString::SkipEmptyParts);
    desktops.clear();

    for (const QString& desktopStr : desktopStrList) {
        int desktopInt = desktopStr.toInt(&ok);
        if (ok) {
            desktops.push_back(desktopInt);
        }
    }
    percent_complete = incident.staticCast<KCalCore::Todo>()->percentComplete();
    priority = incident->priority();
    return true;
}

QString Task::getDesktopStr() const
{
    if (mDesktops.empty()) {
        return QString();
    }

    QString desktopsStr;
    for (const int desktop : mDesktops) {
        desktopsStr += QString::number(desktop) + QString::fromLatin1(",");
    }
    desktopsStr.remove(desktopsStr.length() - 1, 1);
    return desktopsStr;
}

// This is needed e.g. to move a task under its parent when loading.
void Task::cut()
{
    changeParentTotalTimes(-mTotalSessionTime, -mTotalTime);
    if (!parentTask()) {
        m_projectModel->tasksModel()->takeTopLevelItem(m_projectModel->tasksModel()->indexOfTopLevelItem(this));
    } else {
        parentTask()->takeChild(parentTask()->indexOfChild(this));
    }
}

// This is needed e.g. to move a task under its parent when loading.
void Task::paste(TasksModelItem* destination)
{
    destination->insertChild(0, this);
    changeParentTotalTimes(mTotalSessionTime, mTotalTime);
}

// This is used e.g. to move each task under its parent after loading.
void Task::move(TasksModelItem* destination)
{
    cut();
    paste(destination);
}

QVariant Task::data(int column, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    bool b = KTimeTrackerSettings::decimalFormat();
    switch (column) {
        case 0:
            return mName;
        case 1:
            return formatTime(mSessionTime, b);
        case 2:
            return formatTime(mTime, b);
        case 3:
            return formatTime(mTotalSessionTime, b);
        case 4:
            return formatTime(mTotalTime, b);
        case 5:
            return mPriority > 0 ? QString::number(mPriority) : QStringLiteral("--");
        case 6:
            return QString::number(mPercentComplete);
        default:
            return {};
    }
}

// Update a row, containing one task
void Task::update()
{
    QModelIndex first = m_projectModel->tasksModel()->index(this, 0);
    QModelIndex last = m_projectModel->tasksModel()->index(this, 6);
    emit m_projectModel->tasksModel()->dataChanged(first, last, QVector<int>{Qt::DisplayRole});
}

void Task::addComment(const QString& comment, TimeTrackerStorage* storage)
{
    mComment = mComment + QString::fromLatin1("\n") + comment;

    // TODO: Use libkcalcore comments
    // todo->addComment(comment);

    storage->save();
}

void Task::startNewSession()
{
    changeTimes(-mSessionTime, 0, nullptr);
    mSessionStartTiMe = QDateTime::currentDateTime();
}

/* Overriding the < operator in order to sort the names case insensitive and
 * the progress percentage [column 6] numerically.
 */
bool Task::operator<(const TasksModelItem &other) const
{
    const auto& otherTask = dynamic_cast<const Task&>(other);
    const int column = m_taskView->sortColumn();
    if (column == 6) {
        return mPercentComplete < otherTask.mPercentComplete;
    } else if (column == 0) { //task name
        return mName.toLower() < otherTask.mName.toLower();
    } else {
        return data(column, Qt::DisplayRole) < other.data(column, Qt::DisplayRole);
    }
}

//BEGIN Properties
QString Task::uid() const
{
    return mUid;
}

QString Task::comment() const
{
    return mComment;
}

int Task::percentComplete() const
{
    return mPercentComplete;
}

int Task::priority() const
{
    return mPriority;
}

QString Task::name() const
{
    return mName;
}

QString Task::description() const
{
    return mDescription;
}

QDateTime Task::startTime() const
{
    return mLastStart;
}

QDateTime Task::sessionStartTiMe() const
{
    return mSessionStartTiMe;
}

DesktopList Task::desktops() const
{
    return mDesktops;
}
//END
