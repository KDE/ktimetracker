/*
 * Copyright (C) 2021 David Stächele <david@daiwai.de>
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

#include "csveventlog.h"

#include "ktt_debug.h"
#include "model/event.h"
#include "model/eventsmodel.h"
#include "model/projectmodel.h"
#include "model/task.h"
#include "model/tasksmodel.h"

QString exportCSVEventLogToString(ProjectModel *projectModel, const ReportCriteria &rc)
{
    const QDate &from = rc.from;
    const QDate &to = rc.to;
    QString delim = rc.delimiter;
    const QString cr = QStringLiteral("\n");
    const QString dateTimeFormat = QStringLiteral("yyyy-MM-dd HH:mm");
    QStringList events;
    QString row;
    QString header;

    // Write header columns
    header.append("Start");
    header.append(delim);
    header.append("End");
    header.append(delim);
    header.append("Task name");
    header.append(delim);
    header.append("Comment");
    header.append(delim);
    header.append("UID");
    header.append(cr);

    // Export all events
    for (const auto *event : projectModel->eventsModel()->events()) {
        row.clear();

        QDateTime start = event->dtStart();
        QDateTime end = event->dtEnd();

        if (start.date() < from || end.date() > to) {
            continue;
        }

        // Write CSV row
        row.append(start.toString(dateTimeFormat));
        row.append(delim);
        row.append(end.toString(dateTimeFormat));
        row.append(delim);
        row.append(getFullEventName(event, projectModel));
        row.append(delim);

        qDebug() << "event->comments.count() =" << event->comments().count();
        if (event->comments().count() > 0) {
            row.append(event->comments().last());
        }
        row.append(delim);

        row.append(event->uid());
        row.append(cr);

        events.append(row);
    }

    events.sort();
    // prepend header after sorting
    events.prepend(header);

    return events.join("");
}

QString getFullEventName(const Event *event, ProjectModel *projectModel)
{
    QString fullName;

    // maybe the file is corrupt and (*i)->relatedTo is NULL
    if (event->relatedTo().isEmpty()) {
        qCDebug(KTT_LOG) << "There is no 'relatedTo' entry for " << event->summary();
        return fullName;
    }

    Task *parent = dynamic_cast<Task *>(projectModel->tasksModel()->taskByUID(event->relatedTo()));
    if (!parent) {
        qCDebug(KTT_LOG) << "Skipping orphaned (no related parent task) entry " << event->summary();
        return fullName;
    }

    Task *parentTask;
    parentTask = parent;
    fullName += parentTask->name();
    parentTask = parentTask->parentTask();
    while (parentTask) {
        fullName = parentTask->name() + "->" + fullName;
        qCDebug(KTT_LOG) << "Fullname(inside): " << fullName;
        parentTask = parentTask->parentTask();
        qCDebug(KTT_LOG) << "Parent task: " << parentTask;
    }

    return fullName;
}
