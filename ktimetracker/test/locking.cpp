#include <cassert>

#include <QString>
#include <QFile>
#include <QDir>
#include <kcmdlineargs.h>
#include <kapplication.h>

#include <kcal/resourcecalendar.h>
#include <kcal/resourcelocal.h>
#include <kcal/calendarresources.h>

#include "lockerthread.h"

const QString icalfilename = "karmtest.ics";

// If one thread has the file is locked, the other cannot get the lock.
short test1()
{
  short rval = 0;

  KCal::CalendarResources         *calendars = 0;
  KCal::ResourceCalendar          *calendar  = 0;
  KCal::CalendarResources::Ticket *lock      = 0;

  calendars = new KCal::CalendarResources( QString::fromLatin1( "UTC" ) );
  calendar  = new KCal::ResourceLocal( icalfilename );
  lock      = calendars->requestSaveTicket( calendar );

  if ( !lock ) 
  {
    kDebug( 5970 ) << "test1(): failed to lock " << icalfilename << endl;
    rval = 1;
  }

  if ( !rval )
  {
    LockerThread thread( icalfilename );
    thread.run();
    if ( thread.gotlock() )
    {
      kDebug( 5970 ) << "test1(): second thread was able to lock " << icalfilename << endl;
      rval = 1;
    }
  }

  // This frees the lock memory.
  calendars->releaseSaveTicket( lock );

  delete calendar;
  delete calendars;

  return rval;
}

// First thread opens but doesn't lock, second should get lock. 
short test2()
{
  short rval = 0;

  KCal::CalendarResources         *calendars = 0;
  KCal::ResourceCalendar          *calendar  = 0;
  KCal::CalendarResources::Ticket *lock      = 0;

  calendars = new KCal::CalendarResources( QString::fromLatin1( "UTC" ) );
  calendar  = new KCal::ResourceLocal( icalfilename );

  LockerThread thread( icalfilename );
  thread.run();
  if ( !thread.gotlock() )
  {
    kDebug(5970) << "test2(): second thread was not able to lock " << icalfilename << endl;
    rval = 1;
  }

  delete calendar;
  delete calendars;

  return rval;
}

// First thread locks, then unlocks--second should get lock. 
short test3()
{
  short rval = 0;

  KCal::CalendarResources         *calendars = 0;
  KCal::ResourceCalendar          *calendar  = 0;
  KCal::CalendarResources::Ticket *lock      = 0;

  calendars = new KCal::CalendarResources( QString::fromLatin1( "UTC" ) );
  calendar  = new KCal::ResourceLocal( icalfilename );

  // lock then unlock
  lock = calendars->requestSaveTicket( calendar );
  if ( !lock ) 
  {
    kDebug( 5970 ) << "test1(): failed to lock " << icalfilename << endl;
    rval = 1;
  }
  calendars->releaseSaveTicket( lock );

  // second thread should get lock
  if ( !rval )
  {
    LockerThread thread( icalfilename );
    thread.run();
    if ( !thread.gotlock() )
    {
      kDebug( 5970 ) << "test1(): second thread was not able to lock " << icalfilename << endl;
      rval = 1;
    }
  }


  delete calendar;
  delete calendars;

  return rval;
}

// TODO:
// If one thread changes the file, the other is notified.
// What happens if we lock one incident and try to change another?

int main( int argc, char *argv[] )
{
  short rval = 0;

  // Use another directory than the real one, just to keep things clean
  // KDEHOME needs to be writable though, for a ksycoca database
  // FIXME: Delete this directory when done with test.
  setenv( "KDEHOME", QFile::encodeName( QDir::homePath() + "/.kde-testresource" ), true );

  // Copied from Till's test in libkcal.  Not sure what this is for.
  setenv( "KDE_FORK_SLAVES", "yes", true ); // simpler, for the final cleanup

  // Copied from Till's test in libkcal.  Not sure what this is for.
  // KApplication::disableAutoDcopRegistration();

  KCmdLineArgs::init(argc,argv,"testresourcelocking", 0, KLocalizedString(), 0, KLocalizedString());

  KApplication app;

  // basic libkcal locking stuff
  if ( !rval ) rval = test1();
  if ( !rval ) rval = test2();
  if ( !rval ) rval = test3();

  return rval;
}
