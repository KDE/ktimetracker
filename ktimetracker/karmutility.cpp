/*
 *     Copyright (C) 2007 the ktimetracker developers
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

#ifndef KARM_UTILITY_H
#define KARM_UTILITY_H

#include <stdlib.h>

#include <kglobal.h>
#include <klocale.h>
#include "karmutility.h"

QString formatTime( long minutes, bool decimal )
{
  QString time;
  if ( decimal ) {
    time.sprintf("%.2f", minutes / 60.0);
    time.replace( '.', KGlobal::locale()->decimalSymbol() );
  }
  else time.sprintf("%ld:%02ld", minutes / 60, labs(minutes % 60));
  return time;
}

#endif // KARM_UTILITY_H
