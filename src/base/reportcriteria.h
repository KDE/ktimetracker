/*
    SPDX-FileCopyrightText: 2004 Mark Bucciarelli <mark@hubcapconsulting.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef REPORTCRITERIA_H
#define REPORTCRITERIA_H

#include <QDateTime>
#include <QUrl>

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
     * The different report types.
     *
     * These numeric constants are part of D-Bus API, don't change them.
     */
    enum REPORTTYPE {
        /**
         * Writes all tasks and their totals to a Comma-Separated Values file.
         *
         * The format of this file is zero or more lines of:
         *    taskName,subtaskName,..,sessionTime,time,totalSessionTime,totalTime
         * the number of subtasks is determined at runtime.
         */
        CSVTotalsExport = 0,
        /**
         * Export history report as csv, all tasks X all dates in one block.
         * Write task history as comma-delimited data.
         */
        CSVHistoryExport = 1,
        /**
         * Totals for current and all sub tasks.
         * This function stores the user's tasks.
         * rc tells how the user wants his report, e.g. all times or session times.
         */
        TextTotalsExport = 2,
        /**
         * Export the history of all events as csv
         */
        CSVEventLogExport = 3,
    };

    /**
    The type of report we are running.
    */
    REPORTTYPE reportType;

    /**
     For history reports, the lower bound of the date range to report on.
     */
    QDate from;

    /**
     For history reports, the upper bound of the date range to report on.
     */
    QDate to;

    /**
     True if the durations should be output in decimal hours.  Otherwise,
     output durations as HH24:MI
     */
    bool decimalMinutes;

    /**
     True if user chose to export session times, not all times
     */
    bool sessionTimes;

    /**
     True if user chose to export all tasks, not only the selected one
     */
    bool allTasks;

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
