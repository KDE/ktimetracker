#include "idletimedetector.h"

#include <qdatetime.h>
#include <qmessagebox.h>
#include <qtimer.h>

#include <kglobal.h>
#include <klocale.h>    // i18n
#include <QX11Info>

IdleTimeDetector::IdleTimeDetector(int maxIdle)
{
  _maxIdle = maxIdle;

#ifdef HAVE_LIBXSS
  int event_base, error_base;
  if(XScreenSaverQueryExtension(QX11Info::display(), &event_base, &error_base)) {
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

bool IdleTimeDetector::isIdleDetectionPossible()
{
  return _idleDetectionPossible;
}

void IdleTimeDetector::check()
{
#ifdef HAVE_LIBXSS
  if (_idleDetectionPossible)
  {
    _mit_info = XScreenSaverAllocInfo ();
    XScreenSaverQueryInfo(QX11Info::display(), QX11Info::appRootWindow(), _mit_info);
    int idleMinutes = (_mit_info->idle/1000)/secsPerMinute;
    if (idleMinutes >= _maxIdle)
      informOverrun(idleMinutes);
  }
#endif // HAVE_LIBXSS
}

void IdleTimeDetector::setMaxIdle(int maxIdle)
{
  _maxIdle = maxIdle;
}

#ifdef HAVE_LIBXSS
void IdleTimeDetector::informOverrun(int idleMinutes)
{
  if (!_overAllIdleDetect)
    return; // In the preferences the user has indicated that he do not
            // want idle detection.

  _timer->stop();

  QDateTime start = QDateTime::currentDateTime();
  QDateTime idleStart = start.addSecs(-60 * _maxIdle);
  QString backThen = KGlobal::locale()->formatTime(idleStart.time());

  int id =  QMessageBox::warning( 0, i18n("Idle Detection"),
                                     i18n("Desktop has been idle since %1."
                                          " What should we do?").arg(backThen),
                                     i18n("Revert && Stop"),
                                     i18n("Revert && Continue"),
                                     i18n("Continue Timing"),0,2);
  QDateTime end = QDateTime::currentDateTime();
  int diff = start.secsTo(end)/secsPerMinute;

  if (id == 0) {
    // Revert And Stop
    emit(extractTime(idleMinutes+diff));
    emit(stopAllTimers());
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

void IdleTimeDetector::startIdleDetection()
{
#ifdef HAVE_LIBXSS
  if (!_timer->isActive())
    _timer->start(testInterval);
#endif //HAVE_LIBXSS
}

void IdleTimeDetector::stopIdleDetection()
{
#ifdef HAVE_LIBXSS
  if (_timer->isActive())
    _timer->stop();
#endif // HAVE_LIBXSS
}
void IdleTimeDetector::toggleOverAllIdleDetection(bool on)
{
  _overAllIdleDetect = on;
}

#include "idletimedetector.moc"
