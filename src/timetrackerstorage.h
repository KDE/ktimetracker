/*
 *     Copyright (C) 2003 by Mark Bucciarelli <mark@hubcapconsutling.com>
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

#ifndef KTIMETRACKER_STORAGE_H
#define KTIMETRACKER_STORAGE_H

#include <KCalCore/Todo>
#include <KCalCore/Event>

#include "reportcriteria.h"
#include "desktoplist.h"
#include "file/filecalendar.h"

QT_BEGIN_NAMESPACE
class QDateTime;
class QLockFile;
QT_END_NAMESPACE

class ProjectModel;
class Task;
class TaskView;
class TasksModel;
class EventsModel;

/**
 * Class to store/retrieve KTimeTracker data to/from persistent storage.
 *
 * The storage is an iCalendar file. Its name is contained in this class
 * in the variable _icalfile and can be read using the function icalfile().
 * The name gets set by the load() operation.
 *
 * All logic that deals with getting and saving data should go here.
 *
 * This program uses iCalendar to store its data. There are tasks and
 * events. Events have a start and a end date and an associated task.
 *
 * @short Logic that gets and stores KTimeTracker data to disk.
 * @author Mark Bucciarelli <mark@hubcapconsulting.com>
 */

class TimeTrackerStorage : public QObject
{
    Q_OBJECT

public:
    TimeTrackerStorage();
    ~TimeTrackerStorage() override;

    /**
      Load the list view with tasks read from iCalendar file.

      Parses iCalendar file, builds list view items in the proper
      hierarchy, and loads them into the list view widget.

      If the file name passed in is the same as the last file name that was
      loaded, this method does nothing.

      This method considers any of the following conditions errors:

         @li the iCalendar file does not exist
         @li the iCalendar file is not readable
         @li the list group currently has list items
         @li an iCalendar todo has no related to attribute
         @li a todo is related to another todo which does not exist

      @param taskview The list group used in the TaskView. Must not be nullptr.
      @param url      Override preferences' filename

      @return empty string if success, error message if error.
     */
    QString load(TaskView *taskview, const QUrl &url);

   /**
    * Return the name of the iCal file
    */
    QUrl fileUrl();

   /**
    * Build up the taskview.
    *
    * This is needed if the iCal file has been modified.
    */
    QString buildTaskView(const KCalCore::Todo::List& todos, TaskView* view);

    /** Close calendar and clear view.  Release lock if holding one. */
    void closeStorage();

    bool isLoaded() const { return m_model; }

    /** list of all events */
    EventsModel *eventsModel();

    TasksModel *tasksModel();

    ProjectModel *projectModel();

    /**
     * Deliver if all events of a task have an endtime
     *
     * If ktimetracker has been quitted with one task running, it needs to resumeRunning().
     * This function delivers if an enddate of an event has not yet been stored.
     *
     * @param task        The task to be examined
     */
    bool allEventsHaveEndTiMe(Task* task);

    /**
     * Deliver if all events of the actual calendar have an endtime
     *
     * If ktimetracker has been quitted with one task running, it needs to resumeRunning().
     * This function delivers if an enddate of an event has not yet been stored.
     *
     */
    bool allEventsHaveEndTiMe();

    QString deleteAllEvents();

    /**
     * Save all tasks and their totals to an iCalendar file.
     *
     * All tasks must have an associated VTODO object already created in the
     * calendar file; that is, the task->uid() must refer to a valid VTODO in
     * the calendar.
     * Delivers empty string if successful, else error msg.
     */
    QString save();

    bool bookTime(const Task *task, const QDateTime &startDateTime, long durationInSeconds);

    /**
     * Log a change to a task name.
     *
     * For iCalendar storage, there is no need to log an Event for this event,
     * since unique id's are used to link Events to Todos.  No matter how many
     * times you change a task's name, the uid stays the same.
     *
     * @param task     The task
     * @param oldname  The old name of the task.  The new name is in the task
     *   object already.
     */
    void setName(const Task* task, const QString& oldname) { Q_UNUSED(task); Q_UNUSED(oldname); }

    /**
     * Write task history to file as comma-delimited data.
     */
    QString exportCSVHistory(TaskView *taskview, const QDate &from, const QDate &to, const ReportCriteria &rc);

private Q_SLOTS:
    void onFileModified();

private:
    QString writeTaskAsTodo(Task *task, KCalCore::Todo::Ptr parent);

    ProjectModel *m_model;
    QUrl m_url;
    TaskView* m_taskView;
};

#endif // KTIMETRACKER_STORAGE_H
