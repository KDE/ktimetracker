#include<ctype.h>
#include<qfile.h>
#include<stdlib.h>
#include<stdio.h>
#include <kiconloader.h>
#include <qtimer.h>
#include"task.h"


QVector<QPixmap> *Task::icons = 0;

Task::Task(const QString& taskName, long minutes, long sessionTime, QListView *parent) 
	: QObject(), QListViewItem(parent)
{
  init(taskName, minutes, sessionTime);
};

Task::Task(const QString& taskName, long minutes, long sessionTime, QListViewItem *parent)
  :QObject(), QListViewItem(parent)
{
  init(taskName, minutes, sessionTime);
}

void Task::init(const QString& taskName, long minutes, long sessionTime)
{
  if (icons == 0) {
    icons = new QVector<QPixmap>(8);
    for (int i=0; i<8; i++) {
      QPixmap *icon = new QPixmap();
      QString name;
      name.sprintf("watch-%d.xpm",i);
      *icon = UserIcon(name);
      icons->insert(i,icon);
    }
  }
  
	_name = taskName.stripWhiteSpace(); 
	_totalTime = minutes;
  _sessionTime = sessionTime;
  _timer = new QTimer(this);
  connect(_timer, SIGNAL(timeout()), this, SLOT(updateActiveIcon()));
  setPixmap(1, UserIcon(QString::fromLatin1("empty-watch.xpm")));
	update();
  _i = 0;
}


void Task::setRunning(bool on)
{
  if (on) {
    if (!_timer->isActive()) {
      _timer->start(1000);
      _i=7;
      updateActiveIcon();
    }
  }
  else {
    if (_timer->isActive()) {
      _timer->stop();
      setPixmap(1, UserIcon(QString::fromLatin1("empty-watch.xpm")));
    }
  }
}

void Task::setName( const QString& name ) 
{
	_name = name;
	update();
}

void Task::setTotalTime ( long minutes )
{
	_totalTime = minutes;
	update();
}

void Task::setSessionTime ( long minutes )
{
	_sessionTime = minutes;
	update();
}


void Task::incrementTime( long minutes )
{
	_totalTime += minutes;
  _sessionTime += minutes;
	update();
}

void Task::decrementTime(long minutes)
{
	_totalTime -= minutes;
  _sessionTime -= minutes;
	update();
}


void Task::updateActiveIcon()
{
  _i = (_i+1) % 8;
  setPixmap(1, *(*icons)[_i]);
}
