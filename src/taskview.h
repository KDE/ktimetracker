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

#ifndef KTIMETRACKER_TASK_VIEW
#define KTIMETRACKER_TASK_VIEW

#include <QList>

#include "desktoplist.h"
#include "timetrackerstorage.h"
#include "reportcriteria.h"

QT_BEGIN_NAMESPACE
class QTimer;
class QSortFilterProxyModel;
class QMenu;
QT_END_NAMESPACE

class DesktopTracker;
class IdleTimeDetector;
class Task;
class TimeTrackerStorage;
class FocusDetector;
class TasksModel;
class TasksWidget;

/**
 * Container and interface for the tasks.
 */
class TaskView : public QObject
{
    Q_OBJECT

public:
    explicit TaskView(QWidget *parent = nullptr);
    ~TaskView() override;

    //BEGIN model specified
    /** Load the view from storage.  */
    void load(const QUrl& url);

    /** Close the storage and release lock. */
    void closeStorage();

    /** Reset session time to zero for all tasks.  */
    void startNewSession();

    /** Deliver if all events from the storage have and end time.  */
    bool allEventsHaveEndTiMe();

    /** Reset session and total time to zero for all tasks.  */
    void resetTimeForAllTasks();

    /** Reset session and total time for all tasks - do not touch the storage.  */
    void resetDisplayTimeForAllTasks();

    /** Return the total number of items in the view.  */
    long count();

    /** Schedule that we should save very soon */
    void scheduleSave();

    /** return all task names, e.g. for batch processing */
    QStringList tasks();

    /** return the task with the given UID */
    Task* task(const QString& uid);

    /** Add a task to view and storage. */
    QString addTask(
        const QString& taskame, const QString& taskdescription = QString(),
        long total = 0, long session = 0, const DesktopList& desktops = QVector<int>(0,0),
        Task* parent = nullptr);

    /** Returns a list of the current active tasks. */
    QList< Task* > activeTasks() const;
    //END

    //BEGIN behavior
    /** Returns whether the focus tracking is currently active. */
    bool isFocusTrackingActive() const;
    //END

    TasksModel *tasksModel();
    int sortColumn() const;

    TasksWidget *tasksWidget() { return m_tasksWidget; }

public Q_SLOTS:
    /** Save to persistent storage. */
    void save();

    /** Start the timer on the current item (task) in view.  */
    void startCurrentTimer();

    /** Stop the timer for the current item in the view.  */
    void stopCurrentTimer();

    /** Stop all running timers.
     *  @param when When the timer stopped - this makes sense if the idletime-
     *              detector detects the user stopped working 5 minutes ago.
     */
    void stopAllTimers(const QDateTime& when = QDateTime::currentDateTime());

    /** Toggles the automatic tracking of focused windows
     */
    void toggleFocusTracking();

    /** Calls newTask dialog with caption "New Task".  */
    void newTask();

    /** Display edit task dialog and create a new task with results.
     *  @param caption Window title of the edit task dialog
     */
    void newTask(const QString& caption, Task* parent);

    /** Used to refresh (e.g. after import) */
    void refresh();

    /** used to import tasks from imendio planner */
    void importPlanner(const QString& fileName = "");

    /**
     * Call export function for csv totals or history.
     * Output a report based on contents of ReportCriteria.
     */
    QString report(const ReportCriteria& rc);

    /**
     *  Writes all tasks and their totals to a Comma-Separated Values file.
     *
     * The format of this file is zero or more lines of:
     *    taskName,subtaskName,..,sessionTime,time,totalSessionTime,totalTime
     * the number of subtasks is determined at runtime.
     */
    QString exportcsvFile(const ReportCriteria &rc);

    /** Export comma separated values format for task time totals. */
    void exportCSVFileDialog();

    /** Export comma-separated values format for task history. */
    QString exportCSVHistoryDialog();

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
    TimeTrackerStorage* storage();

    /**
     * Deletes the given or the current task (and children) from the view.
     * It does this in batch mode, no user dialog.
     * @param task Task to be deleted. If empty, the current task is deleted.
     *             if non-existent, an error message is displayed.
     */
    void deleteTaskBatch(Task* task = nullptr);

    /**
     * Deletes the given or the current task (and children) from the view.
     * Depending on configuration, there may be a user dialog.
     * @param task Task to be deleted. If empty, the current task is deleted.
     *             if non-existent, an error message is displayed.
     */
    void deleteTask(Task* task = nullptr);

    /** Sets % completed for the current task.
     * @param completion The percentage complete to mark the task as. */
    void setPerCentComplete(int completion);
    void markTaskAsComplete();
    void markTaskAsIncomplete();

    /** Subtracts time from all active tasks, and does not log event. */
    void subtractTime(int minutes);
    /** receiving signal that a task is being deleted */
    void deletingTask(Task* deletedTask);

    /** starts timer for task.
     * @param task      task to start timer of
     * @param startTime if taskview has been modified by another program, we
                            have to set the starting time to not-now. */
    void startTimerFor(Task* task, const QDateTime& startTime = QDateTime::currentDateTime());
    void stopTimerFor(Task* task);

    /** clears all active tasks. Needed e.g. if iCal file was modified by
       another program and taskview is cleared without stopping tasks
        IF YOU DO NOT KNOW WHAT YOU ARE DOING, CALL stopAllTimers INSTEAD */
    void clearActiveTasks();

    /** Copy totals for current and all sub tasks to clipboard. */
    QString clipTotals(const ReportCriteria& rc);

    /** Reconfigures taskView depending on current configuration. */
    void reconfigure();

    /** Refresh the times of the tasks, e.g. when the history has been changed by the user */
    QString reFreshTimes();

    QList<Task*> getAllTasks();

    void onTaskDoubleClicked(Task *task);

Q_SIGNALS:
    void updateButtons();
    void timersActive();
    void timersInactive();

    /** Used to update text in tray icon */
    void tasksChanged(const QList<Task*> &activeTasks);

    void setStatusBarText(const QString &text);
    void contextMenuRequested(const QPoint&);

private: // member variables
    QSortFilterProxyModel* m_filterProxyModel;

    IdleTimeDetector* m_idleTimeDetector;
    QTimer *m_minuteTimer;
    QTimer *m_autoSaveTimer;
    QTimer *m_manualSaveTimer;
    DesktopTracker* m_desktopTracker;

    TimeTrackerStorage *m_storage;
    bool m_focusTrackingActive;
    Task* m_lastTaskWithFocus;
    QList<Task*> m_activeTasks;

    FocusDetector *m_focusDetector;

    TasksWidget *m_tasksWidget;

private:
    void addTimeToActiveTasks(int minutes, bool save_data = true);
    /** item state stores if a task is expanded so you can see the subtasks */

public Q_SLOTS:
    void minuteUpdate();

    /** React on the focus having changed to Window QString **/
    void newFocusWindowDetected(const QString&);

    void slotColumnToggled(int);
};

#endif // KTIMETRACKER_TASK_VIEW
