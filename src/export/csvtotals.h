/*
    SPDX-FileCopyrightText: 2003 Scott Monachello <smonach@cox.net>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTIMETRACKER_CSVTOTALS_H
#define KTIMETRACKER_CSVTOTALS_H

#include "base/reportcriteria.h"

class TasksModel;

QString exportCSVToString(TasksModel *tasksModel, const ReportCriteria &rc);

#endif // KTIMETRACKER_CSVTOTALS_H
