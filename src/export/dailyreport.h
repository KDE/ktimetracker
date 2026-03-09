/*
    SPDX-FileCopyrightText: 2026 KTimeTracker contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DAILYREPORT_H
#define DAILYREPORT_H

#include <QString>

#include "base/reportcriteria.h"

class ProjectModel;

/**
 * Generate a daily report as plain text.
 *
 * The report lists all tasks that have recorded time on rc.from date,
 * showing the task name, all associated comments, and the time spent.
 * A total time line is appended at the end.
 */
QString exportDailyReportToString(ProjectModel *projectModel, const ReportCriteria &rc);

#endif // DAILYREPORT_H
