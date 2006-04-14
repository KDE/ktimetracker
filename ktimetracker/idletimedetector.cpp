#include "idletimedetector.h"

#include <qdatetime.h>
#include <qmessagebox.h>
#include <qtimer.h>

#include <kdialog.h>
#include <kdialogbase.h>
#include <kglobal.h>
#include <klocale.h>    // i18n
#include <qlabel.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
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
    idleminutes = (_mit_info->idle/1000)/secsPerMinute;
    if (idleminutes >= _maxIdle)
      informOverrun(idleminutes);
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

#ifdef HAVE_LIBXSS
void IdleTimeDetector::informOverrun(int idleMinutes)
{
  if (!_overAllIdleDetect)
    return; // In the preferences the user has indicated that he do not
            // want idle detection.

  _timer->stop();

  start = QDateTime::currentDateTime();
  QDateTime idleStart = start.addSecs(-60 * _maxIdle);
  QString backThen = KGlobal::locale()->formatTime(idleStart.time());
  // Create dialog  
    KDialog *dialog=new KDialog( 0 , i18n( "Idle Time Detection" )  ,
                                   KDialog::Ok | KDialog::Cancel );
    QWidget* wid=new QWidget( dialog );
    dialog->setMainWidget( wid );
    QVBoxLayout *lay1 = new QVBoxLayout(wid);  
    QHBoxLayout *lay2 = new QHBoxLayout();
    lay1->addLayout(lay2);  
    QString idlemsg=QString( "Desktop has been idle since %1. What do you want to do ?" ).arg(backThen);
    QLabel *label = new QLabel( idlemsg, wid );
    lay2->addWidget( label );
    connect( dialog , SIGNAL(cancelClicked()) , this , SLOT(revert()) );
    connect( wid , SIGNAL(changed(bool)) , wid , SLOT(enabledButtonApply(bool)) );
    QString explanation=QString("Continue timing. Timing has started at %1").arg(backThen);
    QString explanationrevert=QString( "Stop timing and revert back to the time at %1." ).arg(backThen);
    dialog->setButtonText(KDialogBase::Ok, "Continue timing.");
    dialog->setButtonText(KDialogBase::Cancel, "Revert timing");
    dialog->setButtonWhatsThis(KDialogBase::Ok, explanation);
    dialog->setButtonWhatsThis(KDialogBase::Cancel, explanationrevert);
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
