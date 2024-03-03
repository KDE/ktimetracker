/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "event.h"

#include <KLocalizedString>

#include "base/ktimetrackerutility.h"
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

static int64_t durationInSeconds(const QDateTime &start, const QDateTime &end)
{
    // Ignore milliseconds when calculating duration.
    // This will look like correct calculation for users who do not
    // see milliseconds in the UI.
    QDateTime roundedStart = QDateTime::fromSecsSinceEpoch(start.toSecsSinceEpoch());
    QDateTime roundedEnd = QDateTime::fromSecsSinceEpoch(end.toSecsSinceEpoch());
    return roundedStart.secsTo(roundedEnd);
}

void Event::updateDuration(QDateTime &changedDt, const QDateTime &otherDt)
{
    if (m_duration < 0) {
        // Was a negative duration task
        if (m_dtEnd > m_dtStart) {
            // If range is not trivial or inversed, we assume that
            // event duration now becomes positive, thus we should update it here.
            m_duration = durationInSeconds(m_dtStart, m_dtEnd);
        }
    } else {
        // Was a regular task, and will stay regular. Calculate duration if possible.
        if (hasEndDate()) {
            m_duration = durationInSeconds(m_dtStart, m_dtEnd);
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

void Event::setDuration(int64_t seconds)
{
    m_duration = seconds;
}

int64_t Event::duration() const
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
