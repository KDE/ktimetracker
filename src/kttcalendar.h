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
#include <QWeakPointer>

class KTTCalendar : public KCalCore::MemoryCalendar
{
    Q_OBJECT

public:
    typedef QSharedPointer<KTTCalendar> Ptr;

    KTTCalendar() = delete;
    ~KTTCalendar() override = default;

    bool reload() override;
    bool save() override;

    /**
     * Creates a new KTTCalendar and sets m_weakPointer, so we can do
     * KTTCalendar::Ptr ptr = qobject_cast<KTTCalendar*>( sender() )->weakPointer().toStrongRef()
     * in slots.
     *
     * For this reason, the ctor is private.
     */
    static KTTCalendar::Ptr createInstance(const QString& filename, bool monitorFile);
    QWeakPointer<KTTCalendar> weakPointer() const;

Q_SIGNALS:
    void calendarChanged();

private:
    explicit KTTCalendar(const QString& filename, bool monitorFile);

    QString m_filename;
    QWeakPointer<KTTCalendar> m_weakPtr;
};

#endif
