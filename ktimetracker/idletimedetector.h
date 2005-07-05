#ifndef KARM_IDLE_TIME_DETECTOR_H
#define KARM_IDLE_TIME_DETECTOR_H

#include <qobject.h>
#include "config.h"     // HAVE_LIBXSS

class QTimer;

#ifdef HAVE_LIBXSS
 #include <X11/Xlib.h>
 #include <X11/Xutil.h>
 #include <X11/extensions/scrnsaver.h>
 #include <fixx11h.h>
#endif // HAVE_LIBXSS

// Seconds per minutes - useful for speeding debugging up!
const int secsPerMinute = 60;

// Minutes between each idle overrun test.
const int testInterval= secsPerMinute * 1000;

/**
 * Keep track of how long the computer has been idle.
 */

class IdleTimeDetector :public QObject
{
Q_OBJECT

public:
  /**
     Initializes and idle test timer
     @param maxIdle minutes before the idle timer will go off.
  **/
  IdleTimeDetector(int maxIdle);

  /**
     Returns true if it is possible to do idle detection.
     Idle detection relys on a feature in the X server, which might not
     always be present.
  **/
  bool isIdleDetectionPossible();

signals:
  /**
     Tells the listener to extract time from current timing.
     The time to extract is due to the idle time since the dialog wass
     shown, and until the user answers the dialog.
     @param minutes Minutes to extract.
  **/
  void extractTime(int minutes);

  /**
      Tells the listener to stop timing
   **/
  void stopAllTimers();

public slots:
  /**
     Sets the maximum allowed idle.
     @param maxIdle Maximum allowed idle time in minutes
  **/
  void setMaxIdle(int maxIdle);

  /**
     Starts detecting idle time
  **/
  void startIdleDetection();

  /**
      Stops detecting idle time.
  **/
  void stopIdleDetection();

  /**
     Sets whether idle detection should be done at all
     @param on If true idle detection is done based on
     startIdleDetection and @ref stopIdleDetection
  **/
  void toggleOverAllIdleDetection(bool on);


protected:
#ifdef HAVE_LIBXSS
  void informOverrun(int idle);
#endif // HAVE_LIBXSS

protected slots:
  void check();

private:
#ifdef HAVE_LIBXSS
  XScreenSaverInfo *_mit_info;
#endif
  bool _idleDetectionPossible;
  bool _overAllIdleDetect; // Based on preferences.
  int _maxIdle;
  QTimer *_timer;
};

#endif // KARM_IDLE_TIME_DETECTOR_H
