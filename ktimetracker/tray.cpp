/*
* KTray.
*
* This implements the functionality of the little icon in the kpanel
* tray. Among which are tool tips and the running clock animated icon
*
* Distributed under the GPL.
*/


// #include <qkeycode.h>
// #include <qlayout.h>
#include <qpixmap.h>
#include <q3ptrlist.h>
#include <qstring.h>
#include <qtimer.h>
#include <qtooltip.h>

#include <kaction.h>            // actionPreferences()
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>        // UserIcon
#include <klocale.h>            // i18n
#include <kpopupmenu.h>         // plug()
#include <ksystemtray.h>

#include "mainwindow.h"
#include "task.h"
#include "tray.h"

Q3PtrVector<QPixmap> *KarmTray::icons = 0;

KarmTray::KarmTray(MainWindow* parent)
  : KSystemTray(parent, "Karm Tray")
{
  // the timer that updates the "running" icon in the tray
  _taskActiveTimer = new QTimer(this);
  connect( _taskActiveTimer, SIGNAL( timeout() ), this,
                             SLOT( advanceClock()) );

  if (icons == 0) {
    icons = new Q3PtrVector<QPixmap>(8);
    for (int i=0; i<8; i++) {
      QPixmap *icon = new QPixmap();
      QString name;
      name.sprintf("active-icon-%d.xpm",i);
      *icon = UserIcon(name);
      icons->insert(i,icon);
    }
  }

  parent->actionPreferences->plug( contextMenu() ); 
  parent->actionStopAll->plug( contextMenu() );

  resetClock();
  initToolTip();

  // start of a kind of menu for the tray
  // this are experiments/tests
  /*
  for (int i=0; i<30; i++)
    _tray->insertTitle(i 18n("bla ").arg(i));
  for (int i=0; i<30; i++)
    _tray->insertTitle2(i 18n("bli ").arg(i));
  */
  // experimenting with menus for the tray
  /*
  trayPopupMenu = contextMenu();
  trayPopupMenu2 = new QPopupMenu();
  trayPopupMenu->insertItem(i18n("Submenu"), *trayPopupMenu2);
  */
}

KarmTray::KarmTray(karmPart * parent)
  : KSystemTray( 0 , "Karm Tray")
{
// it is not convenient if every kpart gets an icon in the systray.
  _taskActiveTimer = 0;
}

KarmTray::~KarmTray()
{
}


// experiment
/*
void KarmTray::insertTitle(QString title)
{
  trayPopupMenu->insertTitle(title);
}
*/

void KarmTray::startClock()
{
  if ( _taskActiveTimer ) 
  {
    _taskActiveTimer->start(1000);
    setPixmap( *(*icons)[_activeIcon] );
    show();
  }
}

void KarmTray::stopClock()
{
  if ( _taskActiveTimer )  
  {  
    _taskActiveTimer->stop();
    show();
  }
}

void KarmTray::advanceClock()
{
  _activeIcon = (_activeIcon+1) % 8;
  setPixmap( *(*icons)[_activeIcon]);
}

void KarmTray::resetClock()
{
  _activeIcon = 0;
  setPixmap( *(*icons)[_activeIcon]);
  show();
}

void KarmTray::initToolTip()
{
  updateToolTip(Q3PtrList<Task> ());
}

void KarmTray::updateToolTip(Q3PtrList<Task> activeTasks)
{
  if ( activeTasks.isEmpty() ) {
    QToolTip::add( this, i18n("No active tasks") );
    return;
  }

  QFontMetrics fm( QToolTip::font() );
  const QString continued = i18n( ", ..." );
  const int buffer = fm.boundingRect( continued ).width();
  const int desktopWidth = KGlobalSettings::desktopGeometry(this).width();
  const int maxWidth = desktopWidth - buffer;

  QString qTip;
  QString s;

  // Build the tool tip with all of the names of the active tasks.
  // If at any time the width of the tool tip is larger than the desktop,
  // stop building it.
  Q3PtrListIterator<Task> item( activeTasks );
  for ( int i = 0; item.current(); ++item, ++i ) {
    Task* task = item.current();
    if ( i > 0 )
      s += i18n( ", " ) + task->name();
    else
      s += task->name();
    int width = fm.boundingRect( s ).width();
    if ( width > maxWidth ) {
      qTip += continued;
      break;
    }
    qTip = s;
  }

  QToolTip::add( this, qTip );
}

#include "tray.moc"
