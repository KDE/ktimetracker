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
#include <KDirWatch>
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

KTTCalendar::KTTCalendar( const QString &filename,
                          bool monitorFile ) : KCalCore::MemoryCalendar( KDateTime::LocalZone )
                                             , d( new Private( filename ) )
{
  if ( monitorFile ) {
    connect( KDirWatch::self(), SIGNAL(dirty(QString)), SIGNAL(calendarChanged()) );
    if ( !KDirWatch::self()->contains( filename ) ) {
      KDirWatch::self()->addFile( filename );
    }
  }
}

KTTCalendar::~KTTCalendar()
{
  delete d;
}

bool KTTCalendar::reload()
{
  deleteAllTodos();
  KTTCalendar::Ptr calendar = weakPointer().toStrongRef();
  KCalCore::FileStorage::Ptr fileStorage = FileStorage::Ptr( new FileStorage( calendar,
                                                                              d->m_filename,
                                                                              new ICalFormat() ) );
  const bool result = fileStorage->load();
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
KTTCalendar::Ptr KTTCalendar::createInstance( const QString &filename, bool monitorFile )
{
  KTTCalendar::Ptr calendar( new KTTCalendar( filename, monitorFile ) );
  calendar->setWeakPointer( calendar.toWeakRef() );
  return calendar;
}

/** static */
bool KTTCalendar::save()
{
  KTTCalendar::Ptr calendar = weakPointer().toStrongRef();
  FileStorage::Ptr fileStorage = FileStorage::Ptr( new FileStorage( calendar,
                                                                    d->m_filename,
                                                                    new ICalFormat() ) );

  const bool result = fileStorage->save();
  if ( !result )
    kError() << "KTTCalendar::save: problem saving calendar";
  return result;
}
