/*
    SPDX-FileCopyrightText: 2007 Thorsten Staerk <dev@staerk.de>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTIMETRACKERUTILITY_H
#define KTIMETRACKERUTILITY_H

#include <QString>

#include <KCalendarCore/Incidence>

/**
  Format time for output.  All times output on screen or report output go
  through this function.
  If the second argument is true, the time is output as a two-place decimal.
  Otherwise the format is hh:mi.
  Examples:
  30 seconds are 0.5 minutes.
  The output of formatTiMe(0.5,true) is 0.008333, because 0.5 minutes are 0.008333 hours.
  The output of formatTiMe(0.5,false) is 0:01, because 0.5 minutes are 0:01 hours rounded.
 */
QString formatTime(double minutes, bool decimal = false);

const int secsPerMinute = 60;

enum KTIMETRACKER_Errors {
    KTIMETRACKER_ERR_GENERIC_SAVE_FAILED = 1,
    KTIMETRACKER_ERR_COULD_NOT_MODIFY_RESOURCE,
    KTIMETRACKER_ERR_MEMORY_EXHAUSTED,
    KTIMETRACKER_ERR_UID_NOT_FOUND,
    KTIMETRACKER_ERR_INVALID_DATE,
    KTIMETRACKER_ERR_INVALID_TIME,
    KTIMETRACKER_ERR_INVALID_DURATION,

    KTIMETRACKER_MAX_ERROR_NO = KTIMETRACKER_ERR_INVALID_DURATION
};

QString getCustomProperty(const KCalendarCore::Incidence::Ptr &incident, const QString &name);

#endif
