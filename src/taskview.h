/*
    SPDX-FileCopyrightText: 2003 Scott Monachello <smonach@cox.net>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTIMETRACKER_TASK_VIEW
#define KTIMETRACKER_TASK_VIEW

#include <QList>

#include "desktoplist.h"
#include "reportcriteria.h"
#include "timetrackerstorage.h"

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

    /**
     * Load the view from storage.
     *
     * Must be called only once. If you need to open another file,
     * please create a new TaskView object.
     */
    void load(const QUrl &url);

    /** Close the storage and release lock. */
    void closeStorage();

    /** Add a task to view and storage. */
    Task *addTask(const QString &taskname,
                  const QString &taskdescription = QString(),
                  int64_t total = 0,
                  int64_t session = 0,
                  const DesktopList &desktops = QVector<int>(0, 0),
                  Task *parent = nullptr);

    /** Returns whether the focus tracking is currently active. */
    bool isFocusTrackingActive() const;

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
    void stopAllTimers(const QDateTime &when = QDateTime::currentDateTime());

    /** Toggles the automatic tracking of focused windows
     */
    void toggleFocusTracking();

    /** Display edit task dialog and create a new task with results.
     *  @param caption Window title of the edit task dialog
     */
    void newTask(const QString &caption, Task *parent);

    /** used to import tasks from imendio planner */
    void importPlanner(const QString &fileName);

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
    TimeTrackerStorage *storage();

    /**
     * Deletes the given or the current task (and children) from the view.
     * It does this in batch mode, no user dialog.
     * @param task Task to be deleted. If empty, the current task is deleted.
     *             if non-existent, an error message is displayed.
     */
    void deleteTaskBatch(Task *task);

    /**
     * Deletes the given or the current task (and children) from the view.
     * Depending on configuration, there may be a user dialog.
     * @param task Task to be deleted. If empty, the current task is deleted.
     *             if non-existent, an error message is displayed.
     */
    void deleteTask(Task *task = nullptr);

    /** Sets % completed for the current task.
     * @param completion The percentage complete to mark the task as. */
    void setPerCentComplete(int completion);
    void markTaskAsComplete();
    void markTaskAsIncomplete();

    /** Subtracts time from all active tasks, and does not log event. */
    void subtractTime(int64_t minutes);
    /** receiving signal that a task is being deleted */
    void taskAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    /** receiving signal that a task has been deleted */
    void taskRemoved(const QModelIndex &parent, int first, int last);

    /** starts timer for task.
     * @param task      task to start timer of
     * @param startTime if taskview has been modified by another program, we
                            have to set the starting time to not-now. */
    void startTimerFor(Task *task, const QDateTime &startTime);
    void startTimerForNow(Task *task);
    void stopTimerFor(Task *task);

    /** Reconfigures taskView depending on current configuration. */
    void reconfigureModel();

    /** Refresh the times of the tasks, e.g. when the history has been changed by the user */
    QString reFreshTimes();

    void onTaskDoubleClicked(Task *task);

    void editTaskTime(const QString &taskUid, int64_t minutes);

Q_SIGNALS:
    void updateButtons();
    void timersActive();
    void timersInactive();

    /** Used to update text in tray icon */
    void tasksChanged(const QList<Task *> &activeTasks);
    void minutesUpdated(const QList<Task *> &activeTasks);

    void setStatusBarText(const QString &text);
    void contextMenuRequested(const QPoint &);

private: // member variables
    QSortFilterProxyModel *m_filterProxyModel;

    IdleTimeDetector *m_idleTimeDetector;
    QTimer *m_minuteTimer;
    QTimer *m_autoSaveTimer;
    DesktopTracker *m_desktopTracker;

    TimeTrackerStorage *m_storage;
    bool m_focusTrackingActive;
    Task *m_lastTaskWithFocus;

    FocusDetector *m_focusDetector;

    TasksWidget *m_tasksWidget;

public Q_SLOTS:
    void minuteUpdate();

    /** React on the focus having changed to Window QString **/
    void newFocusWindowDetected(const QString &);

    void slotColumnToggled(int);
};

#endif // KTIMETRACKER_TASK_VIEW
