/*
 * Copyright (c) 2019 Alexander Potashev <aspotashev@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
        return exportCSVHistoryToString(model, rc);
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
        auto* const job = KIO::storedPut(data.toUtf8(), url, -1);
        //KJobWidgets::setWindow(job, &dialog); // TODO: add progress dialog
        if (!job->exec()) {
            err = QString::fromLatin1("Could not upload");
        }
    }

    return err;
}
