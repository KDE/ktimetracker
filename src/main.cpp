/*
 * Copyright (C) 1997 by Stephan Kulow <coolo@kde.org>
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

#include <QFile>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStandardPaths>
#include <QApplication>
#include <QFileInfo>
#include <QDir>

#include <KAboutData>
#include <KLocalizedString>
#include <KDBusService>

#include "desktoplist.h"
#include "mainwindow.h"
#include "timetrackerstorage.h"
#include "model/task.h"
#include "ktt_debug.h"
#include "ktimetracker-version.h"

// Deliver the path/URL to the iCalendar file to be used
QUrl getFileUrl(const QCommandLineParser &parser)
{
    // Get first positional argument ("file")
    const QStringList args = parser.positionalArguments();
    QString file;
    if (!args.isEmpty()) {
        file = args[0];
    }

    if (!file.isEmpty()) {
        // file is given as parameter
        const QUrl& url = QUrl(file);
        if (url.scheme().isEmpty()) {
            // Relative path to local file will be converted to absolute path
            QFileInfo info(file);
            return QUrl::fromLocalFile(info.absoluteFilePath());
        } else {
            return url;
        }
    } else {
        // file is not given as parameter
        QString result = QString(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ktimetracker/ktimetracker.ics"));
        if (result.isEmpty()) {
            result = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + QStringLiteral("ktimetracker.ics");

            QFileInfo fileInfo(result);
            QDir().mkpath(fileInfo.absolutePath());

            QFile oldFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "karm/karm.ics"));
            if (oldFile.exists()) {
                oldFile.copy(result);
            }
        }

        return QUrl::fromLocalFile(result);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(ktimetracker);

    KAboutData aboutData(
        QStringLiteral("ktimetracker"),
        i18n("KTimeTracker"),
        QStringLiteral(KTIMETRACKER_VERSION_STRING),
        i18n("KDE Time tracker tool"),
        KAboutLicense::GPL_V2,
        i18n("Copyright © 1997-2019 KTimeTracker developers"),
        QString(),
        QStringLiteral("https://userbase.kde.org/KTimeTracker"));

    aboutData.addAuthor(i18nc("@info:credit", "Alexander Potashev"),
                        i18nc("@info:credit", "Current Maintainer (since 2019)"),
                        QStringLiteral("aspotashev@gmail.com"));
    aboutData.addAuthor(i18nc("@info:credit", "Thorsten Stärk"),
                        i18nc("@info:credit", "Maintainer (2006-2012)"),
                        QStringLiteral("kde@staerk.de"));
    aboutData.addAuthor(i18nc("@info:credit", "Mark Bucciarelli"),
                        i18nc("@info:credit", "Maintainer (2005-2006)"),
                        QStringLiteral("mark@hubcapconsulting.com"));
    aboutData.addAuthor(i18nc("@info:credit", "Jesper Pedersen"),
                        i18nc("@info:credit", "Maintainer (2000-2005)"),
                        QStringLiteral("blackie@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Sirtaj Singh Kang"),
                        i18nc("@info:credit", "Original Author"),
                        QStringLiteral("taj@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Mathias Soeken"),
                        i18nc("@info:credit", "Developer (in 2007)"),
                        QStringLiteral("msoeken@tzi.de"));
    aboutData.addAuthor(i18nc("@info:credit", "Kalle Dalheimer"),
                        i18nc("@info:credit", "Developer (1999-2000)"),
                        QStringLiteral("kalle@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "Allen Winter"),
                        i18nc("@info:credit", "Developer"),
                        QStringLiteral("winter@kde.org"));
    aboutData.addAuthor(i18nc("@info:credit", "David Faure"),
                        i18nc("@info:credit", "Developer"),
                        QStringLiteral("faure@kde.org"));
    KAboutData::setApplicationData(aboutData);

    KDBusService dbusService(KDBusService::Unique);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    //PORTING SCRIPT: adapt aboutdata variable if necessary
    aboutData.setupCommandLine(&parser);

    parser.addPositionalArgument(QStringLiteral("file"), i18n("The iCalendar file to open"));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    const QString& url = getFileUrl(parser).url();

//        if (!KUniqueApplication::start()) {
//            qCDebug(KTT_LOG) << "Other instance is already running, exiting!";
//            return 0;
//        }
//        KUniqueApplication myApp;
    MainWindow *mainWindow;
    mainWindow = new MainWindow(url);
    mainWindow->show();

    if (app.isSessionRestored()) {
        const QString className = KXmlGuiWindow::classNameOfToplevel(1);
        if (className == QLatin1String("MainWindow")) {
            mainWindow->restore(1);
        } else {
            qCWarning(KTT_LOG) << "Unknown class " << className << " in session saved data!";
        }
    }

    return app.exec();
}
