
/*
* Top Level window for KArm -- Sirtaj Singh Kang <taj@kde.org>
* Distributed under the GPL.
*
* $Id$
*/

#include <qpopupmenu.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qkeycode.h>

#include <kiconloader.h>
#include <kapp.h>
#include <klocale.h>
#include <kglobal.h>
#include <ktopwidget.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <kaccel.h>
#include <kkeydialog.h>

#include "karm.h"
#include "toolicons.h"
#include "top.h"
#include "version.h"

#include"kaccelmenuwatch.h"

KarmWindow::KarmWindow()
	:	KTopLevelWidget(),

		_mainMenu( menuBar() ),
		_toolBar( toolBar() ),
		_statusBar( statusBar() ),
		_accel( new KAccel( this ) ),
		_watcher( new KAccelMenuWatch( _accel, this ) ),

		_karm( new Karm( this ) ),
		
		_totalTime( 0 ),
		_sessionTimeBuffer( new char[100] )

{
	setMenu( _mainMenu );

	setView( _karm, FALSE );
	_karm->show();

	// accelerators
	initAccelItems();

	// status bar
	
	_statusBar->insertItem( i18n( "clock inactive" ), 0 );
	_statusBar->insertItem( i18n( "This session:" ), 1 );
	_statusBar->insertItem( "0:00", 2 );

	// popup menus
	makeMenus();
	connectAccels();
	_watcher->updateMenus();

	// toolbar

	KIconLoader *loader = KGlobal::iconLoader();
	QPixmap icon;

	icon.loadFromData(clock_xpm_data, clock_xpm_len );
	_toolBar->insertButton( icon, 0, SIGNAL(clicked()), 
			_karm, SLOT(startClock()),
                       TRUE, i18n( "Start Clock" ) );

	_toolBar->insertButton( loader->loadIcon("stop.xpm"), 1, 
			SIGNAL(clicked()),
			_karm, SLOT(stopClock()),
			FALSE, i18n( "Stop Clock" ) );

	_toolBar->insertSeparator();
	
	_toolBar->insertButton( loader->loadIcon("filenew.xpm") , 2, 
				SIGNAL(clicked()),_karm, SLOT(newTask()),
				TRUE, i18n( "New Task" ) );

	_toolBar->insertButton( loader->loadIcon("filedel.xpm") , 3, 
				SIGNAL(clicked()),_karm, SLOT(deleteTask()),
				TRUE, i18n( "Delete Task" ) );

	icon.loadFromData( clockedit_xpm_data, clockedit_xpm_len );
	_toolBar->insertButton( icon, 4, SIGNAL(clicked()),
			_karm, SLOT(editTask()),
                       TRUE, i18n( "Edit Task" ) );

	// FIXME: this shouldnt stay. We need to check whether the
	// file exists and if not, create a blank one and ask whether
	// we want to add a task.
	_karm->load();

	// connections
	connect( _karm, SIGNAL(timerStarted()), this, SLOT(clockStartMsg()));
	connect( _karm, SIGNAL(timerStopped()), this, SLOT(clockStopMsg()));
	connect( _karm, SIGNAL(timerTick()), this, SLOT(updateTime()));

	// caption
	setCaption( _karm->KarmName );
}

KarmWindow::~KarmWindow()
{
	delete _fileMenu;
	delete _clockMenu;
	delete _taskMenu;
	delete _helpMenu;
	delete[] _sessionTimeBuffer;
}

void KarmWindow::updateTime()
{
	_totalTime++;

	Karm::formatTime( _sessionTimeBuffer, _totalTime );

	_statusBar->changeItem( _sessionTimeBuffer, 2);
}

void KarmWindow::clockStartMsg()
{
	_toolBar->setItemEnabled( 0, FALSE);
	_toolBar->setItemEnabled( 1, TRUE );
       _statusBar->changeItem( i18n( "clock active" ), 0);
}

void KarmWindow::clockStopMsg()
{
	_toolBar->setItemEnabled( 1, FALSE);
	_toolBar->setItemEnabled( 0, TRUE );
	_statusBar->changeItem( i18n( "clock inactive" ), 0);
}


void KarmWindow::saveProperties( KConfig* )
{
	_karm->save();
}

void KarmWindow::initAccelItems()
{
	_accel->insertItem( i18n( "Preferences" ), "Prefs",
		CTRL + Key_P );
	_accel->insertItem( i18n( "Start Clock" ), "StartClock", 
		CTRL + Key_S );
	_accel->insertItem( i18n( "Stop Clock" ), "StopClock", 
		CTRL + Key_T );
	_accel->insertItem( i18n( "New Task" ), "NewTask",
		CTRL + Key_N );
	_accel->insertItem( i18n( "Delete Task" ), "DeleteTask",
		CTRL + Key_D );
	_accel->insertItem( i18n( "Edit Task" ), "EditTask",
		CTRL + Key_E );

	_accel->insertStdItem( KAccel::Quit );

	_accel->readSettings();
}

void KarmWindow::connectAccels()
{
	_accel->connectItem( "Prefs",		this,	SLOT(prefs()) );
	_accel->connectItem( KAccel::Quit,	kapp,	SLOT(quit()) );
	_accel->connectItem( "StartClock",	_karm,	SLOT(startClock()) );
	_accel->connectItem( "StopClock",	_karm,	SLOT(stopClock()) );
	_accel->connectItem( "NewTask",		_karm,	SLOT(newTask()) );
	_accel->connectItem( "DeleteTask",	_karm,	SLOT(deleteTask()) );
	_accel->connectItem( "EditTask",	_karm,	SLOT(editTask()) );
}

void KarmWindow::prefs()
{
	if( KKeyDialog::configureKeys ( _accel ) ) {
		_watcher->updateMenus();
	}
}

void KarmWindow::makeMenus()
{
	_fileMenu = new QPopupMenu;
	_clockMenu= new QPopupMenu;
	_taskMenu = new QPopupMenu;
	_helpMenu = new QPopupMenu;

	QString about;
	about = i18n("%1 %2 -- Sirtaj Singh Kang\n"
			"taj@kde.org, Oct 1997\n\n"
			"The K Desktop Environment")
			.arg(_karm->KarmName).arg(KARM_VERSION);

	_mainMenu->insertItem( i18n( "&File" ), _fileMenu);
	_mainMenu->insertItem( i18n( "&Clock" ), _clockMenu );
	_mainMenu->insertItem( i18n( "&Task" ), _taskMenu);
	
	_mainMenu->insertSeparator();

	_mainMenu->insertItem( i18n( "&Help" ),
		kapp->getHelpMenu(TRUE, about ) );

	_watcher->setMenu( _fileMenu );
	int id = _fileMenu->insertItem( i18n( "&Preferences..." ), 
			this, SLOT( prefs() ) );
	_watcher->connectAccel( id, "Prefs" );

	id = _fileMenu->insertItem( i18n( "E&xit" ), 
			kapp, SLOT( quit() ) );
	_watcher->connectAccel( id, KAccel::Quit );
	

	_watcher->setMenu( _clockMenu );
	id = _clockMenu->insertItem( i18n( "&Start" ), _karm, 
			SLOT(startClock()) );
	_watcher->connectAccel( id, "StartClock" );

	id = _clockMenu->insertItem( i18n( "S&top" ), _karm, 
			SLOT(stopClock()) );
	_watcher->connectAccel( id, "StopClock" );


	_watcher->setMenu( _taskMenu );
	id = _taskMenu->insertItem( i18n( "&New" ), _karm, SLOT(newTask()) );
	_watcher->connectAccel( id, "NewTask" );

	id = _taskMenu->insertItem( i18n( "&Delete" ), _karm, 
			SLOT(deleteTask()) );
	_watcher->connectAccel( id, "DeleteTask" );

	id = _taskMenu->insertItem( i18n( "&Edit" ), _karm, SLOT(editTask()));
	_watcher->connectAccel( id, "EditTask" );
}
