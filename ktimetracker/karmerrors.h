/*
 *   This file only:
 *     Copyright (C) 2005  Mark Bucciarelli <mark@hubcapconsutling.com>
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
 *      59 Temple Place - Suite 330
 *      Boston, MA  02111-1307  USA.
 *
 */
#ifndef KARM_ERRORS_H
#define KARM_ERRORS_H

enum KARM_Errors {
  KARM_ERR_GENERIC_SAVE_FAILED = 1,
  KARM_ERR_COULD_NOT_MODIFY_RESOURCE,
  KARM_ERR_MEMORY_EXHAUSTED,
  KARM_ERR_UID_NOT_FOUND,
  KARM_ERR_INVALID_DATE,
  KARM_ERR_INVALID_TIME,
  KARM_ERR_INVALID_DURATION,

  KARM_MAX_ERROR_NO = KARM_ERR_INVALID_DURATION
};

#endif // KARM_ERRORS_H


