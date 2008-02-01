/*
 *     Copyright (C) 1997 by Stephan Kulow <coolo@kde.org>
 *                   2007 the ktimetracker developers
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
#include <QFile>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KDebug>
#include <KLocale>
#include <KStandardDirs>
#include <KUniqueApplication>

#include "version.h"
#include "mainwindow.h"
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

int main( int argc, char *argv[] )
{
  KAboutData aboutData( "ktimetracker", 0, ki18n("KTimeTracker"),
      KTIMETRACKER_VERSION, ki18n(description), KAboutData::License_GPL,
      ki18n("(c) 1997-2007, KDE PIM Developers") );

  aboutData.addAuthor( ki18n("Thorsten St&auml;rk"), ki18n( "Current Maintainer" ),
                       "kde@staerk.de" );
  aboutData.addAuthor( ki18n("Sirtaj Singh Kang"), ki18n( "Original Author" ),
                       "taj@kde.org" );
  aboutData.addAuthor( ki18n("Allen Winter"),      KLocalizedString(), "winterz@verizon.net" );
  aboutData.addAuthor( ki18n("David Faure"),       KLocalizedString(), "faure@kde.org" );
  aboutData.addAuthor( ki18n("Mathias Soeken"),    KLocalizedString(), "msoeken@tzi.de" );
  aboutData.addAuthor( ki18n("Jesper Pedersen"),   KLocalizedString(), "blackie@kde.org" );
  aboutData.addAuthor( ki18n("Kalle Dalheimer"),   KLocalizedString(), "kalle@kde.org" );
  aboutData.addAuthor( ki18n("Mark Bucciarelli"),  KLocalizedString(), "mark@hubcapconsulting.com" );

  KCmdLineArgs::init( argc, argv, &aboutData );

  KCmdLineOptions options;
  options.add("+file", ki18n( "The iCalendar file to open" ));
  KCmdLineArgs::addCmdLineOptions( options );
  KUniqueApplication myApp;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  MainWindow *mainWindow;
  if ( args->count() > 0 )
  {
    QString icsfile = args->arg( 0 );

    KUrl* icsfileurl=new KUrl(args->arg( 0 ));
    if (( icsfileurl->protocol() == "http" ) || ( icsfileurl->protocol() == "ftp" ) || ( icsfileurl->isLocalFile() ))
    {
      // leave as is
      ;
    }
    else
    {
      icsfile = KCmdLineArgs::cwd() + '/' + icsfile;
    }

    mainWindow = new MainWindow( icsfile );
  }
  else
  {
    KStandardDirs tmp;
    tmp.localkdedir();
    QString newKarmFile(tmp.localkdedir() + "/share/apps/ktimetracker/karm.ics" );
    if ( !QFile::exists( newKarmFile ) )
    {
      QFile oldFile( tmp.localkdedir() + "/share/apps/karm/karm.ics" );
      if ( oldFile.exists() )
        oldFile.copy( newKarmFile );
      else
        newKarmFile = KStandardDirs::locateLocal( "appdata", QString::fromLatin1( "karm.ics" ) );
    }
    mainWindow = new MainWindow( newKarmFile );
  }

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
