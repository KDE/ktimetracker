/*
 *     Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
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
 * TrayIcon.
 *
 * This implements the functionality of the little icon in the kpanel
 * tray. Among which are tool tips and the running clock animated icon
 */

#include "tray.h"

#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QToolTip>
#include <QMenu>

#include <KAction>
#include <KGlobalSettings>
#include <KLocale>
#include <KMenu>
#include "mainwindow.h"
#include "task.h"
#include "timetrackerwidget.h"

QVector<QPixmap*> *TrayIcon::icons = 0;

TrayIcon::TrayIcon(MainWindow* parent)
  : KStatusNotifierItem(parent)
{
    setObjectName( "Ktimetracker Tray" );
    // the timer that updates the "running" icon in the tray
    _taskActiveTimer = new QTimer(this);
    connect( _taskActiveTimer, SIGNAL( timeout() ), this,
                               SLOT( advanceClock()) );

    if (icons == 0)
    {
        icons = new QVector<QPixmap*>(8);
        for (int i=0; i<8; i++)
        {
            QPixmap *icon = new QPixmap();
        QString name;
        name.sprintf("active-icon-%d.xpm",i);
        *icon = UserIcon(name);
        icons->insert(i,icon);
        }
    }
    TimetrackerWidget *timetrackerWidget = static_cast< TimetrackerWidget * >( parent->centralWidget() );
    if ( timetrackerWidget )
    {
        KAction *action = timetrackerWidget->action( "configure_ktimetracker" );
        if ( action ) contextMenu()->addAction( action );
        action = timetrackerWidget->action( "stopAll" );
        if ( action ) contextMenu()->addAction( action );
    }
    resetClock();
    initToolTip();
}

TrayIcon::TrayIcon(ktimetrackerpart *)
  : KStatusNotifierItem( 0 )
{
    setObjectName( "Ktimetracker Tray" );
    // it is not convenient if every kpart gets an icon in the systray.
    _taskActiveTimer = 0;
}

TrayIcon::TrayIcon()
  : KStatusNotifierItem( 0 )
// will display nothing at all
{
    setObjectName( "Ktimetracker Tray" );
    _taskActiveTimer = 0;
}

TrayIcon::~TrayIcon()
{
}

void TrayIcon::startClock()
{
    kDebug(5970) << "Entering function";
    if ( _taskActiveTimer )
    {
        _taskActiveTimer->start(1000);
        setIconByPixmap( *(*icons)[_activeIcon] );
    }
    kDebug(5970) << "Leaving function";
}

void TrayIcon::stopClock()
{
    kDebug(5970) << "Entering function";
    if ( _taskActiveTimer )
    {
        _taskActiveTimer->stop();
    }
    kDebug(5970) << "Leaving function";
}

void TrayIcon::advanceClock()
{
    _activeIcon = (_activeIcon+1) % 8;
    setIconByPixmap( *(*icons)[_activeIcon]);
}

void TrayIcon::resetClock()
{
    _activeIcon = 0;
    setIconByPixmap( *(*icons)[_activeIcon]);
}

void TrayIcon::initToolTip()
{
    updateToolTip(QList<Task*> ());
}

void TrayIcon::updateToolTip(QList<Task*> activeTasks)
{
    if ( activeTasks.isEmpty() )
    {
        this->setToolTip( "ktimetracker", "ktimetracker", i18n("No active tasks") );
        return;
    }

    QFontMetrics fm( QToolTip::font() );
    const QString continued = i18n( ", ..." );
    const int buffer = fm.boundingRect( continued ).width();
    const int desktopWidth = KGlobalSettings::desktopGeometry(associatedWidget()).width();
    const int maxWidth = desktopWidth - buffer;

    QString qTip;
    QString s;

    // Build the tool tip with all of the names of the active tasks.
    // If at any time the width of the tool tip is larger than the desktop,
    // stop building it.

    for ( int i = 0; i < activeTasks.count(); ++i )
    {
        Task* task = activeTasks.at( i );
        if ( i > 0 )
            s += i18n( ", " ) + task->name();
        else
            s += task->name();
        int width = fm.boundingRect( s ).width();
        if ( width > maxWidth )
        {
            qTip += continued;
            break;
        }
        qTip = s;
    }
    this->setToolTip( "ktimetracker", "ktimetracker", qTip );
}

#include "tray.moc"
