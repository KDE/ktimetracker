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

#include "eventsmodel.h"

#include <KLocalizedString>

#include "task.h"
#include "ktt_debug.h"

EventsModel::EventsModel()
    : m_events()
{
}

void EventsModel::load(const KCalendarCore::Event::List &events)
{
    clear();

    for (const auto& event : events) {
        m_events.append(new Event(event));
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

QList<Event*> EventsModel::eventsForTask(const Task *task) const
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

static KCalendarCore::Event::Ptr baseEvent(const Task *task)
{
    qCDebug(KTT_LOG) << "Entering function";
    KCalendarCore::Event::Ptr e(new KCalendarCore::Event());
    QStringList categories;
    e->setSummary(task->name());

    // Can't use setRelatedToUid()--no error, but no RelatedTo written to disk
    e->setRelatedTo( task->uid() );

    // Debugging: some events where not getting a related-to field written.
    Q_ASSERT(e->relatedTo() == task->uid());

    // Have to turn this off to get datetimes in date fields.
    e->setAllDay(false);
//    e->setDtStart(KDateTime(task->startTime(), KDateTime::Spec::LocalZone()));
    e->setDtStart(task->startTime());

    // So someone can filter this mess out of their calendar display
    categories.append(i18n("KTimeTracker"));
    e->setCategories(categories);

    return e;
}

void EventsModel::changeTime(const Task* task, long deltaSeconds)
{
    qCDebug(KTT_LOG) << "Entering function; deltaSeconds=" << deltaSeconds;
    QDateTime end;
    auto kcalEvent = baseEvent(task);
    auto *e = new Event(kcalEvent);

    // Don't use duration, as ICalFormatImpl::writeIncidence never writes a
    // duration, even though it looks like it's used in event.cpp.
    end = task->startTime();
    if ( deltaSeconds > 0 ) end = task->startTime().addSecs(deltaSeconds);
    e->setDtEnd(end);

    // Use a custom property to keep a record of negative durations
    e->setDuration(deltaSeconds);

    addEvent(e);
}

bool EventsModel::bookTime(const Task* task, const QDateTime& startDateTime, long durationInSeconds)
{
    qCDebug(KTT_LOG) << "Entering function";

    auto kcalEvent = baseEvent(task);
    auto *e = new Event(kcalEvent);

    e->setDtStart(startDateTime);
    e->setDtEnd(startDateTime.addSecs( durationInSeconds));

    // Use a custom property to keep a record of negative durations
    e->setDuration(durationInSeconds);

    addEvent(e);
    return true;
}

void EventsModel::startTask(const Task *task)
{
    auto *e = new Event(baseEvent(task));

    // TODO support passing of a different start time from TimeTrackerStorage::buildTaskView?
    e->setDtStart(QDateTime::currentDateTime());
    addEvent(e);
}

void EventsModel::stopTask(const Task *task, const QDateTime &when)
{
    QList<Event*> openEvents;
    for (auto *event : eventsForTask(task)) {
        if (!event->hasEndDate()) {
            openEvents.append(event);
        }
    }

    if (openEvents.isEmpty()) {
        qCWarning(KTT_LOG) << "Task to stop has no open events, uid=" << task->uid() << ", name=" << task->name();
    } else if (openEvents.size() > 1) {
        qCWarning(KTT_LOG) << "Task to stop has multiple open events (" << openEvents.size() <<
            "), uid=" << task->uid() << ", name=" << task->name();
    }

    for (auto *event : openEvents) {
        qCDebug(KTT_LOG) << "found an event for task, event=" << event->uid();
        event->setDtEnd(when);
    }
}
