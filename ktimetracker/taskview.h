/*
 *     Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
 *                   2007 the ktimetracker developers
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the
 *      Free Software Foundation, Inc.
 *      51 Franklin Street, Fifth Floor
 *      Boston, MA  02110-1301  USA.
 *
 */

#ifndef KARM_TASK_VIEW_H
#define KARM_TASK_VIEW_H

#include <QList>
#include <QTreeWidget>

#include <kcal/resourcecalendar.h>

#include "desktoplist.h"
#include "karmstorage.h"
#include "reportcriteria.h"

class QMouseEvent;
class QString;
class QTableWidget;
class QTextStream;
class QTimer;

class DesktopTracker;
class IdleTimeDetector;
class Preferences;
class Task;
class KarmStorage;
class HistoryEvent;

using namespace KCal;

/**
 * Container and interface for the tasks.
 */

class TaskView : public QTreeWidget
{
  Q_OBJECT

  public:
    explicit TaskView( QWidget *parent = 0 );
    virtual ~TaskView();

    //BEGIN view specified
    /**  Return the current item in the view, cast to a Task pointer.  */
    Task* currentItem() const;

    /**  Return the i'th item (zero-based), cast to a Task pointer.  */
    Task* itemAt( int i );
    //END

    //BEGIN model specified
    /** Load the view from storage.  */
    void load( const QString &filename );

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

    /** Add a task to view and storage. */
    QString addTask( const QString& taskame, long total = 0, long session = 0, const DesktopList& desktops = QVector<int>(0,0),
                     Task* parent = 0 );

    /** Returns a list of the current active tasks. */
    QList< Task* > activeTasks() const;
    //END

    //BEGIN behavior
    /** Returns whether the focus tracking is currently active. */
    bool isFocusTrackingActive() const;
    //END

  public Q_SLOTS:
    /** Save to persistent storage. */
    QString save();

    /** Start the timer on the current item (task) in view.  */
    void startCurrentTimer();

    /** Stop the timer for the current item in the view.  */
    void stopCurrentTimer();

    /** Stop all running timers.  
     *  @param when When the timer stopped - this makes sense if the idletime-
     *              detector detects the user stopped working 5 minutes ago.
     */
    void stopAllTimers( const QDateTime &when = QDateTime::currentDateTime() );

    /** Toggles the automatic tracking of focused windows
     */
    void toggleFocusTracking();

    /** Calls newTask dialog with caption "New Task".  */
    void newTask();

    /** Display edit task dialog and create a new task with results.  */
    void newTask( const QString &caption, Task* parent );

    /** Used to refresh (e.g. after import) */
    void refresh();

    /** used to import tasks from imendio planner */
    void importPlanner( const QString &fileName = "" );

    /** call export function for csv totals or history */
    QString report( const ReportCriteria &rc );

    /** Export comma separated values format for task time totals. */
    void exportcsvFile();

    /** Export comma-separated values format for task history. */
    QString exportcsvHistory();

    /** Calls newTask dialog with caption "New Sub Task". */
    void newSubTask();

    /** Calls editTask dialog for the current task. */
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
                                { emit totalTimesChanged( session, total); }
    /** receiving signal that a task is being deleted */
    void deletingTask(Task* deletedTask);

    /** starts timer for task.
     * @param task      task to start timer of
     * @param startTime if taskview has been modified by another program, we
                            have to set the starting time to not-now. */
    void startTimerFor( Task* task, 
                        const QDateTime &startTime = QDateTime::currentDateTime() );
     void stopTimerFor( Task* task );

    /** clears all active tasks. Needed e.g. if iCal file was modified by
       another program and taskview is cleared without stopping tasks
        IF YOU DO NOT KNOW WHAT YOU ARE DOING, CALL stopAllTimers INSTEAD */
    void clearActiveTasks();

    /** Copy totals for current and all sub tasks to clipboard. */
    QString clipTotals( const ReportCriteria &rc );

    /** Copy history for current and all sub tasks to clipboard. */
    QString clipHistory();

    /** Reconfigures taskView depending on current configuration. */
    void reconfigure();

  Q_SIGNALS:
    void totalTimesChanged( long session, long total );
    void updateButtons();
    void timersActive();
    void timersInactive();
    void tasksChanged( QList<Task*> activeTasks );
    void setStatusBarText(QString);
    void contextMenuRequested( const QPoint & );

  private: // member variables
    IdleTimeDetector* _idleTimeDetector;
    QTimer *_minuteTimer;
    QTimer *_autoSaveTimer;
    QTimer *_manualSaveTimer;
    Preferences *_preferences;
    DesktopTracker* _desktopTracker;
    bool _isloading;
    QTableWidget* historywidget;

    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond

  private:
    void updateParents( Task* task, long totalDiff, long sesssionDiff);
    void deleteChildTasks( Task *item );
    void addTimeToActiveTasks( int minutes, bool save_data = true );
    /** item state stores if a task is expanded so you can see the subtasks */
    void restoreItemState();

  protected:
    void mouseMoveEvent( QMouseEvent* );
    void mousePressEvent( QMouseEvent* );

  protected Q_SLOTS:
    void minuteUpdate();

    /** item state stores if a task is expanded so you can see the subtasks */
    void itemStateChanged( QTreeWidgetItem *item );

    /** React on another process having modified the iCal file we rely on. 
       This is not iCalFileChanged. */
    void iCalFileModified(ResourceCalendar *);
    void slotItemDoubleClicked( QTreeWidgetItem *item, int );

    /** React on the focus having changed to Window QString **/
    void newFocusWindowDetected( const QString & );

    void slotColumnToggled( int );
    void slotCustomContextMenuRequested( const QPoint & );
    void slotSetPercentage( QAction * );
    void slotSetPriority( QAction * );
};

#endif // KARM_TASK_VIEW
