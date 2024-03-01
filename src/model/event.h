/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KTIMETRACKER_EVENT_H
#define KTIMETRACKER_EVENT_H

#include <KCalendarCore/Event>

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
    explicit Event(const KCalendarCore::Event::Ptr &event);

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
    KCalendarCore::Event::Ptr asCalendarEvent(const KCalendarCore::Event::Ptr &event) const;

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
