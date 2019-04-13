/*
 *     Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
 *                   2007 the ktimetracker developers
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the
 *      Free Software Foundation, Inc.
 *      51 Franklin Street, Fifth Floor
 *      Boston, MA  02110-1301  USA.
 *
 */

#ifndef KTIMETRACKER_IDLE_TIME_DETECTOR_H
#define KTIMETRACKER_IDLE_TIME_DETECTOR_H

#include <QObject>
#include <QDateTime>

#include "ktimetrackerutility.h" // SecsPerMinute
#include "config-ktimetracker.h" // HAVE_XSCREENSAVER

class QTimer;

#if defined(HAVE_XSCREENSAVER)
 #include <X11/Xlib.h>
 #include <X11/Xutil.h>
 #include <X11/extensions/scrnsaver.h>
 #include <fixx11h.h>
#endif // HAVE_XSCREENSAVER

// Minutes between each idle overrun test.
const int testInterval = secsPerMinute * 1000;

/**
 * Keep track of how long the computer has been idle.
 */

class IdleTimeDetector : public QObject
{
    Q_OBJECT

public:
    /**
       Initializes and idle test timer
       @param maxIdle minutes before the idle timer will go off.
    **/
    explicit IdleTimeDetector(int maxIdle);

    /**
       Returns true if it is possible to do idle detection.
       Idle detection relys on a feature in the X server, which might not
       always be present.
    **/
    bool isIdleDetectionPossible();

Q_SIGNALS:
    /**
       Tells the listener to subtract time from current timing.
       The time to subtract is due to the idle time since the dialog wass
       shown, and until the user answers the dialog.
       @param minutes Minutes to subtract.
    **/
    void subtractTime(int minutes);

    /**
        Tells the listener to stop timing
     **/
    void stopAllTimers(QDateTime time);

public Q_SLOTS:
    void revert();

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
    void informOverrun();

protected Q_SLOTS:
    void check();

private:
#if defined(HAVE_XSCREENSAVER)
    XScreenSaverInfo *_mit_info;
#endif // HAVE_XSCREENSAVER
    bool _idleDetectionPossible;
    bool _overAllIdleDetect; // Based on preferences.
    int _maxIdle;
    QTimer *_timer;
    QDateTime start; // when the idletimedetectordialog started
    QDateTime idlestart; // when the idleness started
    int idleminutes;
};

#endif // KTIMETRACKER_IDLE_TIME_DETECTOR_H
