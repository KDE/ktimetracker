/*
    SPDX-FileCopyrightText: 2004 Mark Bucciarelli <mark@hubcapconsulting.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialogButtonBox>

#include "base/reportcriteria.h"
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
