/*
    SPDX-FileCopyrightText: 2026 KTimeTracker contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PERIODREPORT_H
#define PERIODREPORT_H

#include <QString>

#include "base/reportcriteria.h"

class ProjectModel;

/**
 * Generate a period report as plain text.
 *
 * The report lists all tasks that have recorded time in the [rc.from, rc.to]
 * date range, showing the task name, all associated comments, and the total
 * time spent. A grand total line is appended at the end.
 */
QString exportPeriodReportToString(ProjectModel *projectModel, const ReportCriteria &rc);

#endif // PERIODREPORT_H
