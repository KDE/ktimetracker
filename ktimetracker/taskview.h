#ifndef KARM_TASK_VIEW_H
#define KARM_TASK_VIEW_H

#include <q3dict.h>
#include <q3ptrlist.h>
#include <q3ptrstack.h>
#include <QMouseEvent>
#include <QList>
#include <QTextStream>

#include <k3listview.h>

#include "desktoplist.h"
#include "resourcecalendar.h"
#include "karmstorage.h"
#include "reportcriteria.h"
#include <qtimer.h>

class Q3ListBox;
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

class TaskView : public K3ListView
{
  Q_OBJECT

  public:
    TaskView( QWidget *parent = 0, const QString &icsfile = "" );
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
    QList<HistoryEvent> getHistory(const QDate& from, const QDate& to) const;

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

    /** Calls newTask dialog with caption "New Task".  */
    void newTask();

    /** Display edit task dialog and create a new task with results.  */
    void newTask( QString caption, Task* parent );

     /** Used to refresh (e.g. after import) */
    void refresh();

   /** Used to import a legacy file format. */
    void loadFromFlatFile();

    /** used to import tasks from imendio planner */
    QString importPlanner( QString fileName="" );

    /** call export function for csv totals or history */
    QString report( const ReportCriteria &rc );

    /** Export comma separated values format for task time totals. */
    void exportcsvFile();

    /** Export comma-separated values format for task history. */
    QString exportcsvHistory();

    /** Calls newTask dialog with caption "New Sub Task". */
    void newSubTask();

    void editTask();

    /** 
     * Returns a pointer to storage object.
     *
     * This is poor object oriented design--the task view should 
     * expose wrappers around the storage methods we want to access instead of
     * giving clients full access to objects that we own.
     *
     * Hopefully, this will be redesigned as part of the Qt4 migration.
     */
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

    /** Reinstates the current task as incomplete.
     * @param completion The percentage complete to mark the task as. */
    void reinstateTask(int completion);
//    void addCommentToTask();
    void markTaskAsComplete();
    void markTaskAsIncomplete();

    /** Subtracts time from all active tasks, and does not log event. */
    void extractTime( int minutes );
    void taskTotalTimesChanged( long session, long total)
                                { emit totalTimesChanged( session, total); };
    void adaptColumns();
    /** receiving signal that a task is being deleted */
    void deletingTask(Task* deletedTask);

    /** starts timer for task.
     * @param task      task to start timer of
     * @param startTime if taskview has been modified by another program, we
                            have to set the starting time to not-now. */
    void startTimerFor( Task* task, QDateTime startTime = QDateTime::currentDateTime() );
     void stopTimerFor( Task* task );

    /** clears all active tasks. Needed e.g. if iCal file was modified by
       another program and taskview is cleared without stopping tasks
        IF YOU DO NOT KNOW WHAT YOU ARE DOING, CALL stopAllTimers INSTEAD */
    void clearActiveTasks();

    /** User has picked a new iCalendar file on preferences screen. */
    void iCalFileChanged(QString file);

    /** Copy totals for current and all sub tasks to clipboard. */
    QString clipTotals( const ReportCriteria &rc );

    /** Copy history for current and all sub tasks to clipboard. */
    QString clipHistory();

  signals:
    void totalTimesChanged( long session, long total );
    void updateButtons();
    void timersActive();
    void timersInactive();
    void tasksChanged( Q3PtrList<Task> activeTasks );

  private: // member variables
    IdleTimeDetector *_idleTimeDetector;
    QTimer *_minuteTimer;
    QTimer *_autoSaveTimer;
    QTimer *_manualSaveTimer;
    Preferences *_preferences;
    Q3PtrList<Task> activeTasks;
    int previousColumnWidths[4];
    DesktopTracker* _desktopTracker;
    bool _isloading;

    //KCal::CalendarLocal _calendar;
    KarmStorage * _storage;

  private:
    void contentsMousePressEvent ( QMouseEvent * e );
    void contentsMouseDoubleClickEvent ( QMouseEvent * e );
    void updateParents( Task* task, long totalDiff, long sesssionDiff);
    void deleteChildTasks( Task *item );
    void addTimeToActiveTasks( int minutes, bool save_data = true );
    /** item state stores if a task is expanded so you can see the subtasks */
    void restoreItemState( Q3ListViewItem *item );

  protected slots:
    void autoSaveChanged( bool );
    void autoSavePeriodChanged( int period );
    void minuteUpdate();
    /** item state stores if a task is expanded so you can see the subtasks */
    void itemStateChanged( Q3ListViewItem *item );
    void iCalFileModified(ResourceCalendar *);
};

#endif // KARM_TASK_VIEW
