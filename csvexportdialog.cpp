/*
 *     Copyright (C) 2004 by Mark Bucciarelli <mark@hubcapconsulting.com>
 *                   2007 the ktimetracker developers
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

#include <QDebug>
#include "ktt_debug.h"
#include <KDateComboBox>
#include <KGlobal>
#include <KLocale>
#include <KPushButton>
#include <KLineEdit>

CSVExportDialog::CSVExportDialog(ReportCriteria::REPORTTYPE rt, QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    ui.urlExportTo->KUrlRequester::setMode(KFile::File);

    ui.buttonBox->button(QDialogButtonBox::Ok)->setText(i18nc("@action:button", "&Export"));

    // TODO: put this button on the left, aside from OK/Cancel
    QPushButton* const clipboardButton = ui.buttonBox->button(QDialogButtonBox::Save);
    clipboardButton->setText(i18nc("@action:button", "E&xport to Clipboard"));
    clipboardButton->setIcon(QIcon::fromTheme("klipper"));

    connect(ui.buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked,
            this, &CSVExportDialog::exPortToCSVFile);
    connect(ui.buttonBox, &QDialogButtonBox::accepted,
            this, &CSVExportDialog::exPortToCSVFile);

    connect(ui.urlExportTo, &KUrlRequester::textChanged, this, &CSVExportDialog::enableExportButton);

    switch (rt) {
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
    QString d = KGlobal::locale()->decimalSymbol();
    if ( "," == d ) ui.radioSemicolon->setChecked(true);
    else ui.radioComma->setChecked(true);
}

void CSVExportDialog::enableExportButton()
{
    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!ui.urlExportTo->lineEdit()->text().isEmpty());
}

void CSVExportDialog::enableTasksToExportQuestion()
{
    return;
    //grpTasksToExport->setEnabled( true );
}

void CSVExportDialog::exPortToClipBoard()
{
    rc.bExPortToClipBoard=true;
    accept();
}

void CSVExportDialog::exPortToCSVFile()
{
    rc.bExPortToClipBoard=false;
    accept();
}

ReportCriteria CSVExportDialog::reportCriteria()
{
    rc.url = ui.urlExportTo->url();
    rc.from = ui.dtFrom->date();
    rc.to = ui.dtTo->date();
    rc.decimalMinutes = (ui.combodecimalminutes->currentText() == i18nc( "format to display times", "Decimal" ) );
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
