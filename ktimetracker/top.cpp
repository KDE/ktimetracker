/*
* Top Level window for KArm -- Sirtaj Singh Kang <taj@kde.org>
* Distributed under the GPL.
*/


/* 
 * $Id$
 * $Log$
 * Revision 1.32  2000/05/29 13:19:31  kalle
 * Icon loading in karm
 *
 * Revision 1.31  2000/05/29 12:30:54  kalle
 * - Replaced the two listboxes with one listview
 * - Times can be specified as HH:MM [(+|-)HH:MM]
 * - uses XML GUI
 * - general KDE 2 face- and codelifting
 *
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
#include "top.h"
#include "version.h"

#include "top.moc"

KarmWindow::KarmWindow()
  : KTMainWindow(),
	_accel( new KAccel( this ) ),
	_watcher( new KAccelMenuWatch( _accel, this ) ),
	_karm( new Karm( this ) ),
	_totalTime( 0 )  
{
  setView( _karm, FALSE );
  _karm->show();

  // status bar
	
  statusBar()->insertItem( i18n( "clock inactive" ), 0 );
  statusBar()->insertItem( i18n( "This session:" ), 1 );
  statusBar()->insertItem( "0:00", 2 );

  // popup menus
  makeMenus();
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


void KarmWindow::prefs()
{
  KKeyDialog::configureKeys( actionCollection(), xmlFile() );
}


void KarmWindow::resetSessionTime()
{
  _totalTime = 0;
  statusBar()->changeItem( "0:00", 2 );
}


void KarmWindow::makeMenus()
{
  (void)KStdAction::quit(this, SLOT(quit()), actionCollection());
  (void)KStdAction::action( KStdAction::Preferences,this,
					SLOT(prefs()),actionCollection(),"preferences");
  (void)new KAction(i18n("&Reset Session Time"), CTRL + Key_R,this,
					SLOT(resetSessionTime()),actionCollection(),"reset_session_time");
  
  (void)new KAction(i18n("&Start"), BarIcon( "clock" ), CTRL + Key_S ,_karm,
					SLOT(startClock()),actionCollection(),"start");
  	
  (void)new KAction(i18n("S&top"), QIconSet(BarIcon("stop")), CTRL + Key_T,_karm,
					SLOT(stopClock()),actionCollection(),"stop");
  (void)KStdAction::action( KStdAction::New, _karm,	SLOT(newTask()),
								 actionCollection(),"new_task");
  
 	
  (void)new KAction(i18n("&Delete"), BarIcon( "filedel" ),Key_Delete,_karm,
					SLOT(deleteTask()),actionCollection(),"delete_task");
 	
  (void)new KAction(i18n("&Edit"), BarIcon( "clockedit" ), CTRL + Key_E,_karm,
					SLOT(editTask()),actionCollection(),"edit_task");
 	
  createGUI("karmui.rc");
}






