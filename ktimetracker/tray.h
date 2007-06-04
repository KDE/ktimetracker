/*
 *     Copyright (C) 2007 the ktimetracker developers
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
#ifndef KARM_TRAY_H
#define KARM_TRAY_H

#include <q3ptrvector.h>
#include <QPixmap>
#include <q3ptrlist.h>
// experiement
// #include <kmenu.h>
#include <ksystemtrayicon.h>

#include "task.h"
#include "karm_part.h"


class QTimer;

class MainWindow;
// experiment
// class KMenu;

class KarmTray : public KSystemTrayIcon
{
  Q_OBJECT

  public:
    KarmTray(MainWindow * parent);
    KarmTray(karmPart * parent);
    KarmTray();
    ~KarmTray();

  private:
    int _activeIcon;
    static Q3PtrVector<QPixmap> *icons;
    QTimer *_taskActiveTimer;

  public slots:
    void startClock();
    void stopClock();
    void resetClock();
    void updateToolTip( Q3PtrList<Task> activeTasks);
    void initToolTip();

  protected slots:
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
