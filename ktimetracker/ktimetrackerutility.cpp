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

#include <KGlobal>
#include <KLocale>

#include <math.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <fixx11h.h>

QString getFocusWindow()
{
    Display *display = XOpenDisplay( 0 );
    char *name;
    Window window = 0;
    int i = 0;
    XGetInputFocus( display, &window, &i );
    XFetchName( display, window, &name );
    XCloseDisplay( display );
    return QString( name );
}

QString formatTime( double minutes, bool decimal )
{
    kDebug(5970) << "Entering function(minutes=" << minutes << ",decimal=" << decimal << ");";
    QString time;
    if ( decimal )
    {
        time.sprintf( "%.2f", minutes / 60.0 );
        time.replace( '.', KGlobal::locale()->decimalSymbol() );
    }
    else time.sprintf("%s%ld:%02ld",
        (minutes < 0) ? KGlobal::locale()->negativeSign().toUtf8().data() : "",
        labs(minutes / 60), labs(((int) round(minutes)) % 60));
    return time;
}

int desktopcount()
{
    int result;
    #ifdef Q_WS_X11
    result = KWindowSystem::numberOfDesktops();
    #else
    #ifdef __GNUC__
    #warning non-X11 support missing
    #endif
    result = -1;
    #endif
    return result;
}
