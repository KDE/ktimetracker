/*
    SPDX-FileCopyrightText: 2003 Mark Bucciarelli <mark@hubcapconsutling.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "totalsastext.h"

#include <QDateTime>

#include <KLocalizedString>

#include "base/ktimetrackerutility.h" // formatTime()
#include "model/task.h"
#include "model/tasksmodel.h"

static const int taskWidth = 40;
static const int timeWidth = 6;
static const int reportWidth = taskWidth + timeWidth;

static const auto cr = QStringLiteral("\n");

// Print out "<indent for level> <task total> <task>", for task and subtasks.
// Used by totalsAsText.
static void printTask(Task *task, QString &s, int level, const ReportCriteria &rc)
{
    QString buf;

    s += buf.fill(QChar::fromLatin1(' '), level);
    if (!rc.sessionTimes) {
        s += QString(QString::fromLatin1("%1    %2"))
                 .arg(formatTime(static_cast<double>(task->totalTime()), rc.decimalMinutes), timeWidth)
                 .arg(task->name());
    } else {
        // print session times
        s += QString(QString::fromLatin1("%1    %2"))
                 .arg(formatTime(static_cast<double>(task->totalSessionTime()), rc.decimalMinutes), timeWidth)
                 .arg(task->name());
    }
    s += cr;

    for (int i = 0; i < task->childCount(); ++i) {
        Task *subTask = dynamic_cast<Task *>(task->child(i));
        if (!rc.sessionTimes) {
            if (subTask->totalTime()) { // to avoid 00:00 entries
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

    line.fill(QChar::fromLatin1('-'), reportWidth);
    line += cr;

    // header
    retval += i18n("Task Totals") + cr;
    retval += QLocale().toString(QDateTime::currentDateTime());
    retval += cr + cr;
    retval += QString(QString::fromLatin1("%1    %2")).arg(i18n("Time"), timeWidth).arg(i18n("Task"));
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
                Task *task = dynamic_cast<Task *>(model->topLevelItem(i));
                if (!rc.sessionTimes) {
                    sum += task->totalTime();
                } else {
                    sum += task->totalSessionTime();
                }
                if ((task->totalTime() && !rc.sessionTimes) || (task->totalSessionTime() && rc.sessionTimes)) {
                    printTask(task, retval, 0, rc);
                }
            }
        }
        // total
        buf.fill(QChar::fromLatin1('-'), reportWidth);
        retval += QString(QString::fromLatin1("%1")).arg(buf, timeWidth) + cr;
        retval += QString(QString::fromLatin1("%1 %2"))
                      .arg(formatTime(sum, rc.decimalMinutes), timeWidth)
                      .arg(i18nc("total time of all tasks", "Total"));
    } else {
        retval += i18n("No tasks.");
    }

    return retval;
}
