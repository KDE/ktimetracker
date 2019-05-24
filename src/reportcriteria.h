/*
 * Copyright (C) 2004 by Mark Bucciarelli <mark@hubcapconsulting.com>
 * Copyright (C) 2019  Alexander Potashev <aspotashev@gmail.com>
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

#ifndef REPORTCRITERIA_H
#define REPORTCRITERIA_H

#include <QDateTime>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

/**
 Stores entries from export dialog.

 Keeps details (like CSV export dialog control names) out of the TaskView
 class, which contains the slot triggered by the export action.  
 
 The dialog and the report logic can change all they want and the TaskView
 logic can stay the same.
 */

class ReportCriteria
{
  public:

    /**
    The different report types.
    */
    enum REPORTTYPE { CSVTotalsExport = 0, CSVHistoryExport = 1 };

    /**
    The type of report we are running.
    */
    REPORTTYPE reportType;

    /**
     For reports that write to a file, the filename to write to.
     */
    QUrl url;

    /**
     For history reports, the lower bound of the date range to report on.
     */
    QDate   from;

    /**
     For history reports, the upper bound of the date range to report on.
     */
    QDate   to;

    /**
     True if the durations should be output in decimal hours.  Otherwise,
     output durations as HH24:MI
     */
    bool    decimalMinutes;

    /**
     True if user chose to export session times, not all times
     */
    bool    sessionTimes;

    /** 
     True if user chose to export all tasks, not only the selected one
     */
    bool    allTasks;

    /**
     True if a clipboard export is wished, not an export to a file
     */
    bool    bExPortToClipBoard;

    /**
     The delimiter to use when outputting comma-separated value reports.
     */
    QString delimiter;

    /**
     The quote to use for text fields when outputting comma-separated reports.
     */
    QString quote;
};

#endif
