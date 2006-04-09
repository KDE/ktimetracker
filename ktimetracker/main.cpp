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
  KAboutData aboutData( "karm", I18N_NOOP("KArm"),
      KARM_VERSION, description, KAboutData::License_GPL,
      "(c) 1997-2006, KDE PIM Developers" );

  aboutData.addAuthor( "Thorsten Staerk", I18N_NOOP( "Current Maintainer" ),
                       "kde@staerk.de" );
  aboutData.addAuthor( "Sirtaj Singh Kang", I18N_NOOP( "Original Author" ),
                       "taj@kde.org" );
  aboutData.addAuthor( "Allen Winter",      0, "winterz@verizon.net" );
  aboutData.addAuthor( "David Faure",       0, "faure@kde.org" );
  aboutData.addAuthor( "Espen Sand",        0, "espen@kde.org" );
  aboutData.addAuthor( "Gioele Barabucci",  0, "gioele@gioelebarabucci.com" );
  aboutData.addAuthor( "Jan Schaumann",     0, "jschauma@netmeister.org" );
  aboutData.addAuthor( "Jesper Pedersen",   0, "blackie@kde.org" );
  aboutData.addAuthor( "Kalle Dalheimer",   0, "kalle@kde.org" );
  aboutData.addAuthor( "Mark Bucciarelli",  0, "mark@hubcapconsulting.com" );
  aboutData.addAuthor( "Scott Monachello",  0, "smonach@cox.net" );
  aboutData.addAuthor( "Tomas Pospisek",    0, "tpo_deb@sourcepole.ch" );
  aboutData.addAuthor( "Willi Richert",     0, "w.richert@gmx.net" );

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
