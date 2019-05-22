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

FileCalendar::Ptr ProjectModel::asCalendar(const QUrl &url) const
{
    auto calendar = FileCalendar::Ptr(new FileCalendar(url));
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
