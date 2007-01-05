#include "idletimedetector.h"

#include <qdatetime.h>
#include <qmessagebox.h>
#include <qtimer.h>

#include <kdialog.h>
#include <kglobal.h>
#include <klocale.h>    // i18n
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#ifdef Q_WS_X11
#include <QX11Info>
#endif

IdleTimeDetector::IdleTimeDetector(int maxIdle)
{
  _maxIdle = maxIdle;

#if defined(HAVE_LIBXSS) && defined(Q_WS_X11)
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
#if defined(HAVE_LIBXSS) && defined(Q_WS_X11)
  if (_idleDetectionPossible)
  {
    _mit_info = XScreenSaverAllocInfo ();
    XScreenSaverQueryInfo(QX11Info::display(), QX11Info::appRootWindow(), _mit_info);
    idleminutes = (_mit_info->idle/1000)/secsPerMinute;
    if (idleminutes >= _maxIdle)
      informOverrun();
  }
#endif // HAVE_LIBXSS
}

void IdleTimeDetector::setMaxIdle(int maxIdle)
{
  _maxIdle = maxIdle;
}

void IdleTimeDetector::revert()
{
  // revert and stop
  kDebug(5970) << "Entering IdleTimeDetector::revert" << endl;
  QDateTime end = QDateTime::currentDateTime();
  int diff = start.secsTo(end)/secsPerMinute;
  emit(extractTime(idleminutes+diff));
  emit(stopAllTimers());
}

#if defined(HAVE_LIBXSS) && defined(Q_WS_X11)
void IdleTimeDetector::informOverrun()
{
  if (!_overAllIdleDetect)
    return; // In the preferences the user has indicated that he do not
            // want idle detection.

  _timer->stop();

  start = QDateTime::currentDateTime();
  idlestart = start.addSecs(-60 * _maxIdle);
  QString backThen = KGlobal::locale()->formatTime(idlestart.time());
  // Create dialog
    KDialog *dialog=new KDialog( 0 );
    QWidget* wid=new QWidget(dialog);
    dialog->setMainWidget( wid );
    QVBoxLayout *lay1 = new QVBoxLayout(wid);
    QHBoxLayout *lay2 = new QHBoxLayout();
    lay1->addLayout(lay2);
    QString idlemsg=QString( "Desktop has been idle since %1. What do you want to do ?" ).arg(backThen);
    QLabel *label = new QLabel( idlemsg, wid );
    lay2->addWidget( label );
    connect( dialog , SIGNAL(cancelClicked()) , this , SLOT(revert()) );
    connect( wid , SIGNAL(changed(bool)) , wid , SLOT(enabledButtonApply(bool)) );
    QString explanation=i18n(QString("Continue timing. Timing has started at %1").arg(backThen).toAscii());
    QString explanationrevert=i18n(QString( "Stop timing and revert back to the time at %1." ).arg(backThen).toAscii());
    dialog->setButtonText(KDialog::Ok, i18n("Continue timing."));
    dialog->setButtonText(KDialog::Cancel, i18n("Revert timing"));
    dialog->setButtonWhatsThis(KDialog::Ok, explanation);
    dialog->setButtonWhatsThis(KDialog::Cancel, explanationrevert);
    dialog->show();
/*
  else {
    // Continue
    _timer->start(testInterval);
  }
*/
}
#endif // HAVE_LIBXSS

void IdleTimeDetector::startIdleDetection()
{
#if defined(HAVE_LIBXSS) && defined(Q_WS_X11)
  if (!_timer->isActive())
    _timer->start(testInterval);
#endif //HAVE_LIBXSS
}

void IdleTimeDetector::stopIdleDetection()
{
#if defined(HAVE_LIBXSS) && defined(Q_WS_X11)
  if (_timer->isActive())
    _timer->stop();
#endif // HAVE_LIBXSS
}
void IdleTimeDetector::toggleOverAllIdleDetection(bool on)
{
  _overAllIdleDetect = on;
}

#include "idletimedetector.moc"
