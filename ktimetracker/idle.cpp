#include "idle.h"
#include <qmessagebox.h>
#include <klocale.h>
#include <qtimer.h>
#include <qdatetime.h>

IdleTimer::IdleTimer(int maxIdle) 
{
  _maxIdle = maxIdle;

#ifdef HAVE_LIBXSS
  int event_base, error_base;
  if(XScreenSaverQueryExtension(qt_xdisplay(), &event_base, &error_base)) {
    _idleDetectionPossible = true;
  }
  else {
    _idleDetectionPossible = false;
  }

  _timer = new QTimer(this);
  connect(_timer, SIGNAL(timeout()), this, SLOT(check()));
#else
  _idleDetectionPossible = false;
#endif // HAVE_LIBXSS

}

bool IdleTimer::isIdleDetectionPossible()
{
  return _idleDetectionPossible;
}

void IdleTimer::check() 
{
#ifdef HAVE_LIBXSS
  if (_idleDetectionPossible) {
    _mit_info = XScreenSaverAllocInfo ();
    XScreenSaverQueryInfo(qt_xdisplay(), qt_xrootwin(), _mit_info);
    int idleMinutes = (_mit_info->idle/1000)/secsPerMinutes;;
    if (idleMinutes >= _maxIdle) {
      informOverrun(idleMinutes);
    }
  }
#endif // HAVE_LIBXSS
}

void IdleTimer::setMaxIdle(int maxIdle)
{
  _maxIdle = maxIdle;
}

#ifdef HAVE_LIBXSS
void IdleTimer::informOverrun(int idleMinutes) 
{
  if (!_overAllIdleDetect) {
    return; // In the preferences the user has indicated that he do not
            // want idle detection.
  }

  _timer->stop();
  
  QDateTime start = QDateTime::currentDateTime();
  QDateTime idleStart = start.addSecs(-60 * _maxIdle);
  QString backThen;
  backThen.sprintf("%d:%02d", idleStart.time().hour(), idleStart.time().minute());
  
  int id =  QMessageBox::warning(0,i18n("Idle detection"),
                                     i18n("Desktop has been idle since %1."
                                          " What should we do?").arg(backThen),
                                     i18n("Revert and Stop"), i18n("Revert and Continue"),
                                     i18n("Continue Timing"),0,2);
  QDateTime end = QDateTime::currentDateTime();
  int diff = start.secsTo(end)/secsPerMinutes;

  if (id == 0) {
    // Revert And Stop
    emit(extractTime(idleMinutes+diff));
    emit(stopTimer());
  }
  else if (id == 1) {
    // Revert and Continue
    emit(extractTime(idleMinutes+diff));
    _timer->start(testInterval);
  }
  else {
    // Continue
    _timer->start(testInterval);      
  }
}
#endif // HAVE_LIBXSS

void IdleTimer::startIdleDetection() 
{
#ifdef HAVE_LIBXSS
  if (!_timer->isActive())
    _timer->start(testInterval);
#endif //HAVE_LIBXSS
}

void IdleTimer::stopIdleDetection()
{
#ifdef HAVE_LIBXSS
  if (_timer->isActive())
    _timer->stop();
#endif // HAVE_LIBXSS
}
void IdleTimer::toggleOverAllIdleDetection(bool on) 
{
  _overAllIdleDetect = on;
}

#include "idle.moc"
