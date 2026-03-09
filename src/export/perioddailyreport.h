/*
    SPDX-FileCopyrightText: 2026 KTimeTracker contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PERIODDAILYREPORT_H
#define PERIODDAILYREPORT_H

#include <QString>

#include "base/reportcriteria.h"

class ProjectModel;

/**
 * Generate a period daily report as plain text.
 *
 * The report covers the [rc.from, rc.to] date range. For each day that has
 * recorded time, it lists all tasks with their comments and time spent that
 * day, followed by a day total. A grand total for the whole period is
 * appended at the end.
 */
QString exportPeriodDailyReportToString(ProjectModel *projectModel, const ReportCriteria &rc);

#endif // PERIODDAILYREPORT_H
