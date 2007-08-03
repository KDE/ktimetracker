/*
 *     Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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

#ifndef TIMETRACKER_WIDGET_H
#define TIMETRACKER_WIDGET_H

#include <KTabWidget>

#include <QDateTime>

class Task;
class TaskView;

class TimetrackerWidget : public KTabWidget {
  Q_OBJECT

  public:
    explicit TimetrackerWidget( QWidget *parent = 0 );
    ~TimetrackerWidget();

  private:
    void addTaskView( const QString &fileName = "" );
    void saveCurrentTaskView();

  public:
    /**
      Returns the TaskView widget of the current opened tabpage.
     */
    TaskView* currentTaskView();

    /**
      Returns the current task of the current opened TaskView widget.
     */
    Task* currentTask();

  public Q_SLOTS:
    /**
      opens a new untitled file which is stored firstly as temporary file
      but with a flag, that it has to be saved in a proper place.
     */
    void newFile();

    /**
      opens an existing ics file.
     */
    void openFile( const QString &fileName );

    /**
      closes the current opened tab widget and saves the data
      of the corresponding taskview.

      @param canCancel If true, the user is allowed to cancel the close
                       progress when the current widget is unsaved
     */
    void closeFile( bool canCancel = true );

    /**
      saves the current taskview. This is especially important on unsaved
      files to give them a non-temporary filename.

      @returns the output of TaskView::save
    */
    QString saveFile();

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
    void importPlanner( const QString &fileName = "" );

  private Q_SLOTS:
    void slotCurrentChanged();

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
