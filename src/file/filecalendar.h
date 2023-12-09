/*
 * Copyright (C) 2012 by Sérgio Martins <iamsergio@gmail.com>
 * Copyright (C) 2019  Alexander Potashev <aspotashev@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the
 *      Free Software Foundation, Inc.
 *      51 Franklin Street, Fifth Floor
 *      Boston, MA  02110-1301  USA.
 *
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
