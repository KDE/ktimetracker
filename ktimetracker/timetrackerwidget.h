/*
 *     Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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

class KAction;
class KActionCollection;
class KUrl;

class Task;
class TaskView;

class TimetrackerWidget : public QWidget {
  Q_OBJECT

  public:
    explicit TimetrackerWidget( QWidget *parent = 0 );
    ~TimetrackerWidget();

  private:
    void addTaskView( const QString &fileName = "" );

    /**
      Opens a file dialog to save the current taskView.

      @returns true if save was successfully, false otherwise.
     */
    bool saveCurrentTaskView();

  public:
    /**
      Returns the TaskView widget of the current opened tabpage.
     */
    TaskView* currentTaskView();

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
    void setupActions( KActionCollection *actionCollection );

    /**
      returns a generated action by name. You have to call setupActions before.

      @param name The name of the action
      @returns A pointer to a KAction instance
     */
    KAction *action( const QString &name ) const;

  public Q_SLOTS:
    /**
      opens a new untitled file which is stored firstly as temporary file
      but with a flag, that it has to be saved in a proper place.
     */
    void newFile();

    /**
      opens an existing ics file.
     */
    void openFile( const QString &fileName = QString() );

    /**
      opens an existing ics file (wrapper for KUrl).
     */
    void openFile( const KUrl &fileName );

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
      call this method when the preferences changed to adjust all
      taskviews.
     */
    void reconfigureFiles();

    /**
      shows/hides the search bar.
     */
    void showSearchBar( bool visible );

     /**
      shows/hides the tabbar.
     */
    void showTabBar( bool visible );

    /**
      tries to close all files. This slot has to be called before quitting
      the application to ensure that no data is lost.

      @returns true if the user has saved or consciously not saved all files,
               otherwise false.
     */
    bool closeAllFiles();


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
    void deleteTask( bool markingascomplete = false );
    void markTaskAsComplete();
    void markTaskAsIncomplete();
    void exportcsvFile();
    void exportcsvHistory();
    void importPlanner( const QString &fileName = "" );
    void startNewSession();
    void editHistory();
    void resetAllTimes();
    void focusTracking();
    void slotSearchBar();
    //END

    //BEGIN dbus slots
    QString version() const;
    QStringList taskIdsFromName( const QString &taskName ) const;
    void addTask( const QString &taskName );
    void setPercentComplete( const QString &taskId, int percent );
    int bookTime( const QString &taskId, const QString &dateTime, int minutes );
    QString error( int errorCode ) const;
    int totalMinutesForTaskId( const QString &taskId ) const;
    void startTimerFor( const QString &taskId );
    void stopTimerFor( const QString &taskId );
    // FIXME rename, when the wrapper slot is removed
    void stopAllTimersDBUS();
    QString exportCSVFile( const QString &filename, const QString &from, 
                        const QString &to, int type, bool decimalMinutes, 
                        bool allTasks, const QString &delimiter, 
                         const QString &quote );
    void importPlannerFile( const QString &filename );
    QStringList tasks() const;
    QStringList activeTasks() const;
    bool isActive( const QString &taskId ) const;
    void saveAll();
    void quit();
    //END

  private Q_SLOTS:
    void slotCurrentChanged();
    void updateTabs();
    void slotAddTask( const QString &taskName );
    void slotUpdateButtons();
    void showSettingsDialog();

  Q_SIGNALS:
    void currentTaskChanged();
    void currentTaskViewChanged();
    void updateButtons();
    void totalTimesChanged( long session, long total );
    void statusBarTextChangeRequested( const QString &text );
    void contextMenuRequested( const QPoint &pos );
    void timersActive();
    void timersInactive();
    void tasksChanged( const QList< Task* >& );

  private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

#endif
