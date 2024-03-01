/*
    SPDX-FileCopyrightText: 2003 Scott Monachello <smonach@cox.net>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
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
            + formatTime(static_cast<double>(task->totalTime()), rc.decimalMinutes) + QStringLiteral("\n");
    }

    return retval;
}
