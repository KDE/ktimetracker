#include <kapp.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include "version.h"
#include "top.h"


static const char *description = 
	I18N_NOOP("KDE Time tracker tool.");


int main( int argc, char *argv[] )
{
	KAboutData aboutData( "karm", I18N_NOOP("KArm"),
		KARM_VERSION, description, KAboutData::License_GPL,
		"(c) 1997-2000, Sirtaj Singh Kang, Espen Sand, Jesper Pedersen,\nKalle Dalheimer, Klar�lvdalens Datakonsult AB");
	aboutData.addAuthor( "Sirtaj Singh Kang", 0, "taj@kde.org" );
	aboutData.addAuthor("Espen Sand",0, "espen@kde.org");
	aboutData.addAuthor( "Jesper Pedersen", 0, "blackie@kde.org" );
	aboutData.addAuthor( "Kalle Dalheimer", 0, "kalle@kde.org" );
	
	KCmdLineArgs::init( argc, argv, &aboutData );
	KApplication myApp;

	KarmWindow *karm = new KarmWindow;

	myApp.setMainWidget( karm );
	karm->show();
	
	int ret = myApp.exec();

	return ret;
}
