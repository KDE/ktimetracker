/*
 * Copyright (C) 2003 by Tomas Pospisek <tpo@sourcepole.ch>
 * Copyright (C) 2019  Alexander Potashev <aspotashev@gmail.com>
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

#ifndef KTIMETRACKER_DESKTOP_TRACKER_H
#define KTIMETRACKER_DESKTOP_TRACKER_H

#include <QObject>
#include <QVector>

#include "desktoplist.h"

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class Task;

typedef QVector<Task *> TaskVector;
const int maxDesktops = 20;

/** A utility to associate tasks with desktops
 *  As soon as a desktop is activated/left - an signal is emitted for
 *  each task tracking that all tasks that want to track that desktop
 */

class DesktopTracker : public QObject
{
    Q_OBJECT

public:
    DesktopTracker();
    /**
      * Start time tracking of tasks by virtual desktop.
      *
      * @returns A QString with the error message, in case of no error an empty QString.
      *
      */
    QString startTracking();
    void registerForDesktops( Task* task, DesktopList dl );
    int desktopCount() const { return m_desktopCount; }

public Q_SLOTS:
    void handleDesktopChange(int desktop);

private Q_SLOTS:
    void changeTimers();

Q_SIGNALS:
    void reachedActiveDesktop(Task *task);
    void leftActiveDesktop(Task *task);

private:
    // define vectors for at most 16 virtual desktops
    // E.g.: desktopTrackerStop[3] contains a vector with
    // all tasks to be notified, when switching to/from desk 3.
    TaskVector m_desktopTracker[maxDesktops];
    int m_previousDesktop;
    int m_desktopCount;
    int m_desktop;
    QTimer *m_timer;
};

#endif // KTIMETRACKER_DESKTOP_TRACKER_H
