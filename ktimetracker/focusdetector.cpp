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


#include <KDebug>

#ifdef Q_WS_X11
#endif

FocusDetector::FocusDetector( int periodFocus )
{
  KWindowSystem* kw=KWindowSystem::self();
  connect(kw, SIGNAL(activeWindowChanged(WId)), SLOT( slotfocuschanged() ) );
}

void FocusDetector::slotfocuschanged()
{
  kDebug(5970) << "The focus changed";
  QString sysanswer = getFocusWindow();
  kDebug(5970) <<"getFocusWindow =" << sysanswer;
  emit( newFocus( sysanswer ) );
}

#include "focusdetector.moc" 
