/*
 * Copyright (C) 1997 by Stephan Kulow <coolo@kde.org>
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

#ifndef KTIMETRACKER_TASK_H
#define KTIMETRACKER_TASK_H

#include <QDateTime>

#include <KCalCore/Todo>

#include "desktoplist.h" // Required b/c DesktopList is a typedef not a class.
#include "model/tasksmodelitem.h"

class TimeTrackerStorage;
class ProjectModel;
class EventsModel;

/** \brief A class representing a task
 *
 * A "Task" object stores information about a task such as it's name,
 * total and session times.
 *
 * It can log when the task is started, stoped or deleted.
 *
 * If a task is associated with some desktop's activity it can remember that
 * too.
 *
 * It can also contain subtasks - these are managed using the
 * QListViewItem class.
 */
class Task : public TasksModelItem
{
public:
    enum {
        SortRole = Qt::UserRole,
    };

    Task(const QString& taskname, const QString& taskdescription, long minutes, long sessionTime,
        DesktopList desktops, ProjectModel *projectModel, Task* parentTask);
    Task(const KCalCore::Todo::Ptr &incident, ProjectModel *projectModel);

    /* destructor */
    ~Task() override = default;

    Task* parentTask() const { return dynamic_cast<Task *>(TasksModelItem::parent()); }

    /** Return unique iCalendar Todo ID for this task. */
    QString uid() const;

    /**
     * Deliver the depth of a task, i.e. how many tasks are supertasks to it.
     * A toplevel task has the depth 0.
     */
    int depth();

    void delete_recursive();

    /** cut Task out of parent Task or the TaskView */
    void cut();

    /** cut Task out of parent Task or the TaskView and into the
     *  destination Task */
    void move(TasksModelItem* destination);

    /** insert Task into the destination Task */
    void paste(TasksModelItem* destination);

    //@{ timing related functions

      /**
       * Change task time.  Adds minutes to both total time and session time by adding an event.
       *
       *  @param minutes        minutes to add to - may be negative
       *  @param storage        Pointer to TimeTrackerStorage instance.
       *                        If zero, don't save changes.
       */
      void changeTime(long minutes, EventsModel *eventsModel);

      /**
       * Add minutes to time and session time by adding an event, and write to storage.
       *
       *  @param minutesSession   minutes to add to task session time
       *  @param minutes          minutes to add to task time
       *  @param storage          Pointer to TimeTrackerStorage instance.
       *                          If zero, don't save changes.
       */
      void changeTimes(long minutesSession, long minutes, EventsModel *eventsModel);

      /** adds minutes to total and session time by adding an event
       *
       *  @param minutesSession   minutes to add to task total session time
       *  @param minutes          minutes to add to task total time
       */
      void changeTotalTimes(long minutesSession, long minutes);

      /** Adds minutes to the time of the task and the total time of its supertasks. This does not add an event.
       *
       *  @param minutes          minutes to add to the time
       *  @returns                A QString with the error message, in case of no error an empty QString.
       *
       */
      QString addTime(long minutes);

      /** Adds minutes to the total time of the task and its supertasks. This does not add an event.
       *
       *  @param minutes          minutes to add to the time
       *  @returns                A QString with the error message, in case of no error an empty QString.
       *
       */
      QString addTotalTime(long minutes);

      /** Adds minutes to the task's session time and its supertasks' total session time. This does not add an event.
       *
       *  @param minutes          minutes to add to the session time
       *  @returns                A QString with the error message, in case of no error an empty QString.
       *
       */
      QString addSessionTime(long minutes);

      /** Adds minutes to the task's and its supertasks' total session time. This does not add an event.
       *
       *  @param minutes          minutes to add to the session time
       *  @returns                A QString with the error message, in case of no error an empty QString.
       *
       */
      QString addTotalSessionTime(long minutes);

      /** Sets the time (not session time). This does not add an event.
       *
       *  @param minutes          minutes to set time to
       *
       */
      QString setTime(long minutes);

      /** Sets the total time, does not change the parent's total time.
        This means the parent's total time can run out of sync.
        */
      void setTotalTime(long minutes) { m_totalTime = minutes; }

      /** Sets the total session time, does not change the parent's total session time.
        This means the parent's total session time can run out of sync.
        */
      void setTotalSessionTime(long minutes) { m_totalSessionTime = minutes; }

      /** A recursive function to calculate the total time of a task. */
      QString recalculatetotaltime();

      /** A recursive function to calculate the total session time of a task. */
      QString recalculatetotalsessiontime();

      /** Sets the session time.
       * Set the session time without changing totalTime nor sessionTime.
       * Do not change the parent's totalTime.
       * Do not add an event.
       * See also: changeTimes(long, long) and resetTimes
       *
       *  @param minutes          minutes to set session time to
       *
       */
      QString setSessionTime(long minutes);

      /**
       * Reset all times to 0 and adjust parent task's totalTiMes.
       */
      void resetTimes();

      /** @return time in minutes */
      long time() const { return m_time; }
      /** @return total time in minutes */
      long totalTime() const { return m_totalTime; }
      long sessionTime() const { return m_sessionTime; }
      long totalSessionTime() const { return m_totalSessionTime; }
      QDateTime sessionStartTiMe() const;

      /**
       * Return time the task was started.
       */
      QDateTime startTime() const;

      /** sets session time to zero. */
      void startNewSession();
    //@}

    //@{ desktop related functions

      void setDesktopList(const DesktopList& desktopList);
      DesktopList desktops() const;

      QString getDesktopStr() const;
    //@}

    //@{ name related functions

      /** sets the name of the task
       *  @param name    a pointer to the name. A deep copy will be made.
       *  @param storage a pointer to a TimeTrackerStorage object.
       */
    void setName(const QString& name, TimeTrackerStorage* storage);

    /** sets the description of the task */
    void setDescription(const QString& description);

    /** returns the name of this task.
    *  @return a pointer to the name.
    */
    QString name() const;

    /** returns the description of this task.
    * @return a pointer to the description.
    */
    QString description() const;

    /**
    * Returns that task name, prefixed by parent tree up to root.
    *
    * Task names are separated by a forward slash:  /
    */
    QString fullName() const;
    //@}

    /** Update the display of the task (all columns) in the UI. */
    void update();

    //@{ the state of a Task - stopped, running

    /** starts or stops a task
     * @param on   True or false for starting or stopping a task
     * @param when Time when the task was started or stopped. Normally
     *             QDateTime::currentDateTime, but if calendar has
     *             been changed by another program and being reloaded
     *             the task is set to running with another start date
     */
    void setRunning(bool on, const QDateTime &when = QDateTime::currentDateTime());

    /**
     * Resume the running state of a task.
     * This is the same as setrunning, but the storage is not modified.
     */
    void resumeRunning();

    /** return the state of a task - if it's running or not
     *  @return         true or false depending on whether the task is running
     */
    bool isRunning() const;
    //@}

    /**
     *  Parses an incidence. This is needed e.g. when you create a task out of a todo.
     *  You read the todo, extract its custom properties (like session time)
     *  and use these data to initialize the task.
     */
    bool parseIncidence( const KCalCore::Incidence::Ptr &, long& minutes,
        long& sessionMinutes, QString& sessionStartTiMe, QString& name, QString& description, DesktopList& desktops,
        int& percent_complete, int& priority );

    /**
     *  Load the todo passed in with this tasks info.
     */
    KCalCore::Todo::Ptr asTodo(const KCalCore::Todo::Ptr &calendar) const;

    /**
     *  Add a comment to this task.
     *  A comment is called "description" in the context of KCalCore::ToDo
     *
     * iCal allows multiple comment tags.  So we just add a new comment to the
     * todo for this task and write the calendar.
     *
     * @param task     The task that gets the comment
     * @param comment  The comment
     */
    void addComment(const QString& comment, TimeTrackerStorage* storage);

    /** Retrieve the entire comment for the task. */
    QString comment() const;

    /** tells you whether this task is the root of the task tree */
    bool isRoot() const { return !parentTask(); }

    /** remove Task with all it's children
     * Removes task as well as all event history for this task.
     *
     * @param storage a pointer to a TimeTrackerStorage object.
     */
    bool remove(TimeTrackerStorage* storage);

    /**
     * Update percent complete for this task.
     *
     * Tasks that are complete (i.e., percent = 100) do not show up in
     * taskview.  If percent NULL, set to zero.  If greater than 100, set to
     * 100.  If less than zero, set to zero.
     */
    void setPercentComplete(int percent);

    int percentComplete() const;

    /**
     * Update priority for this task.
     *
     * Priority is allowed from 0 to 9. 0 unspecified, 1 highest and 9 lowest.
     */
    void setPriority(int priority);

    int priority() const;

    /** Return true if task is complete (percent complete equals 100).  */
    bool isComplete();

    QVariant data(int column, int role) const override;

protected:
    void changeParentTotalTimes(long minutesSession, long minutes);

private:
    /** initialize a task */
    void init(
        const QString& taskname, const QString& taskdescription, long minutes, long sessionTime, QString sessionStartTiMe,
        const DesktopList& desktops, int percent_complete, int priority);

    bool m_isRunning;

    /** The iCal unique ID of the Todo for this task. */
    QString m_uid;

    /** The comment associated with this Task. */
    QString m_comment;

    int m_percentComplete;

    /** task name */
    QString m_name;

    /** task description */
    QString m_description;

    /** Last time this task was started. */
    QDateTime m_lastStart;

    /** totals of the whole subtree including self */
    long m_totalTime;
    long m_totalSessionTime;

    /** times spend on the task itself */
    long m_time;
    long m_sessionTime;

    /** time when the session was started */
    QDateTime m_sessionStartTime;

    DesktopList m_desktops;

    /** Priority of the task. */
    int m_priority;

    ProjectModel *m_projectModel;
};

#endif // KTIMETRACKER_TASK_H
