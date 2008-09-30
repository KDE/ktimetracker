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

#ifndef KTIMETRACKERUTILITY_H
#define KTIMETRACKERUTILITY_H

#include <KDebug>
#include <QString>

/**
  Format time for output.  All times output on screen or report output go
  through this function.
  If the second argument is true, the time is output as a two-place decimal.
  Otherwise the format is hh:mi.
 */
QString formatTime( double minutes, bool decimal = false );

/**
  Get the name of the window that has the focus
 */
QString getFocusWindow();

const int secsPerMinute=60;

enum KTIMETRACKER_Errors
{
  KTIMETRACKER_ERR_GENERIC_SAVE_FAILED = 1,
  KTIMETRACKER_ERR_COULD_NOT_MODIFY_RESOURCE,
  KTIMETRACKER_ERR_MEMORY_EXHAUSTED,
  KTIMETRACKER_ERR_UID_NOT_FOUND,
  KTIMETRACKER_ERR_INVALID_DATE,
  KTIMETRACKER_ERR_INVALID_TIME,
  KTIMETRACKER_ERR_INVALID_DURATION,

  KTIMETRACKER_MAX_ERROR_NO = KTIMETRACKER_ERR_INVALID_DURATION
};

#endif
