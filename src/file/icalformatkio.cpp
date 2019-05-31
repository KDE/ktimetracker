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

#include "icalformatkio.h"

#include <QSaveFile>

#include <KIO/StoredTransferJob>
#include <KCalCore/Exceptions>
#include <KJobWidgets>

#include "ktt_debug.h"

ICalFormatKIO::ICalFormatKIO()
    : KCalCore::ICalFormat()
{
}

bool ICalFormatKIO::load(const KCalCore::Calendar::Ptr &calendar, const QString &fileName)
{
    qCDebug(KTT_LOG) << fileName;

    clearException();

    QUrl url(fileName);
    if (url.isLocalFile()) {
        QFile file(url.toLocalFile());
        if (!file.exists()) {
            // Local file does not exist
            return true;
        }

        // Local file exists
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

bool ICalFormatKIO::save(const KCalCore::Calendar::Ptr &calendar, const QString &fileName)
{
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
        QSaveFile file(url.toLocalFile());
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
