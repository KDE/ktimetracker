/*
    SPDX-FileCopyrightText: 2026 KTimeTracker contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "perioddailyreport.h"

#include <QMap>

#include <KLocalizedString>

#include "base/ktimetrackerutility.h"
#include "model/eventsmodel.h"
#include "model/projectmodel.h"
#include "model/task.h"
#include "model/tasksmodel.h"

struct CommentEntry {
    QString text;
    int64_t seconds = 0;
};

struct TaskDayEntry {
    QString name;
    int64_t totalSeconds = 0;
    QList<CommentEntry> comments;
};

// date -> (taskUid -> entry)
using DayMap = QMap<QDate, QMap<QString, TaskDayEntry>>;

QString exportPeriodDailyReportToString(ProjectModel *projectModel, const ReportCriteria &rc)
{
    const QDate fromDate = rc.from;
    const QDate toDate = rc.to;
    const QString cr = QStringLiteral("\n");

    DayMap dayMap;

    for (const Event *event : projectModel->eventsModel()->events()) {
        if (!event->hasEndDate()) {
            continue; // skip currently running timers
        }

        const QDate eventDate = event->dtStart().date();
        if (eventDate < fromDate || eventDate > toDate) {
            continue;
        }

        const QString taskUid = event->relatedTo();
        if (taskUid.isEmpty()) {
            continue;
        }

        Task *task = dynamic_cast<Task *>(projectModel->tasksModel()->taskByUID(taskUid));
        if (!task) {
            continue;
        }

        TaskDayEntry &entry = dayMap[eventDate][taskUid];
        if (entry.name.isEmpty()) {
            QString fullName = task->name();
            Task *parent = task->parentTask();
            while (parent) {
                fullName = parent->name() + QStringLiteral(" -> ") + fullName;
                parent = parent->parentTask();
            }
            entry.name = fullName;
        }

        const int64_t durationSecs = event->dtEnd().toSecsSinceEpoch() - event->dtStart().toSecsSinceEpoch();
        if (durationSecs > 0) {
            entry.totalSeconds += durationSecs;
        }

        // Use only the last comment — earlier entries are previous edit versions
        const QStringList eventComments = event->comments();
        if (!eventComments.isEmpty()) {
            const QString comment = eventComments.last().trimmed();
            if (!comment.isEmpty()) {
                entry.comments.append({comment, durationSecs > 0 ? durationSecs : 0});
            }
        }
    }

    // Build report string
    QString report;

    const QString title = i18n("Period Daily Report — %1 – %2",
                                QLocale().toString(fromDate, QLocale::ShortFormat),
                                QLocale().toString(toDate, QLocale::ShortFormat));
    report += title + cr;
    report += QString(title.length(), QChar::fromLatin1('=')) + cr + cr;

    if (dayMap.isEmpty()) {
        report += i18n("No tasks recorded for this period.") + cr;
        return report;
    }

    int64_t grandTotalSeconds = 0;

    for (auto dayIt = dayMap.cbegin(); dayIt != dayMap.cend(); ++dayIt) {
        const QDate date = dayIt.key();
        const QMap<QString, TaskDayEntry> &tasks = dayIt.value();

        const QString dayHeader = QLocale().toString(date, QLocale::LongFormat);
        report += dayHeader + cr;
        report += QString(dayHeader.length(), QChar::fromLatin1('-')) + cr;

        int64_t dayTotalSeconds = 0;

        for (const TaskDayEntry &entry : tasks) {
            const double minutes = static_cast<double>(entry.totalSeconds) / 60.0;
            report += i18n("Task: %1", entry.name) + cr;
            report += QStringLiteral("  ") + i18n("Time: %1", formatTime(minutes, rc.decimalMinutes)) + cr;

            if (!entry.comments.isEmpty()) {
                report += QStringLiteral("  ") + i18n("Comments:") + cr;
                for (const CommentEntry &c : entry.comments) {
                    const double mins = static_cast<double>(c.seconds) / 60.0;
                    report += QStringLiteral("    - ") + c.text
                        + QStringLiteral(" [") + formatTime(mins, rc.decimalMinutes) + QStringLiteral("]") + cr;
                }
            }

            report += cr;
            dayTotalSeconds += entry.totalSeconds;
        }

        const double dayMinutes = static_cast<double>(dayTotalSeconds) / 60.0;
        report += i18n("Day total: %1", formatTime(dayMinutes, rc.decimalMinutes)) + cr + cr;
        grandTotalSeconds += dayTotalSeconds;
    }

    const double totalMinutes = static_cast<double>(grandTotalSeconds) / 60.0;
    report += QString(40, QChar::fromLatin1('=')) + cr;
    report += i18n("Total: %1", formatTime(totalMinutes, rc.decimalMinutes)) + cr;

    return report;
}
