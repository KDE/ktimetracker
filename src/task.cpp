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
#include <QString>
#include <QPixmap>
#include <QDebug>
#include <QMovie>

#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "ktt_debug.h"

const QByteArray eventAppName = QByteArray("ktimetracker");

Task::Task(
    const QString& taskName, const QString& taskDescription, long minutes, long sessionTime,
    DesktopList desktops, TaskView *parent)
    : QObject()
    , QTreeWidgetItem(parent)
{
    init(taskName, taskDescription, minutes, sessionTime, 0, desktops, 0, 0);
}

Task::Task( const QString& taskName, const QString& taskDescription, long minutes, long sessionTime,
            DesktopList desktops, Task *parent)
  : QObject(), QTreeWidgetItem(parent)
{
    init( taskName, taskDescription, minutes, sessionTime, 0, desktops, 0, 0 );
}

Task::Task(const KCalCore::Todo::Ptr &todo, TaskView* parent)
  : QObject(), QTreeWidgetItem( parent )
{
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
    const TaskView *taskView = qobject_cast<TaskView*>(treeWidget());
    // If our parent is the taskview then connect our totalTimesChanged
    // signal to its receiver
    if (!parentTask()) {
        connect(this, &Task::totalTimesChanged,
                taskView, &TaskView::taskTotalTimesChanged);
    }

    connect(this, &Task::deletingTask, taskView, &TaskView::deletingTask);

    // Prepare animated icon
    m_clockAnimation = new QMovie(":/pics/watch.gif", QByteArray(), this);
    connect(m_clockAnimation, &QMovie::frameChanged, this, &Task::setActiveIcon);

    mRemoving = false;
    mName = taskName.trimmed();
    mDescription = taskDescription.trimmed();
    mLastStart = QDateTime::currentDateTime();
    mTotalTime = mTime = minutes;
    mTotalSessionTime = mSessionTime = sessionTime;
    mDesktops = desktops;
    setIcon(1, QPixmap(":/pics/empty-watch.xpm"));
    mPercentComplete = percent_complete;
    mPriority = priority;
    mSessionStartTiMe = QDateTime::fromString(sessionStartTiMe);

    update();
    changeParentTotalTimes(mSessionTime, mTime);

    // alignment of the number items
    for (int i = 1; i < columnCount(); ++i) {
        setTextAlignment(i, Qt::AlignRight);
    }

    // .. but not the priority column
    setTextAlignment(5, Qt::AlignCenter);
}

Task::~Task()
{
    emit deletingTask(this);
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
    qCDebug(KTT_LOG) << "Entering function";
    if (on) {
        if (!isRunning()) {
            m_clockAnimation->start();
            mLastStart = when;
            qCDebug(KTT_LOG) << "task has been started for " << when;
        }
    } else {
        if (isRunning()) {
            m_clockAnimation->stop();
            if (!mRemoving) {
                setIcon(1, QPixmap(":/pics/empty-watch.xpm"));
            }
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
        m_clockAnimation->start();
    }
}

void Task::setUid(const QString& uid)
{
    mUid = uid;
}

bool Task::isRunning() const
{
    return m_clockAnimation->state() == QMovie::Running;
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

void Task::setPercentComplete(int percent, TimeTrackerStorage* storage)
{
    qCDebug(KTT_LOG) << "Entering function(" << percent <<", storage):" << mUid;

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
        taskView()->stopTimerFor(this);
    }

    setPixmapProgress();

    // When parent marked as complete, mark all children as complete as well.
    // This behavior is consistent with KOrganizer (as of 2003-09-24).
    if (mPercentComplete == 100) {
        for (int i = 0; i < childCount(); ++i) {
            Task *task = static_cast< Task* >( child( i ) );
            task->setPercentComplete(mPercentComplete, storage);
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

void Task::setPixmapProgress()
{
    qCDebug(KTT_LOG) << "Entering function";

    if (mPercentComplete >= 100) {
        setIcon(0, QPixmap(":/pics/task-complete.xpm"));
    } else {
        setIcon(0, QPixmap(":/pics/task-incomplete.xpm"));
    }

    qCDebug(KTT_LOG) << "Leaving function";
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
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    mTime += minutes;
    this->addTotalTime(minutes);
    qCDebug(KTT_LOG) << "Leaving function";
    return err;
}

QString Task::addTotalTime(long minutes)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    mTotalTime += minutes;
    if (parentTask()) {
        parentTask()->addTotalTime(minutes);
    }
    qCDebug(KTT_LOG) << "Leaving function";
    return err;
}

QString Task::addSessionTime(long minutes)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    mSessionTime += minutes;
    this->addTotalSessionTime(minutes);
    qCDebug(KTT_LOG) << "Leaving function";
    return err;
}

QString Task::addTotalSessionTime(long minutes)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    mTotalSessionTime += minutes;
    if (parentTask()) {
        parentTask()->addTotalSessionTime(minutes);
    }
    qCDebug(KTT_LOG) << "Leaving function";
    return err;
}

QString Task::setTime(long minutes)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    mTime = minutes;
    mTotalTime += minutes;
    qCDebug(KTT_LOG) << "Leaving function";
    return err;
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
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    mSessionTime = minutes;
    mTotalSessionTime += minutes;
    qCDebug(KTT_LOG) << "Leaving function";
    return err;
}

void Task::changeTimes(long minutesSession, long minutes, TimeTrackerStorage* storage)
{
    qCDebug(KTT_LOG) << "Entering function";
    qDebug() << "Task's sessionStartTiMe is " << mSessionStartTiMe;
    if (minutesSession != 0 || minutes != 0) {
        mSessionTime += minutesSession;
        mTime += minutes;
        if (storage) {
            storage->changeTime(this, minutes * secsPerMinute);
        }
        changeTotalTimes(minutesSession, minutes);
    }
    qCDebug(KTT_LOG) << "Leaving function";
}

void Task::changeTime(long minutes, TimeTrackerStorage* storage)
{
    changeTimes(minutes, minutes, storage);
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
    qCDebug(KTT_LOG) << "Leaving function";
}

void Task::resetTimes()
{
    qCDebug(KTT_LOG) << "Entering function";
    mTotalSessionTime -= mSessionTime;
    mTotalTime -= mTime;
    changeParentTotalTimes(-mSessionTime, -mTime);
    mSessionTime = 0;
    mTime = 0;
    update();
    qCDebug(KTT_LOG) << "Leaving function";
}

void Task::changeParentTotalTimes(long minutesSession, long minutes)
{
    if (isRoot()) {
        emit totalTimesChanged(minutesSession, minutes);
    } else {
        parentTask()->changeTotalTimes(minutesSession, minutes);
    }
}

bool Task::remove(TimeTrackerStorage* storage)
{
    qCDebug(KTT_LOG) << "entering function" << mName;
    bool ok = true;

    mRemoving = true;
    storage->removeTask(this);
    if (isRunning()) {
        setRunning(false, storage);
    }

    for (int i = 0; i < childCount(); ++i) {
        Task* task = static_cast<Task*>(child(i));
        if (task->isRunning()) {
            task->setRunning(false, storage);
        }
        task->remove(storage);
    }

    changeParentTotalTimes(-mSessionTime, -mTime);
    mRemoving = false;
    return ok;
}

void Task::setActiveIcon(int frame)
{
    setIcon(1, QIcon(m_clockAnimation->currentPixmap()));
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
    Q_ASSERT(todo != NULL);

    qCDebug(KTT_LOG) <<"Task::asTodo: name() = '" << name() << "'";
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

    todo->setOrganizer(KTimeTrackerSettings::userRealName());
    todo->setPercentComplete(mPercentComplete);
    todo->setPriority( mPriority );
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

    // if a KDE-karm-duration exists and not KDE-ktimetracker-duration, change this
    ok = false;
    if (
        incident->customProperty(eventAppName, QByteArray("totalTaskTime")) == QString::null &&
        incident->customProperty("karm", QByteArray("totalTaskTime")) != QString::null) {
        incident->setCustomProperty(
            eventAppName, QByteArray("totalTaskTime"),
            incident->customProperty("karm", QByteArray("totalTaskTime")));
    }

    minutes = incident->customProperty(eventAppName, QByteArray("totalTaskTime")).toInt(&ok);
    if (!ok) {
        minutes = 0;
    }

    // if a KDE-karm-totalSessionTime exists and not KDE-ktimetracker-totalSessionTime, change this
    ok = false;
    if (
        incident->customProperty(eventAppName, QByteArray("totalSessionTime")) == QString::null &&
        incident->customProperty("karm", QByteArray("totalSessionTime")) != QString::null) {
        incident->setCustomProperty(
            eventAppName, QByteArray("totalSessionTime"),
            incident->customProperty("karm", QByteArray("totalSessionTime")));
    }

    sessionMinutes = incident->customProperty(eventAppName, QByteArray("totalSessionTime")).toInt(&ok);
    if (!ok) {
        sessionMinutes = 0;
    }
    sessionStartTiMe = incident->customProperty(eventAppName, QByteArray("sessionStartTiMe"));

    // if a KDE-karm-deskTopList exists and no KDE-ktimetracker-DeskTopList, change this
    if (incident->customProperty(eventAppName, QByteArray( "desktopList" )) == QString::null &&
        incident->customProperty("karm", QByteArray("desktopList")) != QString::null) {
        incident->setCustomProperty(
            eventAppName, QByteArray("desktopList"),
            incident->customProperty("karm", QByteArray("desktopList")));
    }

    QString desktopList = incident->customProperty(eventAppName, QByteArray("desktopList"));
    QStringList desktopStrList = desktopList.split(QStringLiteral(","), QString::SkipEmptyParts);
    desktops.clear();

    for (QStringList::iterator iter = desktopStrList.begin(); iter != desktopStrList.end(); ++iter) {
        int desktopInt = (*iter).toInt(&ok);
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

    QString desktopstr;
    for (DesktopList::const_iterator iter = mDesktops.begin(); iter != mDesktops.end(); ++iter) {
        desktopstr += QString::number(*iter) + QString::fromLatin1(",");
    }
    desktopstr.remove(desktopstr.length() - 1, 1);
    return desktopstr;
}

void Task::cut()
// This is needed e.g. to move a task under its parent when loading.
{
    qCDebug(KTT_LOG) << "Entering function";

    changeParentTotalTimes(-mTotalSessionTime, -mTotalTime);
    if (!parentTask()) {
        treeWidget()->takeTopLevelItem(treeWidget()->indexOfTopLevelItem(this));
    } else {
        parentTask()->takeChild(indexOfChild(this));
    }

    qCDebug(KTT_LOG) << "Leaving function";
}

void Task::paste(Task* destination)
// This is needed e.g. to move a task under its parent when loading.
{
    qCDebug(KTT_LOG) << "Entering function";
    destination->QTreeWidgetItem::insertChild(0, this);
    changeParentTotalTimes( mTotalSessionTime, mTotalTime);
    qCDebug(KTT_LOG) << "Leaving function";
}

void Task::move(Task* destination)
// This is used e.g. to move each task under its parent after loading.
{
    qCDebug(KTT_LOG) << "Entering function";
    cut();
    paste(destination);
    qCDebug(KTT_LOG) << "Leaving function";
}

void Task::update()
// Update a row, containing one task
{
    qCDebug(KTT_LOG) << "Entering function";
    bool b = KTimeTrackerSettings::decimalFormat();
    setText(0, mName);
    setText(1, formatTime(mSessionTime, b));
    setText(2, formatTime(mTime, b));
    setText(3, formatTime(mTotalSessionTime, b));
    setText(4, formatTime(mTotalTime, b));
    setText(5, mPriority > 0 ? QString::number(mPriority) : "--");
    setText(6, QString::number(mPercentComplete));
    qCDebug(KTT_LOG) << "Leaving function";
}

void Task::addComment(const QString& comment, TimeTrackerStorage* storage)
{
    mComment = mComment + QString::fromLatin1("\n") + comment;
    storage->addComment(this, comment);
}

void Task::startNewSession()
{
    changeTimes(-mSessionTime, 0);
    mSessionStartTiMe = QDateTime::currentDateTime();
}

/* Overriding the < operator in order to sort the names case insensitive and
 * the progress percentage [coloumn 6] numerically.
 */
bool Task::operator<(const QTreeWidgetItem &other) const {
    const int column = treeWidget()->sortColumn();
    if (column == 6) { //progress percent
        return text(column).toInt() < other.text(column).toInt();
    } else if (column == 0) { //task name
        return text(column).toLower() < other.text(column).toLower();
    } else {
        return text(column) < other.text(column);
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
