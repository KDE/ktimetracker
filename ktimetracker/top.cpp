/*
* Top Level window for KArm -- Sirtaj Singh Kang <taj@kde.org>
* Distributed under the GPL.
*/


/* 
 * $Id$
 * $Log$
 * Revision 1.39  2000/07/05 18:11:23  faure
 * QIconSet(BarIcon()) removed. Gives better icons now - but KStdAction::preferences
 * points to "options" which apparently doesn't exist anymore ?
 *
 * Revision 1.38  2000/06/28 21:14:50  bieker
 * * strict compilation - fixing some unicode problems
 * * someone forgot to i18n() some strings... (that's easy to catch with
 *   QT_NO_CAST_ASCII :-)
 *
 * Revision 1.37  2000/06/26 21:10:50  blackie
 * added a preference menu, where stuff soon will come ;-)
 *
 * Revision 1.36  2000/06/02 17:45:39  blackie
 * better print layout + added session time for each task
 *
 * Revision 1.35  2000/06/02 06:04:26  kalle
 * Changing the time in the edit dialog also updates the total time tally
 * in the status bar.
 *
 * Revision 1.34  2000/06/01 15:51:09  blackie
 * added printing capabilities
 *
 * Revision 1.33  2000/05/29 14:17:09  kalle
 * Keyboard acceleration
 *
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
#include <klocale.h>
#include <kmenubar.h>
#include <kaction.h>
#include <kstdaction.h>
#include <qvbox.h>

#include "kaccelmenuwatch.h"
#include "karm.h"
#include "top.h"
#include "version.h"
#include "task.h"
#include "print.h"
#include "docking.h"

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
  connect( _karm, SIGNAL( sessionTimeChanged( long ) ),
		   this, SLOT( updateTime( long ) ) );

  // status bar
	
  statusBar()->insertItem( i18n( "clock inactive" ), 0 );
  statusBar()->insertItem( i18n( "This session:" ), 1 );
  statusBar()->insertItem( i18n( "0:00" ), 2 );

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
  config.setGroup( QString::fromLatin1("Karm") );
  int w = config.readNumEntry( QString::fromLatin1("Width"), 100 );
  int h = config.readNumEntry( QString::fromLatin1("Height"), 100 );
  w = QMAX( w, sizeHint().width() );
  h = QMAX( h, sizeHint().height() );
  resize(w, h);

  DockWidget *dockWidget = new DockWidget(this,"doc widget");
  dockWidget->show();
}

void KarmWindow::quit()
{
  _karm->save();
  KConfig &config = *KGlobal::config();
  config.setGroup( QString::fromLatin1("Karm") );
  config.writeEntry( QString::fromLatin1("Width"), width());
  config.writeEntry( QString::fromLatin1("Height"), height());
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

void KarmWindow::keyBindings()
{
  KKeyDialog::configureKeys( actionCollection(), xmlFile());
}


void KarmWindow::prefs()
{
  dialog = new KDialogBase(KDialogBase::Tabbed, i18n("Preferences"), 
                           KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok);
  
  QVBox *autoSaveMenu = dialog->addVBoxPage(i18n("Auto Save"));
  new QLabel(i18n("Auto Saving will be available from here soooooon\nJesper <blackie@kde.org>"), autoSaveMenu);
  
  QVBox *printerMenu = dialog->addVBoxPage(i18n("Printer Options"));
  new QLabel(i18n("Options describing the printout should come here sooooon\nJesper <blackie@kde.org>"), printerMenu);
  
  dialog->show();
}

void KarmWindow::prefsOk() 
{
  qDebug("OK\n");
  delete dialog;
}

void KarmWindow::prefsCancel() 
{
  qDebug("Cancel\n");
  delete dialog;
}


void KarmWindow::resetSessionTime()
{
  _totalTime = 0;
  statusBar()->changeItem( i18n("0:00"), 2 );
}


void KarmWindow::makeMenus()
{
  (void)KStdAction::quit(this, SLOT(quit()), actionCollection());
  (void)KStdAction::print(this, SLOT(print()), actionCollection());
  (void)KStdAction::keyBindings(this, SLOT(keyBindings()),actionCollection());
  (void)KStdAction::preferences(this, SLOT(prefs()),actionCollection());
  (void)new KAction(i18n("&Reset Session Time"), CTRL + Key_R,this,
		    SLOT(resetSessionTime()),actionCollection(),
		    "reset_session_time");
  
  (void)new KAction(i18n("&Start"), QString::fromLatin1("clock"),
		    CTRL + Key_S ,_karm,
		    SLOT(startClock()),actionCollection(),"start");
  	
  (void)new KAction(i18n("S&top"), QString::fromLatin1("stop"),
		    CTRL + Key_T,_karm,
		    SLOT(stopClock()),actionCollection(),"stop");
  (void)KStdAction::action( KStdAction::New, _karm,	SLOT(newTask()),
			    actionCollection(),"new_task");
  
 	
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
  printer.Print();
}

