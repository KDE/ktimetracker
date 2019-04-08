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

#ifndef _KTIMETRACKER_CALENDAR_H_
#define _KTIMETRACKER_CALENDAR_H_

#include <KCalCore/MemoryCalendar>

class FileCalendar : public KCalCore::MemoryCalendar
{
    Q_OBJECT

public:
    typedef QSharedPointer<FileCalendar> Ptr;

    FileCalendar(const QString& filename, bool monitorFile);
    FileCalendar() = delete;
    ~FileCalendar() override = default;

    bool reload() override;
    bool save() override;

    // hack to work around KCalCore API
    void setWeakPointer(const QWeakPointer<FileCalendar>& ptr);

Q_SIGNALS:
    void calendarChanged();

private:
    QString m_filename;
    QWeakPointer<FileCalendar> m_weakPtr;
};

#endif
