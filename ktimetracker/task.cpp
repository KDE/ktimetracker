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
#include "preferences.h"

const int gSecondsPerMinute = 60;


QVector<QPixmap*> *Task::icons = 0;

Task::Task( const QString& taskName, long minutes, long sessionTime,
            DesktopList desktops, TaskView *parent)
  : QObject(), QTreeWidgetItem(parent)
{ 
  init(taskName, minutes, sessionTime, desktops, 0);
}

Task::Task( const QString& taskName, long minutes, long sessionTime,
            DesktopList desktops, Task *parent)
  : QObject(), QTreeWidgetItem(parent)
{
  init(taskName, minutes, sessionTime, desktops, 0);
}

Task::Task( KCal::Todo* todo, TaskView* parent )
  : QObject(), QTreeWidgetItem( parent )
{
  long minutes = 0;
  QString name;
  long sessionTime = 0;
  int percent_complete = 0;
  DesktopList desktops;

  parseIncidence(todo, minutes, sessionTime, name, desktops, percent_complete);
  init(name, minutes, sessionTime, desktops, percent_complete);
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
                 DesktopList desktops, int percent_complete)
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
      *icon = kil->loadIcon( name, K3Icon::User );
      icons->insert(i,icon);
    }
  }

  _removing = false;
  _name = taskName.trimmed();
  _lastStart = QDateTime::currentDateTime();
  _totalTime = _time = minutes;
  _totalSessionTime = _sessionTime = sessionTime;
  _timer = new QTimer(this);
  _desktops = desktops;
  connect(_timer, SIGNAL(timeout()), this, SLOT(updateActiveIcon()));
  setIcon(1, UserIcon(QString::fromLatin1("empty-watch.xpm")));
  _currentPic = 0;
  _percentcomplete = percent_complete;

  update();
  changeParentTotalTimes( _sessionTime, _time);
  
  // alignment of the number items
  for (int i = 1; i < columnCount(); ++i) {
      setTextAlignment( i, Qt::AlignRight );
  }
}

Task::~Task() {
  emit deletingTask(this);
  delete _timer;
}

void Task::setRunning( bool on, KarmStorage* storage, const QDateTime &when )
{
  kDebug(5970) <<"Entering Task::setRunning"; 
  if ( on ) 
  {
    if (isComplete()) return; // don't start if its marked complete
    if (!_timer->isActive()) 
    {
      _timer->start(1000);
      storage->startTimer(this);
      _currentPic=7;
      _lastStart = when;
      updateActiveIcon();
    }
  }
  else 
  {
    if (_timer->isActive()) 
    {
      _timer->stop();
      if ( ! _removing ) 
      {
        storage->stopTimer(this, when);
        setIcon(1, UserIcon(QString::fromLatin1("empty-watch.xpm")));
      }
    }
  }
}

void Task::setUid( const QString &uid ) {
  _uid = uid;
}

bool Task::isRunning() const
{
  return _timer->isActive();
}

void Task::setName( const QString& name, KarmStorage* storage )
{
  kDebug(5970) <<"Task:setName:" << name;

  QString oldname = _name;
  if ( oldname != name ) {
    _name = name;
    storage->setName(this, oldname);
    update();
  }
}

void Task::setPercentComplete(const int percent, KarmStorage *storage)
{
  kDebug(5970) <<"Task::setPercentComplete(" << percent <<", storage):"
    << _uid;

  if (!percent)
    _percentcomplete = 0;
  else if (percent > 100)
    _percentcomplete = 100;
  else if (percent < 0)
    _percentcomplete = 0;
  else
    _percentcomplete = percent;

  if (isRunning() && _percentcomplete==100) setRunning(false, storage);

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
  if (_percentcomplete == 100)
  {
    for (Task* child= this->firstChild(); child; child = child->nextSibling())
      child->setPercentComplete(_percentcomplete, storage);
  }
  // maybe there is a column "percent completed", so do a ...
  update();
}

void Task::setPixmapProgress()
{
  QPixmap* icon = new QPixmap();
  if (_percentcomplete >= 100)
    *icon = UserIcon("task-complete.xpm");
  else
    *icon = UserIcon("task-incomplete.xpm");
  setIcon(0, *icon);
}

bool Task::isComplete() { return _percentcomplete == 100; }

void Task::removeFromView()
{
  while (Task* child = this->firstChild())
    child->removeFromView();
  delete this;
}

void Task::setDesktopList ( DesktopList desktopList )
{
  _desktops = desktopList;
}

void Task::changeTime( long minutes, KarmStorage* storage )
{
  changeTimes( minutes, minutes, storage); 
}

void Task::changeTimes( long minutesSession, long minutes, KarmStorage* storage)
{
  kDebug(5970) <<"Entering Task::changeTimes";
  if( minutesSession != 0 || minutes != 0) 
  {
    _sessionTime += minutesSession;
    _time += minutes;
    if ( storage ) storage->changeTime(this, minutes * gSecondsPerMinute);
    changeTotalTimes( minutesSession, minutes );
  }
}

void Task::changeTotalTimes( long minutesSession, long minutes )
{
  kDebug(5970)
    << "Task::changeTotalTimes(" << minutesSession << ","
    << minutes << ") for" << name();

  _totalSessionTime += minutesSession;
  _totalTime += minutes;
  update();
  changeParentTotalTimes( minutesSession, minutes );
}

void Task::resetTimes()
{
  _totalSessionTime -= _sessionTime;
  _totalTime -= _time;
  changeParentTotalTimes( -_sessionTime, -_time);
  _sessionTime = 0;
  _time = 0;
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
  kDebug(5970) <<"Task::remove:" << _name;

  bool ok = true;

  _removing = true;
  storage->removeTask(this);
  if( isRunning() ) setRunning( false, storage );

  for (Task* child = this->firstChild(); child; child = child->nextSibling())
  {
    if (child->isRunning())
      child->setRunning(false, storage);
    child->remove(storage);
  }

  changeParentTotalTimes( -_sessionTime, -_time);

  _removing = false;

  return ok;
}

void Task::updateActiveIcon()
{
  _currentPic = (_currentPic+1) % 8;
  setIcon(1, *(*icons)[_currentPic]);
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
      QByteArray( "totalTaskTime" ), QString::number( _time ) );
  todo->setCustomProperty( KGlobal::mainComponent().componentName().toUtf8(),
      QByteArray( "totalSessionTime" ), QString::number( _sessionTime) );

  if (getDesktopStr().isEmpty())
    todo->removeCustomProperty(KGlobal::mainComponent().componentName().toUtf8(), QByteArray("desktopList"));
  else
    todo->setCustomProperty( KGlobal::mainComponent().componentName().toUtf8(),
        QByteArray( "desktopList" ), getDesktopStr() );

  todo->setOrganizer( Preferences::instance()->userRealName() );

  todo->setPercentComplete(_percentcomplete);

  return todo;
}

bool Task::parseIncidence( KCal::Incidence* incident, long& minutes,
    long& sessionMinutes, QString& name, DesktopList& desktops,
    int& percent_complete )
{
  bool ok;

  name = incident->summary();
  _uid = incident->uid();

  _comment = incident->description();

  ok = false;
  minutes = incident->customProperty( KGlobal::mainComponent().componentName().toUtf8(),
      QByteArray( "totalTaskTime" )).toInt( &ok );
  if ( !ok )
    minutes = 0;

  ok = false;
  sessionMinutes = incident->customProperty( KGlobal::mainComponent().componentName().toUtf8(),
      QByteArray( "totalSessionTime" )).toInt( &ok );
  if ( !ok )
    sessionMinutes = 0;

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

  //kDebug(5970) <<"Task::parseIncidence:"
  //  << name << ", Minutes:" << minutes
  //  <<  ", desktop:" << desktopList;

  return true;
}

QString Task::getDesktopStr() const
{
  if ( _desktops.empty() )
    return QString();

  QString desktopstr;
  for ( DesktopList::const_iterator iter = _desktops.begin();
        iter != _desktops.end();
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

  changeParentTotalTimes( -_totalSessionTime, -_totalTime);
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
  changeParentTotalTimes( _totalSessionTime, _totalTime);

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
  kDebug(5970) <<"Entering Task::update";
  bool b=taskView()->preferences()->decimalFormat();
  setText(0, _name);
  setText(1, formatTime(_sessionTime, b));
  setText(2, formatTime(_time, b));
  setText(3, formatTime(_totalSessionTime, b));
  setText(4, formatTime(_totalTime, b));
  setText(5, QString::number(_percentcomplete));
  kDebug(5970) <<"Exiting Task::update";
}

void Task::addComment( const QString &comment, KarmStorage* storage )
{
  _comment = _comment + QString::fromLatin1("\n") + comment;
  storage->addComment(this, comment);
}

QString Task::comment() const
{
  return _comment;
}

#include "task.moc"
