/*
    SPDX-FileCopyrightText: 2012 Sérgio Martins <iamsergio@gmail.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTIMETRACKER_FILECALENDAR_H
#define KTIMETRACKER_FILECALENDAR_H

#include <KCalendarCore/MemoryCalendar>

class FileCalendar
{
public:
    explicit FileCalendar(QUrl url);
    FileCalendar() = delete;
    ~FileCalendar() = default;

    bool reload();
    bool save();

    void addTodo(const KCalendarCore::Todo::Ptr &todo);
    KCalendarCore::Todo::List rawTodos() const;

    void addEvent(const KCalendarCore::Event::Ptr &event);
    KCalendarCore::Event::List rawEvents() const;
    KCalendarCore::Event::List rawEventsForDate(const QDate &date) const;

private:
    QUrl m_url;
    KCalendarCore::MemoryCalendar::Ptr m_calendar;
};

#endif // KTIMETRACKER_FILECALENDAR_H
