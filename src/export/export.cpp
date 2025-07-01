/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "export.h"

#include <KIO/StoredTransferJob>
#include <KLocalizedString>

#include "csveventlog.h"
#include "csvhistory.h"
#include "csvtotals.h"
#include "ktt_debug.h"
#include "model/projectmodel.h"
#include "totalsastext.h"

QString exportToString(ProjectModel *model, Task *currentTask, const ReportCriteria &rc)
{
    switch (rc.reportType) {
    case ReportCriteria::CSVTotalsExport:
        return exportCSVToString(model->tasksModel(), rc);
    case ReportCriteria::CSVHistoryExport:
        return exportCSVHistoryToString(model, currentTask, rc);
    case ReportCriteria::CSVEventLogExport:
        return exportCSVEventLogToString(model, rc);
    case ReportCriteria::TextTotalsExport:
        return totalsAsText(model->tasksModel(), currentTask, rc);
    default:
        return QString();
    }
}

QString writeExport(const QString &data, const QUrl &url)
{
    QString err;

    // store the file locally or remote
    if (url.isLocalFile()) {
        qCDebug(KTT_LOG) << "storing a local file";
        QString filename = url.toLocalFile();
        if (filename.isEmpty()) {
            filename = url.url();
        }

        QFile f(filename);
        if (!f.open(QIODevice::WriteOnly)) {
            err = i18n("Could not open \"%1\".", filename);
            qCDebug(KTT_LOG) << "Could not open file";
        }
        qDebug() << "Err is " << err;
        if (err.length() == 0) {
            QTextStream stream(&f);
            qCDebug(KTT_LOG) << "Writing to file: " << data;
            // Export to file
            stream << data;
            f.close();
        }
    } else {
        // use remote file
        auto *const job = KIO::storedPut(data.toUtf8(), url, -1);
        // KJobWidgets::setWindow(job, &dialog); // TODO: add progress dialog
        if (!job->exec()) {
            err = QString::fromLatin1("Could not upload");
        }
    }

    return err;
}
