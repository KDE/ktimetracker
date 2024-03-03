/*
    SPDX-FileCopyrightText: 2003 Tomas Pospisek <tpo@sourcepole.ch>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
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
    int numDesktops = 0;
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
    int m_previousDesktop = 0;
    int m_desktopCount;
    int m_desktop = 0;
    QTimer *m_timer;
};

#endif // KTIMETRACKER_DESKTOP_TRACKER_H
