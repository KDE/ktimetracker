
#include <kapp.h>

#include "top.h"

int main( int argc, char *argv[] )
{
	KApplication *myApp = new KApplication( argc, argv );
	KarmWindow *karm = new KarmWindow;

	myApp->setMainWidget( karm );
	karm->show();
	
	int ret = myApp->exec();

	delete karm;
	delete myApp;

	return ret;
}
