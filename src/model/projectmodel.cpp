/*
 * Copyright (c) 2019 Alexander Potashev <aspotashev@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "projectmodel.h"

#include "tasksmodel.h"
#include "task.h"
#include "eventsmodel.h"
#include "event.h"

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
        Task *task = dynamic_cast<Task*>(item);

        KCalCore::Todo::Ptr todo(new KCalCore::Todo());
        calendar->addTodo(task->asTodo(todo));
    }


    // TODO: Use libkcalcore comments
    // todo->addComment(comment);
    // temporary
//     todo->setDescription(task->comment());


    for (Event *event : m_eventsModel->events()) {
        KCalCore::Event::Ptr calEvent(new KCalCore::Event());
        calendar->addEvent(event->asCalendarEvent(calEvent));
    }

    return calendar;
}
