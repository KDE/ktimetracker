#include <kapp.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

#include "top.h"


static const char *description = 
	I18N_NOOP("KDE Time tracker tool.");

static const char *version = "v0.0.1";


int main( int argc, char *argv[] )
{
	KAboutData aboutData( "karm", I18N_NOOP("KArm"),
		version, description, KAboutData::License_GPL,
		"(c) 1999, Espen Sand");
	aboutData.addAuthor("Espen Sand",0, "espen@kde.org");
	
	KCmdLineArgs::init( argc, argv, &aboutData );
	KApplication myApp;

	KarmWindow *karm = new KarmWindow;

	myApp.setMainWidget( karm );
	karm->show();
	
	int ret = myApp.exec();

	return ret;
}
