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
#include <QTimer>
#include <QPixmap>

#include <KDebug>
#include <KIconLoader>

#include <kcal/event.h>

#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "preferences.h"

QVector<QPixmap*> *Task::icons = 0;

Task::Task( const QString& taskName, long minutes, long sessionTime,
            DesktopList desktops, TaskView *parent, bool konsolemode )
  : QObject(), QTreeWidgetItem(parent)
{ 
  init( taskName, minutes, sessionTime, 0, desktops, 0, 0, konsolemode );
}

Task::Task( const QString& taskName, long minutes, long sessionTime,
            DesktopList desktops, Task *parent)
  : QObject(), QTreeWidgetItem(parent) 
{
  init( taskName, minutes, sessionTime, 0, desktops, 0, 0 );
}

Task::Task( KCal::Todo* todo, TaskView* parent, bool konsolemode )
  : QObject(), QTreeWidgetItem( parent )
{
    long minutes = 0;
    QString name;
    long sessionTime = 0;
    QString sessionStartTiMe;
    int percent_complete = 0;
    int priority = 0;
    DesktopList desktops;

    parseIncidence( todo, minutes, sessionTime, sessionStartTiMe, name, desktops, percent_complete,
                  priority );
    init( name, minutes, sessionTime, sessionStartTiMe, desktops, percent_complete, priority, konsolemode );
}

int Task::depth()
// Deliver the depth of a task, i.e. how many tasks are supertasks to it. 
// A toplevel task has the depth 0.
{
    kDebug(5970) << "Entering function";
    int res=0;
    Task* t=this;
    while ( ( t = t->parent() ) ) res++;
    kDebug(5970) << "Leaving function. depth is:" << res;
    return res;
}

void Task::init( const QString& taskName, long minutes, long sessionTime, QString sessionStartTiMe,
                 DesktopList desktops, int percent_complete, int priority, bool konsolemode )
{
    // If our parent is the taskview then connect our totalTimesChanged
    // signal to its receiver
    if ( ! parent() )
        connect( this, SIGNAL( totalTimesChanged ( long, long ) ),
             treeWidget(), SLOT( taskTotalTimesChanged( long, long) ));

    connect( this, SIGNAL( deletingTask( Task* ) ),
           treeWidget(), SLOT( deletingTask( Task* ) ));

    if (icons == 0)
    {
        icons = new QVector<QPixmap*>(8);
        if (!konsolemode)
        {
            KIconLoader kil("ktimetracker"); // always load icons from the KArm application
            for (int i=0; i<8; i++)
            {
                QPixmap *icon = new QPixmap();
                QString name;
                name.sprintf("watch-%d.xpm",i);
                *icon = kil.loadIcon( name, KIconLoader::User );
                icons->insert(i,icon);
            }
        }
    }

    mRemoving = false;
    mName = taskName.trimmed();
    mLastStart = QDateTime::currentDateTime();
    mTotalTime = mTime = minutes;
    mTotalSessionTime = mSessionTime = sessionTime;
    mTimer = new QTimer(this);
    mDesktops = desktops;
    connect(mTimer, SIGNAL(timeout()), this, SLOT(updateActiveIcon()));
    if ( !konsolemode ) setIcon(1, UserIcon(QString::fromLatin1("empty-watch.xpm")));
    mCurrentPic = 0;
    mPercentComplete = percent_complete;
    mPriority = priority;
    mSessionStartTiMe=KDateTime::fromString(sessionStartTiMe);

    update();
    changeParentTotalTimes( mSessionTime, mTime);
  
    // alignment of the number items
    for (int i = 1; i < columnCount(); ++i)
    {
        setTextAlignment( i, Qt::AlignRight );
    }

    // .. but not the priority column
    setTextAlignment( 5, Qt::AlignCenter );
}

Task::~Task() 
{
    emit deletingTask(this);
    delete mTimer;
}

void Task::delete_recursive()
{
    while ( this->child(0) )
    {
        Task* t=(Task*) this->child(0);
        t->delete_recursive();
    }
    delete this;
}

void Task::setRunning( bool on, timetrackerstorage* storage, const QDateTime &when )
// This is the back-end, the front-end is StartTimerFor()
{
    kDebug(5970) << "Entering function";
    if ( on )
    {
        if (!mTimer->isActive())
        {
            mTimer->start(1000);
            storage->startTimer(this);
            mCurrentPic=7;
            mLastStart = when;
            kDebug(5970) << "task has been started for " << when;
            updateActiveIcon();
        }
    }
    else
    {
        if (mTimer->isActive())
        {
            mTimer->stop();
            if ( ! mRemoving )
            {
                storage->stopTimer(this, when);
                setIcon(1, UserIcon(QString::fromLatin1("empty-watch.xpm")));
            }
        }
    }
}

void Task::resumeRunning()
// setRunning is the back-end, the front-end is StartTimerFor().
// resumeRunning does the same as setRunning, but not add a new 
// start date to the storage.
{
    kDebug(5970) << "Entering function";
    if (!mTimer->isActive())
    {
        mTimer->start(1000);
        mCurrentPic=7;
        updateActiveIcon();
    }
}

void Task::setUid( const QString &uid ) 
{
    mUid = uid;
}

bool Task::isRunning() const
{
    return mTimer->isActive();
}

void Task::setName( const QString& name, timetrackerstorage* storage )
{
    kDebug(5970) << "Entering function, name=" << name;

    QString oldname = mName;
    if ( oldname != name )
    {
        mName = name;
        storage->setName(this, oldname);
        update();
    }
}

void Task::setPercentComplete(const int percent, timetrackerstorage *storage)
{
    kDebug(5970) << "Entering function(" << percent <<", storage):" << mUid;

    if (!percent)
        mPercentComplete = 0;
    else if (percent > 100)
        mPercentComplete = 100;
    else if (percent < 0)
        mPercentComplete = 0;
    else
        mPercentComplete = percent;

    if (isRunning() && mPercentComplete==100) taskView()->stopTimerFor(this);

    setPixmapProgress();

    // When parent marked as complete, mark all children as complete as well.
    // This behavior is consistent with KOrganizer (as of 2003-09-24).
    if (mPercentComplete == 100)
    {
        for ( int i = 0; i < childCount(); ++i )
        {
            Task *task = static_cast< Task* >( child( i ) );
            task->setPercentComplete(mPercentComplete, storage);
        }
    }
    // maybe there is a column "percent completed", so do a ...
    update();
}

void Task::setPriority( int priority )
{
    if ( priority < 0 )
    {
        priority = 0;
    }
    else if ( priority > 9 )
    {
        priority = 9;
    }

    mPriority = priority;
    update();
}

void Task::setPixmapProgress()
{
    kDebug(5970) << "Entering function";
    QPixmap icon;
    KIconLoader* kil = new KIconLoader();
    if (mPercentComplete >= 100)
    {
        const QString iconcomplete=QString("task-complete.xpm");
        icon = kil->loadIcon( iconcomplete, KIconLoader::User );
    }
    else
    {
        const QString iconincomplete=QString("task-incomplete.xpm");
        icon = kil->loadIcon( iconincomplete, KIconLoader::User );
    }
    setIcon(0, icon);
    kDebug(5970) << "Leaving function";
}

bool Task::isComplete() { return mPercentComplete == 100; }

void Task::setDesktopList ( DesktopList desktopList )
{
    mDesktops = desktopList;
}

void Task::changeTime( long minutes, timetrackerstorage* storage )
{
    changeTimes( minutes, minutes, storage);
}

QString Task::addTime( long minutes )
{
    kDebug(5970) << "Entering function";
    QString err;
    mTime+=minutes;
    this->addTotalTime( minutes );
    kDebug(5970) << "Leaving function";
    return err;
}

QString Task::addTotalTime( long minutes )
{
    kDebug(5970) << "Entering function";
    QString err;
    mTotalTime+=minutes;
    if ( parent() ) parent()->addTotalTime( minutes );
    kDebug(5970) << "Leaving function";
    return err;
}

QString Task::addSessionTime( long minutes )
{
    kDebug(5970) << "Entering function";
    QString err;
    mSessionTime+=minutes;
    this->addTotalSessionTime( minutes );
    kDebug(5970) << "Leaving function";
    return err;
}

QString Task::addTotalSessionTime( long minutes )
{
    kDebug(5970) << "Entering function";
    QString err;
    mTotalSessionTime+=minutes;
    if ( parent() ) parent()->addTotalSessionTime( minutes );
    kDebug(5970) << "Leaving function";
    return err;
}

QString Task::setTime( long minutes )
{
    kDebug(5970) << "Entering function";
    QString err;
    mTime=minutes;
    mTotalTime+=minutes;
    kDebug(5970) << "Leaving function";
    return err;
}

QString Task::setSessionTime( long minutes )
{
    kDebug(5970) << "Entering function";
    QString err;
    mSessionTime=minutes;
    mTotalSessionTime+=minutes;
    kDebug(5970) << "Leaving function";
    return err;
}

void Task::changeTimes( long minutesSession, long minutes, timetrackerstorage* storage)
{
    kDebug(5970) << "Entering function";
    kDebug() << "Task's sessionStartTiMe is " << mSessionStartTiMe;
    if( minutesSession != 0 || minutes != 0)
    {
        mSessionTime += minutesSession;
        mTime += minutes;
        if ( storage ) storage->changeTime(this, minutes * secsPerMinute);
        changeTotalTimes( minutesSession, minutes );
    }
    kDebug(5970) << "Leaving function";
}

void Task::changeTotalTimes( long minutesSession, long minutes )
{
    kDebug(5970)
        << "Task::changeTotalTimes(" << minutesSession << ","
        << minutes << ") for" << name();
    mTotalSessionTime += minutesSession;
    mTotalTime += minutes;
    update();
    changeParentTotalTimes( minutesSession, minutes );
    kDebug(5970) << "Leaving function";
}

void Task::resetTimes()
{
    kDebug(5970) << "Entering function";
    mTotalSessionTime -= mSessionTime;
    mTotalTime -= mTime;
    changeParentTotalTimes( -mSessionTime, -mTime);
    mSessionTime = 0;
    mTime = 0;
    update();
    kDebug(5970) << "Leaving function";
}

void Task::changeParentTotalTimes( long minutesSession, long minutes )
{
    if ( isRoot() )
        emit totalTimesChanged( minutesSession, minutes );
    else
        parent()->changeTotalTimes( minutesSession, minutes );
}

bool Task::remove( timetrackerstorage* storage)
{
    kDebug(5970) << "entering function" << mName;
    bool ok = true;

    mRemoving = true;
    storage->removeTask(this);
    if( isRunning() ) setRunning( false, storage );

    for ( int i = 0; i < childCount(); ++i )
    {
        Task *task = static_cast< Task* >( child( i ) );
        if ( task->isRunning() )
            task->setRunning( false, storage );
        task->remove( storage );
    }

    changeParentTotalTimes( -mSessionTime, -mTime);
    mRemoving = false;
    return ok;
}

void Task::updateActiveIcon()
{
    mCurrentPic = (mCurrentPic+1) % 8;
    setIcon(1, *(*icons)[mCurrentPic]);
}

QString Task::fullName() const
{
    if (isRoot())
        return name();
    else
        return parent()->fullName() + QString::fromLatin1("/") + name();
}

KCal::Todo* Task::asTodo(KCal::Todo* todo) const
{
    Q_ASSERT( todo != NULL );

    kDebug(5970) <<"Task::asTodo: name() = '" << name() <<"'";
    todo->setSummary( name() );

    // Note: if the date start is empty, the KOrganizer GUI will have the
    // checkbox blank, but will prefill the todo's starting datetime to the
    // time the file is opened.
    // todo->setDtStart( current );

    todo->setCustomProperty( KGlobal::mainComponent().componentName().toUtf8(),
        QByteArray( "totalTaskTime" ), QString::number( mTime ) );
    todo->setCustomProperty( KGlobal::mainComponent().componentName().toUtf8(),
        QByteArray( "totalSessionTime" ), QString::number( mSessionTime) );
    todo->setCustomProperty( KGlobal::mainComponent().componentName().toUtf8(),
        QByteArray( "sessionStartTiMe" ), mSessionStartTiMe.toString() );
    kDebug() << "mSessionStartTiMe=" << mSessionStartTiMe.toString() ;

    if (getDesktopStr().isEmpty())
        todo->removeCustomProperty(KGlobal::mainComponent().componentName().toUtf8(), QByteArray("desktopList"));
    else
        todo->setCustomProperty( KGlobal::mainComponent().componentName().toUtf8(),
            QByteArray( "desktopList" ), getDesktopStr() );

    todo->setOrganizer( KTimeTrackerSettings::userRealName() );
    todo->setPercentComplete(mPercentComplete);
    todo->setPriority( mPriority );
    return todo;
}

bool Task::parseIncidence( KCal::Incidence* incident, long& minutes,
    long& sessionMinutes, QString& sessionStartTiMe, QString& name, DesktopList& desktops,
    int& percent_complete, int& priority )
{
    kDebug(5970) << "Entering function";
    bool ok;
    name = incident->summary();
    mUid = incident->uid();
    mComment = incident->description();
    ok = false;

    // if a KDE-karm-duration exists and not KDE-ktimetracker-duration, change this
    if (
        incident->customProperty( KGlobal::mainComponent().componentName().toUtf8(),
        QByteArray( "totalTaskTime" )) == QString::null && incident->customProperty( "karm",
        QByteArray( "totalTaskTime" )) != QString::null )
            incident->setCustomProperty(
                KGlobal::mainComponent().componentName().toUtf8(),
                QByteArray( "totalTaskTime" ), incident->customProperty( "karm",
                QByteArray( "totalTaskTime" )));
                minutes = incident->customProperty( KGlobal::mainComponent().componentName().toUtf8(),
                QByteArray( "totalTaskTime" )).toInt( &ok );
    if ( !ok )
    minutes = 0;
    ok = false;

    // if a KDE-karm-totalSessionTime exists and not KDE-ktimetracker-totalSessionTime, change this
    if (
        incident->customProperty( KGlobal::mainComponent().componentName().toUtf8(),
        QByteArray( "totalSessionTime" )) == QString::null && incident->customProperty( "karm",
        QByteArray( "totalSessionTime" )) != QString::null )
            incident->setCustomProperty(
                KGlobal::mainComponent().componentName().toUtf8(),
                QByteArray( "totalSessionTime" ), incident->customProperty( "karm",
                QByteArray( "totalSessionTime" )));
    sessionMinutes = incident->customProperty( KGlobal::mainComponent().componentName().toUtf8(),
        QByteArray( "totalSessionTime" )).toInt( &ok );
    if ( !ok )
        sessionMinutes = 0;
    sessionStartTiMe=incident->customProperty( KGlobal::mainComponent().componentName().toUtf8(), QByteArray( "sessionStartTiMe" ));

    // if a KDE-karm-deskTopList exists and no KDE-ktimetracker-DeskTopList, change this
    if (
        incident->customProperty( KGlobal::mainComponent().componentName().toUtf8(),
        QByteArray( "desktopList" )) == QString::null && incident->customProperty( "karm",
        QByteArray( "desktopList" )) != QString::null )
            incident->setCustomProperty(
                KGlobal::mainComponent().componentName().toUtf8(),
                QByteArray( "desktopList" ), incident->customProperty( "karm",
                QByteArray( "desktopList" )));

    QString desktopList = incident->customProperty( KGlobal::mainComponent().componentName().toUtf8(),
        QByteArray( "desktopList" ) );
    QStringList desktopStrList = desktopList.split( QString::fromLatin1(","),
        QString::SkipEmptyParts );
    desktops.clear();

    for ( QStringList::iterator iter = desktopStrList.begin();
        iter != desktopStrList.end();
        ++iter )
    {
        int desktopInt = (*iter).toInt( &ok );
        if ( ok )
        {
            desktops.push_back( desktopInt );
        }
    }
    percent_complete = static_cast<KCal::Todo*>(incident)->percentComplete();
    priority = incident->priority();
    return true;
}

QString Task::getDesktopStr() const
{
    if ( mDesktops.empty() )
        return QString();

    QString desktopstr;
    for ( DesktopList::const_iterator iter = mDesktops.begin();
        iter != mDesktops.end();
        ++iter )
    {
        desktopstr += QString::number( *iter ) + QString::fromLatin1( "," );
    }
    desktopstr.remove( desktopstr.length() - 1, 1 );
    return desktopstr;
}

void Task::cut()
// This is needed e.g. to move a task under its parent when loading.
{
    kDebug(5970) << "Entering function";
    changeParentTotalTimes( -mTotalSessionTime, -mTotalTime);
    if ( ! parent() )
        treeWidget()->takeTopLevelItem(treeWidget()->indexOfTopLevelItem(this));
    else
        parent()->takeChild(indexOfChild(this));
    kDebug(5970) << "Leaving function";
}

void Task::paste(Task* destination)
// This is needed e.g. to move a task under its parent when loading.
{
    kDebug(5970) << "Entering function";
    destination->QTreeWidgetItem::insertChild(0,this);
    changeParentTotalTimes( mTotalSessionTime, mTotalTime);
    kDebug(5970) << "Leaving function";
}

void Task::move(Task* destination)
// This is used e.g. to move each task under its parent after loading.
{
    kDebug(5970) << "Entering function";
    cut();
    paste(destination);
    kDebug(5970) << "Leaving function";
}

void Task::update()
// Update a row, containing one task
{
    kDebug( 5970 ) << "Entering function";
    bool b = KTimeTrackerSettings::decimalFormat();
    setText( 0, mName );
    setText( 1, formatTime( mSessionTime, b ) );
    setText( 2, formatTime( mTime, b ) );
    setText( 3, formatTime( mTotalSessionTime, b ) );
    setText( 4, formatTime( mTotalTime, b ) );
    setText( 5, mPriority > 0 ? QString::number( mPriority ) : "--" );
    setText( 6, QString::number( mPercentComplete ) );
    kDebug( 5970 ) << "Leaving function";
}

void Task::addComment( const QString &comment, timetrackerstorage* storage )
{
    mComment = mComment + QString::fromLatin1("\n") + comment;
    storage->addComment(this, comment);
}

void Task::startNewSession()
{
    changeTimes( -mSessionTime, 0 );
    mSessionStartTiMe=KDateTime::currentLocalDateTime();
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

QDateTime Task::startTime() const
{
    return mLastStart;
}

long Task::time() const
{
    return mTime;
}

long Task::totalTime() const
{
    return mTotalTime;
}

long Task::sessionTime() const
{
    return mSessionTime;
}

long Task::totalSessionTime() const
{
    return mTotalSessionTime;
}

KDateTime Task::sessionStartTiMe() const
{
    return mSessionStartTiMe;
}

DesktopList Task::desktops() const
{
    return mDesktops;
}
//END

#include "task.moc"
