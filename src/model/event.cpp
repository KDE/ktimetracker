#include "event.h"

#include <KLocalizedString>

#include "eventsmodel.h"
#include "ktt_debug.h"
#include "ktimetrackerutility.h"

static const QByteArray eventAppName = QByteArray("ktimetracker");

Event::Event(const KCalCore::Event::Ptr &event, EventsModel *model)
    : m_summary(event->summary())
    , m_dtStart(event->dtStart())
    , m_dtEnd(event->hasEndDate() ? event->dtEnd() : QDateTime())
    , m_uid(event->uid())
    , m_relatedTo(event->relatedTo())
    , m_comments(event->comments())
    , m_duration(0)
    , m_model(model)
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

void Event::setDtStart(const QDateTime &dtStart)
{
    m_dtStart = dtStart;
//    m_model->updateInStorage(this);
}

QDateTime Event::dtStart() const
{
    return m_dtStart;
}

void Event::setDtEnd(const QDateTime &dtEnd)
{
    m_dtEnd = dtEnd;
//    m_model->updateInStorage(this);
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
    m_comments.append(comment);
}

QStringList Event::comments() const
{
    return m_comments;
}

void Event::setDuration(long duration)
{
    m_duration = duration;
}

KCalCore::Event::Ptr Event::asCalendarEvent(const KCalCore::Event::Ptr &event) const
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
    for (auto comment : comments()) {
        event->addComment(comment);
    }

    // Use a custom property to keep a record of negative durations
    event->setCustomProperty(eventAppName, QByteArray("duration"), QString::number(m_duration));

    event->setCategories({i18n("KTimeTracker")});

    return event;
}
