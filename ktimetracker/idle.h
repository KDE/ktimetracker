#ifndef __IDLETIMER
#define __IDLETIMER
#include <qobject.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h>
class QTimer;

// Seconds per minutes - usefull for speeding debugging up!
const int secsPerMinutes = 60;

// Minutes between each idle overrun test.
const int testInterval= secsPerMinutes * 1000;

class IdleTimer :public QObject
{
Q_OBJECT

public:
  /**
     Initializes and idle test timer
     @param maxIdle minutes before the idle timer will go off.
  **/
  IdleTimer(int maxIdle);

  /**
     Returns true if it is possible to do idle detection.
     Idle detection relys on a feature in the X server, which might not
     allways be present.
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
  void stopTimer();

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
     @param on If true idle detection is done based on @ref
     startIdleDetection and @ref stopIdleDetection
  **/
  void toggleOverAllIdleDetection(bool);
  
  
protected:
  void informOverrun(int idle);

protected slots:
  void check();
  
private:
  XScreenSaverInfo *_mit_info;
  bool _idleDetectionPossible;
  bool _overAllIdleDetect; // Based on preferences.
  int _maxIdle;
  QTimer *_timer;
};

#endif // __IDLETIMER
