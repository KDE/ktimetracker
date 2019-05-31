/*
 * Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
 * Copyright (C) 2019  Alexander Potashev <aspotashev@gmail.com>
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

#ifndef TIMETRACKER_WIDGET_H
#define TIMETRACKER_WIDGET_H

#include <QWidget>
#include <QDateTime>
#include <QUrl>

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
    explicit TimeTrackerWidget(QWidget* parent = nullptr);
    ~TimeTrackerWidget() override = default;

    /** 
      Delivers if all task have an end time. 
      This is useful e.g. at the start of the program to see if a timer needs to be resumed.
      This function checks all (TaskView) mTabWidget->widget() to see if any task is open.
     */
    bool allEventsHaveEndTiMe();

private:
    void addTaskView(const QUrl &url = QUrl());

public:
    /**
      Returns the TaskView widget of the current opened tabpage.
     */
    TaskView* currentTaskView() const;

    /**
      Returns the current task of the current opened TaskView widget.
     */
    Task* currentTask();

    /**
      initializes the KActionCollection object of a main window for example.
      The actions are connected to the TreeWidget itself to ensure reusability.

      @param actionCollection The KActionCollection instance of the host
      object.
     */
    void setupActions(KActionCollection* actionCollection);

    /**
      returns a generated action by name. You have to call setupActions before.

      @param name The name of the action
      @returns A pointer to a QAction instance
     */
    QAction* action(const QString &name) const;

public Q_SLOTS:
    /**
      opens an existing ics file.
     */
    void openFile(const QUrl &url = QUrl());

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

    //BEGIN wrapper slots
    /*
     * The following slots are wrapper slots which fires the corresponding
     * slot of the current taskview.
     */
    void startCurrentTimer();
    void stopCurrentTimer();
    void stopAllTimers( const QDateTime &when = QDateTime::currentDateTime() );
    void newTask();
    void newSubTask();
    void editTask();
    void deleteTask();
    void markTaskAsComplete();
    void markTaskAsIncomplete();
    void startNewSession();
    void editHistory();
    void resetAllTimes();
    void focusTracking();
    void slotSearchBar();
    //END

    /** \defgroup dbus slots ‘‘dbus slots’’ */
    /* @{ */
    QString version() const;
    QStringList taskIdsFromName( const QString &taskName ) const;
    void addTask( const QString &taskName );
    void addSubTask( const QString& taskName, const QString &taskId );
    void deleteTask( const QString &taskId );
    void setPercentComplete( const QString &taskId, int percent );
    int bookTime(const QString &taskId, const QString &dateTime, int minutes);
    int changeTime( const QString &taskId, int minutes );
    QString error( int errorCode ) const;
    bool isIdleDetectionPossible() const;
    int totalMinutesForTaskId( const QString &taskId ) const;
    void startTimerFor( const QString &taskId );
    void stopTimerFor( const QString &taskId );

    bool startTimerForTaskName( const QString &taskName );
    bool stopTimerForTaskName( const QString &taskName );

  // FIXME rename, when the wrapper slot is removed
    void stopAllTimersDBUS();
    QString exportCSVFile( const QString &filename, const QString &from,
                        const QString &to, int type, bool decimalMinutes,
                        bool allTasks, const QString &delimiter,
                         const QString &quote );
    void importPlannerFile( const QString &filename );

    /** return all task names, e.g. for batch processing */
    QStringList tasks() const;

    QStringList activeTasks() const;
    bool isActive( const QString &taskId ) const;
    bool isTaskNameActive( const QString &taskId ) const;
    void saveAll();
    void quit();
    // END of dbus slots group
    /* @} */

    // Triggered on changes in the Settings dialog
    void loadSettings();

protected:
    bool event(QEvent *event) override; // inherited from QWidget

private Q_SLOTS:
    void slotCurrentChanged();
    void slotAddTask( const QString &taskName );
    void slotUpdateButtons();
    void showSettingsDialog();

Q_SIGNALS:
    void setCaption( const QString& qs );
    void currentTaskChanged();
    void currentTaskViewChanged();
    void updateButtons();
    void statusBarTextChangeRequested(const QString& text);
    void contextMenuRequested( const QPoint &pos );
    void timersActive();
    void timersInactive();

    /** Used to update text in tray icon */
    void tasksChanged(const QList<Task*> &activeTasks);

private:
    void fillLayout(TasksWidget *tasksWidget);

    SearchLine *m_searchLine;
    TaskView *m_taskView;
    KActionCollection *m_actionCollection;
};

#endif
