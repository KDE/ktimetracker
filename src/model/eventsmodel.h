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

#ifndef KTIMETRACKER_EVENTSMODEL_H
#define KTIMETRACKER_EVENTSMODEL_H

#include <KCalCore/Calendar>

#include "event.h"

class Task;

class EventsModel : public QObject
{
    Q_OBJECT

public:
    EventsModel() = default;
    ~EventsModel() override;

    void load(const KCalCore::Event::List &events);
    QList<Event *> events() const;
    QList<Event *> eventsForTask(const Task *task) const;
    Event *eventByUID(const QString &uid) const;

    // Delete all events
    void clear();

    void removeAllForTask(const Task *task);
    void removeByUID(const QString &uid);

    void addEvent(Event *event);

    /**
     * Log the change in a task's time.
     *
     * This is also called when a timer is stopped.
     * We create an iCalendar event to store each change. The event start
     * date is set to the current datetime. If time is added to the task, the
     * task end date is set to start time + delta. If the time is negative,
     * the end date is set to the start time.
     *
     * In both cases (positive or negative delta), we create a custom iCalendar
     * property that stores the delta (in seconds). This property is called
     * X-KDE-ktimetracker-duration.
     *
     * Note that the ktimetracker UI allows the user to change both the session and
     * the total task time, and this routine does not account for all possible
     * cases. For example, it is possible for the user to do something crazy
     * like add 10 minutes to the session time and subtract 50 minutes from
     * the total time. Although this change violates a basic law of physics,
     * it is allowed.
     *
     * For now, you should pass in the change to the total task time.
     *
     * @param task   The task the change is for.
     * @param delta  Change in task time, in seconds.  Can be negative.
     */
    void changeTime(const Task *task, int64_t deltaSeconds);

    /**
     * Book time to a task.
     *
     * Creates an iCalendar event and adds it to the calendar. Does not write
     * calendar to disk, just adds event to calendar in memory. However, the
     * resource framework does try to get a lock on the file. After a
     * successful lock, the calendar marks this incidence as modified and then
     * releases the lock.
     *
     * @param task Task
     * @param startDateTime Date and time the booking starts.
     * @param durationInSeconds Duration of time to book, in seconds.
     *
     * @return true if event was added, false if not (if, for example, the
     * attempted file lock failed).
     */
    bool bookTime(const Task *task, const QDateTime &startDateTime, int64_t durationInSeconds);

    void startTask(const Task *task);
    void stopTask(const Task *task, const QDateTime &when);

Q_SIGNALS:
    void timesChanged();

private:
    QList<Event *> m_events;
};

#endif // KTIMETRACKER_EVENTSMODEL_H
