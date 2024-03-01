/*
    SPDX-FileCopyrightText: 2012 Sérgio Martins <iamsergio@gmail.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
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
