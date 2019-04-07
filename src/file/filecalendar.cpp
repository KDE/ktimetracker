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

#include <KDirWatch>
#include <KCalCore/FileStorage>
#include <KCalCore/MemoryCalendar>
#include <KCalCore/ICalFormat>

#include "ktt_debug.h"

FileCalendar::FileCalendar(const QString &filename, bool monitorFile)
    : KCalCore::MemoryCalendar(QTimeZone::systemTimeZone())
    , m_filename(filename)
    , m_weakPtr()
{
    if (monitorFile) {
        connect(KDirWatch::self(), &KDirWatch::dirty, this, &FileCalendar::calendarChanged);
        if (!KDirWatch::self()->contains(filename)) {
            KDirWatch::self()->addFile(filename);
        }
    }
}

bool FileCalendar::reload()
{
//  deleteAllTodos();
    FileCalendar::Ptr calendar = weakPointer().toStrongRef();
    KCalCore::FileStorage::Ptr fileStorage = KCalCore::FileStorage::Ptr(
        new KCalCore::FileStorage(calendar, m_filename, new KCalCore::ICalFormat()));
    const bool result = fileStorage->load();
    if (!result) {
        qCritical() << "FileCalendar::reload: problem loading calendar";
    }
    return result;
}

QWeakPointer<FileCalendar> FileCalendar::weakPointer() const
{
    return m_weakPtr;
}

/** static */
FileCalendar::Ptr FileCalendar::createInstance(const QString& filename, bool monitorFile)
{
    FileCalendar::Ptr calendar(new FileCalendar(filename, monitorFile));
    calendar->m_weakPtr = calendar.toWeakRef();
    return calendar;
}

/** static */
bool FileCalendar::save()
{
    FileCalendar::Ptr calendar = weakPointer().toStrongRef();
    KCalCore::FileStorage::Ptr fileStorage = KCalCore::FileStorage::Ptr(
        new KCalCore::FileStorage(calendar, m_filename, new KCalCore::ICalFormat()));

    const bool result = fileStorage->save();
    if (!result) {
        qCritical() << "FileCalendar::save: problem saving calendar";
    }
    return result;
}
