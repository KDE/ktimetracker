/*
    SPDX-FileCopyrightText: 2004 Mark Bucciarelli <mark@hubcapconsulting.com>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "exportdialog.h"

#include <QClipboard>
#include <QFileDialog>
#include <QFontDatabase>
#include <QPushButton>

#include <KMessageBox>

#include "base/taskview.h"
#include "export/export.h"
#include "ktt_debug.h"
#include "widgets/taskswidget.h"

ExportDialog::ExportDialog(QWidget *parent, TaskView *taskView)
    : QDialog(parent)
    , ui()
    , m_taskView(taskView)
{
    ui.setupUi(this);

    ui.previewText->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    // If decimal symbol is a comma, then default field separator to semi-colon.
    // In France and Germany, one-and-a-half is written as 1,5 not 1.5
    QString d = QLocale().decimalPoint();
    if (QChar::fromLatin1(',') == d) {
        ui.radioSemicolon->setChecked(true);
    } else {
        ui.radioComma->setChecked(true);
    }

    connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui.btnToClipboard, &QPushButton::clicked, this, &ExportDialog::exportToClipboard);
    connect(ui.btnSaveAs, &QPushButton::clicked, this, &ExportDialog::exportToFile);

    connect(ui.radioTimesCsv, &QRadioButton::toggled, this, &ExportDialog::updateUI);
    connect(ui.radioHistoryCsv, &QRadioButton::toggled, this, &ExportDialog::updateUI);
    connect(ui.radioEventLogCsv, &QRadioButton::toggled, this, &ExportDialog::updateUI);
    connect(ui.radioTimesText, &QRadioButton::toggled, this, &ExportDialog::updateUI);
    connect(ui.radioComma, &QRadioButton::toggled, this, &ExportDialog::updateUI);
    connect(ui.radioSemicolon, &QRadioButton::toggled, this, &ExportDialog::updateUI);
    connect(ui.radioOther, &QRadioButton::toggled, this, &ExportDialog::updateUI);
    connect(ui.radioTab, &QRadioButton::toggled, this, &ExportDialog::updateUI);
    connect(ui.radioSpace, &QRadioButton::toggled, this, &ExportDialog::updateUI);
    connect(ui.txtOther, &QLineEdit::textChanged, this, &ExportDialog::updateUI);
    connect(ui.combodecimalminutes, &QComboBox::currentTextChanged, this, &ExportDialog::updateUI);
    connect(ui.comboalltasks, &QComboBox::currentTextChanged, this, &ExportDialog::updateUI);
    connect(ui.combosessiontimes, &QComboBox::currentTextChanged, this, &ExportDialog::updateUI);
    connect(ui.cboQuote, &QComboBox::currentTextChanged, this, &ExportDialog::updateUI);
    connect(ui.dtFrom, &KDateComboBox::dateChanged, this, &ExportDialog::updateUI);
    connect(ui.dtTo, &KDateComboBox::dateChanged, this, &ExportDialog::updateUI);

    updateUI();
}

void ExportDialog::enableTasksToExportQuestion()
{
    return;
    // grpTasksToExport->setEnabled( true );
}

void ExportDialog::exportToClipboard()
{
    QString output = exportToString(m_taskView->storage()->projectModel(), m_taskView->tasksWidget()->currentItem(), reportCriteria());
    QApplication::clipboard()->setText(output);
}

void ExportDialog::exportToFile()
{
    const QUrl &url = QFileDialog::getSaveFileUrl(this, i18nc("@title:window", "Export to File"));
    if (url.isEmpty()) {
        return;
    }

    QString output = exportToString(m_taskView->storage()->projectModel(), m_taskView->tasksWidget()->currentItem(), reportCriteria());
    QString err = writeExport(output, url);
    if (err.isEmpty()) {
        // Close export dialog after saving
        accept();
    } else {
        KMessageBox::error(parentWidget(), i18n(err.toLatin1().constData()));
    }
}

ReportCriteria ExportDialog::reportCriteria()
{
    rc.from = ui.dtFrom->date();
    rc.to = ui.dtTo->date();
    rc.decimalMinutes = (ui.combodecimalminutes->currentText() == i18nc("format to display times", "Decimal"));
    qCDebug(KTT_LOG) << "rc.decimalMinutes is" << rc.decimalMinutes;

    if (ui.radioComma->isChecked()) {
        rc.delimiter = QChar::fromLatin1(',');
    } else if (ui.radioTab->isChecked()) {
        rc.delimiter = QChar::fromLatin1('\t');
    } else if (ui.radioSemicolon->isChecked()) {
        rc.delimiter = QChar::fromLatin1(';');
    } else if (ui.radioSpace->isChecked()) {
        rc.delimiter = QChar::fromLatin1(' ');
    } else if (ui.radioOther->isChecked()) {
        rc.delimiter = ui.txtOther->text();
    } else {
        qCDebug(KTT_LOG) << "*** ExportDialog::reportCriteria: Unexpected "
                            "delimiter choice '";
        rc.delimiter = QChar::fromLatin1('\t');
    }

    rc.quote = ui.cboQuote->currentText();
    rc.sessionTimes = (i18n("Session Times") == ui.combosessiontimes->currentText());
    rc.allTasks = (i18n("All Tasks") == ui.comboalltasks->currentText());
    return rc;
}

void ExportDialog::updateUI()
{
    ReportCriteria::REPORTTYPE rt;
    if (ui.radioTimesCsv->isChecked()) {
        rt = ReportCriteria::CSVTotalsExport;
    } else if (ui.radioHistoryCsv->isChecked()) {
        rt = ReportCriteria::CSVHistoryExport;
    } else if (ui.radioEventLogCsv->isChecked()) {
        rt = ReportCriteria::CSVEventLogExport;
    } else if (ui.radioTimesText->isChecked()) {
        rt = ReportCriteria::TextTotalsExport;
    } else {
        qCWarning(KTT_LOG) << "*** ExportDialog::updateUI: Unexpected report type choice";
        rt = ReportCriteria::TextTotalsExport;
    }
    rc.reportType = rt;

    switch (rt) {
    case ReportCriteria::CSVHistoryExport:
    case ReportCriteria::CSVEventLogExport:
        ui.grpDateRange->setEnabled(true);
        ui.grpDateRange->show();
        break;
    case ReportCriteria::TextTotalsExport:
    case ReportCriteria::CSVTotalsExport:
    default:
        ui.grpDateRange->setEnabled(false);
        ui.grpDateRange->hide();
        break;
    }

    ui.previewText->setText(exportToString(m_taskView->storage()->projectModel(), m_taskView->tasksWidget()->currentItem(), reportCriteria()));
}

#include "moc_exportdialog.cpp"
