#ifndef KARM_TASK_VIEW_H
#define KARM_TASK_VIEW_H

#include <qdict.h>
#include <qptrlist.h>
#include <qptrstack.h>

#include <klistview.h>

#include "desktoplist.h"
#include "resourcecalendar.h"
#include "karmstorage.h"
#include <qtimer.h>
//#include "desktoptracker.h"

//#include "karmutility.h"

class QListBox;
class QString;
class QTextStream;
class QTimer;

class KMenuBar;
class KToolBar;

class DesktopTracker;
class EditTaskDialog;
class IdleTimeDetector;
class Preferences;
class Task;
class KarmStorage;
class HistoryEvent;

using namespace KCal;

/**
 * Container and interface for the tasks.
 */

class TaskView : public KListView
{
  Q_OBJECT

  public:
    TaskView( QWidget *parent = 0, const char *name = 0, const QString &icsfile = "" );
    virtual ~TaskView();

    /**  Return the first item in the view, cast to a Task pointer.  */
    Task* first_child() const;

    /**  Return the current item in the view, cast to a Task pointer.  */
    Task* current_item() const;

    /**  Return the i'th item (zero-based), cast to a Task pointer.  */
    Task* item_at_index(int i);

    /** Load the view from storage.  */
    void load( QString filename="" );

    /** Close the storage and release lock. */
    void closeStorage();

    /** Reset session time to zero for all tasks.   */
    void startNewSession();

    /** Reset session and total time to zero for all tasks.  */
    void resetTimeForAllTasks();

    /** Return the total number if items in the view.  */
    long count();

    /** Return list of start/stop events for given date range. */
    QValueList<HistoryEvent> getHistory(const QDate& from, const QDate& to) const;

    /** Schedule that we should save very soon */
    void scheduleSave();

    /** Return preferences user selected on settings dialog. **/
    Preferences *preferences();

    /** Add a task to view and storage. */
    QString addTask( const QString& taskame, long total, long session, const DesktopList& desktops, 
                     Task* parent = 0 );

  public slots:
    /** Save to persistent storage. */
    QString save();

    /** Start the timer on the current item (task) in view.  */
    void startCurrentTimer();

    /** Stop the timer for the current item in the view.  */
    void stopCurrentTimer();

    /** Stop all running timers.  */
    void stopAllTimers();

    /** Stop all running timers, and start timer on current item.  */
    void changeTimer( QListViewItem * = 0 );

    /** Calls newTask with caption "New Task".  */
    void newTask();

    /** Display edit task dialog and create a new task with results.  */
    void newTask( QString caption, Task* parent );

     /** Used to refresh (e.g. after import) */
    void refresh();

   /** Used to import a legacy file format. */
    void loadFromFlatFile();

    /** used to import tasks from imendio planner */
    void importPlanner();

    /** Export comma separated values format for task time totals. */
    void exportcsvFile();

    /** Export comma-separated values format for task history. */
    QString exportcsvHistory();

    /** Calls newTask with caption "New Sub Task". */
    void newSubTask();

    void editTask();
    KarmStorage* storage();

    /**
     * Delete task (and children) from view.
     *
     * @param markingascomplete  If false (the default), deletes history for
     * current task and all children.  If markingascomplete is true, then sets
     * percent complete to 100 and removes task and all it's children from the
     * list view.
     */
    void deleteTask(bool markingascomplete=false);
//    void addCommentToTask();
    void markTaskAsComplete();

    /** Subtracts time from all active tasks, and does not log event. */
    void extractTime( int minutes );
    void taskTotalTimesChanged( long session, long total)
                                { emit totalTimesChanged( session, total); };
    void adaptColumns();
    /** receiving signal that a task is being deleted */
    void deletingTask(Task* deletedTask);
    void startTimerFor( Task* task );
    void stopTimerFor( Task* task );

    /** User has picked a new iCalendar file on preferences screen. */
    void iCalFileChanged(QString file);

    /** Copy totals for current and all sub tasks to clipboard. */
    void clipTotals();

    /** Copy history for current and all sub tasks to clipboard. */
    void clipHistory();

  signals:
    void totalTimesChanged( long session, long total );
    void updateButtons();
    void timersActive();
    void timersInactive();
    void tasksChanged( QPtrList<Task> activeTasks );

  private: // member variables
    IdleTimeDetector *_idleTimeDetector;
    QTimer *_minuteTimer;
    QTimer *_autoSaveTimer;
    QTimer *_manualSaveTimer;
    Preferences *_preferences;
    QPtrList<Task> activeTasks;
    int previousColumnWidths[4];
    DesktopTracker* _desktopTracker;
    bool _isloading;

    //KCal::CalendarLocal _calendar;
    KarmStorage * _storage;

  private:
    void updateParents( Task* task, long totalDiff, long sesssionDiff);
    void deleteChildTasks( Task *item );
    void addTimeToActiveTasks( int minutes, bool save_data = true );
    void restoreItemState( QListViewItem *item );

  protected slots:
    void autoSaveChanged( bool );
    void autoSavePeriodChanged( int period );
    void minuteUpdate();
    void itemStateChanged( QListViewItem *item );
    void deleteItemState( QListViewItem *item );
    void iCalFileModified(ResourceCalendar *);
};

#endif // KARM_TASK_VIEW
