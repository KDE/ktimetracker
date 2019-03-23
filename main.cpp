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

#include "desktoplist.h"
#include <iostream>
#include <signal.h>
#include <QFile>
#include <KAboutData>

#include <kontactinterface/pimuniqueapplication.h>

#include "mainwindow.h"
#include "mainadaptor.h"
#include "timetrackerstorage.h"
#include "task.h"
#include <QDebug>
#include <QCommandLineParser>
#include <QCommandLineOption>

namespace
{
    const char* description = I18N_NOOP("KDE Time tracker tool");

    void cleanup( int )
    {
        kDebug(5970) << i18n("Just caught a software interrupt.");
        kapp->exit();
    }
}

QString icsfile( const KCmdLineArgs* args ) // deliver the name of the iCalendar file to be used
{
    QString result;
    if ( args->count() > 0 ) // file is given as parameter
    {
        result = args->arg( 0 );
        QUrl* icsfileurl=new QUrl(args->arg( 0 ));
        if (( icsfileurl->protocol() == "http" ) || ( icsfileurl->protocol() == "ftp" ) || ( icsfileurl->isLocalFile() ))
        {
            // leave as is
            ;
        }
        else
        {
            result = KCmdLineArgs::cwd() + '/' + result;
        }
        delete icsfileurl;
    }
    else // file is not given as parameter
    {
        result=QString(KStandardDirs::locate( "data", "ktimetracker/ktimetracker.ics" ));
        if ( !QFile::exists( result ) )
        {
            QFile oldFile( KStandardDirs::locate( "data", "karm/karm.ics" ) );
            result = KStandardDirs::locateLocal( "appdata", QString::fromLatin1( "ktimetracker.ics" ) );
            if ( oldFile.exists() )
                oldFile.copy( result );
        }
    }
    return result;
}

int main( int argc, char *argv[] )
{
    QApplication app(argc, argv);

    KAboutData aboutData( "ktimetracker", 0, ki18n("KTimeTracker"),
        KDEPIM_VERSION, ki18n(description), KAboutLicense::GPL,
        ki18n("Copyright © 1997-2012 KDE PIM authors"), KLocalizedString(),
        QByteArray("http://userbase.kde.org/KTimeTracker") );

    aboutData.addAuthor( ki18n("Thorsten Stärk"), ki18n( "Current Maintainer" ),
                       "kde@staerk.de" );
    aboutData.addAuthor( ki18n("Sirtaj Singh Kang"), ki18n( "Original Author" ),
                       "taj@kde.org" );
    aboutData.addAuthor( ki18n("Allen Winter"),      KLocalizedString(), "winter@kde.org" );
    aboutData.addAuthor( ki18n("David Faure"),       KLocalizedString(), "faure@kde.org" );
    aboutData.addAuthor( ki18n("Mathias Soeken"),    KLocalizedString(), "msoeken@tzi.de" );
    aboutData.addAuthor( ki18n("Jesper Pedersen"),   KLocalizedString(), "blackie@kde.org" );
    aboutData.addAuthor( ki18n("Kalle Dalheimer"),   KLocalizedString(), "kalle@kde.org" );
    aboutData.addAuthor( ki18n("Mark Bucciarelli"),  KLocalizedString(), "mark@hubcapconsulting.com" );
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    //PORTING SCRIPT: adapt aboutdata variable if necessary
    aboutData.setupCommandLine(&parser);
    parser.process(app); // PORTING SCRIPT: move this to after any parser.addOption
    aboutData.processCommandLine(&parser);

    parser.addPositionalArgument(QLatin1String("file"), i18n( "The iCalendar file to open" ));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("listtasknames"), i18n( "List all tasks as text output" )));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("addtask"), i18n( "Add task <taskname>" ), QLatin1String("taskname")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("deletetask"), i18n( "Delete task <taskid>" ), QLatin1String("taskid")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("taskidsfromname"), i18n( "Print the task ids for all tasks named <taskname>" ), QLatin1String("taskname")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("starttask"), i18n( "Start timer for task <taskid>" ), QLatin1String("taskid")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("stoptask"), i18n( "Stop timer for task <taskid>" ), QLatin1String("taskid")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("totalminutesfortaskid"), i18n( "Deliver total minutes for task id" ), QLatin1String("taskid")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("version"), i18n( "Outputs the version" )));
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
        if (!KUniqueApplication::start()) {
            kDebug(5970) << "Other instance is already running, exiting!";
            return 0;
        }
        KUniqueApplication myApp;
        MainWindow *mainWindow;
        mainWindow = new MainWindow( icsfile( args ) );
        if (kapp->isSessionRestored() && KMainWindow::canBeRestored( 1 ))
            mainWindow->restore( 1, false );
        else
        mainWindow->show();

        signal( SIGQUIT, cleanup );
        signal( SIGINT, cleanup );
        
        int ret = myApp.exec();

        delete mainWindow;
        return ret;
    }
    else // we are running in konsole mode
    {
        kDebug(5970) << "We are running in konsole mode";
        KApplication myApp(false);
        // listtasknames
        if ( parser.isSet("listtasknames") )
        {
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
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
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
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
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
            const QString& taskid=parser.value("deletetask");
            sto->removeTask( taskid );
            delete sto;
        }
        // taskidsfromname
        if ( !parser.value("taskidsfromname").isEmpty() )
        {
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
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
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
            Task* task=sto->task( parser.value("totalminutesfortaskid"), 0 );
            if (task!=0)
            {
                kDebug(5970) << "taskname=" << task->name();
                std::cout << task->totalTime();
            }
            delete sto;
        }
        // starttask
        if ( !parser.value("starttask").isEmpty() )
        {
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
            sto->startTimer(parser.value("starttask"));
            delete sto;
        }
        // stoptask
        if ( !parser.value("stoptask").isEmpty() )
        {
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
            sto->stopTimer(sto->task( parser.value("stoptask"), 0 ));
            delete sto;
        }
        
    }
    return err;
}
