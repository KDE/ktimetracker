/*
 *     Copyright (C) 2007 by René Mérou <ochominutosdearco@gmail.com>
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

#include "focusdetector.h"
#include "karmutility.h"

#include <QProcess>
#include <QTimer>

#include <KDebug>

#ifdef Q_WS_X11
#include <QX11Info>
#endif

FocusDetector::FocusDetector( int periodFocus )
{
  mTimer = new QTimer( this );
  connect( mTimer, SIGNAL( timeout() ), this, SLOT( check() ) );

  /* do not start the timer immediately, b/c it is focus detection
     is activated by menu action. */
  //_timer->start(1000*periodFocus);

  setPeriodFocus( periodFocus );
}

void FocusDetector::check()
{
  QProcess focusQuestion;

  QString sysanswer = getFocusWindow();
  kDebug() << "getFocusWindow = " << sysanswer << endl;

  if ( mLastWindow != sysanswer ) {
    mLastWindow = sysanswer;
    kDebug() << "NEW WINDOW WITH FOCUS; Sending signal." << endl;
    emit( newFocus( sysanswer ) );
  }
}

void FocusDetector::setPeriodFocus( int periodFocus )
{
  mTimer->setInterval( periodFocus * 1000 );
}

void FocusDetector::startFocusDetection()
{
  if ( !( mTimer->isActive() ) )
    mTimer->start();
}

void FocusDetector::stopFocusDetection()
{
  if ( mTimer->isActive() )
    mTimer->stop();
}

#include "focusdetector.moc" 
