#include <q3cstring.h>
#include <qdatetime.h>
#include <qstring.h>
#include <qtimer.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3PtrList>

#include <kiconloader.h>

#include "kapplication.h"       // kapp
#include "kdebug.h"

#include "event.h"

#include "karmutility.h"
#include "task.h"
#include "taskview.h"
#include "preferences.h"


const int gSecondsPerMinute = 60;


Q3PtrVector<QPixmap> *Task::icons = 0;

Task::Task( const QString& taskName, long minutes, long sessionTime,
            DesktopList desktops, TaskView *parent)
  : QObject(), Q3ListViewItem(parent)
{ 
  init(taskName, minutes, sessionTime, desktops, 0);
}

Task::Task( const QString& taskName, long minutes, long sessionTime,
            DesktopList desktops, Task *parent)
  : QObject(), Q3ListViewItem(parent)
{
  init(taskName, minutes, sessionTime, desktops, 0);
}

Task::Task( KCal::Todo* todo, TaskView* parent )
  : QObject(), Q3ListViewItem( parent )
{
  long minutes = 0;
  QString name;
  long sessionTime = 0;
  int percent_complete = 0;
  DesktopList desktops;

  parseIncidence(todo, minutes, sessionTime, name, desktops, percent_complete);
  init(name, minutes, sessionTime, desktops, percent_complete);
}

void Task::init( const QString& taskName, long minutes, long sessionTime,
                 DesktopList desktops, int percent_complete)
{
  // If our parent is the taskview then connect our totalTimesChanged
  // signal to its receiver
  if ( ! parent() )
    connect( this, SIGNAL( totalTimesChanged ( long, long ) ),
             listView(), SLOT( taskTotalTimesChanged( long, long) ));

  connect( this, SIGNAL( deletingTask( Task* ) ),
           listView(), SLOT( deletingTask( Task* ) ));

  if (icons == 0) {
    icons = new Q3PtrVector<QPixmap>(8);
    KIconLoader* kil = new KIconLoader("karm"); // always load icons from the KArm application
    for (int i=0; i<8; i++)
    {
      QPixmap *icon = new QPixmap();
      QString name;
      name.sprintf("watch-%d.xpm",i);
      *icon = kil->loadIcon( name, KIcon::User );
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
  setPixmap(1, UserIcon(QString::fromLatin1("empty-watch.xpm")));
  _currentPic = 0;
  _percentcomplete = percent_complete;

  update();
  changeParentTotalTimes( _sessionTime, _time);
}

Task::~Task() {
  emit deletingTask(this);
  delete _timer;
}

void Task::setRunning( bool on, KarmStorage* storage, QDateTime whenStarted  )
{
  if ( on ) {
    if (isComplete()) return; // don't start if its marked complete
    if (!_timer->isActive()) {
      _timer->start(1000);
      storage->startTimer(this);
      _currentPic=7;
      _lastStart = whenStarted;
      updateActiveIcon();
    }
  }
  else {
    if (_timer->isActive()) {
      _timer->stop();
      if ( ! _removing ) {
        storage->stopTimer(this);
        setPixmap(1, UserIcon(QString::fromLatin1("empty-watch.xpm")));
      }
    }
  }
}

void Task::setUid(QString uid) {
  _uid = uid;
}

bool Task::isRunning() const
{
  return _timer->isActive();
}

void Task::setName( const QString& name, KarmStorage* storage )
{
  kDebug(5970) << "Task:setName: " << name << endl;

  QString oldname = _name;
  if ( oldname != name ) {
    _name = name;
    storage->setName(this, oldname);
    update();
  }
}

void Task::setPercentComplete(const int percent, KarmStorage *storage)
{
  kDebug(5970) << "Task::setPercentComplete(" << percent << ", storage): "
    << _uid << endl;

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
}

void Task::setPixmapProgress()
{
  QPixmap* icon = new QPixmap();
  if (_percentcomplete >= 100)
    *icon = UserIcon("task-complete.xpm");
  else
    *icon = UserIcon("task-incomplete.xpm");
  setPixmap(0, *icon);
}

bool Task::isComplete() { return _percentcomplete == 100; }

void Task::removeFromView()
{
  for (Task* child= this->firstChild(); child; child= child->nextSibling())
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
    << "Task::changeTotalTimes(" << minutesSession << ", "
    << minutes << ") for " << name() << endl;

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
  //  << "Task::changeParentTotalTimes(" << minutesSession << ", "
  //  << minutes << ") for " << name() << endl;

  if ( isRoot() )
    emit totalTimesChanged( minutesSession, minutes );
  else
    parent()->changeTotalTimes( minutesSession, minutes );
}

bool Task::remove( Q3PtrList<Task>& activeTasks, KarmStorage* storage)
{
  kDebug(5970) << "Task::remove: " << _name << endl;

  bool ok = true;

  _removing = true;
  storage->removeTask(this);
  if( isRunning() ) setRunning( false, storage );

  for (Task* child = this->firstChild(); child; child = child->nextSibling())
  {
    if (child->isRunning())
      child->setRunning(false, storage);
    child->remove(activeTasks, storage);
  }

  changeParentTotalTimes( -_sessionTime, -_time);

  _removing = false;

  return ok;
}

void Task::updateActiveIcon()
{
  _currentPic = (_currentPic+1) % 8;
  setPixmap(1, *(*icons)[_currentPic]);
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

  kDebug(5970) << "Task::asTodo: name() = '" << name() << "'" << endl;
  todo->setSummary( name() );

  // Note: if the date start is empty, the KOrganizer GUI will have the
  // checkbox blank, but will prefill the todo's starting datetime to the
  // time the file is opened.
  // todo->setDtStart( current );

  todo->setCustomProperty( kapp->instanceName(),
      QByteArray( "totalTaskTime" ), QString::number( _time ) );
  todo->setCustomProperty( kapp->instanceName(),
      QByteArray( "totalSessionTime" ), QString::number( _sessionTime) );

  if (getDesktopStr().isEmpty())
    todo->removeCustomProperty(kapp->instanceName(), QByteArray("desktopList"));
  else
    todo->setCustomProperty( kapp->instanceName(),
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
  minutes = incident->customProperty( kapp->instanceName(),
      QByteArray( "totalTaskTime" )).toInt( &ok );
  if ( !ok )
    minutes = 0;

  ok = false;
  sessionMinutes = incident->customProperty( kapp->instanceName(),
      QByteArray( "totalSessionTime" )).toInt( &ok );
  if ( !ok )
    sessionMinutes = 0;

  QString desktopList = incident->customProperty( kapp->instanceName(),
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

  //kDebug(5970) << "Task::parseIncidence: "
  //  << name << ", Minutes: " << minutes
  //  <<  ", desktop: " << desktopList << endl;

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
{
  //kDebug(5970) << "Task::cut - " << name() << endl;
  changeParentTotalTimes( -_totalSessionTime, -_totalTime);
  if ( ! parent())
    listView()->takeItem(this);
  else
    parent()->takeItem(this);
}

void Task::move(Task* destination)
{
  cut();
  paste(destination);
}

void Task::paste(Task* destination)
{
  destination->insertItem(this);
  changeParentTotalTimes( _totalSessionTime, _totalTime);
}

void Task::update()
{
  setText(0, _name);
  setText(1, formatTime(_sessionTime));
  setText(2, formatTime(_time));
  setText(3, formatTime(_totalSessionTime));
  setText(4, formatTime(_totalTime));
}

void Task::addComment( QString comment, KarmStorage* storage )
{
  _comment = _comment + QString::fromLatin1("\n") + comment;
  storage->addComment(this, comment);
}

QString Task::comment() const
{
  return _comment;
}

int Task::compare ( Q3ListViewItem * i, int col, bool ascending ) const
{
  long thistime = 0;
  long thattime = 0;
  Task *task = static_cast<Task*>(i);

  switch ( col )
  {
    case 1: 
      thistime = _sessionTime;
      thattime = task->sessionTime();
      break;
    case 2:
      thistime = _time;
      thattime = task->time();
      break;
    case 3:
      thistime = _totalSessionTime;
      thattime = task->totalSessionTime();
      break;
    case 4:
      thistime = _totalTime;
      thattime = task->totalTime();
      break;
    default:
      return key(col, ascending).localeAwareCompare( i->key(col, ascending) );
  }

  if ( thistime < thattime ) return -1;
  if ( thistime > thattime ) return 1;
  return 0;

}

#include "task.moc"
