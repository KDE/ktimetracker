#ifndef ssk_karm_h
#define ssk_karm_h

#include <qstring.h>
#include <qlist.h>
#include <qobject.h>
#include <qlistview.h>
#include "karm.h"

class QFile;

/**
	Encapsulates a task.
*/

class Task :public QListViewItem
{

private:
	QString _name;
	long _time;

public:
	/** constructor */
	Task(const QString& taskame, long minutes, QListView *parent = 0);

	/**increments the total task time
	* @param minutes to increment by
	*/
	void incrementTime( long minutes );

	/** decrements the total task time
	* @param minutes to decrement by
	*/
	void decrementTime( long minutes );

	/** sets the total time accumulated by the task
	* @param minutes time in minutes
	*/
	void setTime ( long minutes );

	/** returns the total time accumulated by the task
	* @return total time in minutes
	*/
	long time() const
		{ return _time; };

	/** sets the name of the task
	* @param name	a pointer to the name. A deep copy will be made.
	*/
	void setName( const QString& name );

	/** returns the name of this task.
	* @return a pointer to the name.
	*/
	const char *name() const
		{ return _name.data(); };

	/** Updates the content of the QListViewItem with respect to _name and _time
	 */
	inline void update() {
		setText(0, Karm::formatTime(_time));
		setText(1, _name);
	}
};

#endif
