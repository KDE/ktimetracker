#include <kapp.h>
#include <klocale.h>
#include <kcmdlineargs.h>

#include "top.h"


static const char *description = 
	I18N_NOOP("KDE Time tracker tool.");

static const char *version = "v0.0.1";


int main( int argc, char *argv[] )
{
	KCmdLineArgs::init(argc, argv, "karm", description, version);
	KApplication myApp;

	KarmWindow *karm = new KarmWindow;

	myApp.setMainWidget( karm );
	karm->show();
	
	int ret = myApp.exec();

	return ret;
}
