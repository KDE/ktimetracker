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
#include <KCmdLineArgs>
#include <KDebug>
#include <KLocale>
#include <KStandardDirs>
#include <kontactinterface/pimuniqueapplication.h>

#include "version.h"
#include "mainwindow.h"
#include "mainadaptor.h"
#include "timetrackerstorage.h"
#include "task.h"
#include <QDebug>

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
        KUrl* icsfileurl=new KUrl(args->arg( 0 ));
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
    KAboutData aboutData( "ktimetracker", 0, ki18n("KTimeTracker"),
        KTIMETRACKER_VERSION, ki18n(description), KAboutData::License_GPL,
        ki18n("(c) 1997-2009, KDE PIM Developers") );

    aboutData.addAuthor( ki18n("Thorsten St&auml;rk"), ki18n( "Current Maintainer" ),
                       "kde@staerk.de" );
    aboutData.addAuthor( ki18n("Sirtaj Singh Kang"), ki18n( "Original Author" ),
                       "taj@kde.org" );
    aboutData.addAuthor( ki18n("Allen Winter"),      KLocalizedString(), "winter@kde.org" );
    aboutData.addAuthor( ki18n("David Faure"),       KLocalizedString(), "faure@kde.org" );
    aboutData.addAuthor( ki18n("Mathias Soeken"),    KLocalizedString(), "msoeken@tzi.de" );
    aboutData.addAuthor( ki18n("Jesper Pedersen"),   KLocalizedString(), "blackie@kde.org" );
    aboutData.addAuthor( ki18n("Kalle Dalheimer"),   KLocalizedString(), "kalle@kde.org" );
    aboutData.addAuthor( ki18n("Mark Bucciarelli"),  KLocalizedString(), "mark@hubcapconsulting.com" );
    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
    options.add("+file", ki18n( "The iCalendar file to open" ));
    options.add("listtasknames", ki18n( "List all tasks as text output" ));
    options.add("addtask <taskname>", ki18n( "Add task <taskname>" ));
    options.add("deletetask <taskid>", ki18n( "Delete task <taskid>" ));
    options.add("taskidsfromname <taskname>", ki18n( "Print the task ids for all tasks named <taskname>" ));
    options.add("starttask <taskid>", ki18n( "Start timer for task <taskid>" ));
    options.add("stoptask <taskid>", ki18n( "Stop timer for task <taskid>" ));
    options.add("totalminutesfortaskid <taskid>", ki18n( "Deliver total minutes for task id" ));
    options.add("version", ki18n( "Outputs the version" ));
    KCmdLineArgs::addCmdLineOptions( options );
    KUniqueApplication::addCmdLineOptions();
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    int err=0;  // error code
    bool konsolemode=false;  // open a gui and wait for user input?
    if ( args->isSet("listtasknames") ) konsolemode=true;
    if ( !args->getOption("addtask").isEmpty() ) konsolemode=true;
    if ( !args->getOption("deletetask").isEmpty() ) konsolemode=true;
    if ( !args->getOption("taskidsfromname").isEmpty() ) konsolemode=true;
    if ( !args->getOption("totalminutesfortaskid").isEmpty() ) konsolemode=true;
    if ( !args->getOption("starttask").isEmpty() ) konsolemode=true;
    if ( !args->getOption("stoptask").isEmpty() ) konsolemode=true;

    if ( !konsolemode )
    {  // no konsole mode
        KontactInterface::PimUniqueApplication myApp;
        MainWindow *mainWindow;
        mainWindow = new MainWindow( icsfile( args ) );
       // if (kapp->isSessionRestored() && KMainWindow::canBeRestored( 1 ))
         //   mainWindow->restore( 1, false );
        //else
        mainWindow->show();

        signal( SIGQUIT, cleanup );
        signal( SIGINT, cleanup );
        args->clear();
        int ret = myApp.exec();

        delete mainWindow;
        return ret;
    }
    else // we are running in konsole mode
    {
        kDebug(5970) << "We are running in konsole mode";
        KCmdLineArgs::addCmdLineOptions( options );
        KApplication myApp(false);
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
        // listtasknames
        if ( args->isSet("listtasknames") )
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
        if ( !args->getOption("addtask").isEmpty() )
        {
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
            const QString& s=args->getOption("addtask");
            QVector<int> vec;
            vec.push_back(0);
            DesktopList dl=vec;
            Task* task=new Task( s,(long int) 0,(long int) 0, dl, 0, true );
            sto->addTask( task );
            sto->save( 0 );
            delete sto;
        }
        // deletetask
        if ( !args->getOption("deletetask").isEmpty() )
        {
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
            const QString& taskid=args->getOption("deletetask");
            sto->removeTask( taskid );
            delete sto;
        }
        // taskidsfromname
        if ( !args->getOption("taskidsfromname").isEmpty() )
        {
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
            const QString& taskname=args->getOption("taskidsfromname");
            QStringList taskids=sto->taskidsfromname( taskname );
            for ( int i=0; i<taskids.count(); ++i )
            {
                char* line = taskids[i].toLatin1().data();
                std::cout << line << std::endl;
            }
            delete sto;
        }
        // totalminutesfortaskid
        if ( !args->getOption("totalminutesfortaskid").isEmpty() )
        {
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
            Task* task=sto->task( args->getOption("totalminutesfortaskid"), 0 );
            if (task!=0)
            {
                kDebug(5970) << "taskname=" << task->name();
                std::cout << task->totalTime();
            }
            delete sto;
        }
        // starttask
        if ( !args->getOption("starttask").isEmpty() )
        {
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
            sto->startTimer(args->getOption("starttask"));
            delete sto;
        }
        // stoptask
        if ( !args->getOption("stoptask").isEmpty() )
        {
            timetrackerstorage* sto=new timetrackerstorage();
            sto->load( 0, icsfile( args ) );
            sto->stopTimer(sto->task( args->getOption("stoptask"), 0 ));
            delete sto;
        }
        args->clear();
    }
    return err;
}
