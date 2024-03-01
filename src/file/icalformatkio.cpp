/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "icalformatkio.h"

#include <QSaveFile>

#include <KCalendarCore/Exceptions>

#include <KIO/StoredTransferJob>
#include <KJobWidgets>
#include "ktt_debug.h"

bool ICalFormatKIO::load(const KCalendarCore::Calendar::Ptr &calendar, const QString &urlString)
{
    qCDebug(KTT_LOG) << "ICalFormatKIO::load:" << urlString;

    clearException();

    QUrl url(urlString);
    if (url.isLocalFile()) {
        QFile file(url.toLocalFile());
        if (!file.exists()) {
            // Local file does not exist
            return true;
        }

        // Local file exists
        if (!file.open(QIODevice::ReadOnly)) {
            qCWarning(KTT_LOG) << "load file open error: " << file.errorString() << ";filename=" << urlString;
            setException(new KCalendarCore::Exception(KCalendarCore::Exception::LoadError));
            return false;
        }
        const QByteArray text = file.readAll().trimmed();
        file.close();

        if (text.isEmpty()) {
            // empty files are valid
            return true;
        } else {
            return fromRawString(calendar, text);
        }
    } else {
        // use remote file
        auto *const job = KIO::storedGet(url, KIO::Reload);
        KJobWidgets::setWindow(job, nullptr); // hide progress notification
        if (!job->exec()) {
            setException(new KCalendarCore::Exception(KCalendarCore::Exception::SaveErrorSaveFile, QStringList(url.url())));
            return false;
        }

        const QByteArray text = job->data();
        if (text.isEmpty()) {
            // empty files are valid
            return true;
        } else {
            return fromRawString(calendar, text);
        }
    }
}

bool ICalFormatKIO::save(const KCalendarCore::Calendar::Ptr &calendar, const QString &urlString)
{
    qCDebug(KTT_LOG) << "ICalFormatKIO::save:" << urlString;

    clearException();

    QString text = toString(calendar);
    if (text.isEmpty()) {
        // TODO: save empty file if we have 0 tasks?
        return false;
    }

    QByteArray textUtf8 = text.toUtf8();

    // TODO: Write backup file (i.e. backup the existing file somewhere, e.g. to ~/.local/share/apps/ktimetracker/backups/)
//    const QString backupFile = urlString + QLatin1Char('~');
//    QFile::remove(backupFile);
//    QFile::copy(urlString, backupFile);

    // save, either locally or remote
    QUrl url(urlString);
    if (url.isLocalFile()) {
        QSaveFile file(url.toLocalFile());
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(KTT_LOG) << "save file open error: " << file.errorString() << ";local path" << file.fileName();
            setException(new KCalendarCore::Exception(KCalendarCore::Exception::SaveErrorOpenFile, QStringList(urlString)));
            return false;
        }

        file.write(textUtf8.data(), textUtf8.size());

        if (!file.commit()) {
            qCWarning(KTT_LOG) << "file finalize error:" << file.errorString() << ";local path" << file.fileName();
            setException(new KCalendarCore::Exception(KCalendarCore::Exception::SaveErrorSaveFile, QStringList(urlString)));
            return false;
        }
    } else {
        // use remote file
        auto *const job = KIO::storedPut(textUtf8, url, -1, KIO::Overwrite);
        KJobWidgets::setWindow(job, nullptr); // hide progress notification
        if (!job->exec()) {
            setException(new KCalendarCore::Exception(KCalendarCore::Exception::SaveErrorSaveFile, QStringList(url.url())));
            qCWarning(KTT_LOG) << "save remote error: " << job->errorString();
            return false;
        }
    }

    return true;
}
