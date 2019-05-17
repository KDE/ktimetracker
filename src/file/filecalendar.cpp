/*
 *     Copyright (C) 2012 by Sérgio Martins <iamsergio@gmail.com>
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

#include <KCalCore/FileStorage>
#include <KCalCore/MemoryCalendar>

#include "icalformatkio.h"
#include "ktt_debug.h"

FileCalendar::FileCalendar(const QUrl &url)
    : KCalCore::MemoryCalendar(QTimeZone::systemTimeZone())
    , m_url(url)
    , m_weakPtr()
{
}

bool FileCalendar::reload()
{
    close(); // delete all TODOs
    KCalCore::FileStorage fileStorage(m_weakPtr.toStrongRef(), m_url.url(), new ICalFormatKIO());
    const bool result = fileStorage.load();
    if (!result) {
        qCritical() << "FileCalendar::reload: problem loading calendar";
    }
    return result;
}

bool FileCalendar::save()
{
    KCalCore::FileStorage fileStorage(m_weakPtr.toStrongRef(), m_url.url(), new ICalFormatKIO());
    const bool result = fileStorage.save();
    if (!result) {
        qCritical() << "FileCalendar::save: problem saving calendar";
    }
    return result;
}

void FileCalendar::setWeakPointer(const QWeakPointer<FileCalendar>& ptr)
{
    m_weakPtr = ptr;
}
