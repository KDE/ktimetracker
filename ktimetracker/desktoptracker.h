#ifndef KARM_DESKTOP_TRACKER_H
#define KARM_DESKTOP_TRACKER_H

#include <vector>

#include <kwinmodule.h>

#include "desktoplist.h"

class Task;
class QTimer;

typedef std::vector<Task *> TaskVector;
const int maxDesktops = 16;

/** A utility to associate tasks with desktops
 *  As soon as a desktop is activated/left - an signal is emited for
 *  each task tracking that all tasks that want to track that desktop
 */

class DesktopTracker: public QObject
{
  Q_OBJECT

  public:
    DesktopTracker();
    void printTrackers();
    void startTracking();
    void registerForDesktops( Task* task, DesktopList dl );
    int desktopCount() const { return _desktopCount; };

  private: // member variables
#ifdef Q_WS_X11
    KWinModule kWinModule;
#endif

    // define vectors for at most 16 virtual desktops
    // E.g.: desktopTrackerStop[3] contains a vector with
    // all tasks to be notified, when switching to/from desk 3.
    TaskVector desktopTracker[maxDesktops];
    int _previousDesktop;
    int _desktopCount;
    int _desktop;
    QTimer *_timer;

  signals:
    void reachedtActiveDesktop( Task* task );
    void leftActiveDesktop( Task* task );

  public slots:
    void handleDesktopChange( int desktop );

  private slots:
    void changeTimers();
};

#endif // KARM_DESKTOP_TRACKER_H
