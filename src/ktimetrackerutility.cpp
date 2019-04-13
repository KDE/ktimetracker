/*
 *     Copyright (C) 2007 by Thorsten Staerk <dev@staerk.de>
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

#include "ktimetrackerutility.h"

#include <KWindowSystem>

#include <math.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <fixx11h.h>

QString getFocusWindow()
{
    return KWindowSystem::windowInfo(KWindowSystem::activeWindow(),NET::WMName, 0).name();
}

QString formatTime( double minutes, bool decimal )
{
    qCDebug(KTT_LOG) << "Entering function(minutes=" << minutes << ",decimal=" << decimal << ");";
    QString time;
    if ( decimal )
    {
        time.sprintf("%.2f", minutes / 60.0 );
        time.replace('.', QLocale().decimalPoint());
    }
    else time.sprintf("%s%ld:%02ld",
        (minutes < 0) ? QString(QLocale().negativeSign()).toUtf8().data() : "",
        labs(minutes / 60), labs(((int) round(minutes)) % 60));
    return time;
}

int desktopcount()
{
    return KWindowSystem::numberOfDesktops();
}
