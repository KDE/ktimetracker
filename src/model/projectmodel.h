#ifndef KTIMETRACKER_PROJECTMODEL_H
#define KTIMETRACKER_PROJECTMODEL_H

#include "file/filecalendar.h"

class TasksModel;
class EventsModel;

class ProjectModel
{
public:
    ProjectModel();

    TasksModel *tasksModel();
    EventsModel *eventsModel();

    FileCalendar::Ptr asCalendar(const QUrl &url) const;

private:
    TasksModel *m_tasksModel;
    EventsModel *m_eventsModel;
};

#endif // KTIMETRACKER_PROJECTMODEL_H
