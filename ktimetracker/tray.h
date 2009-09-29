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

#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#include <QList>
#include <QVector>

#include <KNotificationItem>

#include "task.h"
#include "ktimetrackerpart.h"

class QTimer;
class MainWindow;

class TrayIcon : public KNotificationItem
{
  Q_OBJECT

  public:
    TrayIcon(MainWindow * parent);
    TrayIcon(ktimetrackerpart *);
    TrayIcon();
    ~TrayIcon();

  private:
    int _activeIcon;
    static QVector<QPixmap*> *icons;
    QTimer *_taskActiveTimer;

  public Q_SLOTS:
    void startClock();
    void stopClock();
    void resetClock();
    void updateToolTip( QList<Task*> activeTasks);
    void initToolTip();

  protected Q_SLOTS:
    void advanceClock();

  // experiment
  /*
    void insertTitle(QString title);

  private:
    KMenu *trayPopupMenu;
    QPopupMenu *trayPopupMenu2;
    */
};

#endif // KARM_TRAY_H
