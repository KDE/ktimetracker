#ifndef SSK_KARM_H
#define SSK_KARM_H

#include <stdio.h>
#include <qsplitter.h>
#include <qlistview.h>

class KMenuBar;
class KToolBar;
class QListBox;
class AddTaskDialog;

///
class Karm	: public QListView
{
	Q_OBJECT
private:
	bool _timerRunning;
	int  _timerId;

	AddTaskDialog	*_addDlg;
	AddTaskDialog	*_editDlg;

signals:
	void sessionTimeChanged( long difference );

public:
	/** constructor */
	Karm( QWidget *parent = 0, const char *name = 0 );	
	/** destructor */
	virtual ~Karm();

	///
	static QString formatTime(long minutes);

	// Application "Name"
	QString KarmName;

public slots:
  /*
	  File format:
   		zero or more lines of
		  1 number	time in minutes
  		string		task name
	*/
	void load();
	void readFromFile(const char *);
	///
	void save();
	bool writeToFile(const char *fname);
	

	///
	void stopClock();
	///
	void startClock();
	///
	void newTask();
	///
	void editTask();
	void editTask(QListViewItem *);
	///
	void deleteTask();

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

	/** raised on file read or write error.
	*/
	void fileError( const char * );

	/** raised on changes to the list, rather than to a
	* particular item.
	*/
	void dataChanged();

};

inline QString Karm::formatTime( long minutes )
{
	QString time;
	time.sprintf("%ld:%02ld", minutes / 60, minutes % 60);
	return time;
}

#endif
