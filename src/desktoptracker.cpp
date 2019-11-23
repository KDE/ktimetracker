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

#include "desktoptracker.h"

#include <QTimer>

#include <KWindowSystem>

#include "ktt_debug.h"
#include "ktimetracker.h"
#include "ktimetrackerutility.h"

DesktopTracker::DesktopTracker()
    : m_desktopTracker()
    // currentDesktop will return 0 if no window manager is started
    , m_previousDesktop(std::max(KWindowSystem::currentDesktop() - 1, 0))
    , m_desktopCount(KWindowSystem::numberOfDesktops())
    // currentDesktop will return 0 if no window manager is started
    , m_desktop(std::max(KWindowSystem::currentDesktop() - 1, 0))
    , m_timer(new QTimer(this))
{
    // Setup desktop change handling
    connect(KWindowSystem::self(), &KWindowSystem::currentDesktopChanged, this, &DesktopTracker::handleDesktopChange);

    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &DesktopTracker::changeTimers);
}

void DesktopTracker::handleDesktopChange(int desktop)
{
    m_desktop = desktop;

    // If user changes back and forth between desktops rapidly and frequently,
    // the data file can get huge fast if logging is turned on. Then saving
    // gets slower, etc. There's no benefit in saving a lot of start/stop
    // events that are very small. Wait a bit to make sure the user is settled.
    m_timer->start(KTimeTrackerSettings::minActiveTime() * 1000);
}

void DesktopTracker::changeTimers()
{
    --m_desktop; // desktopTracker starts with 0 for desktop 1
    // notify start all tasks setup for running on desktop

    // stop trackers for m_previousDesktop
    for (Task *task : m_desktopTracker[m_previousDesktop]) {
        emit leftActiveDesktop(task);
    }

    // start trackers for desktop
    for (Task *task : m_desktopTracker[m_desktop]) {
        emit reachedActiveDesktop(task);
    }

    m_previousDesktop = m_desktop;
}

QString DesktopTracker::startTracking()
{
    int currentDesktop = KWindowSystem::currentDesktop() - 1;
    if (currentDesktop < 0) {
        currentDesktop = 0;
    }

    if (currentDesktop >= maxDesktops) {
        return QStringLiteral("desktop number too high, desktop tracking will not work");
    }

    for (Task *task : m_desktopTracker[currentDesktop]) {
        emit reachedActiveDesktop(task);
    }

    return QString();
}

void DesktopTracker::registerForDesktops( Task* task, DesktopList desktopList )
{
    qCDebug(KTT_LOG) << "Entering function";
    // if no desktop is marked, disable auto tracking for this task
    if (desktopList.empty()) {
        for (int i = 0; i < maxDesktops; ++i) {
            TaskVector *v = &(m_desktopTracker[i]);
            TaskVector::iterator tit = std::find(v->begin(), v->end(), task);
            if (tit != v->end()) {
                m_desktopTracker[i].erase( tit );
            }
            // if the task was previously tracking this desktop then
            // emit a signal that is not tracking it any more
            if (i == KWindowSystem::currentDesktop() - 1) {
                emit leftActiveDesktop(task);
            }
        }
        qCDebug(KTT_LOG) << "Leaving function, desktopList.size=0";
        return;
    }

    // If desktop contains entries then configure desktopTracker
    // If a desktop was disabled, it will not be stopped automatically.
    // If enabled: Start it now.
    if (!desktopList.empty()) {
        for (int i = 0; i < maxDesktops; ++i) {
            TaskVector& v = m_desktopTracker[i];
            TaskVector::iterator tit = std::find(v.begin(), v.end(), task);
            // Is desktop i in the desktop list?
            if (std::find( desktopList.begin(), desktopList.end(), i) != desktopList.end()) {

                if (tit == v.end()) {
                    // not yet in start vector
                    v.push_back(task); // track in desk i
                }
            } else {
                // delete it
                if (tit != v.end()) {
                    // not in start vector any more
                    v.erase(tit); // so we delete it from desktopTracker
                    // if the task was previously tracking this desktop then
                    // emit a signal that is not tracking it any more
                    if (i == KWindowSystem::currentDesktop() - 1) {
                        emit leftActiveDesktop(task);
                    }
                }
            }
        }
        startTracking();
    }
    qCDebug(KTT_LOG) << "Leaving function";
}
