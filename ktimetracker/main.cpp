/*
 *     Copyright (C) 2007 the ktimetracker developers
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

#include <signal.h>
#include <kapplication.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include "version.h"
#include "mainwindow.h"


namespace
{
  const char* description = I18N_NOOP("KDE Time tracker tool");

  void cleanup( int )
  {
    kDebug(5970) << i18n("Just caught a software interrupt.") << endl;
    kapp->exit();
  }
}

static const KCmdLineOptions options[] =
{
  { "+file", I18N_NOOP( "The iCalendar file to open" ), 0 },
  KCmdLineLastOption
};

int main( int argc, char *argv[] )
{
  KAboutData aboutData( "karm", I18N_NOOP("KTimeTracker"),
      KARM_VERSION, description, KAboutData::License_GPL,
      "(c) 1997-2007, KDE PIM Developers" );

  aboutData.addAuthor( "Thorsten St&auml;rk", I18N_NOOP( "Current Maintainer" ),
                       "kde@staerk.de" );
  aboutData.addAuthor( "Sirtaj Singh Kang", I18N_NOOP( "Original Author" ),
                       "taj@kde.org" );
  aboutData.addAuthor( "Allen Winter",      0, "winterz@verizon.net" );
  aboutData.addAuthor( "David Faure",       0, "faure@kde.org" );
  aboutData.addAuthor( "Mathias Soeken",    0, "msoeken@tzi.de" );
  aboutData.addAuthor( "Jesper Pedersen",   0, "blackie@kde.org" );
  aboutData.addAuthor( "Kalle Dalheimer",   0, "kalle@kde.org" );
  aboutData.addAuthor( "Mark Bucciarelli",  0, "mark@hubcapconsulting.com" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication myApp;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  MainWindow *mainWindow;
  if ( args->count() > 0 ) 
  {
    QString icsfile = QString::fromLocal8Bit( args->arg( 0 ) );
    
    KUrl* icsfileurl=new KUrl(QString::fromLocal8Bit( args->arg( 0 ) ));
    if (( icsfileurl->protocol() == "http" ) || ( icsfileurl->protocol() == "ftp" ) || ( icsfileurl->isLocalFile() ))
    {
      // leave as is
      ;
    }
    else
    {
      icsfile = KCmdLineArgs::cwd() + "/" + icsfile;
    }
    mainWindow = new MainWindow( icsfile );
  }
  else
  {
    mainWindow = new MainWindow();
  }

  myApp.setMainWidget( mainWindow );

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
