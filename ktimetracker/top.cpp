/*
* Top Level window for KArm -- Sirtaj Singh Kang <taj@kde.org>
* Distributed under the GPL.
*/


/* 
 * $Id$
 * $Log$
 * Revision 1.30  2000/05/21 16:06:28  kulow
 * adding ... behind New and Edit
 *
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
#include <kaction.h>
#include <kstdaction.h>

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
	_totalTime( 0 )  
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

void KarmWindow::quit()
{
  _karm->save();
  KConfig &config = *kapp->config();
  config.setGroup( "Karm" );
  config.writeEntry("Width", width());
  config.writeEntry("Height", height());
  config.sync();
  kapp->quit();
}

	
// For some reasons, it doesn't seem like the destructor is being invoked
// when selecting quit in the menu, so maybe it should be removed.
// 26 maj. 2000 15:24 -- Jesper Pedersen <blackie@kde.org>
KarmWindow::~KarmWindow()
{
  quit();
}

void KarmWindow::updateTime()
{
  _totalTime++;
  QString time = Karm::formatTime( _totalTime );
  statusBar()->changeItem( time, 2);
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
  (void)KStdAction::quit(this, SLOT(quit()), actionCollection());
  (void)new KAction(i18n("&Preferences..."), 0,this,
					SLOT(prefs()),actionCollection(),"preferences");
  (void)new KAction(i18n("&Reset Session Time"), 0,this,
					SLOT(resetSessionTime()),actionCollection(),"reset_session_time");
  
  QPixmap icon;
 	
  icon.loadFromData(clock_xpm_data, clock_xpm_len );
  (void)new KAction(i18n("&Start"), icon,0,_karm,
					SLOT(startClock()),actionCollection(),"start");
  	
  (void)new KAction(i18n("S&top"), QIconSet(BarIcon("stop")),0,_karm,
					SLOT(stopClock()),actionCollection(),"stop");
  (void)new KAction(i18n("&New"), QIconSet(BarIcon("filenew")),0,_karm,
					SLOT(newTask()),actionCollection(),"new_task");
  
 	
  icon = QPixmap(delete_xpm);
  (void)new KAction(i18n("&Delete"), icon,0,_karm,
					SLOT(deleteTask()),actionCollection(),"delete_task");
 	
  icon.loadFromData( clockedit_xpm_data, clockedit_xpm_len );
  (void)new KAction(i18n("&Edit"), icon, 0,_karm,
					SLOT(editTask()),actionCollection(),"edit_task");
 	
  createGUI("karmui.rc");
}






