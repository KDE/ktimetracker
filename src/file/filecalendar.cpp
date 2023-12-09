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

#include "filecalendar.h"

#include <QDebug>

#include <KCalendarCore/FileStorage>

#include "icalformatkio.h"
#include "ktt_debug.h"

FileCalendar::FileCalendar(QUrl url)
    : m_url(std::move(url))
    , m_calendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()))
{
}

bool FileCalendar::reload()
{
    KCalendarCore::FileStorage fileStorage(m_calendar, m_url.url(), new ICalFormatKIO());
    const bool result = fileStorage.load();
    if (!result) {
        qCritical() << "FileCalendar::reload: problem loading calendar";
    }
    return result;
}

bool FileCalendar::save()
{
    KCalendarCore::FileStorage fileStorage(m_calendar, m_url.url(), new ICalFormatKIO());
    const bool result = fileStorage.save();
    if (!result) {
        qCritical() << "FileCalendar::save: problem saving calendar";
    }
    return result;
}

KCalendarCore::Event::List FileCalendar::rawEvents() const
{
    return m_calendar->rawEvents();
}

KCalendarCore::Event::List FileCalendar::rawEventsForDate(const QDate &date) const
{
    return m_calendar->rawEventsForDate(date);
}

void FileCalendar::addTodo(const KCalendarCore::Todo::Ptr &todo)
{
    m_calendar->addTodo(todo);
}

void FileCalendar::addEvent(const KCalendarCore::Event::Ptr &event)
{
    m_calendar->addEvent(event);
}

KCalendarCore::Todo::List FileCalendar::rawTodos() const
{
    return m_calendar->rawTodos();
}
