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

#include "desktoptracker.h"
#include <KDebug>
#include "ktimetracker.h"
#include "ktimetrackerutility.h"
#include <KWindowSystem>
#include <QTimer>

DesktopTracker::DesktopTracker ()
{
  // Setup desktop change handling
#ifdef Q_WS_X11
    connect( KWindowSystem::self(), SIGNAL(currentDesktopChanged(int)),
           this, SLOT(handleDesktopChange(int)) );
    mDesktopCount = desktopCount();
    mPreviousDesktop = KWindowSystem::self()->currentDesktop()-1;
#else
#ifdef __GNUC__
#warning non-X11 support missing
#endif
#endif
    // currentDesktop will return 0 if no window manager is started
    if( mPreviousDesktop < 0 ) mPreviousDesktop = 0;
    mTimer = new QTimer( this );
    mTimer->setSingleShot( true );
    connect( mTimer, SIGNAL(timeout()), this, SLOT(changeTimers()) );
}

void DesktopTracker::handleDesktopChange( int desktop )
{
    mDesktop = desktop;

    // If user changes back and forth between desktops rapidly and frequently,
    // the data file can get huge fast if logging is turned on. Then saving
    // gets slower, etc. There's no benefit in saving a lot of start/stop
    // events that are very small. Wait a bit to make sure the user is settled.
    mTimer->start( KTimeTrackerSettings::minActiveTime() * 1000 );
}

void DesktopTracker::changeTimers()
{
    --mDesktop; // desktopTracker starts with 0 for desktop 1
    // notify start all tasks setup for running on desktop

    // stop trackers for mPreviousDesktop
    foreach ( Task *task, mDesktopTracker[mPreviousDesktop] )
    {
        emit leftActiveDesktop( task );
    }

    // start trackers for desktop
    foreach ( Task *task, mDesktopTracker[mDesktop] )
    {
        emit reachedActiveDesktop( task );
    }
    mPreviousDesktop = mDesktop;
}

QString DesktopTracker::startTracking()
{
    QString err;
#ifdef Q_WS_X11
    int currentDesktop = KWindowSystem::self()->currentDesktop() -1;
#else
    int currentDesktop = 0;
#endif
    if ( currentDesktop < 0 ) currentDesktop = 0;
    if ( currentDesktop >= maxDesktops ) err="desktop number too high, desktop tracking will not work";
    else
        foreach ( Task *task, mDesktopTracker[ currentDesktop ] )
        {
            emit reachedActiveDesktop( task );
        }
    return err;
}

void DesktopTracker::registerForDesktops( Task* task, DesktopList desktopList )
{
    kDebug(5970) << "Entering function";
    // if no desktop is marked, disable auto tracking for this task
    if ( desktopList.size() == 0 )
    {
        for ( int i = 0; i < maxDesktops; ++i )
        {
            TaskVector *v = &( mDesktopTracker[i] );
            TaskVector::iterator tit = qFind( v->begin(), v->end(), task );
            if ( tit != v->end() )
            mDesktopTracker[i].erase( tit );
            // if the task was priviously tracking this desktop then
            // emit a signal that is not tracking it any more
#ifdef Q_WS_X11
            if ( i == KWindowSystem::self()->currentDesktop() - 1 )
                emit leftActiveDesktop( task );
#endif
        }
        kDebug(5970) << "Leaving function, desktopList.size=0";
        return;
    }

    // If desktop contains entries then configure desktopTracker
    // If a desktop was disabled, it will not be stopped automatically.
    // If enabled: Start it now.
    if ( desktopList.size() > 0 )
    {
        for ( int i = 0; i < maxDesktops; ++i )
        {
            TaskVector& v = mDesktopTracker[i];
            TaskVector::iterator tit = qFind( v.begin(), v.end(), task );
            // Is desktop i in the desktop list?
            if ( qFind( desktopList.begin(), desktopList.end(), i )
                != desktopList.end() )
            {
                if ( tit == v.end() )  // not yet in start vector
                v.push_back( task ); // track in desk i
            }
            else
            { // delete it
                if ( tit != v.end() ) // not in start vector any more
                {
                    v.erase( tit ); // so we delete it from desktopTracker
                    // if the task was priviously tracking this desktop then
                    // emit a signal that is not tracking it any more
#ifdef Q_WS_X11
                    if( i == KWindowSystem::self()->currentDesktop() -1)
                        emit leftActiveDesktop( task );
#endif
                }
            }
        }
        startTracking();
    }
    kDebug(5970) << "Leaving function";
}

#include "desktoptracker.moc"
