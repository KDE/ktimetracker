#ifndef KTIMETRACKER_EVENTSMODEL_H
#define KTIMETRACKER_EVENTSMODEL_H

#include <KCalCore/Calendar>

#include "event.h"

class Task;

class EventsModel
{
public:
    EventsModel();
    virtual ~EventsModel();

    void load(const KCalCore::Calendar &calendar);
    QList<Event*> events() const;
    QList<Event*> eventsForTask(Task *task) const;
    Event *eventByUID(const QString &uid) const;

    // Delete all events
    void clear();

    void removeAllForTask(const Task *task);
    void removeByUID(const QString &uid);

    void addEvent(Event *event);

private:
    QList<Event*> m_events;
};

#endif // KTIMETRACKER_EVENTSMODEL_H
