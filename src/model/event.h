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

#ifndef KTIMETRACKER_EVENT_H
#define KTIMETRACKER_EVENT_H

#include <KCalCore/Event>

/**
 * We have three types of events:
 *  1. Finished events of positive duration:
 *     - dtStart and dtEnd are defined
 *     - dtEnd >= dtStart
 *     - duration >= 0
 *  2. Unfinished events:
 *     - dtStart is defined
 *     - dtEnd is not defined, thus hasEndDate() returns false
 *  3. Events of negative duration:
 *     - dtStart and dtEnd are defined
 *     - dtEnd <= dtStart (TODO make dtEnd undefined in the case of negative duration?)
 *     - duration < 0
 */
class Event
{
public:
    explicit Event(const KCalCore::Event::Ptr &event);

    QString summary() const;

    void setDtStart(const QDateTime &dtStart);
    QDateTime dtStart() const;

    void setDtEnd(const QDateTime &dtEnd);
    bool hasEndDate() const;
    QDateTime dtEnd() const;

    QString uid() const;

    QString relatedTo() const;

    void addComment(const QString &comment);
    QStringList comments() const;

    void setDuration(int64_t seconds);
    int64_t duration() const;

    /**
     *  Load the event passed in with this event's info.
     */
    KCalCore::Event::Ptr asCalendarEvent(const KCalCore::Event::Ptr &event) const;

private:
    void updateDuration(QDateTime &changedDt, const QDateTime &otherDt);

    QString m_summary;
    QDateTime m_dtStart;
    QDateTime m_dtEnd;
    QString m_uid;
    QString m_relatedTo;
    QStringList m_comments;
    int64_t m_duration;
};

#endif // KTIMETRACKER_EVENT_H
