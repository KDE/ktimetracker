
#include<ctype.h>
#include<qfile.h>
#include<stdlib.h>
#include<stdio.h>

#include"task.h"


#define T_LINESIZE 1023

/*
	File format:
	zero or more lines of
		1 number	time in minutes
		string		task name
*/

int TaskBroker::moveCursor(int d)
{

        if( _taskList->count() == 0 
                || ((unsigned)(_taskList->at() + d) >=_taskList->count()))
                return 0;

	_taskList->at( _taskList->at() + d );

        emit cursorMoved( _taskList->at() );

        return _taskList->at();
}

int TaskBroker::readFromFile( const char *fname ) 
{
	if ( fname == 0 )
		return 0;

	QFile *file = new QFile( fname );

	if( file == 0 ) {
		warning( "out of memory creating file object");
		return 0;
	}

	if( !file->open( IO_ReadOnly ) ) {
		// file open error
		emit fileError( fname );
		delete file;
		return 0;
	}

	_taskList->setAutoDelete( TRUE );
	_taskList->clear();

	char *line = new char[T_LINESIZE+1];

	if( line == 0 ) {
		delete file;
		warning("out of memory creating string.");
		return 0;
	}

	while( !file->atEnd() ) {
		Task *newTask;

		if ( file->readLine( line, T_LINESIZE ) == 0 )
			break;

		newTask = readTask( line );

		if( newTask != 0 )
			_taskList->append( newTask );
	}

	file->close();
	_taskList->first();

	delete file;
	delete[] line;

	return _taskList->count();
}

Task *TaskBroker::readTask(const char *buffer)
{
	int totalTime;

	while( isspace( *buffer ) )
		buffer++;

	// return an error if we didn't find a number
	// possible empty line?
	if( !isdigit( *buffer) )
		return 0;

	// let atoi find our number
	totalTime = atol(buffer);

	// skip number and following white space
	while( isdigit(*buffer) )
		buffer++;
	
	while( isspace(*buffer) )
		buffer++;

	// assume that buffer now points to the name of the task

	return new Task( buffer, totalTime );
}

bool TaskBroker::writeToFile( const char *fname )
{
	if( fname == 0 ) return 0;

	FILE *file = fopen( fname, "w" );

	if( file == 0 ) {
		emit fileError( fname );
		return FALSE;
	}


	fputs( "# Karm save data\n", file );	// file comment

	_taskList->first();

	while( _taskList->current() != 0 )  {
		
		fprintf(file, "%ld\t%s\n", 
			_taskList->current()->time(),
			_taskList->current()->name() );

		_taskList->next();
	}

	_taskList->at();

	fclose( file );

	emit dataChanged();

	return TRUE;
}

void TaskBroker::addTask( const char *task, long time )
{
	if( task == 0 )
		return;

	_taskList->append( new Task( task, time ) );

	emit cursorMoved( _taskList->at() );
}

void TaskBroker::deleteCurrentTask()
{
	if( _taskList->current() != 0 ) {
		_taskList->setAutoDelete( TRUE );
		_taskList->remove();
	}
}
