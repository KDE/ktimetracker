#ifndef SSK_KARM_H
#define SSK_KARM_H

#include <stdio.h>
#include <qsplitter.h>

class TaskBroker;
class KMenuBar;
class KToolBar;
class QListBox;
class AddTaskDialog;

///
class Karm	: public QSplitter
{
	Q_OBJECT
private:
	TaskBroker 	*_broker;

	QListBox	*_timeList;
	QListBox	*_nameList;

	bool		_timerRunning;
	int		_timerId;

	char		*_timeStr;

	AddTaskDialog	*_addDlg;
	AddTaskDialog	*_editDlg;

	

	/** Fills the list boxes with the current task list. 
	* The list is cleared first.
	*/
	void fillListBoxes();

public:
	/** constructor */
	Karm( QWidget *parent = 0 );	
	/** destructor */
	virtual ~Karm();

	///
	static void formatTime(char *dest, long minutes);

	// Application "Name"
	QString KarmName;

public slots:
	/// 
	void load();
	///
	void save();

	///
	void stopClock();
	///
	void startClock();

	///
	void moveTo( int );

	///
	void newTask();
	///
	void editTask();
	///
	void deleteTask();

	/** Rereads the task data and updates the corresponding
	* list entry.
	*/
	void updateCurrentItem();

protected slots:
	/** creates a new task.
	* Used as a callback from the new task dialog, creates
	* a new task only if returned is TRUE.
	*/
	void createNewTask( bool returned );

	/** updates an existing task.
	* Used as a callback from the new task dialog, updates
	* the task from the Dialog fields only if returned is TRUE.
	*/
	void updateExistingTask( bool returned );

protected:
	///
	virtual void timerEvent( QTimerEvent * );

signals:
	///
	void timerStarted();
	///
	void timerStopped();
	///
	void timerTick();

};

inline void Karm::formatTime( char *dest, long minutes )
{
	sprintf( dest, "%ld:%02ld", minutes / 60, minutes % 60 );
}

#endif
