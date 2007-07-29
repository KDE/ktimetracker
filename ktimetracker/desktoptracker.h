/*
 *     Copyright (C) 2003 by Tomas Pospisek <tpo@sourcepole.ch>
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

#ifndef KARM_DESKTOP_TRACKER_H
#define KARM_DESKTOP_TRACKER_H

#include <QObject>
#include <QVector>

#include "desktoplist.h"

class Task;
class QTimer;

typedef QVector<Task *> TaskVector;
const int maxDesktops = 16;

/** A utility to associate tasks with desktops
 *  As soon as a desktop is activated/left - an signal is emitted for
 *  each task tracking that all tasks that want to track that desktop
 */

class DesktopTracker : public QObject
{
  Q_OBJECT

  public:
    DesktopTracker();
    void startTracking();
    void registerForDesktops( Task* task, DesktopList dl );
    int desktopCount() const { return mDesktopCount; }

  private: // member variables

    // define vectors for at most 16 virtual desktops
    // E.g.: desktopTrackerStop[3] contains a vector with
    // all tasks to be notified, when switching to/from desk 3.
    TaskVector mDesktopTracker[maxDesktops];
    int mPreviousDesktop;
    int mDesktopCount;
    int mDesktop;
    QTimer *mTimer;

  Q_SIGNALS:
    void reachedActiveDesktop( Task* task );
    void leftActiveDesktop( Task* task );

  public Q_SLOTS:
    void handleDesktopChange( int desktop );

  private Q_SLOTS:
    void changeTimers();
};

#endif // KARM_DESKTOP_TRACKER_H
