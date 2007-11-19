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

#include <KApplication>
#include <KDebug>
#include <KIconLoader>

#include <kcal/event.h>

#include "karmutility.h"
#include "ktimetracker.h"
#include "preferences.h"

//BEGIN private data
//@cond PRIVATE
class Task::Private {
  public:
    Private() {}

    /** The iCal unique ID of the Todo for this task. */
    QString mUid;

    /** The comment associated with this Task. */
    QString mComment;

    int mPercentComplete;

    /** task name */
    QString mName;

    /** Last time this task was started. */
    QDateTime mLastStart;

    /** totals of the whole subtree including self */
    long mTotalTime;
    long mTotalSessionTime;

    /** times spend on the task itself */
    long mTime;
    long mSessionTime;

    DesktopList mDesktops;
    QTimer *mTimer;
    int mCurrentPic;

    /** Don't need to update storage when deleting task from list. */
    bool mRemoving;

    /** Priority of the task. */
    int mPriority;
};
//@endcond
//END

const int gSecondsPerMinute = 60;

QVector<QPixmap*> *Task::icons = 0;

Task::Task( const QString& taskName, long minutes, long sessionTime,
            DesktopList desktops, TaskView *parent)
  : QObject(), QTreeWidgetItem(parent), d( new Private() )
{ 
  init( taskName, minutes, sessionTime, desktops, 0, 0 );
}

Task::Task( const QString& taskName, long minutes, long sessionTime,
            DesktopList desktops, Task *parent)
  : QObject(), QTreeWidgetItem(parent), d( new Private() )
{
  init( taskName, minutes, sessionTime, desktops, 0, 0 );
}

Task::Task( KCal::Todo* todo, TaskView* parent )
  : QObject(), QTreeWidgetItem( parent ), d( new Private() )
{
  long minutes = 0;
  QString name;
  long sessionTime = 0;
  int percent_complete = 0;
  int priority = 0;
  DesktopList desktops;

  parseIncidence( todo, minutes, sessionTime, name, desktops, percent_complete,
                  priority );
  init( name, minutes, sessionTime, desktops, percent_complete, priority );
}

int Task::depth()
// Deliver the depth of a task, e.g. how many tasks are supertasks to it. 
// A toplevel task has the depth 0.
{
  kDebug(5970) <<"Entering Task::depth";
  int res=0;
  Task* t=this;
  while ( ( t = t->parent() ) ) res++;
  kDebug(5970) <<"depth is" << res;
  return res;
}

void Task::init( const QString& taskName, long minutes, long sessionTime,
                 DesktopList desktops, int percent_complete, int priority )
{
  // If our parent is the taskview then connect our totalTimesChanged
  // signal to its receiver
  if ( ! parent() )
    connect( this, SIGNAL( totalTimesChanged ( long, long ) ),
             treeWidget(), SLOT( taskTotalTimesChanged( long, long) ));

  connect( this, SIGNAL( deletingTask( Task* ) ),
           treeWidget(), SLOT( deletingTask( Task* ) ));

  if (icons == 0) {
    icons = new QVector<QPixmap*>(8);
    KIconLoader* kil = new KIconLoader("karm"); // always load icons from the KArm application
    for (int i=0; i<8; i++)
    {
      QPixmap *icon = new QPixmap();
      QString name;
      name.sprintf("watch-%d.xpm",i);
      *icon = kil->loadIcon( name, KIconLoader::User );
      icons->insert(i,icon);
    }
  }

  d->mRemoving = false;
  d->mName = taskName.trimmed();
  d->mLastStart = QDateTime::currentDateTime();
  d->mTotalTime = d->mTime = minutes;
  d->mTotalSessionTime = d->mSessionTime = sessionTime;
  d->mTimer = new QTimer(this);
  d->mDesktops = desktops;
  connect(d->mTimer, SIGNAL(timeout()), this, SLOT(updateActiveIcon()));
  setIcon(1, UserIcon(QString::fromLatin1("empty-watch.xpm")));
  d->mCurrentPic = 0;
  d->mPercentComplete = percent_complete;
  d->mPriority = priority;

  update();
  changeParentTotalTimes( d->mSessionTime, d->mTime);
  
  // alignment of the number items
  for (int i = 1; i < columnCount(); ++i) {
      setTextAlignment( i, Qt::AlignRight );
  }

  // .. but not the priority column
  setTextAlignment( 5, Qt::AlignCenter );
}

Task::~Task() {
  emit deletingTask(this);
  delete d->mTimer;
}

void Task::setRunning( bool on, KarmStorage* storage, const QDateTime &when )
{
  kDebug(5970) <<"Entering Task::setRunning"; 
  if ( on ) 
  {
    if (isComplete()) return; // don't start if its marked complete
    if (!d->mTimer->isActive()) 
    {
      d->mTimer->start(1000);
      storage->startTimer(this);
      d->mCurrentPic=7;
      d->mLastStart = when;
      updateActiveIcon();
    }
  }
  else 
  {
    if (d->mTimer->isActive()) 
    {
      d->mTimer->stop();
      if ( ! d->mRemoving ) 
      {
        storage->stopTimer(this, when);
        setIcon(1, UserIcon(QString::fromLatin1("empty-watch.xpm")));
      }
    }
  }
}

void Task::setUid( const QString &uid ) {
  d->mUid = uid;
}

bool Task::isRunning() const
{
  return d->mTimer->isActive();
}

void Task::setName( const QString& name, KarmStorage* storage )
{
  kDebug(5970) <<"Task:setName:" << name;

  QString oldname = d->mName;
  if ( oldname != name ) {
    d->mName = name;
    storage->setName(this, oldname);
    update();
  }
}

void Task::setPercentComplete(const int percent, KarmStorage *storage)
{
  kDebug(5970) <<"Task::setPercentComplete(" << percent <<", storage):"
    << d->mUid;

  if (!percent)
    d->mPercentComplete = 0;
  else if (percent > 100)
    d->mPercentComplete = 100;
  else if (percent < 0)
    d->mPercentComplete = 0;
  else
    d->mPercentComplete = percent;

  if (isRunning() && d->mPercentComplete==100) taskView()->stopTimerFor(this);

  setPixmapProgress();

  // When parent marked as complete, mark all children as complete as well.
  // Complete tasks are not displayed in the task view, so if a parent is
  // marked as complete and some of the children are not, then we get an error
  // message.  KArm actually keep chugging along in this case and displays the
  // child tasks just fine, so an alternative solution is to remove that error
  // message (from KarmStorage::load).  But I think it makes more sense that
  // if you mark a parent task as complete, then all children should be
  // complete as well.
  //
  // This behavior is consistent with KOrganizer (as of 2003-09-24).
  if (d->mPercentComplete == 100)
  {
    for ( int i = 0; i < childCount(); ++i ) {
      Task *task = static_cast< Task* >( child( i ) );
      task->setPercentComplete(d->mPercentComplete, storage);
    }
  }
  // maybe there is a column "percent completed", so do a ...
  update();
}

void Task::setPriority( int priority )
{
  if ( priority < 0 ) {
    priority = 0;
  } else if ( priority > 9 ) {
    priority = 9;
  }

  d->mPriority = priority;
  update();
}

void Task::setPixmapProgress()
{
  QPixmap* icon = new QPixmap();
  if (d->mPercentComplete >= 100)
    *icon = UserIcon("task-complete.xpm");
  else
    *icon = UserIcon("task-incomplete.xpm");
  setIcon(0, *icon);
}

bool Task::isComplete() { return d->mPercentComplete == 100; }

void Task::removeFromView()
{
  while ( childCount() > 0 ) {
    static_cast< Task* >( child( 0 ) )->removeFromView();
  }
  delete this;
}

void Task::setDesktopList ( DesktopList desktopList )
{
  d->mDesktops = desktopList;
}

void Task::changeTime( long minutes, KarmStorage* storage )
{
  changeTimes( minutes, minutes, storage); 
}

QString Task::setTime( long minutes, KarmStorage* storage )
{
  QString err=QString();
  d->mTime=minutes;
  d->mTotalTime+=minutes;
  return err;
}

void Task::changeTimes( long minutesSession, long minutes, KarmStorage* storage)
{
  kDebug(5970) <<"Entering Task::changeTimes";
  if( minutesSession != 0 || minutes != 0) 
  {
    d->mSessionTime += minutesSession;
    d->mTime += minutes;
    if ( storage ) storage->changeTime(this, minutes * gSecondsPerMinute);
    changeTotalTimes( minutesSession, minutes );
  }
}

void Task::changeTotalTimes( long minutesSession, long minutes )
{
  kDebug(5970)
    << "Task::changeTotalTimes(" << minutesSession << ","
    << minutes << ") for" << name();

  d->mTotalSessionTime += minutesSession;
  d->mTotalTime += minutes;
  update();
  changeParentTotalTimes( minutesSession, minutes );
}

void Task::resetTimes()
{
  d->mTotalSessionTime -= d->mSessionTime;
  d->mTotalTime -= d->mTime;
  changeParentTotalTimes( -d->mSessionTime, -d->mTime);
  d->mSessionTime = 0;
  d->mTime = 0;
  update();
}

void Task::changeParentTotalTimes( long minutesSession, long minutes )
{
  //kDebug(5970)
  //  << "Task::changeParentTotalTimes(" << minutesSession << ","
  //  << minutes << ") for" << name();

  if ( isRoot() )
    emit totalTimesChanged( minutesSession, minutes );
  else
    parent()->changeTotalTimes( minutesSession, minutes );
}

bool Task::remove( KarmStorage* storage)
{
  kDebug(5970) <<"Task::remove:" << d->mName;

  bool ok = true;

  d->mRemoving = true;
  storage->removeTask(this);
  if( isRunning() ) setRunning( false, storage );

  for ( int i = 0; i < childCount(); ++i ) {
    Task *task = static_cast< Task* >( child( i ) );
    if ( task->isRunning() )
      task->setRunning( false, storage );
    task->remove( storage );
  }

  changeParentTotalTimes( -d->mSessionTime, -d->mTime);

  d->mRemoving = false;

  return ok;
}

void Task::updateActiveIcon()
{
  d->mCurrentPic = (d->mCurrentPic+1) % 8;
  setIcon(1, *(*icons)[d->mCurrentPic]);
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
      QByteArray( "totalTaskTime" ), QString::number( d->mTime ) );
  todo->setCustomProperty( KGlobal::mainComponent().componentName().toUtf8(),
      QByteArray( "totalSessionTime" ), QString::number( d->mSessionTime) );

  if (getDesktopStr().isEmpty())
    todo->removeCustomProperty(KGlobal::mainComponent().componentName().toUtf8(), QByteArray("desktopList"));
  else
    todo->setCustomProperty( KGlobal::mainComponent().componentName().toUtf8(),
        QByteArray( "desktopList" ), getDesktopStr() );

  todo->setOrganizer( KTimeTrackerSettings::userRealName() );

  todo->setPercentComplete(d->mPercentComplete);
  todo->setPriority( d->mPriority );

  return todo;
}

bool Task::parseIncidence( KCal::Incidence* incident, long& minutes,
    long& sessionMinutes, QString& name, DesktopList& desktops,
    int& percent_complete, int& priority )
{
  bool ok;

  name = incident->summary();
  d->mUid = incident->uid();

  d->mComment = incident->description();

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
        ++iter ) {
    int desktopInt = (*iter).toInt( &ok );
    if ( ok ) {
      desktops.push_back( desktopInt );
    }
  }

  percent_complete = static_cast<KCal::Todo*>(incident)->percentComplete();
  priority = incident->priority();

  //kDebug(5970) <<"Task::parseIncidence:"
  //  << name << ", Minutes:" << minutes
  //  <<  ", desktop:" << desktopList;

  return true;
}

QString Task::getDesktopStr() const
{
  if ( d->mDesktops.empty() )
    return QString();

  QString desktopstr;
  for ( DesktopList::const_iterator iter = d->mDesktops.begin();
        iter != d->mDesktops.end();
        ++iter ) {
    desktopstr += QString::number( *iter ) + QString::fromLatin1( "," );
  }
  desktopstr.remove( desktopstr.length() - 1, 1 );
  return desktopstr;
}

void Task::cut()
// This is needed e.g. to move a task under its parent when loading.
{
  kDebug(5970) <<"Task::cut -" << name();

  changeParentTotalTimes( -d->mTotalSessionTime, -d->mTotalTime);
  if ( ! parent())
    treeWidget()->takeTopLevelItem(treeWidget()->indexOfTopLevelItem(this));
  else
    parent()->takeChild(indexOfChild(this));

}

void Task::paste(Task* destination)
// This is needed e.g. to move a task under its parent when loading.
{
  kDebug(5970) <<"Entering Task::paste";

  destination->QTreeWidgetItem::insertChild(0,this);
  changeParentTotalTimes( d->mTotalSessionTime, d->mTotalTime);

  kDebug(5970) <<"Leaving Task::paste";
}

void Task::move(Task* destination)
// This is used e.g. to move each task under its parent after loading.
{
  kDebug(5970) <<"Entering Task::move";
  cut();
  paste(destination);
  kDebug(5970) <<"Leaving Task::move";
}

void Task::update()
// Update a row, containing one task
{
  kDebug( 5970 ) << "Entering Task::update";
  bool b = KTimeTrackerSettings::decimalFormat();
  setText( 0, d->mName );
  setText( 1, formatTime( d->mSessionTime, b ) );
  setText( 2, formatTime( d->mTime, b ) );
  setText( 3, formatTime( d->mTotalSessionTime, b ) );
  setText( 4, formatTime( d->mTotalTime, b ) );
  setText( 5, d->mPriority > 0 ? QString::number( d->mPriority ) : "--" );
  setText( 6, QString::number( d->mPercentComplete ) );
  kDebug( 5970 ) << "Exiting Task::update";
}

void Task::addComment( const QString &comment, KarmStorage* storage )
{
  d->mComment = d->mComment + QString::fromLatin1("\n") + comment;
  storage->addComment(this, comment);
}

void Task::startNewSession()
{
  changeTimes( -d->mSessionTime, 0 );
}

//BEGIN Properties
QString Task::uid() const
{
  return d->mUid;
}

QString Task::comment() const
{
  return d->mComment;
}

int Task::percentComplete() const
{
  return d->mPercentComplete;
}

int Task::priority() const
{
  return d->mPriority;
}

QString Task::name() const
{
  return d->mName;
}

QDateTime Task::startTime() const
{
  return d->mLastStart;
}

long Task::time() const
{
  return d->mTime;
}

long Task::totalTime() const
{
  return d->mTotalTime;
}

long Task::sessionTime() const
{
  return d->mSessionTime;
}

long Task::totalSessionTime() const
{
  return d->mTotalSessionTime;
}

DesktopList Task::desktops() const
{
  return d->mDesktops;
}
//END

#include "task.moc"
