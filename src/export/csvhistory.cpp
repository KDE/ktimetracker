/*
 * Copyright (C) 2003, 2004 by Mark Bucciarelli <mark@hubcapconsutling.com>
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

#include "csvhistory.h"

#include "ktimetrackerutility.h"
#include "ktt_debug.h"
#include "model/projectmodel.h"
#include "model/task.h"
#include "model/tasksmodel.h"

static int64_t todaySeconds(const QDate &date, const KCalCore::Event::Ptr &event)
{
    if (!event) {
        return 0;
    }

    qCDebug(KTT_LOG) << "found an event for task, event=" << event->uid();
    QDateTime startTime = event->dtStart();
    QDateTime endTime = event->dtEnd();
    QDateTime NextMidNight = startTime;
    NextMidNight.setTime(QTime(0, 0));
    NextMidNight = NextMidNight.addDays(1);
    // LastMidNight := mdate.setTime(0:00) as it would read in a decent
    // programming language
    QDateTime LastMidNight = QDateTime::currentDateTime();
    LastMidNight.setDate(date);
    LastMidNight.setTime(QTime(0, 0));
    int64_t secsstartTillMidNight = startTime.secsTo(NextMidNight);
    int64_t secondsToAdd = 0; // seconds that need to be added to the actual cell

    if (startTime.date() == date && event->dtEnd().date() == date) {
        // all the event occurred today
        secondsToAdd = startTime.secsTo(endTime);
    }

    if (startTime.date() == date && endTime.date() > date) {
        // the event started today, but ended later
        secondsToAdd = secsstartTillMidNight;
    }

    if (startTime.date() < date && endTime.date() == date) {
        // the event started before today and ended today
        secondsToAdd = LastMidNight.secsTo(event->dtEnd());
    }

    if (startTime.date() < date && endTime.date() > date) {
        // the event started before today and ended after
        secondsToAdd = 86400;
    }

    return secondsToAdd;
}

QString exportCSVHistoryToString(ProjectModel *projectModel, const ReportCriteria &rc)
{
    const QDate &from = rc.from;
    const QDate &to = rc.to;
    QString delim = rc.delimiter;
    const QString cr = QStringLiteral("\n");
    const int64_t intervalLength = from.daysTo(to) + 1;
    QMap<QString, QVector<int64_t>> secsForUid;
    QMap<QString, QString> uidForName;

    QString retval;

    // Step 1: Prepare two hashmaps:
    // * "uid -> seconds each day": used while traversing events, as uid is their id
    //                              "seconds each day" are stored in a vector
    // * "name -> uid", ordered by name: used when creating the csv file at the end
    auto tasks = projectModel->tasksModel()->getAllTasks();
    qCDebug(KTT_LOG) << "Tasks count: " << tasks.size();
    for (Task *task : tasks) {
        qCDebug(KTT_LOG) << ", Task Name: " << task->name() << ", UID: " << task->uid();
        // uid -> seconds each day
        // * Init each element to zero
        QVector<int64_t> vector(intervalLength, 0);
        secsForUid[task->uid()] = vector;

        // name -> uid
        // * Create task fullname concatenating each parent's name
        QString fullName;
        Task *parentTask;
        parentTask = task;
        fullName += parentTask->name();
        parentTask = parentTask->parentTask();
        while (parentTask) {
            fullName = parentTask->name() + "->" + fullName;
            qCDebug(KTT_LOG) << "Fullname(inside): " << fullName;
            parentTask = parentTask->parentTask();
            qCDebug(KTT_LOG) << "Parent task: " << parentTask;
        }

        uidForName[fullName] = task->uid();

        qCDebug(KTT_LOG) << "Fullname(end): " << fullName;
    }

    qCDebug(KTT_LOG) << "secsForUid" << secsForUid;
    qCDebug(KTT_LOG) << "uidForName" << uidForName;

    std::unique_ptr<FileCalendar> calendar = projectModel->asCalendar(QUrl());

    // Step 2: For each date, get the events and calculate the seconds
    // Store the seconds using secsForUid hashmap, so we don't need to translate
    // uids We rely on rawEventsForDate to get the events
    qCDebug(KTT_LOG) << "Let's iterate for each date: ";
    int dayCount = 0;
    for (QDate mdate = from; mdate.daysTo(to) >= 0; mdate = mdate.addDays(1)) {
        if (dayCount++ > 365 * 100) {
            return QStringLiteral("too many days to process");
        }

        qCDebug(KTT_LOG) << mdate.toString();
        for (const auto &event : calendar->rawEventsForDate(mdate)) {
            qCDebug(KTT_LOG) << "Summary: " << event->summary() << ", Related to uid: " << event->relatedTo();
            qCDebug(KTT_LOG) << "Today's seconds: " << todaySeconds(mdate, event);
            secsForUid[event->relatedTo()][from.daysTo(mdate)] += todaySeconds(mdate, event);
        }
    }

    // Step 3: For each task, generate the matching row for the CSV file
    // We use the two hashmaps to have direct access using the task name

    // First CSV file line
    // FIXME: localize strings and date formats
    retval.append("\"Task name\"");
    for (int i = 0; i < intervalLength; ++i) {
        retval.append(delim);
        retval.append(from.addDays(i).toString());
    }
    retval.append(cr);

    // Rest of the CSV file
    QMapIterator<QString, QString> nameUid(uidForName);
    double time;
    while (nameUid.hasNext()) {
        nameUid.next();
        retval.append(rc.quote + nameUid.key() + rc.quote);
        qCDebug(KTT_LOG) << nameUid.key() << ": " << nameUid.value() << endl;

        for (int day = 0; day < intervalLength; day++) {
            qCDebug(KTT_LOG) << "Secs for day " << day << ":" << secsForUid[nameUid.value()][day];
            retval.append(delim);
            time = secsForUid[nameUid.value()][day] / 60.0;
            retval.append(formatTime(time, rc.decimalMinutes));
        }

        retval.append(cr);
    }

    qDebug() << "Retval is \n" << retval;
    return retval;
}
