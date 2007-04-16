/*
 *     Copyright (C) 2007 the ktimetracker developers
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
#ifndef KARM_TASK_H
#define KARM_TASK_H

// Required b/c QPtrList is a struct, not a class.
#include <q3ptrlist.h>

#include <QPixmap>
#include <QVector>

#include <QDateTime>

// Required b/c DesktopList is a typedef not a class.
#include "desktoplist.h"

// Required b/c of static cast below?  (How else can compiler know that a
// TaskView is a subclass or QListView?)
#include "taskview.h"

class QFile;
class QString;
class KarmStorage;

class QTimer;
namespace KCal {
class Incidence;
class Todo;
}
class QObject;
class QPixmap;

/// \class Task
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
class Task : public QObject, public QTreeWidgetItem
{
  Q_OBJECT

  public:
    //@{ constructors
    Task( const QString& taskame, long minutes, long sessionTime,
          DesktopList desktops, TaskView* parent = 0);
    Task( const QString& taskame, long minutes, long sessionTime,
          DesktopList desktops, Task* parent = 0);
    Task( KCal::Todo* incident, TaskView* parent );
    //@}
    /* destructor */
    ~Task();

    /** return parent Task or null in case of TaskView.
     *  same as QListViewItem::parent()
     */
    Task* firstChild() const  { return (Task*)QTreeWidgetItem::child(0); }
    Task* nextSibling() const { static int sibling=0; sibling++; return (Task*)QTreeWidgetItem::child(sibling); }
    Task* parent() const      { return (Task*)QTreeWidgetItem::parent(); }

    /** Return task view for this task */
    TaskView* taskView() const {
        return static_cast<TaskView *>( treeWidget() );
    }

    /** Return unique iCalendar Todo ID for this task. */
    QString uid() const       { return _uid; }
 
    int depth();

    /**
     * Set unique id for the task.
     *
     * The uid is the key used to update the storage.
     *
     * @param uid  The new unique id.
     */
    void setUid(const QString uid);

    /** cut Task out of parent Task or the TaskView */
    void cut();
    /** cut Task out of parent Task or the TaskView and into the
     *  destination Task */
    void move(Task* destination);
    /** insert Task into the destination Task */
    void paste(Task* destination);

    //@{ timing related functions

      /**
       * Change task time.  Adds minutes to both total time and session time.
       *
       *  @param minutes        minutes to add to - may be negative
       *  @param storage        Pointer to KarmStorage instance.
       *                        If zero, don't save changes.
       */
      void changeTime( long minutes, KarmStorage* storage );

      /**
       * Add minutes to time and session time, and write to storage.
       *
       *  @param minutesSession   minutes to add to task session time
       *  @param minutes          minutes to add to task time
       *  @param storage          Pointer to KarmStorage instance.
       *                          If zero, don't save changes.
       */
      void changeTimes
        ( long minutesSession, long minutes, KarmStorage* storage=0);

      /** adds minutes to total and session time
       *
       *  @param minutesSession   minutes to add to task total session time
       *  @param minutes          minutes to add to task total time
       */
      void changeTotalTimes( long minutesSession, long minutes );

      /**
       * Reset all times to 0
       */
      void resetTimes();

      /*@{ returns the times accumulated by the task
       * @return total time in minutes
       */
      long time() const { return _time; };
      long totalTime() const { return _totalTime; };
      long sessionTime() const { return _sessionTime; };
      long totalSessionTime() const { return _totalSessionTime; };

      /**
       * Return time the task was started.
       */
      QDateTime startTime() const { return _lastStart; };

      /** sets session time to zero. */
      void startNewSession() { changeTimes( -_sessionTime, 0 ); };
    //@}

    //@{ desktop related functions

      void setDesktopList ( DesktopList dl );
      DesktopList getDesktops() const { return _desktops;}

      QString getDesktopStr() const;
    //@}

    //@{ name related functions

      /** sets the name of the task
       *  @param name    a pointer to the name. A deep copy will be made.
       *  @param storage a pointer to a KarmStorage object.
       */
      void setName( const QString& name, KarmStorage* storage );

      /** returns the name of this task.
       *  @return a pointer to the name.
       */
      QString name() const  { return _name; };

      /**
       * Returns that task name, prefixed by parent tree up to root.
       *
       * Task names are seperated by a forward slash:  /
       */
      QString fullName() const;
    //@}

    /** Update the display of the task (all columns) in the UI. */
    void update();

    //@{ the state of a Task - stopped, running

      /** starts or stops a task
       *  @param on       true or false for starting or stopping a task
       *  @param storage a pointer to a KarmStorage object.
       *  @param when time when the task was started or stopped. Normally
				    QDateTime::currentDateTime, but if calendar has 
				    been changed by another program and being reloaded
 				    the task is set to running with another start date
       */
      void setRunning( bool on, KarmStorage* storage, QDateTime when= QDateTime::currentDateTime());

      /** return the state of a task - if it's running or not
       *  @return         true or false depending on whether the task is running
       */
      bool isRunning() const;
    //@}

    bool parseIncidence(KCal::Incidence*, long& minutes,
        long& sessionMinutes, QString& name, DesktopList& desktops,
        int& percent_complete);

    /**
     *  Load the todo passed in with this tasks info.
     */
    KCal::Todo* asTodo(KCal::Todo* calendar) const;

    /**
     *  Set a task's description
     *  A description is a comment.
     */
    void setDescription( QString desc, KarmStorage* storage );

    /** 
     *  Add a comment to this task. 
     *  A comment is called "description" in the context of KCal::ToDo
     */
    void addComment( QString comment, KarmStorage* storage );

    /** Retrieve the entire comment for the task. */
    QString comment() const;

    /** tells you whether this task is the root of the task tree */
    bool isRoot() const                 { return parent() == 0; }

    /** remove Task with all it's children
     * @param   activeTasks - list of aktive tasks
     * @param storage a pointer to a KarmStorage object.
     */
    bool remove( Q3PtrList<Task>& activeTasks, KarmStorage* storage );

    /**
     * Update percent complete for this task.
     *
     * Tasks that are complete (i.e., percent = 100) do not show up in
     * taskview.  If percent NULL, set to zero.  If greater than 100, set to
     * 100.  If less than zero, set to zero.
     */
    void setPercentComplete(const int percent, KarmStorage *storage);
    int percentComplete() const { return _percentcomplete; }


    /** Sets an appropriate icon for this task based on its level of
     * completion */
    void setPixmapProgress();

    /** Return true if task is complete (percent complete equals 100).  */
    bool isComplete();

    /** Remove current task and all it's children from the view.  */
    void removeFromView();

    /** delivers when the task was started last */
    QDateTime lastStart() { return _lastStart; }

  protected:
    void changeParentTotalTimes( long minutesSession, long minutes );

  signals:
    void totalTimesChanged( long minutesSession, long minutes);
    /** signal that we're about to delete a task */
    void deletingTask(Task* thisTask);

  protected slots:
    /** animate the active icon */
    void updateActiveIcon();

  private:

    /** The iCal unique ID of the Todo for this task. */
    QString _uid;

    /** The comment associated with this Task. */
    QString _comment;

    int _percentcomplete;

    long totalTimeInSeconds() const      { return _totalTime * 60; }

    /** if the time or session time is negative set them to zero */
    void noNegativeTimes();

    /** initialize a task */
    void init( const QString& taskame, long minutes, long sessionTime,
               DesktopList desktops, int percent_complete);


    /** task name */
    QString _name;

    /** Last time this task was started. */
    QDateTime _lastStart;

    //@{ totals of the whole subtree including self
      long _totalTime;
      long _totalSessionTime;
    //@}

    //@{ times spend on the task itself
      long _time;
      long _sessionTime;
    //@}
    DesktopList _desktops;
    QTimer *_timer;
    int _currentPic;
    static QVector<QPixmap*> *icons;

    /** Don't need to update storage when deleting task from list. */
    bool _removing;

};

#endif // KARM_TASK_H
