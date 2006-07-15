#include <QThread>
#include <QString>

#include <kcal/resourcecalendar.h>
#include <kcal/resourcelocal.h>
#include <kcal/calendarresources.h>

#include "lockerthread.h"

LockerThread::LockerThread( const QString &icsfile )
{
  m_gotlock = false;
  m_icsfile = icsfile;
}

/*
void LockerThread::setIcsFile( const QString &filename )
{
  m_icsfile = filename;
}
*/

void LockerThread::run()
{
  KCal::CalendarResources         *calendars = 0;
  KCal::ResourceCalendar          *calendar  = 0;
  KCal::CalendarResources::Ticket *lock      = 0;

  calendars = new KCal::CalendarResources( QString::fromLatin1( "UTC" ) );
  calendar  = new KCal::ResourceLocal( m_icsfile );
  lock      = calendars->requestSaveTicket( calendar );
  if ( lock )
  {
    m_gotlock = true;
    calendars->releaseSaveTicket( lock );
  }
  else
  {
    m_gotlock = false;
  }

  delete calendar;
  delete calendars;
}
