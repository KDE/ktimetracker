#ifndef SSK_KARM_H
#define SSK_KARM_H

#include <stdio.h>
#include <qsplitter.h>
#include <qlistview.h>
#include <qlist.h>

class KMenuBar;
class KToolBar;
class QListBox;
class AddTaskDialog;
class IdleTimer;
class QTimer;
class Preferences;
class Task;

class Karm : public QListView
{
	Q_OBJECT

private: // member variables
  IdleTimer *_idleTimer;
  QTimer *_minuteTimer;
  QTimer *_autoSaveTimer;

	AddTaskDialog	*_addDlg;
	AddTaskDialog	*_editDlg;
  Preferences *_preferences;
  
  QList<Task> activeTasks;

public:
	Karm( QWidget *parent = 0, const char *name = 0 );	
	virtual ~Karm();
	static QString formatTime(long minutes);

public slots:
  /*
	  File format:
   		zero or more lines of
		  1 number	time in minutes
  		string		task name
	*/
	void load();
	void save();
	void readFromFile(const QString &s);
	bool writeToFile(const QString &fname);
	void stopCurrentTimer();
	void stopAllTimers();
	void startTimer();
  void changeTimer(QListViewItem *);
	void newTask();
	void editTask();
	void editTask(QListViewItem *);
	void deleteTask();
  void extractTime(int minutes);

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

  void autoSaveChanged(bool);
  void autoSavePeriodChanged(int period);
  
signals:
	void sessionTimeChanged( long difference );

	/** raised on file read or write error.
	*/
	void fileError( const QString & );

	/** raised on changes to the list, rather than to a
	* particular item.
	*/
	void dataChanged();
	void timerTick();


protected slots:
  void minuteUpdate();
};

inline QString Karm::formatTime( long minutes )
{
	QString time;
	time.sprintf("%ld:%02ld", minutes / 60, minutes % 60);
	return time;
}

#endif
