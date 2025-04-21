/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "projectmodel.h"

#include "event.h"
#include "eventsmodel.h"
#include "import/plannerparser.h"
#include "ktt_debug.h"
#include "task.h"
#include "tasksmodel.h"

ProjectModel::ProjectModel()
    : m_tasksModel(new TasksModel())
    , m_eventsModel(new EventsModel())
{
}

TasksModel *ProjectModel::tasksModel()
{
    return m_tasksModel;
}

EventsModel *ProjectModel::eventsModel()
{
    return m_eventsModel;
}

std::unique_ptr<FileCalendar> ProjectModel::asCalendar(const QUrl &url) const
{
    std::unique_ptr<FileCalendar> calendar(new FileCalendar(url));
    for (auto *item : m_tasksModel->getAllItems()) {
        Task *task = dynamic_cast<Task *>(item);

        KCalendarCore::Todo::Ptr todo(new KCalendarCore::Todo());
        calendar->addTodo(task->asTodo(todo));
    }

    // TODO: Use libkcalcore comments
    // todo->addComment(comment);
    // temporary
    //     todo->setDescription(task->comment());

    for (Event *event : m_eventsModel->events()) {
        KCalendarCore::Event::Ptr calEvent(new KCalendarCore::Event());
        calendar->addEvent(event->asCalendarEvent(calEvent));
    }

    return calendar;
}

void ProjectModel::resetTimeForAllTasks()
{
    for (Task *task : tasksModel()->getAllTasks()) {
        task->resetTimes();
    }

    eventsModel()->clear();
}

void ProjectModel::importPlanner(const QString &fileName, Task *currentTask)
{
    auto *handler = new PlannerParser(this, currentTask);
    QFile xmlFile(fileName);
    QXmlInputSource source(&xmlFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(handler);
    reader.parse(source);

    refresh();
}

void ProjectModel::refresh()
{
    for (Task *task : tasksModel()->getAllTasks()) {
        task->invalidateCompletedState();
        task->update(); // maybe there was a change in the times's format
    }
}

void ProjectModel::refreshTimes()
{
    // This procedure resets all times (session and overall) for all tasks and subtasks.
    // Reset session and total time for all tasks - do not touch the storage.
    for (Task *task : tasksModel()->getAllTasks()) {
        task->resetTimes();
    }

    for (Task *task : tasksModel()->getAllTasks()) {
        // get all events for task
        for (const auto *event : eventsModel()->eventsForTask(task)) {
            QDateTime eventStart = event->dtStart();
            QDateTime eventEnd = event->dtEnd();

            const int64_t duration = event->duration() / 60;
            task->addTime(duration);
            qCDebug(KTT_LOG) << "duration is" << duration;

            if (task->sessionStartTiMe().isValid()) {
                // if there is a session
                if (task->sessionStartTiMe().secsTo(eventStart) > 0 && task->sessionStartTiMe().secsTo(eventEnd) > 0) {
                    // if the event is after the session start
                    task->addSessionTime(duration);
                }
            } else {
                // so there is no session at all
                task->addSessionTime(duration);
            }
        }
    }

    // Recalculate total times after changing hierarchy by drag&drop
    for (Task *task : tasksModel()->getAllTasks()) {
        // Start recursive method recalculateTotalTimesSubtree() for each top-level task.
        if (task->isRoot()) {
            task->recalculateTotalTimesSubtree();
        }
    }

    refresh();
}
