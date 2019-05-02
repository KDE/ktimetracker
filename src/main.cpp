/*
 *     Copyright (C) 1997 by Stephan Kulow <coolo@kde.org>
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

#include <KAboutData>
#include <KLocalizedString>
#include <KDBusService>

#include "desktoplist.h"
#include "mainwindow.h"
#include "mainadaptor.h"
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

    KAboutData aboutData(
        QStringLiteral("ktimetracker"),
        i18n("KTimeTracker"),
        QStringLiteral(KTIMETRACKER_VERSION),
        i18n("KDE Time tracker tool"),
        KAboutLicense::GPL,
        i18n("Copyright © 1997-2012 KDE PIM authors"),
        QString(),
        QStringLiteral("http://userbase.kde.org/KTimeTracker"));

    aboutData.addAuthor(i18n("Thorsten Stärk"), i18n("Current Maintainer"), QStringLiteral("kde@staerk.de"));
    aboutData.addAuthor(i18n("Sirtaj Singh Kang"), i18n("Original Author"), QStringLiteral("taj@kde.org"));
    aboutData.addAuthor(i18n("Allen Winter"),      QString(), QStringLiteral("winter@kde.org"));
    aboutData.addAuthor(i18n("David Faure"),       QString(), QStringLiteral("faure@kde.org"));
    aboutData.addAuthor(i18n("Mathias Soeken"),    QString(), QStringLiteral("msoeken@tzi.de"));
    aboutData.addAuthor(i18n("Jesper Pedersen"),   QString(), QStringLiteral("blackie@kde.org"));
    aboutData.addAuthor(i18n("Kalle Dalheimer"),   QString(), QStringLiteral("kalle@kde.org"));
    aboutData.addAuthor(i18n("Mark Bucciarelli"),  QString(), QStringLiteral("mark@hubcapconsulting.com"));
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
