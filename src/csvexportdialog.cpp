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

#include "csvexportdialog.h"

#include <QPushButton>

#include <KMessageBox>
#include <QtWidgets/QFileDialog>

#include "taskview.h"
#include "ktt_debug.h"

CSVExportDialog::CSVExportDialog(QWidget *parent, TaskView *taskView, ReportCriteria::REPORTTYPE rt)
    : QDialog(parent)
    , ui()
    , m_taskView(taskView)
{
    ui.setupUi(this);

    ui.buttonBox->button(QDialogButtonBox::Ok)->setText(i18nc("@action:button", "&Export"));

    // TODO: put this button on the left, aside from OK/Cancel
    QPushButton* const clipboardButton = ui.buttonBox->button(QDialogButtonBox::Save);
    clipboardButton->setText(i18nc("@action:button", "E&xport to Clipboard"));
    clipboardButton->setIcon(QIcon::fromTheme("klipper"));

    connect(ui.buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked,
            this, &CSVExportDialog::exPortToClipBoard);
    connect(ui.buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
            this, &CSVExportDialog::exPortToCSVFile);

    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    switch (rt) {
    case ReportCriteria::TextTotalsExport:
    case ReportCriteria::CSVTotalsExport:
        ui.grpDateRange->setEnabled(false);
        ui.grpDateRange->hide();
        rc.reportType = rt;
        break;
    case ReportCriteria::CSVHistoryExport:
        ui.grpDateRange->setEnabled(true);
        rc.reportType = rt;
        break;
    default:
        break;
    }

    // If decimal symbol is a comma, then default field separator to semi-colon.
    // In France and Germany, one-and-a-half is written as 1,5 not 1.5
    QChar d = QLocale().decimalPoint();
    if (QChar(',') == d) {
        ui.radioSemicolon->setChecked(true);
    } else {
        ui.radioComma->setChecked(true);
    }
}

void CSVExportDialog::enableTasksToExportQuestion()
{
    return;
    //grpTasksToExport->setEnabled( true );
}

void CSVExportDialog::exPortToClipBoard()
{
    if (rc.reportType == ReportCriteria::CSVTotalsExport) {
        rc.reportType = ReportCriteria::TextTotalsExport;
    }

    rc.bExPortToClipBoard = true;

    QString err = m_taskView->report(reportCriteria(), QUrl());
    if (!err.isEmpty()) {
        KMessageBox::error(parentWidget(), i18n(err.toLatin1()));
    }
}

void CSVExportDialog::exPortToCSVFile()
{
    rc.bExPortToClipBoard = false;

    const QUrl &url = QFileDialog::getSaveFileUrl(this, i18nc("@title:window", "Export as CSV"));
    QString err = m_taskView->report(reportCriteria(), url);
    if (!err.isEmpty()) {
        KMessageBox::error(parentWidget(), i18n(err.toLatin1()));
    }

    // Close export dialog after saving
    accept();
}

ReportCriteria CSVExportDialog::reportCriteria()
{
    rc.from = ui.dtFrom->date();
    rc.to = ui.dtTo->date();
    rc.decimalMinutes = (ui.combodecimalminutes->currentText() == i18nc("format to display times", "Decimal"));
    qCDebug(KTT_LOG) <<"rc.decimalMinutes is" << rc.decimalMinutes;

    if (ui.radioComma->isChecked()) {
        rc.delimiter = ",";
    } else if (ui.radioTab->isChecked()) {
        rc.delimiter = "\t";
    } else if (ui.radioSemicolon->isChecked()) {
        rc.delimiter = ";";
    } else if (ui.radioSpace->isChecked()) {
        rc.delimiter = " ";
    } else if (ui.radioOther->isChecked()) {
        rc.delimiter = ui.txtOther->text();
    } else {
        qCDebug(KTT_LOG) << "*** CSVExportDialog::reportCriteria: Unexpected delimiter choice '";
        rc.delimiter = "\t";
    }

    rc.quote = ui.cboQuote->currentText();
    rc.sessionTimes = (i18n("Session Times") == ui.combosessiontimes->currentText());
    rc.allTasks = (i18n("All Tasks") == ui.comboalltasks->currentText());
    return rc;
}
