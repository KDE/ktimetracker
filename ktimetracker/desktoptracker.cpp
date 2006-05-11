#include <algorithm>            // std::find

#include <QTimer>
#include <kdebug.h>

#include "desktoptracker.h"

// TODO: Put in config dialog
const int minimumInterval = 5;  // seconds

DesktopTracker::DesktopTracker ()
{
  // Setup desktop change handling
#ifdef Q_WS_X11
  connect( &kWinModule, SIGNAL( currentDesktopChanged(int) ),
           this, SLOT( handleDesktopChange(int) ));

  _desktopCount = kWinModule.numberOfDesktops();
  _previousDesktop = kWinModule.currentDesktop()-1;
#else
#warning non-X11 support missing
#endif
  // TODO: removed? fixed by Lubos?
  // currentDesktop will return 0 if no window manager is started
  if( _previousDesktop < 0 ) _previousDesktop = 0;

  _timer = new QTimer(this);
  connect( _timer, SIGNAL( timeout() ), this, SLOT( changeTimers() ) );
}

void DesktopTracker::handleDesktopChange( int desktop )
{
  _desktop = desktop;

  // If user changes back and forth between desktops rapidly and frequently,
  // the data file can get huge fast if logging is turned on.  Then saving
  // get's slower, etc.  There's no benefit in saving a lot of start/stop 
  // events that are very small.  Wait a bit to make sure the user is settled.
  if ( !_timer->start( minimumInterval * 1000, true ) ) changeTimers();
}

void DesktopTracker::changeTimers()
{
  _desktop--; // desktopTracker starts with 0 for desktop 1
  // notify start all tasks setup for running on desktop
  TaskVector::iterator it;

  // stop trackers for _previousDesktop
  TaskVector tv = desktopTracker[_previousDesktop];
  for (it = tv.begin(); it != tv.end(); ++it) {
    emit leftActiveDesktop(*it);
  }

  // start trackers for desktop
  tv = desktopTracker[_desktop];
  for (it = tv.begin(); it != tv.end(); ++it) {
    emit reachedtActiveDesktop(*it);
  }
  _previousDesktop = _desktop;

  // emit updateButtons();
}

void DesktopTracker::startTracking()
{
#ifdef Q_WS_X11
  int currentDesktop = kWinModule.currentDesktop() -1;
#else
#warning non-X11 support missing
  int currentDesktop = 0;
#endif
  // TODO: removed? fixed by Lubos?
  // currentDesktop will return 0 if no window manager is started
  if ( currentDesktop < 0 ) currentDesktop = 0;

  TaskVector &tv = desktopTracker[ currentDesktop ];
  TaskVector::iterator tit = tv.begin();
  while(tit!=tv.end()) {
    emit reachedtActiveDesktop(*tit);
    tit++;
  }
}

void DesktopTracker::registerForDesktops( Task* task, DesktopList desktopList)
{
  kDebug(5970) << "Entering registerForDesktops" << endl;
  // if no desktop is marked, disable auto tracking for this task
  if (desktopList.size()==0) 
  {
    for (int i=0; i<maxDesktops; i++)  
    {
      TaskVector *v = &(desktopTracker[i]);
      TaskVector::iterator tit = std::find(v->begin(), v->end(), task);
      if (tit != v->end())
        desktopTracker[i].erase(tit);
      // if the task was priviously tracking this desktop then
      // emit a signal that is not tracking it any more
#ifdef Q_WS_X11
      if( i == kWinModule.currentDesktop() -1)
        emit leftActiveDesktop(task);
#else
#warning non-X11 support missing
#endif
    }

    return;
  }

  // If desktop contains entries then configure desktopTracker
  // If a desktop was disabled, it will not be stopped automatically.
  // If enabled: Start it now.
  if (desktopList.size()>0) {
    for (int i=0; i<maxDesktops; i++) {
      TaskVector& v = desktopTracker[i];
      TaskVector::iterator tit = std::find(v.begin(), v.end(), task);
      // Is desktop i in the desktop list?
      if ( std::find( desktopList.begin(), desktopList.end(), i)
           != desktopList.end()) {
        if (tit == v.end())  // not yet in start vector
          v.push_back(task); // track in desk i
      }
      else { // delete it
        if (tit != v.end()) // not in start vector any more
        {
          v.erase(tit); // so we delete it from desktopTracker
          // if the task was priviously tracking this desktop then
          // emit a signal that is not tracking it any more
#ifdef Q_WS_X11
          if( i == kWinModule.currentDesktop() -1)
            emit leftActiveDesktop(task);
#else
#warning non-X11 support missing
#endif
        }
      }
    }
    startTracking();
  }
}

void DesktopTracker::printTrackers() {
  TaskVector::iterator it;
  for (int i=0; i<maxDesktops; i++) {
    TaskVector& start = desktopTracker[i];
    it = start.begin();
    while (it != start.end()) {
      it++;
    }
  }
}
#include "desktoptracker.moc"
