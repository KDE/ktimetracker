/*
 *   This file only:
 *     Copyright (C) 2003  Mark Bucciarelli <mark@hubcapconsutling.com>
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
 *
 */

// #include <iostream>

#include <qdatetime.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qmap.h>

#include <kglobal.h>
#include <kdebug.h>
#include <klocale.h>            // i18n
#include <event.h>

#include "karmutility.h"        // formatTime()
#include "timekard.h"
#include "task.h"
#include "taskview.h"
#include <assert.h>

const int taskWidth = 40;
const int timeWidth = 6;
const int totalTimeWidth = 7;
const int reportWidth = taskWidth + timeWidth;

const QString cr = QString::fromLatin1("\n");

QString TimeKard::totalsAsText(TaskView* taskview, bool justThisTask)
{
  QString retval;
  QString line;
  QString buf;
  long sum;

  line.fill('-', reportWidth);
  line += cr;

  // header
  retval += i18n("Task Totals") + cr;
  retval += KGlobal::locale()->formatDateTime(QDateTime::currentDateTime());
  retval += cr + cr;
  retval += QString(QString::fromLatin1("%1    %2"))
    .arg(i18n("Time"), timeWidth)
    .arg(i18n("Task"));
  retval += cr;
  retval += line;

  // tasks
  if (taskview->current_item())
  {
    if (justThisTask)
    {
      // a task's total time includes the sum of all subtask times
      sum = taskview->current_item()->totalTime();
      printTask(taskview->current_item(), retval, 0);
    }
    else
    {
      sum = 0;
      for (Task* task= taskview->current_item(); task;
          task= task->nextSibling())
      {
        sum += task->totalTime();
        //if ( task->totalTime() )
          printTask(task, retval, 0);
      }
    }

    // total
    buf.fill('-', reportWidth);
    retval += QString(QString::fromLatin1("%1")).arg(buf, timeWidth) + cr;
    retval += QString(QString::fromLatin1("%1 %2"))
      .arg(formatTime(sum),timeWidth)
      .arg(i18n("Total"));
  }
  else
    retval += i18n("No tasks.");

  return retval;
}

// Print out "<indent for level> <task total> <task>", for task and subtasks. Used by totalsAsText.
void TimeKard::printTask(Task *task, QString &s, int level)
{
  QString buf;

  s += buf.fill(' ', level);
  s += QString(QString::fromLatin1("%1    %2"))
    .arg(formatTime(task->totalTime()), timeWidth)
    .arg(task->name());
  s += cr;

  for (Task* subTask = task->firstChild();
      subTask;
      subTask = subTask->nextSibling())
  {
    //if ( subTask->totalTime() ) // to avoid 00:00 entries
    printTask(subTask, s, level+1);
  }
}

void TimeKard::printTaskHistory(const Task *task,
    const QMap<QString,long>& taskdaytotals,
    QMap<QString,long>& daytotals,
    const QDate& from,
    const QDate& to,
    const int level, QString& s)
{
  long sectionsum = 0;
  for ( QDate day = from; day <= to; day = day.addDays(1) )
  {
    QString daykey = day.toString(QString::fromLatin1("yyyyMMdd"));
    QString daytaskkey = QString::fromLatin1("%1_%2")
                         .arg(daykey)
                         .arg(task->uid());

    if (taskdaytotals.contains(daytaskkey))
    {
      s += QString::fromLatin1("%1")
        .arg(formatTime(taskdaytotals[daytaskkey]/60), timeWidth);
      sectionsum += taskdaytotals[daytaskkey];  // in seconds

      if (daytotals.contains(daykey))
        daytotals.replace(daykey, daytotals[daykey] + taskdaytotals[daytaskkey]);
      else
        daytotals.insert(daykey, taskdaytotals[daytaskkey]);
    }
    else
    {
      QString buf;
      buf.fill(' ', timeWidth);
      s += buf;
    }
  }

  // Total for task this section (e.g. week)
  s += QString::fromLatin1("%1").arg(formatTime(sectionsum/60), totalTimeWidth);

  // Task name
  QString buf;
  s += buf.fill(' ', level + 1);
  s += QString::fromLatin1("%1").arg(task->name());
  s += cr;

  for (Task* subTask = task->firstChild();
      subTask;
      subTask = subTask->nextSibling())
  {
    // recursive
    printTaskHistory(subTask, taskdaytotals, daytotals, from, to, level+1, s);
  }
}

QString TimeKard::sectionHistoryAsText(
  TaskView* taskview,
  const QDate& sectionFrom, const QDate& sectionTo,
  const QDate& from, const QDate& to,
  const QString& name,
  bool justThisTask )
{

  const int sectionReportWidth = taskWidth + sectionFrom.daysTo(sectionTo) * timeWidth + totalTimeWidth;
  assert( sectionReportWidth > 0 );
  QString line;
  line.fill('-', sectionReportWidth);
  line += cr;

  QValueList<HistoryEvent> events;
  if ( sectionFrom < from && sectionTo > to)
  {
    events = taskview->getHistory(from, to);
  }
  else if ( sectionFrom < from )
  {
    events = taskview->getHistory(from, sectionTo);
  }
  else if ( sectionTo > to)
  {
    events = taskview->getHistory(sectionFrom, to);
  }
  else
  {
    events = taskview->getHistory(sectionFrom, sectionTo);
  }

  QMap<QString, long> taskdaytotals;
  QMap<QString, long> daytotals;

  // Build lookup dictionary used to output data in table cells.  keys are
  // in this format: YYYYMMDD_NNNNNN, where Y = year, M = month, d = day and
  // NNNNN = the VTODO uid.  The value is the total seconds logged against
  // that task on that day.  Note the UID is the todo id, not the event id,
  // so times are accumulated for each task.
  for (QValueList<HistoryEvent>::iterator event = events.begin(); event != events.end(); ++event)
  {
    QString daykey = (*event).start().date().toString(QString::fromLatin1("yyyyMMdd"));
    QString daytaskkey = QString::fromLatin1("%1_%2")
                         .arg(daykey)
                         .arg((*event).todoUid());

    if (taskdaytotals.contains(daytaskkey))
      taskdaytotals.replace(daytaskkey,
                            taskdaytotals[daytaskkey] + (*event).duration());
    else
      taskdaytotals.insert(daytaskkey, (*event).duration());
  }

  QString retval;
  // section name (e.g. week name)
  retval += cr + cr;
  QString buf;
  if ( name.length() < (unsigned int)sectionReportWidth )
    buf.fill(' ', int((sectionReportWidth - name.length()) / 2));
  retval += buf + name + cr;

  // day headings
  for (QDate day = sectionFrom; day <= sectionTo; day = day.addDays(1))
  {
    retval += QString::fromLatin1("%1").arg(day.day(), timeWidth);
  }
  retval += cr;
  retval += line;

  // the tasks
  if (events.empty())
  {
    retval += i18n("  No hours logged.");
  }
  else
  {
    if (justThisTask)
    {
      printTaskHistory(taskview->current_item(), taskdaytotals, daytotals,
                       sectionFrom, sectionTo, 0, retval);
    }
    else
    {
      for (Task* task= taskview->current_item(); task;
           task= task->nextSibling())
      {
        printTaskHistory(task, taskdaytotals, daytotals, sectionFrom, sectionTo, 0, retval);
      }
    }
    retval += line;

    // per-day totals at the bottom of the section
    long sum = 0;
    for (QDate day = sectionFrom; day <= sectionTo; day = day.addDays(1))
    {
      QString daykey = day.toString(QString::fromLatin1("yyyyMMdd"));

      if (daytotals.contains(daykey))
      {
        retval += QString::fromLatin1("%1")
                  .arg(formatTime(daytotals[daykey]/60), timeWidth);
        sum += daytotals[daykey];  // in seconds
      }
      else
      {
        buf.fill(' ', timeWidth);
        retval += buf;
      }
    }

    retval += QString::fromLatin1("%1 %2")
              .arg(formatTime(sum/60), totalTimeWidth)
              .arg(i18n("Total"));
  }
  return retval;
}

QString TimeKard::historyAsText(TaskView* taskview, const QDate& from,
    const QDate& to, bool justThisTask, bool perWeek)
{
  // header
  QString retval;
  retval += i18n("Task History") + cr;
  retval += i18n("From %1 to %2")
    .arg(KGlobal::locale()->formatDate(from))
    .arg(KGlobal::locale()->formatDate(to));
  retval += cr;
  retval += i18n("Printed on: %1")
    .arg(KGlobal::locale()->formatDateTime(QDateTime::currentDateTime()));

  if ( perWeek )
  {
    // output one time card table for each week in the date range
    QValueList<Week> weeks = Week::weeksFromDateRange(from, to);
    for (QValueList<Week>::iterator week = weeks.begin(); week != weeks.end(); ++week)
    {
      retval += sectionHistoryAsText( taskview, (*week).start(), (*week).end(), from, to, (*week).name(), justThisTask );
    }
  } else
  {
    retval += sectionHistoryAsText( taskview, from, to, from, to, "", justThisTask );
  }
  return retval;
}

Week::Week() {}

Week::Week(QDate from)
{
  _start = from;
}

QDate Week::start() const
{
  return _start;
}

QDate Week::end() const
{
  return _start.addDays(7);
}

QString Week::name() const
{
  return i18n("Week of %1").arg(KGlobal::locale()->formatDate(start()));
}

QValueList<Week> Week::weeksFromDateRange(const QDate& from, const QDate& to)
{
  QDate start;
  QValueList<Week> weeks;

  // The QDate weekNumber() method always puts monday as the first day of the
  // week.
  //
  // Not that it matters here, but week 1 always includes the first Thursday
  // of the year.  For example, January 1, 2000 was a Saturday, so
  // QDate(2000,1,1).weekNumber() returns 52.

  // Since report always shows a full week, we generate a full week of dates,
  // even if from and to are the same date.  The week starts on the day
  // that is set in the locale settings.
  start = from.addDays(
      -((7 - KGlobal::locale()->weekStartDay() + from.dayOfWeek()) % 7));

  for (QDate d = start; d <= to; d = d.addDays(7))
    weeks.append(Week(d));

  return weeks;
}

