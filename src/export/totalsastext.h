/*
 *     Copyright (C) 2003 by Mark Bucciarelli <mark@hubcapconsutling.com>
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

#ifndef KTIMETRACKER_TOTALSASTEXT_H
#define KTIMETRACKER_TOTALSASTEXT_H

#include "reportcriteria.h"

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
