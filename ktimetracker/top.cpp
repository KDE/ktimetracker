/*
* Top Level window for KArm.
* Distributed under the GPL.
*/


/* 
 * $Id$
 */

#include "top.h"
#include <qstring.h>
#include <qkeycode.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <kconfig.h>
#include <kaccel.h>
#include <kapp.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kaction.h>
#include <kstdaction.h>
#include <qvbox.h>
#include <qtimer.h>
/*
#include "kaccelmenuwatch.h"
#include "karm.h"
#include "version.h"
#include "task.h"
#include "print.h"
#include "docking.h"
#include "preferences.h"
#include "idle.h"
*/
#include <qstring.h>

KarmWindow::KarmWindow()
  : KTMainWindow(),
	_accel( new KAccel( this ) ),
	_watcher( new KAccelMenuWatch( _accel, this ) ),
	_karm( new Karm( this ) ),
	_totalTime( 0 )  
{
  setView( _karm, FALSE );
  _karm->show();
  connect( _karm, SIGNAL( sessionTimeChanged( long ) ),
		   this, SLOT( updateTime( long ) ) );

  // status bar
	
  statusBar()->insertItem( i18n( "This session: %1" )
                           .arg(QString::fromLatin1("0:00")), 0, 0, true );

  // setup PreferenceDialog.
  _preferences = Preferences::instance();
  
  // popup menus
  makeMenus();
  _watcher->updateMenus();

  // connections
  connect( _karm, SIGNAL(timerTick()), this, SLOT(updateTime()));
  
  _preferences->load();
  loadGeometry();

  // FIXME: this shouldnt stay. We need to check whether the
  // file exists and if not, create a blank one and ask whether
  // we want to add a task.
  _karm->load();

  DockWidget *dockWidget = new DockWidget(this,"doc widget");
  dockWidget->show();
}

void KarmWindow::save() 
{
  _karm->save();
  saveGeometry();
}

void KarmWindow::quit()
{
  save();
  kapp->quit();
}

	
KarmWindow::~KarmWindow()
{
  quit();
}

/**
 * Updates the total time tally by one minute.
 */
void KarmWindow::updateTime()
{
  _totalTime++;
  updateStatusBar();
}

/**
 * Updates the total time tally by the value specified in newval.
 */
void KarmWindow::updateTime( long difference )
{
  _totalTime += difference;
  updateStatusBar();
}

void KarmWindow::updateStatusBar()
{
  QString time = Karm::formatTime( _totalTime );
  statusBar()->changeItem( i18n("This session: %1" ).arg(time), 0);
}

void KarmWindow::saveProperties( KConfig* )
{
  _karm->save();
}

void KarmWindow::keyBindings()
{
  KKeyDialog::configureKeys( actionCollection(), xmlFile());
}

void KarmWindow::resetSessionTime()
{
  _totalTime = 0;
  statusBar()->changeItem( i18n("This session: %1").arg(QString::fromLatin1("0:00")), 0 );
}


void KarmWindow::makeMenus()
{
  (void)KStdAction::quit(this, SLOT(quit()), actionCollection());
  (void)KStdAction::print(this, SLOT(print()), actionCollection());
  (void)KStdAction::keyBindings(this, SLOT(keyBindings()),actionCollection());
  (void)KStdAction::preferences(_preferences, SLOT(showDialog()),actionCollection());
  (void)KStdAction::save(_preferences, SLOT(save()),actionCollection());
  (void)new KAction(i18n("&Reset Session Time"), CTRL + Key_R,this,
		    SLOT(resetSessionTime()),actionCollection(),
		    "reset_session_time");
  
  (void)new KAction(i18n("&Start"), QString::fromLatin1("clock"),
		    CTRL + Key_S ,_karm,
		    SLOT(startTimer()),actionCollection(),"start");
  	
  (void)new KAction(i18n("S&top"), QString::fromLatin1("stop"),
		    CTRL + Key_T,_karm,
		    SLOT(stopCurrentTimer()),actionCollection(),"stop");

  (void)KStdAction::action( KStdAction::New, _karm,	SLOT(newTask()),
			    actionCollection(),"new_task");

  (void)new KAction(i18n("New Subtask"), CTRL+ALT+Key_N,
                         _karm, SLOT(newSubTask()),
                    actionCollection(), "new_sub_task");
                         
 	
  (void)new KAction(i18n("&Delete"), QString::fromLatin1("filedel"),
		    Key_Delete,_karm,
		    SLOT(deleteTask()),actionCollection(),"delete_task");
 	
  (void)new KAction(i18n("&Edit"), QString::fromLatin1("clockedit"),
		    CTRL + Key_E,_karm,
		    SLOT(editTask()),actionCollection(),"edit_task");
 	
  createGUI( QString::fromLatin1("karmui.rc") );
}

void KarmWindow::print() 
{
  MyPrinter printer(_karm);
  printer.print();
}

void KarmWindow::loadGeometry()
{
  KConfig &config = *kapp->config();
  
  config.setGroup( QString::fromLatin1("Main Window Geometry") );
  int w = config.readNumEntry( QString::fromLatin1("Width"), 100 );
  int h = config.readNumEntry( QString::fromLatin1("Height"), 100 );
  w = QMAX( w, sizeHint().width() );
  h = QMAX( h, sizeHint().height() );
  resize(w, h);
}


void KarmWindow::saveGeometry()
{
  KConfig &config = *KGlobal::config();
  config.setGroup( QString::fromLatin1("Main Window Geometry"));
  config.writeEntry( QString::fromLatin1("Width"), width());
  config.writeEntry( QString::fromLatin1("Height"), height());
  config.sync();
}
