
#include<qpopmenu.h>
#include<qlayout.h>
#include<qpixmap.h>
#include<qkeycode.h>

#include<kiconloader.h>
#include<kapp.h>
#include <klocale.h>
#include<ktopwidget.h>
#include<kmenubar.h>
#include<ktoolbar.h>
#include<kstatusbar.h>
#include<kmsgbox.h>

#include "karm.h"
#include "toolicons.h"

#include "top.h"

KarmWindow::KarmWindow()
	:	KTopLevelWidget()
{
	_totalTime = 0;
	_sessionTimeBuffer = new char[100];

	_mainMenu = new KMenuBar(this, "_mainMenu" );
	_mainMenu->show();
	setMenu( _mainMenu );

	_toolBar = new KToolBar( this, "_toolBar" );
//	_toolBar->setPos( KToolBar::Top );
	
	_karm = new Karm(this);
	setView( _karm, FALSE );
	_karm->show();

	_statusBar = new KStatusBar( this, "_statusBar");
       _statusBar->insertItem(klocale->translate( "clock inactive" ), 0);
       _statusBar->insertItem(klocale->translate( "This session:" ), 1);
	_statusBar->insertItem("0:00", 2);
	_statusBar->show();
	setStatusBar( _statusBar );

	// setup menus

	_fileMenu = new QPopupMenu;
	_clockMenu= new QPopupMenu;
	_taskMenu = new QPopupMenu;
	_helpMenu = new QPopupMenu;

       _mainMenu->insertItem( klocale->translate( "&File" ), _fileMenu);
               _fileMenu->insertItem(klocale->translate( "E&xit" ), KApplication::getKApplication(), 
						SLOT( quit() ), ALT + Key_Q );

       _mainMenu->insertItem( klocale->translate( "&Clock" ), _clockMenu );
               _clockMenu->insertItem( klocale->translate( "&Start" ), _karm, SLOT(startClock()) );
               _clockMenu->insertItem( klocale->translate( "S&top" ), _karm, SLOT(stopClock()) );

       _mainMenu->insertItem( klocale->translate( "&Task" ), _taskMenu);
               _taskMenu->insertItem( klocale->translate( "&New" ), _karm, SLOT(newTask()), 
					Key_Insert );
               _taskMenu->insertItem( klocale->translate( "&Delete" ), _karm, SLOT(deleteTask()),
					Key_Delete );
               _taskMenu->insertItem( klocale->translate( "&Edit" ), _karm, SLOT(editTask()),
					CTRL + Key_E );

	_mainMenu->insertSeparator();

	QString about;
	about.sprintf(i18n("%s 0.3 -- Sirtaj Singh Kang\n"
			"taj@kde.org, Oct 1997\n\n"
                       "The K Desktop Environment"), _karm->KarmName.data());
	
	_mainMenu->insertItem( klocale->translate( "&Help" ),
		KApplication::getKApplication()->getHelpMenu(TRUE, about ) );
	
	KIconLoader *loader = kapp->getIconLoader();
	QPixmap icon;

	// FIXME: dummy locations for icons till final
	// install script is written.
	icon.loadFromData(clock_xpm_data, clock_xpm_len );
	_toolBar->insertButton( icon, 0, SIGNAL(clicked()), 
			_karm, SLOT(startClock()),
                       TRUE, klocale->translate( "Start Clock" ) );

	_toolBar->insertButton( loader->loadIcon("stop.xpm"), 1, SIGNAL(clicked()),
				_karm, SLOT(stopClock()),
				FALSE, klocale->translate( "Stop Clock" ) );
	
	_toolBar->insertSeparator();
	
	_toolBar->insertButton( loader->loadIcon("filenew.xpm") , 2, 
				SIGNAL(clicked()),_karm, SLOT(newTask()),
				TRUE, klocale->translate( "New Task" ) );

	_toolBar->insertButton( loader->loadIcon("filedel.xpm") , 3, 
				SIGNAL(clicked()),_karm, SLOT(deleteTask()),
				TRUE, klocale->translate( "Delete Task" ) );

	icon.loadFromData( clockedit_xpm_data, clockedit_xpm_len );
	_toolBar->insertButton( icon, 4, SIGNAL(clicked()),
			_karm, SLOT(editTask()),
                       TRUE, klocale->translate( "Edit Task" ) );

	_toolBar->show();
	enableToolBar( KToolBar::Show, addToolBar( _toolBar ) );


	// FIXME: this shouldnt stay. We need to check whether the
	// file exists and if not, create a blank one and ask whether
	// we want to add a task.
	_karm->load();

	// connections
	connect( _karm, SIGNAL(timerStarted()), this, SLOT(clockStartMsg()));
	connect( _karm, SIGNAL(timerStopped()), this, SLOT(clockStopMsg()));
	connect( _karm, SIGNAL(timerTick()), this, SLOT(updateTime()));

	// Peter Putzer: added Application "Name"
	setCaption(_karm->KarmName);
}

KarmWindow::~KarmWindow()
{
	delete _fileMenu;
	delete _clockMenu;
	delete _taskMenu;
	delete _helpMenu;
	delete _mainMenu;
	delete _toolBar;
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
       _statusBar->changeItem( klocale->translate( "clock active" ), 0);
}

void KarmWindow::clockStopMsg()
{
	_toolBar->setItemEnabled( 1, FALSE);
	_toolBar->setItemEnabled( 0, TRUE );
	_statusBar->changeItem( klocale->translate( "clock inactive" ), 0);
}


void KarmWindow::saveProperties( KConfig* )
{
  _karm->save();
}

