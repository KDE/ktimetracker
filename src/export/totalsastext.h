/*
    SPDX-FileCopyrightText: 2003 Mark Bucciarelli <mark@hubcapconsutling.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTIMETRACKER_TOTALSASTEXT_H
#define KTIMETRACKER_TOTALSASTEXT_H

#include "base/reportcriteria.h"

class TasksModel;
class Task;

/**
 * Generates ascii text of task totals, for current task on down.
 *
 * Formatted for pasting into clipboard.
 *
 * @param model The model whose tasks need to be formatted.
 * @param currentItem The task that needs to be formatted along with its subtasks.
 * @param rc Criteria which filters the task information.
 */
QString totalsAsText(TasksModel *model, Task *currentItem, const ReportCriteria &rc);

#endif // KTIMETRACKER_TOTALSASTEXT_H
