/*
 *     Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
 *                   2007 the ktimetracker developers
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the
 *      Free Software Foundation, Inc.
 *      51 Franklin Street, Fifth Floor
 *      Boston, MA  02110-1301  USA.
 *
 */

/*
 * KarmTray.
 *
 * This implements the functionality of the little icon in the kpanel
 * tray. Among which are tool tips and the running clock animated icon
 */

#include "tray.h"

#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QToolTip>

#include <KGlobalSettings>
#include <KIconLoader>
#include <KLocale>
#include <KMenu>

#include "mainwindow.h"
#include "task.h"

QVector<QPixmap*> *KarmTray::icons = 0;

KarmTray::KarmTray(MainWindow* parent)
  : KSystemTrayIcon(parent)
{
  setObjectName( "Karm Tray" );
  // the timer that updates the "running" icon in the tray
  _taskActiveTimer = new QTimer(this);
  connect( _taskActiveTimer, SIGNAL( timeout() ), this,
                             SLOT( advanceClock()) );

  if (icons == 0) {
    icons = new QVector<QPixmap*>(8);
    for (int i=0; i<8; i++) {
      QPixmap *icon = new QPixmap();
      QString name;
      name.sprintf("active-icon-%d.xpm",i);
      *icon = UserIcon(name);
      icons->insert(i,icon);
    }
  }

  contextMenu()->addAction( parent->actionPreferences );
// FIXME
//  contextMenu()->addAction( parent->actionStopAll );

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

KarmTray::KarmTray(karmPart *)
  : KSystemTrayIcon( 0 )
{
  setObjectName( "Karm Tray" );
// it is not convenient if every kpart gets an icon in the systray.
  _taskActiveTimer = 0;
}

KarmTray::KarmTray()
  : KSystemTrayIcon( 0 )
// will display nothing at all
{
  setObjectName( "Karm Tray" );
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
    setIcon( *(*icons)[_activeIcon] );
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
  setIcon( *(*icons)[_activeIcon]);
}

void KarmTray::resetClock()
{
  _activeIcon = 0;
  setIcon( *(*icons)[_activeIcon]);
  show();
}

void KarmTray::initToolTip()
{
  updateToolTip(QList<Task*> ());
}

void KarmTray::updateToolTip(QList<Task*> activeTasks)
{
  if ( activeTasks.isEmpty() ) {
    this->setToolTip( i18n("No active tasks") );
    return;
  }

  QFontMetrics fm( QToolTip::font() );
  const QString continued = i18n( ", ..." );
  const int buffer = fm.boundingRect( continued ).width();
  const int desktopWidth = KGlobalSettings::desktopGeometry(parentWidget()).width();
  const int maxWidth = desktopWidth - buffer;

  QString qTip;
  QString s;

  // Build the tool tip with all of the names of the active tasks.
  // If at any time the width of the tool tip is larger than the desktop,
  // stop building it.

  for ( int i = 0; i < activeTasks.count(); ++i ) {
    Task* task = activeTasks.at( i );
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

  this->setToolTip( qTip );
}

#include "tray.moc"
