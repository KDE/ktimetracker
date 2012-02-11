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

#include "kttcalendar.h"

#include <KDateTime>

using namespace KTimeTracker;

class KTTCalendar::Private {
public:
  Private( const QString &filename ) : m_filename( filename )
  {
  }
  QString m_filename;
  QWeakPointer<KTTCalendar> m_weakPtr;
};

KTTCalendar::KTTCalendar( const QString &filename ) : KCalCore::MemoryCalendar( KDateTime::LocalZone )
                                                    , d( new Private( filename ) )
{
}

KTTCalendar::~KTTCalendar()
{
  delete d;
}

bool KTTCalendar::reload()
{
    return true;
}

QWeakPointer<KTTCalendar> KTTCalendar::weakPointer() const
{
  return d->m_weakPtr;
}

void KTTCalendar::setWeakPointer(const QWeakPointer<KTTCalendar> &ptr )
{
  d->m_weakPtr = ptr;
}

/** static */
KTTCalendar::Ptr KTTCalendar::createInstance( const QString &filename )
{
  KTTCalendar::Ptr calendar( new KTTCalendar( filename ) );
  calendar->setWeakPointer( calendar.toWeakRef() );
  return calendar;
}
