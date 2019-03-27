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

#ifndef KTIMETRACKER_TIMEKARD_H
#define KTIMETRACKER_TIMEKARD_H

#undef Color // X11 headers
#undef GrayScale // X11 headers

#include "reportcriteria.h"

class Task;
class TaskView;

/**
 *  Routines to output timecard data.
 */
class TimeKard
{
public:
    TimeKard() = default;

    /**
     * Generates ascii text of task totals, for current task on down.
     *
     * Formatted for pasting into clipboard.
     *
     * @param taskview The view whose tasks need to be formatted.
     *
     * @param rc Criteria which filters the task information.
     */
    QString totalsAsText(TaskView* taskview, ReportCriteria rc);

private:
    void printTask(Task* t, QString& s, int level, const ReportCriteria &rc);
};

#endif // KTIMETRACKER_TIMEKARD_H
