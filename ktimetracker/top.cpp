
/*
* Top Level window for KArm -- Sirtaj Singh Kang <taj@kde.org>
* Distributed under the GPL.
*/


/* 
 * $Id$
 * $Log$
 * Revision 1.29  2000/02/07 21:17:31  hadacek
 * KAccel/KStdAccel API cleanup + KAction/KAccel extension
 *
 * Revision 1.28  1999/12/30 19:48:15  granroth
 * loadIcon("*.xpm") => BarIcon("*")
 *
 * Revision 1.27  1999/11/20 19:00:32  espensa
 * Cleanup. Using new Add/Edit Task dialog box
 *
 */


#include <qkeycode.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpopupmenu.h>

#include <kaccel.h>
#include <kapp.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kmenubar.h>

#include "kaccelmenuwatch.h"
#include "karm.h"
#include "toolicons.h"
#include "top.h"
#include "version.h"


KarmWindow::KarmWindow()
  : KTMainWindow(),
  _accel( new KAccel( this ) ),
  _watcher( new KAccelMenuWatch( _accel, this ) ),
  _karm( new Karm( this ) ),
  _totalTime( 0 ),
  _sessionTimeBuffer( new char[100] )
  
{
  setView( _karm, FALSE );
  _karm->show();

  // accelerators
  initAccelItems();

  // status bar
	
  statusBar()->insertItem( i18n( "clock inactive" ), 0 );
  statusBar()->insertItem( i18n( "This session:" ), 1 );
  statusBar()->insertItem( "0:00", 2 );

  // popup menus
  makeMenus();
  connectAccels();
  _watcher->updateMenus();

  // toolbar
  QPixmap icon;
  
  icon.loadFromData(clock_xpm_data, clock_xpm_len );
  toolBar(0)->insertButton( icon, 0, SIGNAL(clicked()), 
			    _karm, SLOT(startClock()),
			    TRUE, i18n( "Start Clock" ) );

  toolBar(0)->insertButton( BarIcon("stop"), 1, 
			    SIGNAL(clicked()),
			    _karm, SLOT(stopClock()),
			    FALSE, i18n( "Stop Clock" ) );

  toolBar(0)->insertSeparator();
	
  toolBar(0)->insertButton( BarIcon("filenew") , 2, 
				SIGNAL(clicked()),_karm, SLOT(newTask()),
			  TRUE, i18n( "New Task" ) );
  
  toolBar(0)->insertButton( BarIcon("filedel") , 3, 
			  SIGNAL(clicked()),_karm, SLOT(deleteTask()),
			  TRUE, i18n( "Delete Task" ) );

  icon.loadFromData( clockedit_xpm_data, clockedit_xpm_len );
  toolBar(0)->insertButton( icon, 4, SIGNAL(clicked()),
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


  KConfig &config = *kapp->config();
  config.setGroup( "Karm" );
  int w = config.readNumEntry("Width", 100 );
  int h = config.readNumEntry("Height", 100 );
  w = QMAX( w, sizeHint().width() );
  h = QMAX( h, sizeHint().height() );
  resize(w, h);
}

KarmWindow::~KarmWindow()
{
  KConfig &config = *kapp->config();
  config.setGroup( "Karm" );
  config.writeEntry("Width", width());
  config.writeEntry("Height", height());
  config.sync();

  delete _fileMenu;
  delete _clockMenu;
  delete _taskMenu;
  delete[] _sessionTimeBuffer;
}

void KarmWindow::updateTime()
{
  _totalTime++;
  Karm::formatTime( _sessionTimeBuffer, _totalTime );
  statusBar()->changeItem( _sessionTimeBuffer, 2);
}

void KarmWindow::clockStartMsg()
{
  toolBar(0)->setItemEnabled( 0, FALSE);
  toolBar(0)->setItemEnabled( 1, TRUE );
  statusBar()->changeItem( i18n( "clock active" ), 0);
}

void KarmWindow::clockStopMsg()
{
  toolBar(0)->setItemEnabled( 1, FALSE);
  toolBar(0)->setItemEnabled( 0, TRUE );
  statusBar()->changeItem( i18n( "clock inactive" ), 0);
}


void KarmWindow::saveProperties( KConfig* )
{
  _karm->save();
}

void KarmWindow::initAccelItems()
{
  _accel->insertItem( i18n( "Preferences" ), "Prefs",
		      CTRL + Key_P );
  _accel->insertItem( i18n( "Reset Session Time" ), "ResetSess",
		      CTRL + Key_R );
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
  _accel->insertStdItem( KStdAccel::Quit );
  _accel->readSettings();
}

void KarmWindow::connectAccels()
{
  _accel->connectItem( "Prefs",		this,	SLOT(prefs()) );
  _accel->connectItem( "ResetSess",     this, SLOT( resetSessionTime() ) );
  _accel->connectItem( KStdAccel::Quit,	kapp, SLOT(closeAllWindows()));
  _accel->connectItem( "StartClock",	_karm,	SLOT(startClock()) );
  _accel->connectItem( "StopClock",	_karm,	SLOT(stopClock()) );
  _accel->connectItem( "NewTask",	_karm,	SLOT(newTask()) );
  _accel->connectItem( "DeleteTask",	_karm,	SLOT(deleteTask()) );
  _accel->connectItem( "EditTask",	_karm,	SLOT(editTask()) );
}

void KarmWindow::prefs()
{
	if( KKeyDialog::configureKeys ( _accel, true, topLevelWidget() ) ) {
		_watcher->updateMenus();
	}
}


void KarmWindow::resetSessionTime()
{
  _totalTime = 0;
  statusBar()->changeItem( "0:00", 2 );
}


void KarmWindow::makeMenus()
{
  _fileMenu = new QPopupMenu;
  _clockMenu= new QPopupMenu;
  _taskMenu = new QPopupMenu;

  QString about = i18n(""
    "%1 %2 -- Sirtaj Singh Kang\n"
    "taj@kde.org, Oct 1997\n\n"
    "The K Desktop Environment")
    .arg(_karm->KarmName).arg(KARM_VERSION);
  
  menuBar()->insertItem( i18n( "&File" ), _fileMenu);
  menuBar()->insertItem( i18n( "&Clock" ), _clockMenu );
  menuBar()->insertItem( i18n( "&Task" ), _taskMenu);
  menuBar()->insertSeparator();
  menuBar()->insertItem( i18n( "&Help" ), helpMenu( about ) );

  _watcher->setMenu( _fileMenu );
  int id = _fileMenu->insertItem( i18n( "&Preferences..." ), 
				  this, SLOT( prefs() ) );
  _watcher->connectAccel( id, "Prefs" );

  id = _fileMenu->insertItem( i18n( "&Reset Session Time" ),
			      this, SLOT( resetSessionTime() ) );
  _watcher->connectAccel( id, "ResetSess" );

  _fileMenu->insertSeparator( - 1 );
  id = _fileMenu->insertItem( i18n( "&Quit" ), kapp, SLOT( quit() ) );
  _watcher->connectAccel( id, KStdAccel::Quit );
	

  _watcher->setMenu( _clockMenu );
  id = _clockMenu->insertItem( i18n( "&Start" ), _karm, SLOT(startClock()) );
  _watcher->connectAccel( id, "StartClock" );

  id = _clockMenu->insertItem( i18n( "S&top" ), _karm, SLOT(stopClock()) );
  _watcher->connectAccel( id, "StopClock" );
	

  _watcher->setMenu( _taskMenu );
  id = _taskMenu->insertItem( i18n( "&New..." ), _karm, SLOT(newTask()) );
  _watcher->connectAccel( id, "NewTask" );

  id = _taskMenu->insertItem( i18n( "&Delete" ), _karm, SLOT(deleteTask()) );
  _watcher->connectAccel( id, "DeleteTask" );
  id = _taskMenu->insertItem( i18n( "&Edit..." ), _karm, SLOT(editTask()));
  _watcher->connectAccel( id, "EditTask" );
}






