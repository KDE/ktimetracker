/*
 *     Copyright (C) 2003 by Mark Bucciarelli <mark@hubcapconsutling.com>
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

#include "timekard.h"

#include <QDateTime>

#include <KLocalizedString>

#include "ktimetrackerutility.h"        // formatTime()
#include "model/task.h"
#include "taskview.h"
#include "ktt_debug.h"

const int taskWidth = 40;
const int timeWidth = 6;
//const int totalTimeWidth = 7;
const int reportWidth = taskWidth + timeWidth;

const QString cr = QString::fromLatin1("\n");

QString TimeKard::totalsAsText(TaskView* taskview, ReportCriteria rc)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString retval;
    QString line;
    QString buf;
    long sum;
    bool justThisTask = !rc.allTasks;

    line.fill('-', reportWidth);
    line += cr;

    // header
    retval += i18n("Task Totals") + cr;
    retval += QLocale().toString(QDateTime::currentDateTime());
//    retval += KGlobal::locale()->formatDateTime(QDateTime::currentDateTime());
    retval += cr + cr;
    retval += QString(QString::fromLatin1("%1    %2"))
        .arg(i18n("Time"), timeWidth)
        .arg(i18n("Task"));
    retval += cr;
    retval += line;

    // tasks
    if (taskview->currentItem()) {
        if (justThisTask) {
            if (!rc.sessionTimes) {
                sum = taskview->currentItem()->totalTime();
            } else {
                sum = taskview->currentItem()->totalSessionTime();
            }

            printTask(taskview->currentItem(), retval, 0, rc);
        } else { // print all tasks
            sum = 0;
            for (int i = 0; i < taskview->tasksModel()->topLevelItemCount(); ++i) {
                Task *task = static_cast<Task*>(taskview->tasksModel()->topLevelItem(i));
                if (!rc.sessionTimes) {
                    sum += task->totalTime();
                } else {
                    sum += task->totalSessionTime();
                }
                if ((task->totalTime() && !rc.sessionTimes) || (task->totalSessionTime() && rc.sessionTimes) ) {
                    printTask(task, retval, 0, rc);
                }
            }
        }
        // total
        buf.fill('-', reportWidth);
        retval += QString(QString::fromLatin1("%1")).arg(buf, timeWidth) + cr;
        retval += QString(QString::fromLatin1("%1 %2"))
            .arg(formatTime(sum), timeWidth)
            .arg(i18nc("total time of all tasks", "Total"));
    } else {
        retval += i18n("No tasks.");
    }

    return retval;
}

// Print out "<indent for level> <task total> <task>", for task and subtasks. Used by totalsAsText.
void TimeKard::printTask(Task *task, QString &s, int level, const ReportCriteria &rc)
{
    qCDebug(KTT_LOG) << "Entering function";
    QString buf;

    s += buf.fill(' ', level);
    if (!rc.sessionTimes) {
        s += QString(QString::fromLatin1("%1    %2"))
            .arg(formatTime(task->totalTime()), timeWidth)
            .arg(task->name());
    } else {
        // print session times
        s += QString(QString::fromLatin1("%1    %2"))
            .arg(formatTime(task->totalSessionTime()), timeWidth)
            .arg(task->name());
    }
    s += cr;

    for (int i = 0; i < task->childCount(); ++i) {
        Task *subTask = static_cast<Task*>(task->child(i));
        if (!rc.sessionTimes) {
            if (subTask->totalTime()) {// to avoid 00:00 entries
                printTask(subTask, s, level + 1, rc);
            }
        } else {
            if (subTask->totalSessionTime()) { // to avoid 00:00 entries
                printTask(subTask, s, level + 1, rc);
            }
        }
    }
}
