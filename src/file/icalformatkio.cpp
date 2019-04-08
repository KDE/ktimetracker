#include "icalformatkio.h"

#include <QSaveFile>

#include <KIO/StoredTransferJob>
#include <KCalCore/Exceptions>
#include <KJobWidgets>

#include "ktt_debug.h"

ICalFormatKIO::ICalFormatKIO()
    : KCalCore::ICalFormat() {
}

bool ICalFormatKIO::load(const KCalCore::Calendar::Ptr &calendar, const QString &fileName) {
    qCDebug(KTT_LOG) << fileName;

    clearException();

    QUrl url(fileName);
    if (url.isLocalFile()) {
        QFile file(url.path());
        if (!file.open(QIODevice::ReadOnly)) {
            qCWarning(KTT_LOG) << "load file open error: " << file.errorString() << ";filename=" << fileName;
            setException(new KCalCore::Exception(KCalCore::Exception::LoadError));
            return false;
        }
        const QByteArray text = file.readAll().trimmed();
        file.close();

        if (text.isEmpty()) {
            // empty files are valid
            return true;
        } else {
            return fromRawString(calendar, text, false, fileName);
        }
    } else {
        // use remote file
        auto* const job = KIO::storedGet(url, KIO::Reload);
        KJobWidgets::setWindow(job, nullptr); // hide progress notification
        if (!job->exec()) {
            setException(new KCalCore::Exception(KCalCore::Exception::SaveErrorSaveFile, QStringList(url.url())));
            return false;
        }

        const QByteArray text = job->data();
        if (text.isEmpty()) {
            // empty files are valid
            return true;
        } else {
            return fromRawString(calendar, text, false, fileName);
        }
    }
}

bool ICalFormatKIO::save(const KCalCore::Calendar::Ptr &calendar, const QString &fileName) {
    qCDebug(KTT_LOG) << fileName;

    clearException();

    QString text = toString(calendar);
    if (text.isEmpty()) {
        // TODO: save empty file if we have 0 tasks?
        return false;
    }

    QByteArray textUtf8 = text.toUtf8();

    // TODO: Write backup file (i.e. backup the existing file somewhere, e.g. to ~/.local/share/apps/ktimetracker/backups/)
//    const QString backupFile = fileName + QLatin1Char('~');
//    QFile::remove(backupFile);
//    QFile::copy(fileName, backupFile);

    // save, either locally or remote
    QUrl url(fileName);
    if (url.isLocalFile()) {
        QSaveFile file(url.path());
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(KTT_LOG) << "save file open error: " << file.errorString() << ";filename=" << fileName;
            setException(new KCalCore::Exception(KCalCore::Exception::SaveErrorOpenFile, QStringList(fileName)));
            return false;
        }

        file.write(textUtf8.data(), textUtf8.size());

        if (!file.commit()) {
            qCWarning(KTT_LOG) << "file finalize error:" << file.errorString();
            setException(new KCalCore::Exception(KCalCore::Exception::SaveErrorSaveFile, QStringList(fileName)));
            return false;
        }
    } else {
        // use remote file
        auto* const job = KIO::storedPut(textUtf8, url, -1, KIO::Overwrite);
        KJobWidgets::setWindow(job, nullptr); // hide progress notification
        if (!job->exec()) {
            setException(new KCalCore::Exception(KCalCore::Exception::SaveErrorSaveFile, QStringList(url.url())));
            qCWarning(KTT_LOG) << "save remote error: " << job->errorString();
            return false;
        }
    }

    return true;
}
