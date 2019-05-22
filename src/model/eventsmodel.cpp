#include "eventsmodel.h"

#include "task.h"

EventsModel::EventsModel()
    : m_events()
{
}

void EventsModel::load(const KCalCore::Event::List &events)
{
    clear();

    for (const auto& event : events) {
        m_events.append(new Event(event, this));
    }
}

EventsModel::~EventsModel()
{
    clear();
}

QList<Event*> EventsModel::events() const
{
    return m_events;
}

QList<Event*> EventsModel::eventsForTask(Task *task) const
{
    QList<Event*> res;
    for (auto *event : events()) {
        if (event->relatedTo() == task->uid()) {
            res.append(event);
        }
    }

    return res;
}

Event *EventsModel::eventByUID(const QString &uid) const
{
    for (auto *event : events()) {
        if (event->uid() == uid) {
            return event;
        }
    }

    return nullptr;
}

void EventsModel::clear()
{
    for (auto *event : m_events) {
        delete event;
    }
    m_events.clear();
}

void EventsModel::removeAllForTask(const Task *task)
{
    for (auto it = m_events.begin(); it != m_events.end();) {
        Event *event = *it;
        if (event->relatedTo() == task->uid()) {
            delete event;
            it = m_events.erase(it);
        } else {
            ++it;
        }
    }
}

void EventsModel::removeByUID(const QString &uid)
{
    for (auto it = m_events.begin(); it != m_events.end(); ++it) {
        Event *event = *it;
        if (event->uid() == uid) {
            delete event;
            m_events.erase(it);
            return;
        }
    }
}

void EventsModel::addEvent(Event *event)
{
    m_events.append(event);
}
