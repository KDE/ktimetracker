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

#include "event.h"

#include <KLocalizedString>

#include "ktimetrackerutility.h"
#include "ktt_debug.h"

static const QByteArray eventAppName = QByteArray("ktimetracker");

Event::Event(const KCalendarCore::Event::Ptr &event)
    : m_summary(event->summary())
    , m_dtStart(event->dtStart())
    , m_dtEnd(event->hasEndDate() ? event->dtEnd() : QDateTime())
    , m_uid(event->uid())
    , m_relatedTo(event->relatedTo())
    , m_comments(event->comments())
    , m_duration(0)
{
    if (!m_dtStart.isValid()) {
        qFatal("1");
    }

    bool ok = false;
    m_duration = getCustomProperty(event, QStringLiteral("duration")).toInt(&ok);
    if (!ok) {
        m_duration = 0;
    }
}

QString Event::summary() const
{
    return m_summary;
}

void Event::updateDuration(QDateTime &changedDt, const QDateTime &otherDt)
{
    if (m_duration < 0) {
        // Was a negative duration task
        if (m_dtEnd > m_dtStart) {
            // If range is not trivial or inversed, we assume that
            // event duration now becomes positive, thus we should update it here.
            m_duration = m_dtStart.secsTo(m_dtEnd);
        }
    } else {
        // Was a regular task, and will stay regular. Calculate duration if possible.
        if (hasEndDate()) {
            m_duration = m_dtStart.secsTo(m_dtEnd);
            if (m_duration < 0) {
                // We cannot make duration negative.
                // May be the user tried to make duration zero, let us help him.
                m_duration = 0;
                changedDt = otherDt;
            }
        }
    }
}

void Event::setDtStart(const QDateTime &dtStart)
{
    m_dtStart = dtStart;
    updateDuration(m_dtStart, m_dtEnd);
}

QDateTime Event::dtStart() const
{
    return m_dtStart;
}

void Event::setDtEnd(const QDateTime &dtEnd)
{
    m_dtEnd = dtEnd;
    updateDuration(m_dtEnd, m_dtStart);
}

bool Event::hasEndDate() const
{
    return m_dtEnd.isValid();
}

QDateTime Event::dtEnd() const
{
    return m_dtEnd;
}

QString Event::uid() const
{
    return m_uid;
}

QString Event::relatedTo() const
{
    return m_relatedTo;
}

void Event::addComment(const QString &comment)
{
    if (m_comments.isEmpty() || comment != m_comments.last()) {
        m_comments.append(comment);
    }
}

QStringList Event::comments() const
{
    return m_comments;
}

void Event::setDuration(long seconds)
{
    m_duration = seconds;
}

long Event::duration() const
{
    return m_duration;
}

KCalendarCore::Event::Ptr Event::asCalendarEvent(const KCalendarCore::Event::Ptr &event) const
{
    Q_ASSERT(event != nullptr);

    qCDebug(KTT_LOG) << "Task::asTodo: uid" << m_uid << ", summary" << m_summary;

    event->setSummary(summary());
    event->setDtStart(m_dtStart);
    if (hasEndDate()) {
        event->setDtEnd(m_dtEnd);
    }
    event->setUid(uid());
    event->setRelatedTo(relatedTo());
    for (const auto &comment : comments()) {
        event->addComment(comment);
    }

    // Use a custom property to keep a record of negative durations
    event->setCustomProperty(eventAppName, QByteArray("duration"), QString::number(m_duration));

    event->setCategories({i18n("KTimeTracker")});

    return event;
}
