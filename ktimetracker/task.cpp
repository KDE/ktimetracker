#include<ctype.h>
#include<qfile.h>
#include<stdlib.h>
#include<stdio.h>

#include"task.h"


Task::Task( const QString& taskName, long minutes, QListView *parent) 
	: QListViewItem(parent)
{ 
	_name = taskName.stripWhiteSpace(); 
	_time = minutes;
	update();
};

void Task::setName( const QString& name ) 
{
	_name = name;
	update();
}

void Task::setTime ( long minutes )
{
	_time = minutes;
	update();
}


void Task::incrementTime( long minutes )
{
	_time += minutes;
	update();
}

void Task::decrementTime(long minutes)
{
	_time -= minutes;
	update();
}


