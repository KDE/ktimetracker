/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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

    /**
     * Refresh the times of the tasks, e.g. when the history has been changed by the user.
     * Re-calculate the time for every task based on events in the history.
     */
    void refreshTimes();

private:
    TasksModel *m_tasksModel;
    EventsModel *m_eventsModel;
};

#endif // KTIMETRACKER_PROJECTMODEL_H
