/*
 * Copyright (C) 2003 by Mark Bucciarelli <mark@hubcapconsutling.com>
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

#include "totalsastext.h"

#include <QDateTime>

#include <KLocalizedString>

#include "ktimetrackerutility.h"        // formatTime()
#include "model/task.h"
#include "model/tasksmodel.h"

static const int taskWidth = 40;
static const int timeWidth = 6;
static const int reportWidth = taskWidth + timeWidth;

static const auto cr = QStringLiteral("\n");

// Print out "<indent for level> <task total> <task>", for task and subtasks. Used by totalsAsText.
static void printTask(Task *task, QString &s, int level, const ReportCriteria &rc)
{
    QString buf;

    s += buf.fill(' ', level);
    if (!rc.sessionTimes) {
        s += QString(QString::fromLatin1("%1    %2"))
            .arg(formatTime(task->totalTime(), rc.decimalMinutes), timeWidth)
            .arg(task->name());
    } else {
        // print session times
        s += QString(QString::fromLatin1("%1    %2"))
            .arg(formatTime(task->totalSessionTime(), rc.decimalMinutes), timeWidth)
            .arg(task->name());
    }
    s += cr;

    for (int i = 0; i < task->childCount(); ++i) {
        Task *subTask = dynamic_cast<Task*>(task->child(i));
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

QString totalsAsText(TasksModel *model, Task *currentItem, const ReportCriteria &rc)
{
    QString retval;
    QString line;
    QString buf;
    int64_t sum;
    bool justThisTask = !rc.allTasks;

    line.fill('-', reportWidth);
    line += cr;

    // header
    retval += i18n("Task Totals") + cr;
    retval += QLocale().toString(QDateTime::currentDateTime());
    retval += cr + cr;
    retval += QString(QString::fromLatin1("%1    %2"))
        .arg(i18n("Time"), timeWidth)
        .arg(i18n("Task"));
    retval += cr;
    retval += line;

    // tasks
    if (currentItem) {
        if (justThisTask) {
            if (!rc.sessionTimes) {
                sum = currentItem->totalTime();
            } else {
                sum = currentItem->totalSessionTime();
            }

            printTask(currentItem, retval, 0, rc);
        } else { // print all tasks
            sum = 0;
            for (int i = 0; i < model->topLevelItemCount(); ++i) {
                Task *task = dynamic_cast<Task*>(model->topLevelItem(i));
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
            .arg(formatTime(sum, rc.decimalMinutes), timeWidth)
            .arg(i18nc("total time of all tasks", "Total"));
    } else {
        retval += i18n("No tasks.");
    }

    return retval;
}
