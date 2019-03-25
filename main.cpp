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

#include <iostream>

#include <QFile>
#include <QDebug>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStandardPaths>

#include <KAboutData>
#include <KLocalizedString>

#include <kontactinterface/pimuniqueapplication.h>

#include "desktoplist.h"
#include "mainwindow.h"
#include "mainadaptor.h"
#include "timetrackerstorage.h"
#include "task.h"
#include "ktt_debug.h"
#include "ktimetracker-version.h"

QString icsfile(const QCommandLineParser &parser) // deliver the name of the iCalendar file to be used
{
    // Get first positional argument ("file")
    const QStringList args = parser.positionalArguments();
    QString file;
    if (args.size() > 0) {
        file = args[0];
    }

    QString result;
    if (!file.isEmpty()) // file is given as parameter
    {
        result = file;
        const QUrl& icsfileurl = QUrl(file);
        if (icsfileurl.scheme() == "http" || icsfileurl.scheme() == "ftp" || icsfileurl.isLocalFile())
        {
            // leave as is
            ;
        }
        else
        {
            QFileInfo info(result);
            result = info.absoluteFilePath();
        }
    }
    else // file is not given as parameter
    {
        result = QString(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "ktimetracker/ktimetracker.ics"));
        if (!QFile::exists(result))
        {
            QFile oldFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "karm/karm.ics"));
            result = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + QStringLiteral("ktimetracker.ics");
            if (oldFile.exists()) {
                oldFile.copy(result);
            }
        }
    }
    return result;
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

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    //PORTING SCRIPT: adapt aboutdata variable if necessary
    aboutData.setupCommandLine(&parser);

    parser.addPositionalArgument(QStringLiteral("file"), i18n("The iCalendar file to open"));
    parser.addOption(QCommandLineOption(
        QStringList() << QStringLiteral("listtasknames"), i18n("List all tasks as text output")));
    parser.addOption(QCommandLineOption(
        QStringList() << QStringLiteral("addtask"), i18n("Add task <taskname>"),
        QStringLiteral("taskname")));
    parser.addOption(QCommandLineOption(
        QStringList() << QStringLiteral("deletetask"), i18n("Delete task <taskid>"),
        QStringLiteral("taskid")));
    parser.addOption(QCommandLineOption(
        QStringList() << QStringLiteral("taskidsfromname"), i18n("Print the task ids for all tasks named <taskname>"),
        QStringLiteral("taskname")));
    parser.addOption(QCommandLineOption(
        QStringList() << QStringLiteral("starttask"), i18n("Start timer for task <taskid>"),
        QStringLiteral("taskid")));
    parser.addOption(QCommandLineOption(
        QStringList() << QStringLiteral("stoptask"), i18n("Stop timer for task <taskid>"),
        QStringLiteral("taskid")));
    parser.addOption(QCommandLineOption(
        QStringList() << QStringLiteral("totalminutesfortaskid"), i18n("Deliver total minutes for task id"),
        QStringLiteral("taskid")));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    int err=0;  // error code
    bool konsolemode=false;  // open a gui and wait for user input?
    if ( parser.isSet("listtasknames") ) konsolemode=true;
    if ( !parser.value("addtask").isEmpty() ) konsolemode=true;
    if ( !parser.value("deletetask").isEmpty() ) konsolemode=true;
    if ( !parser.value("taskidsfromname").isEmpty() ) konsolemode=true;
    if ( !parser.value("totalminutesfortaskid").isEmpty() ) konsolemode=true;
    if ( !parser.value("starttask").isEmpty() ) konsolemode=true;
    if ( !parser.value("stoptask").isEmpty() ) konsolemode=true;

    if ( !konsolemode )
    {  // no konsole mode
//        if (!KUniqueApplication::start()) {
//            qCDebug(KTT_LOG) << "Other instance is already running, exiting!";
//            return 0;
//        }
//        KUniqueApplication myApp;
        MainWindow *mainWindow;
        mainWindow = new MainWindow( icsfile( parser ) );
//        if (kapp->isSessionRestored() && KMainWindow::canBeRestored( 1 ))
//            mainWindow->restore( 1, false );
//        else
        mainWindow->show();

        int ret = app.exec();

        delete mainWindow;
        return ret;
    }
    else // we are running in konsole mode
    {
        qCDebug(KTT_LOG) << "We are running in konsole mode";
//        KApplication myApp(false);
        // listtasknames
        if ( parser.isSet("listtasknames") )
        {
            auto* sto=new TimeTrackerStorage();
            sto->load( 0, icsfile( parser ) );
            QStringList tasknameslist=sto->taskNames();
            for ( int i=0; i<tasknameslist.count(); ++i )
            {
                char* line = tasknameslist[i].toLatin1().data();
                std::cout << line << std::endl;
            }
        delete sto;  // make valgrind happy
        }
        // addtask
        if ( !parser.value("addtask").isEmpty() )
        {
            auto* sto = new TimeTrackerStorage();
            sto->load( 0, icsfile( parser ) );
            const QString& s=parser.value("addtask");
            QVector<int> vec;
            DesktopList dl=vec;
            Task* task=new Task( s, QString(), (long int) 0,(long int) 0, dl, 0, true );
            sto->addTask( task );
            sto->save( 0 );
            delete sto;
        }
        // deletetask
        if ( !parser.value("deletetask").isEmpty() )
        {
            auto* sto = new TimeTrackerStorage();
            sto->load( 0, icsfile( parser ) );
            const QString& taskid=parser.value("deletetask");
            sto->removeTask( taskid );
            delete sto;
        }
        // taskidsfromname
        if ( !parser.value("taskidsfromname").isEmpty() )
        {
            auto* sto = new TimeTrackerStorage();
            sto->load( 0, icsfile( parser ) );
            const QString& taskname=parser.value("taskidsfromname");
            QStringList taskids=sto->taskidsfromname( taskname );
            for ( int i=0; i<taskids.count(); ++i )
            {
                char* line = taskids[i].toLatin1().data();
                std::cout << line << std::endl;
            }
            delete sto;
        }
        // totalminutesfortaskid
        if ( !parser.value("totalminutesfortaskid").isEmpty() )
        {
            auto* sto = new TimeTrackerStorage();
            sto->load( 0, icsfile( parser ) );
            Task* task=sto->task( parser.value("totalminutesfortaskid"), 0 );
            if (task!=0)
            {
                qCDebug(KTT_LOG) << "taskname=" << task->name();
                std::cout << task->totalTime();
            }
            delete sto;
        }
        // starttask
        if ( !parser.value("starttask").isEmpty() )
        {
            auto* sto = new TimeTrackerStorage();
            sto->load( 0, icsfile( parser ) );
            sto->startTimer(parser.value("starttask"));
            delete sto;
        }
        // stoptask
        if ( !parser.value("stoptask").isEmpty() )
        {
            auto* sto = new TimeTrackerStorage();
            sto->load( 0, icsfile( parser ) );
            sto->stopTimer(sto->task( parser.value("stoptask"), 0 ));
            delete sto;
        }
        
    }
    return err;
}
