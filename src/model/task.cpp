/*
 * Copyright (C) 1997 by Stephan Kulow <coolo@kde.org>
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

#include "task.h"

#include <KCalendarCore/CalFormat>

#include "ktimetracker.h"
#include "ktimetrackerutility.h"
#include "ktt_debug.h"
#include "model/eventsmodel.h"
#include "model/projectmodel.h"
#include "model/tasksmodel.h"
#include "timetrackerstorage.h"

static const QByteArray eventAppName = QByteArray("ktimetracker");

Task::Task(const QString& taskName, const QString& taskDescription, int64_t minutes, int64_t sessionTime,
           const DesktopList& desktops, ProjectModel *projectModel, Task *parentTask)
    : TasksModelItem(projectModel->tasksModel(), parentTask)
    , m_projectModel(projectModel)
{
    if (parentTask) {
        parentTask->addChild(this);
    } else {
        m_projectModel->tasksModel()->addChild(this);
    }

    init(taskName, taskDescription, minutes, sessionTime, nullptr, desktops, 0, 0);

    m_uid = KCalendarCore::CalFormat::createUniqueId();
}

Task::Task(const KCalendarCore::Todo::Ptr &todo, ProjectModel *projectModel)
    : TasksModelItem(projectModel->tasksModel(), nullptr)
    , m_projectModel(projectModel)
{
    projectModel->tasksModel()->addChild(this);

    int64_t minutes = 0;
    QString name;
    QString description;
    int64_t sessionTime = 0;
    QString sessionStartTiMe;
    int percent_complete = 0;
    int priority = 0;
    DesktopList desktops;

    parseIncidence( todo, minutes, sessionTime, sessionStartTiMe, name, description, desktops, percent_complete,
                  priority );
    init( name, description, minutes, sessionTime, sessionStartTiMe, desktops, percent_complete, priority);
}

Task::~Task()
{
    disconnectFromParent();
}

int Task::depth()
{
    int res = 0;
    for (Task* t = parentTask(); t; t = t->parentTask()) {
        res++;
    }

    qCDebug(KTT_LOG) << "Leaving function. depth is:" << res;
    return res;
}

void Task::init(
    const QString& taskName, const QString& taskDescription, int64_t minutes, int64_t sessionTime,
    const QString& sessionStartTiMe,
    const DesktopList& desktops, int percent_complete, int priority)
{
    m_isRunning = false;
    m_name = taskName.trimmed();
    m_description = taskDescription.trimmed();
    m_lastStart = QDateTime::currentDateTime();
    m_totalTime = m_time = minutes;
    m_totalSessionTime = m_sessionTime = sessionTime;
    m_desktops = desktops;

    m_percentComplete = percent_complete;
    m_priority = priority;
    m_sessionStartTime = QDateTime::fromString(sessionStartTiMe);

    update();
    changeParentTotalTimes(m_sessionTime, m_time);
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
void Task::setRunning(bool on, const QDateTime& when)
{
    if (on != m_isRunning) {
        m_isRunning = on;
        invalidateRunningState();

        if (on) {
            m_lastStart = when;
            qCDebug(KTT_LOG) << "task has been started for " << when;

//            m_projectModel->eventsModel()->startTask(this, when);
            m_projectModel->eventsModel()->startTask(this);
        } else {
            m_projectModel->eventsModel()->stopTask(this, when);
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

void Task::setName(const QString& name)
{
    qCDebug(KTT_LOG) << "Entering function, name=" << name;

    QString oldname = m_name;
    if (oldname != name) {
        m_name = name;
        update();
    }
}

void Task::setDescription(const QString& description)
{
    qCDebug(KTT_LOG) << "Entering function, description=" << description;

    QString olddescription = m_description;
    if (olddescription != description) {
        m_description = description;
        update();
    }
}

void Task::setPercentComplete(int percent)
{
    qCDebug(KTT_LOG) << "Entering function(" << percent << "):" << m_uid;

    if (!percent) {
        m_percentComplete = 0;
    } else if (percent > 100) {
        m_percentComplete = 100;
    } else if (percent < 0) {
        m_percentComplete = 0;
    } else {
        m_percentComplete = percent;
    }

    if (isRunning() && m_percentComplete == 100) {
        emit m_projectModel->tasksModel()->taskCompleted(this);
    }

    invalidateCompletedState();

    // When parent marked as complete, mark all children as complete as well.
    // This behavior is consistent with KOrganizer (as of 2003-09-24).
    if (m_percentComplete == 100) {
        for (int i = 0; i < childCount(); ++i) {
            Task *task = dynamic_cast<Task*>(child(i));
            task->setPercentComplete(m_percentComplete);
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

    m_priority = priority;
    update();
}

bool Task::isComplete()
{
    return m_percentComplete == 100;
}

void Task::setDesktopList(const DesktopList& desktopList)
{
    m_desktops = desktopList;
}

void Task::addTime(int64_t minutes)
{
    m_time += minutes;
    this->addTotalTime(minutes);
}

void Task::addTotalTime(int64_t minutes)
{
    m_totalTime += minutes;
    if (parentTask()) {
        parentTask()->addTotalTime(minutes);
    }
}

void Task::addSessionTime(int64_t minutes)
{
    m_sessionTime += minutes;
    this->addTotalSessionTime(minutes);
}

void Task::addTotalSessionTime(int64_t minutes)
{
    m_totalSessionTime += minutes;
    if (parentTask()) {
        parentTask()->addTotalSessionTime(minutes);
    }
}

QString Task::setTime(int64_t minutes)
{
    m_time = minutes;
    m_totalTime += minutes;
    return QString();
}

void Task::recalculateTotalTimesSubtree()
{
    int64_t totalMinutes = time();
    int64_t totalSessionMinutes = sessionTime();
    for (int i = 0; i < this->childCount(); ++i) {
        Task *subTask = dynamic_cast<Task*>(child(i));
        subTask->recalculateTotalTimesSubtree();

        totalMinutes += subTask->totalTime();
        totalSessionMinutes += subTask->totalSessionTime();
    }

    setTotalTime(totalMinutes);
    setTotalSessionTime(totalSessionMinutes);
}

QString Task::setSessionTime(int64_t minutes)
{
    m_sessionTime = minutes;
    m_totalSessionTime += minutes;
    return QString();
}

void Task::changeTimes(int64_t minutesSession, int64_t minutes, EventsModel *eventsModel)
{
    qDebug() << "Task's sessionStartTiMe is " << m_sessionStartTime;
    if (minutesSession != 0 || minutes != 0) {
        m_sessionTime += minutesSession;
        m_time += minutes;
        if (eventsModel) {
            eventsModel->changeTime(this, minutes * secsPerMinute);
        }
        changeTotalTimes(minutesSession, minutes);
    }
}

void Task::changeTime(int64_t minutes, EventsModel *eventsModel)
{
    changeTimes(minutes, minutes, eventsModel);
}

void Task::changeTotalTimes(int64_t minutesSession, int64_t minutes)
{
    qCDebug(KTT_LOG)
        << "Task::changeTotalTimes(" << minutesSession << ","
        << minutes << ") for" << name();
    m_totalSessionTime += minutesSession;
    m_totalTime += minutes;
    update();
    changeParentTotalTimes(minutesSession, minutes);
}

void Task::resetTimes()
{
    m_totalSessionTime -= m_sessionTime;
    m_totalTime -= m_time;
    changeParentTotalTimes(-m_sessionTime, -m_time);
    m_sessionTime = 0;
    m_time = 0;
    update();
}

void Task::changeParentTotalTimes(int64_t minutesSession, int64_t minutes)
{
    if (parentTask()) {
        parentTask()->changeTotalTimes(minutesSession, minutes);
    }
}

bool Task::remove(TimeTrackerStorage* storage)
{
    qCDebug(KTT_LOG) << "entering function" << m_name;
    bool ok = true;

    for (int i = 0; i < childCount(); ++i) {
        Task* task = dynamic_cast<Task*>(child(i));
        if (!task) {
            qFatal("Task::remove: task is nullptr");
        }

        task->remove(storage);
    }

    setRunning(false);

    m_projectModel->eventsModel()->removeAllForTask(this);

    changeParentTotalTimes(-m_sessionTime, -m_time);

    // TODO check return value
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

KCalendarCore::Todo::Ptr Task::asTodo(const KCalendarCore::Todo::Ptr& todo) const
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

    todo->setCustomProperty(eventAppName, QByteArray("totalTaskTime"), QString::number(m_time));
    todo->setCustomProperty(eventAppName, QByteArray("totalSessionTime"), QString::number(m_sessionTime));
    todo->setCustomProperty(eventAppName, QByteArray("sessionStartTiMe"), m_sessionStartTime.toString());
    qDebug() << "m_sessionStartTime=" << m_sessionStartTime.toString();

    if (getDesktopStr().isEmpty()) {
        todo->removeCustomProperty(eventAppName, QByteArray("desktopList"));
    } else {
        todo->setCustomProperty(eventAppName, QByteArray("desktopList"), getDesktopStr());
    }

    todo->setPercentComplete(m_percentComplete);
    todo->setPriority( m_priority );

    if (parentTask()) {
        todo->setRelatedTo(parentTask()->uid());
    }

    return todo;
}

bool Task::parseIncidence(
    const KCalendarCore::Incidence::Ptr &incident, int64_t& minutes,
    int64_t& sessionMinutes, QString& sessionStartTiMe, QString& name, QString& description, DesktopList& desktops,
    int& percent_complete, int& priority)
{
    qCDebug(KTT_LOG) << "Entering function";
    bool ok;
    name = incident->summary();
    description = incident->description();
    m_uid = incident->uid();
    m_comment = incident->description();

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
    percent_complete = incident.staticCast<KCalendarCore::Todo>()->percentComplete();
    priority = incident->priority();
    return true;
}

QString Task::getDesktopStr() const
{
    if (m_desktops.empty()) {
        return QString();
    }

    QString desktopsStr;
    for (const int desktop : m_desktops) {
        desktopsStr += QString::number(desktop) + QString::fromLatin1(",");
    }
    desktopsStr.remove(desktopsStr.length() - 1, 1);
    return desktopsStr;
}

// This is needed e.g. to move a task under its parent when loading.
void Task::cut()
{
    changeParentTotalTimes(-m_totalSessionTime, -m_totalTime);
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
    changeParentTotalTimes(m_totalSessionTime, m_totalTime);
}

// This is used e.g. to move each task under its parent after loading.
void Task::move(TasksModelItem* destination)
{
    cut();
    paste(destination);
}

QVariant Task::data(int column, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            bool b = KTimeTrackerSettings::decimalFormat();
            switch (column) {
                case 0:
                    return m_name;
                case 1:
                    return formatTime(m_sessionTime, b);
                case 2:
                    return formatTime(m_time, b);
                case 3:
                    return formatTime(m_totalSessionTime, b);
                case 4:
                    return formatTime(m_totalTime, b);
                case 5:
                    return m_priority > 0 ? QString::number(m_priority) : QStringLiteral("--");
                case 6:
                    return QString::number(m_percentComplete);
                default:
                    return {};
            }
        }
        case SortRole: {
            // QSortFilterProxyModel::lessThan() supports comparison of a few data types,
            // here we use some of those: QString, qlonglong, int.
            switch (column) {
                case 0:
                    return m_name;
                case 1:
                    return QVariant::fromValue<int64_t>(m_sessionTime);
                case 2:
                    return QVariant::fromValue<int64_t>(m_time);
                case 3:
                    return QVariant::fromValue<int64_t>(m_totalSessionTime);
                case 4:
                    return QVariant::fromValue<int64_t>(m_totalTime);
                case 5:
                    return QVariant::fromValue<int>(m_priority);
                case 6:
                    return QVariant::fromValue<int>(m_percentComplete);
                default:
                    return {};
            }
        }
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
    m_comment = m_comment + QString::fromLatin1("\n") + comment;

    // TODO: Use libkcalcore comments
    // todo->addComment(comment);

    // TODO check return value
    storage->save();
}

void Task::startNewSession()
{
    changeTimes(-m_sessionTime, 0, nullptr);
    m_sessionStartTime = QDateTime::currentDateTime();
}

//BEGIN Properties
QString Task::uid() const
{
    return m_uid;
}

QString Task::comment() const
{
    return m_comment;
}

int Task::percentComplete() const
{
    return m_percentComplete;
}

int Task::priority() const
{
    return m_priority;
}

QString Task::name() const
{
    return m_name;
}

QString Task::description() const
{
    return m_description;
}

QDateTime Task::startTime() const
{
    return m_lastStart;
}

QDateTime Task::sessionStartTiMe() const
{
    return m_sessionStartTime;
}

DesktopList Task::desktops() const
{
    return m_desktops;
}
//END
