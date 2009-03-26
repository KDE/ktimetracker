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

#ifndef KARM_STORAGE_H
#define KARM_STORAGE_H

#include <QList>
#include <QStack>
#include <QVector>

#include "reportcriteria.h"

#include "desktoplist.h"

#include <kcal/journal.h>
#include <kcal/calendarresources.h>
#include <kcal/resourcecalendar.h>

namespace KCal {
  class Todo;
}
class QDateTime;
class Task;
class TaskView;
class HistoryEvent;

/**
 * Class to store/retrieve KTimeTracker data to/from persistent storage.
 *
 * The storage is an iCalendar file.
 *
 * All logic that deals with getting and saving data should go here. The
 * storage logic has changed at least twice already in KArm's history, and
 * chances are good it will change again.
 *
 * Prior to KDE 3.2, KArm just stored totals for each task--a session total
 * and a task total.  The session total was reset to zero each time KArm
 * started up or after the user reset the session times to zero.  With the
 * release of KDE 3.2, KArm now stores these task totals as well as logging
 * the history of each start/stop event; that is, every time you start a timer
 * and then stop a timer on a task, KArm records this as an iCalendar event.
 *
 * @short Logic that gets and stores KTimeTracker data to disk.
 * @author Mark Bucciarelli <mark@hubcapconsulting.com>
 */

class KarmStorage : public QObject
{
  Q_OBJECT

  public:
    KarmStorage();
    ~KarmStorage();

    QString setTaskParent( Task* task, Task* parent);

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

      @param taskview     The list group used in the TaskView
      @param fileName     Override preferences' filename

      @return empty string if success, error message if error.
     */
    QString load( TaskView* taskview, const QString &fileName );

   /**
    * Return the name of the iCal file
    */
    QString icalfile();

   /**
    * Build up the taskview.
    *
    * This is needed if the iCal file has been modified.
    */
    QString buildTaskView(KCal::ResourceCalendar *rc, TaskView *view);

   /**
    * Build up the taskview.
    *
    * This is needed if a subtask has been deleted.
    */
    QString buildTaskView(TaskView *view);

    /** Close calendar and clear view.  Release lock if holding one. */
    void closeStorage();

    /** list of all events */
    KCal::Event::List rawevents();

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
   
    /**
     * Save all tasks and their totals to an iCalendar file.
     *
     * All tasks must have an associated VTODO object already created in the
     * calendar file; that is, the task->uid() must refer to a valid VTODO in
     * the calendar.
     * Delivers empty string if successful, else error msg.
     *
     * @param taskview    The list group used in the TaskView
     */
    QString save(TaskView* taskview);

    /**
     Output a report based on contents of ReportCriteria.
     */
    QString report( TaskView *taskview, const ReportCriteria &rc );

    /**
     * Log the change in a task's time.
     *
     * This is also called when a timer is stopped.
     * We create an iCalendar event to store each change.  The event start
     * date is set to the current datetime.  If time is added to the task, the
     * task end date is set to start time + delta.  If the time is negative,
     * the end date is set to the start time.
     *
     * In both cases (postive or negative delta), we create a custom iCalendar
     * property that stores the delta (in seconds). This property is called
     * X-KDE-ktimetracker-duration.
     *
     * Note that the ktimetracker UI allows the user to change both the session and
     * the total task time, and this routine does not account for all posibile
     * cases.  For example, it is possible for the user to do something crazy
     * like add 10 minutes to the session time and subtract 50 minutes from
     * the total time. Although this change violates a basic law of physics,
     * it is allowed.
     *
     * For now, you should pass in the change to the total task time.
     *
     * @param task   The task the change is for.
     * @param delta  Change in task time, in seconds.  Can be negative.
     */
    void changeTime(const Task* task, const long deltaSeconds);

    /**
     * Book time to a task.
     *
     * Creates an iCalendar event and adds it to the calendar.  Does not write
     * calendar to disk, just adds event to calendar in memory.  However, the
     * resource framework does try to get a lock on the file.  After a
     * successful lock, the calendar marks this incidence as modified and then
     * releases the lock.
     *
     * @param task Task
     * @param startDateTime Date and time the booking starts.
     * @param durationInSeconds Duration of time to book, in seconds.
     *
     * @return true if event was added, false if not (if, for example, the
     * attempted file lock failed).
     */
    bool bookTime(const Task* task, const QDateTime& startDateTime,
                  const long durationInSeconds);

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
     * Log the event that a timer has started for a task.
     *
     * For the iCalendar storage, there is no need to log anything for this
     * event.  We log an event when the timer is stopped.
     *
     * @param task    The task the timer was started for.
     */
    void startTimer(const Task* task, const KDateTime &when=KDateTime::currentLocalDateTime());

    /** 
     * Start the timer for a given task ID
     * 
     * @param taskID  The task ID of the task to be started
     */
    void startTimer( QString taskID );

    /**
     * Log the event that the timer has stopped for this task.
     *
     * The task stores the last time a timer was started, so we log a new iCal
     * Event with the start and end times for this task.
     * @see KarmStorage::changeTime
     *
     * @param task   The task the timer was stopped for.
     * @param when   When the timer stopped.
     */
    void stopTimer( const Task* task, const QDateTime &when = QDateTime::currentDateTime() );

    /**
     * Log a new comment for this task.
     *
     * iCal allows multiple comment tags.  So we just add a new comment to the
     * todo for this task and write the calendar.
     *
     * @param task     The task that gets the comment
     * @param comment  The comment
     */
    void addComment(const Task* task, const QString& comment);


    /**
     * Remove this task from iCalendar file.
     *
     * Removes task as well as all event history for this task.
     *
     * @param task   The task to be removed.
     * @return true if change was saved, false otherwise
     */
    bool removeTask(Task* task);

    /**
     * Remove this task from iCalendar file.
     *
     * Removes task as well as all event history for this task.
     *
     * @param task   The task to be removed.
     * @return true if change was saved, false otherwise
     */
    bool removeTask(QString taskid);

    /**
     * Add this task from iCalendar file.
     *
     * Create a new KCal::Todo object and load with task information.  If
     * parent is not zero, then set the RELATED-TO attribute for this Todo.
     *
     * @param task   The task to be removed.
     * @param parent The parent of this task.  Must have a uid() that is in
     * the existing calendar.  If zero, this task is considered a root task.
     * @return The unique ID for the new VTODO.  Return an null QString if
     * there was an error creating the new calendar object.
     */
    QString addTask(const Task* task, const Task* parent=0);

    /**
     *  Check if the iCalendar file currently loaded has any Todos in it.
     *
     *  @return true if iCalendar file has any todos
     */
    bool isEmpty();

    /** Return a list of all task ids for taskname */
    QStringList taskidsfromname(QString taskname);

    /** 
     * Find the task with the given uid 
     * @param  uid  The uid that identifies the task
     * @param  view The TaskView that contains the task
     * @return the task identified by uid, residing in the TaskView view
     */
    Task* task( const QString& uid, TaskView* view );

    /** Return a list of all task names */
    QStringList taskNames() const;

    /** Return a list of start/stop events for the given date range. */
    QList<HistoryEvent> getHistory(const QDate& from, const QDate& to);

  private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
    KCal::ResourceCalendar            *_calendar;
    QString                           _icalfile;

    void adjustFromLegacyFileFormat(Task* task);
    bool parseLine(QString line, long *time, QString *name, int *level,
        DesktopList* desktopList);
    QString writeTaskAsTodo( Task* task, QStack<KCal::Todo*>& parents );
    QString saveCalendar();

    KCal::Event* baseEvent(const Task*);
    KCal::Event* baseEvent(const KCal::Todo*);
    bool remoteResource( const QString& file ) const;

    /**
     *  Writes all tasks and their totals to a Comma-Separated Values file.
     *
     * The format of this file is zero or more lines of:
     *    taskName,subtaskName,..,sessionTime,time,totalSessionTime,totalTime
     * the number of subtasks is determined at runtime.
     */
    QString exportcsvFile( TaskView *taskview, const ReportCriteria &rc );

    /**
     *  Write task history to file as comma-delimited data.
     */
    QString exportcsvHistory (
            TaskView* taskview,
            const QDate& from,
            const QDate& to,
            const ReportCriteria &rc
            );
};

/**
 * One start/stop event that has been logged.
 *
 * When a task is running and the user stops it, KArm logs this event and
 * saves it in the history.  This class represents such an event read from
 * storage, and abstracts it from the specific storage used.
 */
class HistoryEvent
{
  public:
    /** Needed to be used in a value list. */
    HistoryEvent() {}
    HistoryEvent( const QString &uid, const QString &name, long duration,
                  const KDateTime &start, const KDateTime &stop, 
                  const QString &todoUid );
    QString uid() {return _uid; }
    QString name() {return _name; }
    /** In seconds. */
    long duration() {return _duration; }
    KDateTime start() {return _start; }
    KDateTime stop() { return _stop; }
    QString todoUid() {return _todoUid; }

  private:
    QString _uid;
    QString _todoUid;
    QString _name;
    long _duration;
    KDateTime _start;
    KDateTime _stop;

};

#endif // KARM_STORAGE_H
