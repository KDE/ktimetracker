/*
 *     Copyright (C) 2007 by Thorsten Staerk <dev@staerk.de>
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

#include "karmutility.h"

#include <KGlobal>
#include <KLocale>

#include <X11/Xlib.h>
#include <fixx11h.h>

QString getFocusWindow()
{
  Display *display = XOpenDisplay( 0 );
  char *name = "blahblah";
  Window window = 0;
  int i = 0;
  XGetInputFocus( display, &window, &i );
  XFetchName( display, window, &name );
  XCloseDisplay( display );

  return QString( name );
}

QString formatTime( long minutes, bool decimal )
{
  QString time;
  if ( decimal ) {
    time.sprintf( "%.2f", minutes / 60.0 );
    time.replace( '.', KGlobal::locale()->decimalSymbol() );
  }
  else 
    time.sprintf( "%ld:%02ld", minutes / 60, labs( minutes % 60 ) );

  return time;
}
