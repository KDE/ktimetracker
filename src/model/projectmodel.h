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

#ifndef KTIMETRACKER_PROJECTMODEL_H
#define KTIMETRACKER_PROJECTMODEL_H

#include <memory>

#include "file/filecalendar.h"

class EventsModel;
class Task;
class TasksModel;

class ProjectModel
{
public:
    ProjectModel();

    TasksModel *tasksModel();
    EventsModel *eventsModel();

    std::unique_ptr<FileCalendar> asCalendar(const QUrl &url) const;

    /**
     * Reset session and total time to zero for all tasks.
     *
     * This procedure resets all times (session and overall) for all tasks and subtasks.
     */
    void resetTimeForAllTasks();

    /** Import tasks from Imendio Planner */
    void importPlanner(const QString &fileName, Task *currentTask);

    /** Used to refresh (e.g. after import) */
    void refresh();

private:
    TasksModel *m_tasksModel;
    EventsModel *m_eventsModel;
};

#endif // KTIMETRACKER_PROJECTMODEL_H
