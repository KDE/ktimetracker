/*
    SPDX-FileCopyrightText: 2026 KTimeTracker contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dailyreport.h"

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

struct TaskDayData {
    QString name;
    int64_t totalSeconds = 0;
    QList<CommentEntry> comments;
};

QString exportDailyReportToString(ProjectModel *projectModel, const ReportCriteria &rc)
{
    const QDate reportDate = rc.from;
    const QString cr = QStringLiteral("\n");

    // Collect data: group events by task UID for the given day
    QMap<QString, TaskDayData> taskDataMap;

    for (const Event *event : projectModel->eventsModel()->events()) {
        if (!event->hasEndDate()) {
            continue; // skip currently running timers
        }

        if (event->dtStart().date() != reportDate) {
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

        TaskDayData &data = taskDataMap[taskUid];
        if (data.name.isEmpty()) {
            // Build full task name including parent chain
            QString fullName = task->name();
            Task *parent = task->parentTask();
            while (parent) {
                fullName = parent->name() + QStringLiteral(" -> ") + fullName;
                parent = parent->parentTask();
            }
            data.name = fullName;
        }

        const int64_t durationSecs = event->dtEnd().toSecsSinceEpoch() - event->dtStart().toSecsSinceEpoch();
        if (durationSecs > 0) {
            data.totalSeconds += durationSecs;
        }

        // Use only the last comment — earlier entries are previous edit versions
        const QStringList eventComments = event->comments();
        if (!eventComments.isEmpty()) {
            const QString comment = eventComments.last().trimmed();
            if (!comment.isEmpty()) {
                data.comments.append({comment, durationSecs > 0 ? durationSecs : 0});
            }
        }
    }

    // Build report string
    QString report;

    const QString title = i18n("Daily Report — %1", QLocale().toString(reportDate, QLocale::LongFormat));
    report += title + cr;
    report += QString(title.length(), QChar::fromLatin1('=')) + cr + cr;

    if (taskDataMap.isEmpty()) {
        report += i18n("No tasks recorded for this day.") + cr;
        return report;
    }

    int64_t grandTotalSeconds = 0;

    for (const TaskDayData &data : taskDataMap) {
        const double minutes = static_cast<double>(data.totalSeconds) / 60.0;
        report += i18n("Task: %1", data.name) + cr;
        report += QStringLiteral("  ") + i18n("Time: %1", formatTime(minutes, rc.decimalMinutes)) + cr;

        if (!data.comments.isEmpty()) {
            report += QStringLiteral("  ") + i18n("Comments:") + cr;
            for (const CommentEntry &c : data.comments) {
                const double mins = static_cast<double>(c.seconds) / 60.0;
                report += QStringLiteral("    - ") + c.text
                    + QStringLiteral(" [") + formatTime(mins, rc.decimalMinutes) + QStringLiteral("]") + cr;
            }
        }

        report += cr;
        grandTotalSeconds += data.totalSeconds;
    }

    const double totalMinutes = static_cast<double>(grandTotalSeconds) / 60.0;
    report += QString(40, QChar::fromLatin1('-')) + cr;
    report += i18n("Total: %1", formatTime(totalMinutes, rc.decimalMinutes)) + cr;

    return report;
}
