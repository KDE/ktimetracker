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

#include <KCalCore/FileStorage>
#include <KCalCore/MemoryCalendar>
#include <KCalCore/ICalFormat>

#include <KDateTime>
#include <KDebug>

using namespace KCalCore;
using namespace KTimeTracker;

class KTTCalendar::Private {
public:
  Private( const QString &filename ) : m_filename( filename )
  {
  }
  QString m_filename;
  QWeakPointer<KTTCalendar> m_weakPtr;
  KCalCore::FileStorage::Ptr m_fileStorage;
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
  KTTCalendar::Ptr calendar = weakPointer().toStrongRef();
  d->m_fileStorage = FileStorage::Ptr( new FileStorage( calendar,
                                                        d->m_filename,
                                                        new ICalFormat() ) );
  const bool result = d->m_fileStorage->load();
  if ( !result )
    kError() << "KTTCalendar::reload: problem loading calendar";
  return result;
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

/** static */
bool KTTCalendar::save()
{
  if ( !d->m_fileStorage ) {
    kWarning() << "KTTCalendar::save() save called before load";
    if ( !reload() ) {
      kError() << "KTTCalendar::save: problem loading the calendar";
      return false;
    }
  }

  const bool result = d->m_fileStorage->save();
  if ( !result )
    kError() << "KTTCalendar::save: problem saving calendar";
  return result;
}
