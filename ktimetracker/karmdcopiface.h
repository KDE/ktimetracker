/*
 *   This file only:
 *     Copyright (C) 2004  Mark Bucciarelli <mark@hubcapconsulting.com>
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
 *      59 Temple Place - Suite 330
 *      Boston, MA  02111-1307  USA.
 */
#ifndef KARM_DCOP_IFAC_H
#define KARM_DCOP_IFAC_H

#include <dcopobject.h>

/** Define DCOP interface to karm.  Methods implemented in MainWindow */
class KarmDCOPIface : virtual public DCOPObject
{
  K_DCOP
  k_dcop:

  /** Return karm version. */
  virtual QString version() const = 0;

  /** Return id of task found, empty string if no match. */
  virtual QString taskIdFromName( const QString& taskName ) const = 0;

  /** 
   * Add a new top-level task.
   *
   * A top-level task is one that has no parent tasks.
   * 
   * @param taskName Name of new task.
   *
   * @return 0 on success, error number on failure.
   */
  virtual int addTask( const QString& taskName ) = 0;

  /** 
   * Set percent complete to a task
   *
   * @param taskName Name of new task.
   * @param perCent  percent, e.g. 99
   *
   * @return "" on success, error msg on failure.
   */
   virtual QString setPerCentComplete( const QString& taskName, int perCent ) = 0;

  /** 
   * Add time to a task.  
   *
   * The GUI will be non-responsive until this method returns.
   *
   * @return 0 on success, error number on failure.
   *
   * @param taskId Unique ID of task to add time to 
   *
   * @param iso8601StartDateTime Date and time the booking starts, in extended
   * ISO-8601 format; for example, YYYY-MM-DDTHH:MI:SS format (see
   * Qt::ISODate).  No timezone support--time is interpreted as the local time.
   * If just the date is passed in (i.e., YYYY-MM-DD) , then the time is set to
   * noon.
   *
   * @param durationInMinutes The amount of time to book against the taskId.
   * 
   */ 
  virtual int bookTime( const QString& taskId, const QString& iso8601StartDateTime, 
                        long durationInMinutes ) = 0;

  /** 
   * Return error string associated with karm error number. 
   *
   * @param karmErrorNumber An integer error number.
   *
   * @return String associated with error number.  These strings are
   * internationalized.  An unknown error number produces an empty string as
   * the return value.
   *
   */
  virtual QString getError( int karmErrorNumber ) const = 0;

  /**
   * Total time currently associated with a task.
   *
   * A task has two counters: the total session time and the total time.  Note
   * that th euser can reset both counters.
   *
   * @param taskId Unique ID of task to lookup bookings for.
   */
  virtual int totalMinutesForTaskId( const QString& taskId ) = 0;

  /** Start timer for all tasks with the summary taskname.  */
  // may conflict with unitaskmode
  virtual QString starttimerfor( const QString& taskname ) = 0;

  /** Stop timer for all tasks with the summary taskname.  */
  // may conflict with unitaskmode
  virtual QString stoptimerfor( const QString& taskname ) = 0;

  /** delete the current item */
  virtual QString deletetodo() = 0;

  /** set if prompted on deleting a task */
  virtual QString setpromptdelete( bool prompt ) = 0;

  /** get if prompted on deleting a task */
  virtual bool getpromptdelete() = 0;

  /** export csv history or totals file */
  virtual QString exportcsvfile( QString filename, QString from, QString to, int type = 0, bool decimalMinutes=true, bool allTasks=true, QString delimiter=";", QString quote="'" ) = 0;

  /** import planner project file */
  virtual QString importplannerfile( QString filename ) = 0;

  /** save your tasks */
  virtual bool save() = 0;

  /** Graceful shutdown. */
  virtual void quit() = 0;
};

#endif // KARM_DCOP_IFAC_H
