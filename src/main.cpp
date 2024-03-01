/*
    SPDX-FileCopyrightText: 1997 Stephan Kulow <coolo@kde.org>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QStandardPaths>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>

#include "desktoplist.h"
#include "ktimetracker-version.h"
#include "ktt_debug.h"
#include "mainwindow.h"
#include "model/task.h"
#include "timetrackerstorage.h"

// Deliver the path/URL to the iCalendar file to be used
QUrl getFileUrl(const QCommandLineParser &parser)
{
    // Get first positional argument ("file")
    const QStringList args = parser.positionalArguments();
    QString customFile;
    if (!args.isEmpty()) {
        customFile = args[0];
    }

    if (!customFile.isEmpty()) {
        // customFile is given as parameter

        const QUrl url = QUrl::fromUserInput(customFile, QDir::currentPath(), QUrl::AssumeLocalFile);
        if (!url.isValid()) {
            qCWarning(KTT_LOG) << "Invalid URL: " << customFile;
        }

        return url;
    } else {
        // customFile is not given as parameter
        QString result =
            QString(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("ktimetracker/ktimetracker.ics")));
        if (result.isEmpty()) {
            result = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QLatin1Char('/')
                + QStringLiteral("ktimetracker.ics");

            QFileInfo fileInfo(result);
            QDir().mkpath(fileInfo.absolutePath());

            QFile oldFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("karm/karm.ics")));
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

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    // Force Breeze theme on Windows.
    // Kate text editor does the same.
    QApplication::setStyle(QStringLiteral("breeze"));
#endif

    KLocalizedString::setApplicationDomain("ktimetracker");

    KAboutData aboutData(QStringLiteral("ktimetracker"),
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

    Q_INIT_RESOURCE(icons);
    QIcon::setThemeSearchPaths(QStringList() << QStringLiteral(":/icons"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("ktimetracker")));

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    parser.addPositionalArgument(QStringLiteral("url"), i18nc("@info:shell", "Path or URL to iCalendar file to open."));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService dbusService(KDBusService::Unique);

    const QUrl &url = getFileUrl(parser);

//        if (!KUniqueApplication::start()) {
//            qCDebug(KTT_LOG) << "Other instance is already running, exiting!";
//            return 0;
//        }
//        KUniqueApplication myApp;
    QPointer<MainWindow> mainWindow = new MainWindow(url);
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
