
#ifndef ssk_karm_h
#define ssk_karm_h

#include<qstring.h>
#include<qlist.h>
#include<qobject.h>

class QFile;

/**
	Encapsulates a task.
*/

class Task
{

private:
	QString *_name;
	long _time;

public:
	/** constructor */
	Task( const char *name=0, long minutes=0 );

	/** destructor*/
	~Task()
		{ if( _name != 0 ) delete _name; };

	/**increments the total task time
	* @param minutes to increment by
	*/
	void incrementTime( long minutes )
		{ _time += minutes; };

	/** decrements the total task time
	* @param minutes to decrement by
	*/
	void decrementTime( long minutes )
		{ _time -= minutes; };

	/** sets the total time accumulated by the task
	* @param minutes time in minutes
	*/
	void setTime ( long minutes )
		{ _time = minutes; };

	/** returns the total time accumulated by the task
	* @return total time in minutes
	*/
	long time() const
		{ return _time; };

	/** sets the name of the task
	* @param name	a pointer to the name. A deep copy will be made.
	*/
	void setName( const char *name )
		{ *_name = name; };

	/** returns the name of this task.
	* @return a pointer to the name.
	*/
	const char *name() const
		{ return _name->data(); };

	
		
};

/** Handles multiple views of the list */
class TaskBroker	:	public QObject
{
	Q_OBJECT
private:
	QList<Task> *_taskList;

	/** create a task from saved data. The buffer is parsed
	* and a new Task is created from it. Returns 0 if no
	* task was found.
	*/
	static Task *readTask(const char *buffer);

public:
	/** constructor
	*/
	TaskBroker();

	/** destructor
	*/
	~TaskBroker();

	/** reads in a saved task list.
	*/
	int readFromFile( const char *file );

	/** saves the current task list
	*/
	bool writeToFile( const char *file );

	/** moves the cursor. If the new position is out of bounds, the
	* cursor is not moved and <0 is returned.
	* @param d	move the cursor by d items. Negative d moves the
	*		cursor upward.
	* @return	the position of the new item, <0 if out of bounds.
	*/
	int moveCursor(int d);

	/** the current cursor position
	*/
	int current() const
		{ return _taskList->at(); };

	/** The task element at the current position.
	* @return the task at the current element, or NULL if there is
	* no current element (empty list).
	*/
	Task *currentTask() const
		{ return _taskList->current(); };

	/** Add a new task */
	void addTask( const char *name, long time = 0 );


	/** delete the current Task */
	void deleteCurrentTask();


	/** returns TRUE if list is empty or cursor is at last element */
	bool atLast() const;

	/** returns the number of tasks in the list */
	int count() const
		{ return _taskList->count(); };

public slots:


	/**moves cursor to the next element*/
	void next()
		{ moveCursor(1); };	

	/**moves cursor to be previous element*/
	void previous()
		{ moveCursor(-1); };

	/**moves cursor to the top of the list*/
	void top();

	/**moves cursor to the end of the list*/
	void bottom();

	/** moves the cursor to the absolute index
	*/
	void moveTo( int );

signals:
	/** raised on cursor move */
	void cursorMoved( int );

	/** raised on changes to the list, rather than to a
	* particular item.
	*/
	void dataChanged();

	/** raised on change in a particular item.
	*/
	void currentChanged( int i );

	/** raised on file read or write error.
	*/
	void fileError( const char * );
};


inline Task::Task( const char *name=0, long minutes=0 ) 
{ 
	_name = new QString( name ); 
	*_name = _name->stripWhiteSpace();
	_time = minutes; 
};

inline TaskBroker::TaskBroker() 
	: QObject()
{ 
	_taskList = new QList<Task>; 
}

inline TaskBroker::~TaskBroker()
{ 
	delete _taskList;
}

inline void TaskBroker::bottom()
{
	_taskList->last();  
	emit cursorMoved( _taskList->at() ) ; 
}

inline void TaskBroker::top()
{ 
	_taskList->first();
	emit cursorMoved( _taskList->at() );
}

inline void TaskBroker::moveTo( int pos )
{ 
	_taskList->at( pos );
	emit cursorMoved( _taskList->at() );
}

inline bool TaskBroker::atLast() const
{
	if( _taskList->count() == 0L ||
		(unsigned)_taskList->at() == _taskList->count()-1 )
		return TRUE;

	return FALSE;
}

#endif
