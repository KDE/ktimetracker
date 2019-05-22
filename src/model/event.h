#ifndef KTIMETRACKER_EVENT_H
#define KTIMETRACKER_EVENT_H

#include <KCalCore/Event>

class EventsModel;

class Event
{
public:
    explicit Event(const KCalCore::Event::Ptr &event, EventsModel *model);

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

    void setDuration(long duration);

    /**
     *  Load the event passed in with this event's info.
     */
    KCalCore::Event::Ptr asCalendarEvent(const KCalCore::Event::Ptr &event) const;

private:
    QString m_summary;
    QDateTime m_dtStart;
    QDateTime m_dtEnd;
    QString m_uid;
    QString m_relatedTo;
    QStringList m_comments;
    long m_duration;

    EventsModel *m_model;
};

#endif // KTIMETRACKER_EVENT_H
