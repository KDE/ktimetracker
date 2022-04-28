/*
 * Copyright (C) 2004 by Mark Bucciarelli <mark@hubcapconsulting.com>
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

#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialogButtonBox>

#include "reportcriteria.h"
#include "ui_exportdialog.h"

class TaskView;

class ExportDialog : public QDialog
{
    Q_OBJECT

public Q_SLOTS:
    void exportToClipboard();
    void exportToFile();

    void updateUI();

public:
    explicit ExportDialog(QWidget *parent, TaskView *taskView);

    /**
     Enable the "Tasks to export" question in the dialog.

     Since ktimetracker does not have the concept of a single root task, when the user
     requests a report on a top-level task, it is impossible to know if they
     want all tasks or just the currently selected top-level task.

     Stubbed for 3.3 release as CSV export of totals doesn't support this option.
     */
    void enableTasksToExportQuestion();

    /**
     Return an object that encapsulates the choices the user has made.
     */
    ReportCriteria reportCriteria();

private:
    Ui::ExportDialog ui;
    TaskView *m_taskView;
    ReportCriteria rc;
};

#endif
