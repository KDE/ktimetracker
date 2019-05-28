#include "export.h"

#include <QApplication>
#include <QClipboard>

#include <KLocalizedString>
#include <KIO/StoredTransferJob>

#include "model/projectmodel.h"
#include "csvtotals.h"
#include "csvhistory.h"
#include "ktt_debug.h"

QString exportToString(ProjectModel *model, Task *currentTask, const ReportCriteria &rc)
{
    switch (rc.reportType) {
        case ReportCriteria::CSVTotalsExport:
            return exportCSVToString(model->tasksModel(), rc);
        case ReportCriteria::CSVHistoryExport:
            return exportCSVHistoryToString(model, rc);
        default:
            return QString();
    }
}

QString writeExport(const ReportCriteria &rc, const QString &data)
{
    QString err = QString::null;

    if (rc.bExPortToClipBoard) {
        QApplication::clipboard()->setText(data);
    } else {
        // store the file locally or remote
        if (rc.url.isLocalFile()) {
            qCDebug(KTT_LOG) << "storing a local file";
            QString filename = rc.url.toLocalFile();
            if (filename.isEmpty()) {
                filename = rc.url.url();
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
            auto* const job = KIO::storedPut(data.toUtf8(), rc.url, -1);
            //KJobWidgets::setWindow(job, &dialog); // TODO: add progress dialog
            if (!job->exec()) {
                err = QString::fromLatin1("Could not upload");
            }
        }
    }
    return err;
}
