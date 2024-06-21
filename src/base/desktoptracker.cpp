/*
    SPDX-FileCopyrightText: 2003 Tomas Pospisek <tpo@sourcepole.ch>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "desktoptracker.h"

#include <QTimer>

#ifdef Q_OS_LINUX
    #include <KX11Extras>
    #include <KWindowSystem>
#endif // Q_OS_LINUX

#include "ktimetracker.h"
#include "ktt_debug.h"

DesktopTracker::DesktopTracker()
    : m_desktopCount(numDesktops)
    , m_timer(new QTimer(this))
{
#ifdef Q_OS_LINUX
    if(KWindowSystem::isPlatformX11())
    {
        // currentDesktop will return 0 if no window manager is started
        m_previousDesktop = std::max(KX11Extras::currentDesktop() - 1, 0);
        m_desktop = std::max(KX11Extras::currentDesktop() - 1, 0);
        numDesktops = KX11Extras::numberOfDesktops();
        // Setup desktop change handling
        connect(KX11Extras::self(), &KX11Extras::currentDesktopChanged, this, &DesktopTracker::handleDesktopChange);
    }
#endif
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
        Q_EMIT leftActiveDesktop(task);
    }

    // start trackers for desktop
    for (Task *task : m_desktopTracker[m_desktop]) {
        Q_EMIT reachedActiveDesktop(task);
    }

    m_previousDesktop = m_desktop;
}

QString DesktopTracker::startTracking()
{
#ifdef Q_OS_LINUX
    if(KWindowSystem::isPlatformX11())
    {
        int currentDesktop = KX11Extras::currentDesktop() - 1;
        if (currentDesktop < 0) {
            currentDesktop = 0;
        }

        if (currentDesktop >= maxDesktops) {
            return QStringLiteral("desktop number too high, desktop tracking will not work");
        }

        for (Task *task : m_desktopTracker[currentDesktop]) {
            Q_EMIT reachedActiveDesktop(task);
        }
    }
#endif
    return QString();
}

void DesktopTracker::registerForDesktops(Task *task, DesktopList desktopList)
{
    qCDebug(KTT_LOG) << "Entering function";
    // if no desktop is marked, disable auto tracking for this task
    if (desktopList.empty()) {
        for (int i = 0; i < maxDesktops; ++i) {
            TaskVector *v = &(m_desktopTracker[i]);
            TaskVector::iterator tit = std::find(v->begin(), v->end(), task);
            if (tit != v->end()) {
                m_desktopTracker[i].erase(tit);
            }
#ifdef Q_OS_LINUX
            if(KWindowSystem::isPlatformX11())
            {
                // if the task was previously tracking this desktop then
                // emit a signal that is not tracking it any more
                if (i == KX11Extras::currentDesktop() - 1) {
                    Q_EMIT leftActiveDesktop(task);
                }
            }
#endif
        }
        qCDebug(KTT_LOG) << "Leaving function, desktopList.size=0";
        return;
    }

    // If desktop contains entries then configure desktopTracker
    // If a desktop was disabled, it will not be stopped automatically.
    // If enabled: Start it now.
    if (!desktopList.empty()) {
        for (int i = 0; i < maxDesktops; ++i) {
            TaskVector &v = m_desktopTracker[i];
            TaskVector::iterator tit = std::find(v.begin(), v.end(), task);
            // Is desktop i in the desktop list?
            if (std::find(desktopList.begin(), desktopList.end(), i) != desktopList.end()) {
                if (tit == v.end()) {
                    // not yet in start vector
                    v.push_back(task); // track in desk i
                }
            } else {
                // delete it
                if (tit != v.end()) {
                    // not in start vector any more
                    v.erase(tit); // so we delete it from desktopTracker
#ifdef Q_OS_LINUX
                    if(KWindowSystem::isPlatformX11())
                    {
                        // if the task was previously tracking this desktop then
                        // emit a signal that is not tracking it any more
                        if (i == KX11Extras::currentDesktop() - 1) {
                            Q_EMIT leftActiveDesktop(task);
                        }
                    }
#endif
                }
            }
        }
        startTracking();
    }
    qCDebug(KTT_LOG) << "Leaving function";
}

#include "moc_desktoptracker.cpp"
