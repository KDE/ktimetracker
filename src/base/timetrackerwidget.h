/*
    SPDX-FileCopyrightText: 2007 Mathias Soeken <msoeken@tzi.de>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TIMETRACKER_WIDGET_H
#define TIMETRACKER_WIDGET_H

#include <QDateTime>
#include <QUrl>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

class KActionCollection;

class Task;
class TaskView;
class SearchLine;
class TasksWidget;

class TimeTrackerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimeTrackerWidget(QWidget *parent);
    ~TimeTrackerWidget() override = default;

private:
    /**
     * Load the specified file in the tasks widget.
     * @param url Must not be empty.
     */
    void addTaskView(const QUrl &url);

public:
    /**
      Returns the TaskView widget of the current opened tabpage.
     */
    TaskView *currentTaskView() const;

    /**
      Returns the current task of the current opened TaskView widget.
     */
    Task *currentTask();

    /**
      initializes the KActionCollection object of a main window for example.
      The actions are connected to the TreeWidget itself to ensure reusability.

      @param actionCollection The KActionCollection instance of the host
      object.
     */
    void setupActions(KActionCollection *actionCollection);

    /**
      returns a generated action by name. You have to call setupActions before.

      @param name The name of the action
      @returns A pointer to a QAction instance
     */
    QAction *action(const QString &name) const;

public Q_SLOTS:
    /**
     * Load the specified .ics file in the tasks widget.
     * @param url Must not be empty.
     */
    void openFile(const QUrl &url);

    void openFileDialog();

    /**
      closes the current opened tab widget and saves the data
      of the corresponding taskview.

      @returns whether the file has been closed.
     */
    bool closeFile();

    /**
      saves the current taskview. This is especially important on unsaved
      files to give them a non-temporary filename.
    */
    void saveFile();

    /**
      this method puts the input focus onto the search bar
     */
    int focusSearchBar();

    /**
      shows/hides the search bar.
     */
    void showSearchBar(bool visible);

    /**
      tries to close all files. This slot has to be called before quitting
      the application to ensure that no data is lost.

      @returns true if the user has saved or consciously not saved all files,
               otherwise false.
     */
    bool closeAllFiles();

    /** Open export dialog. */
    void exportDialog();

    // BEGIN wrapper slots
    /*
     * The following slots are wrapper slots which fires the corresponding
     * slot of the current taskview.
     */
    void startCurrentTimer();
    void stopCurrentTimer();
    void stopAllTimers();
    /** Calls newTask dialog with caption "New Task".  */
    void newTask();
    void newSubTask();
    void editTask();
    void editTaskTime();
    void deleteTask();
    void markTaskAsComplete();
    void markTaskAsIncomplete();
    void startNewSession();
    void editHistory();
    void resetAllTimes();
    void focusTracking();
    void slotSearchBar();
    // END

    /** \defgroup dbus slots ‘‘dbus slots’’ */
    /* @{ */
    QString version() const;
    QStringList taskIdsFromName(const QString &taskName) const;
    void addTask(const QString &taskName);
    void addSubTask(const QString &taskName, const QString &taskId);
    void deleteTask(const QString &taskId);
    void setPercentComplete(const QString &taskId, int percent);
    int bookTime(const QString &taskId, const QString &dateTime, int64_t minutes);
    int changeTime(const QString &taskId, int64_t minutes);
    QString error(int errorCode) const;
    bool isIdleDetectionPossible() const;
    int totalMinutesForTaskId(const QString &taskId) const;
    void startTimerFor(const QString &taskId);
    void stopTimerFor(const QString &taskId);

    bool startTimerForTaskName(const QString &taskName);
    bool stopTimerForTaskName(const QString &taskName);

    // FIXME rename, when the wrapper slot is removed
    void stopAllTimersDBUS();
    QString exportCSVFile(const QString &filename,
                          const QString &from,
                          const QString &to,
                          int type,
                          bool decimalMinutes,
                          bool allTasks,
                          const QString &delimiter,
                          const QString &quote);
    void importPlannerFile(const QString &filename);

    /** return all task names, e.g. for batch processing */
    QStringList tasks() const;

    QStringList activeTasks() const;
    bool isActive(const QString &taskId) const;
    bool isTaskNameActive(const QString &taskName) const;
    void saveAll();
    void quit();
    // END of dbus slots group
    /* @} */

    // Triggered on changes in the Settings dialog
    void loadSettings();

protected:
    bool event(QEvent *event) override; // inherited from QWidget

public Q_SLOTS:
    void showSettingsDialog();

private Q_SLOTS:
    void slotCurrentChanged();
    void slotAddTask(const QString &taskName);
    void slotUpdateButtons();

Q_SIGNALS:
    void currentFileChanged(const QUrl &file);
    void currentTaskChanged();
    void currentTaskViewChanged();
    void updateButtons();
    void statusBarTextChangeRequested(const QString &text);
    void contextMenuRequested(const QPoint &pos);
    void timersActive();
    void timersInactive();

    // Used to update text in tray icon
    // and window title if the window title is configured to display the active tasks
    void tasksChanged(const QList<Task *> &activeTasks);

    // update recorded minutes of current task
    // if the window title is configured to display the active tasks
    void minutesUpdated(const QList<Task *> &activeTasks);

private:
    void fillLayout(TasksWidget *tasksWidget);
    void registerDBus();

    SearchLine *m_searchLine;
    TaskView *m_taskView;
    KActionCollection *m_actionCollection;
};

#endif
