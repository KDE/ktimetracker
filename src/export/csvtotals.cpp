/*
 * Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
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

#include "csvtotals.h"

#include <QApplication>

#include "ktimetrackerutility.h"
#include "model/task.h"
#include "model/tasksmodel.h"

QString exportCSVToString(TasksModel *tasksModel, const ReportCriteria &rc)
{
    QString delim = rc.delimiter;
    QString dquote = rc.quote;
    QString double_dquote = dquote + dquote;

    QString retval;

    // Find max task depth
    int maxdepth = 0;
    for (Task *task : tasksModel->getAllTasks()) {
//        if (dialog.wasCanceled()) {
//            break;
//        }
//
//        dialog.setValue(dialog.value() + 1);

//        if (tasknr % 15 == 0) {
//            QApplication::processEvents(); // repainting is slow
//        }
        QApplication::processEvents();

        if (task->depth() > maxdepth) {
            maxdepth = task->depth();
        }
    }

    // Export to file
    for (Task *task : tasksModel->getAllTasks()) {
//        if (dialog.wasCanceled()) {
//            break;
//        }
//
//        dialog.setValue(dialog.value() + 1);

//        if (tasknr % 15 == 0) {
//            QApplication::processEvents(); // repainting is slow
//        }
        QApplication::processEvents();

        // indent the task in the csv-file:
        for (int i = 0; i < task->depth(); ++i) {
            retval += delim;
        }

        /*
        // CSV compliance
        // Surround the field with quotes if the field contains
        // a comma (delim) or a double quote
        if (task->name().contains(delim) || task->name().contains(dquote))
        to_quote = true;
        else
        to_quote = false;
        */
        bool to_quote = true;

        if (to_quote) {
            retval += dquote;
        }

        // Double quotes replaced by a pair of consecutive double quotes
        retval += task->name().replace(dquote, double_dquote);

        if (to_quote) {
            retval += dquote;
        }

        // maybe other tasks are more indented, so to align the columns:
        for (int i = 0; i < maxdepth - task->depth(); ++i) {
            retval += delim;
        }

        retval += delim + formatTime(static_cast<double>(task->sessionTime()), rc.decimalMinutes) + delim
            + formatTime(static_cast<double>(task->time()), rc.decimalMinutes) + delim
            + formatTime(static_cast<double>(task->totalSessionTime()), rc.decimalMinutes) + delim
            + formatTime(static_cast<double>(task->totalTime()), rc.decimalMinutes) + '\n';
    }

    return retval;
}
