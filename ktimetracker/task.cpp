#include<ctype.h>
#include<qfile.h>
#include<stdlib.h>
#include<stdio.h>

#include"task.h"


Task::Task( const QString& taskName, long minutes, long sessionTime, QListView *parent) 
	: QListViewItem(parent)
{ 
	_name = taskName.stripWhiteSpace(); 
	_totalTime = minutes;
  _sessionTime = sessionTime;
	update();
};

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


