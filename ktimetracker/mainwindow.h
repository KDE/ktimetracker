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

#ifndef KARM_MAIN_WINDOW_H
#define KARM_MAIN_WINDOW_H

#include <KParts/MainWindow>

#include "karmerrors.h"
#include "reportcriteria.h"

class KAccel;
class KAccelMenuWatch;
class KAction;
class TrayIcon;
class QPoint;
class QString;

class Task;
class TimetrackerWidget;

/**
 * Main window to tie the application together.
 */

class MainWindow : public KParts::MainWindow
{
  Q_OBJECT

  private:
    void             makeMenus();

    KAccel*          _accel;
    KAccelMenuWatch* _watcher;
    long             _totalSum;
    long             _sessionSum;
    TrayIcon*        _tray;
    KAction*         actionKeyBindings;

    TimetrackerWidget *mainWidget;

    friend class TrayIcon;

  public:
    MainWindow( const QString &icsfile = "" );
    virtual ~MainWindow();

  public Q_SLOTS:
    void slotSetCaption( const QString& );
    void setStatusBar( const QString& );
    /** Quit ktimetracker (what else...) */
    void quit();
  protected Q_SLOTS:
    void keyBindings();
    void startNewSession();
    void updateTime( long, long );
    /** Set total application time and total application session time to zero.
      Typically, these times are shown as the lowest and rightest figures in
      the widget.
     */
    void reSetTimes();
    void updateStatusBar();
    void taskViewCustomContextMenuRequested( const QPoint& );

  protected:
    void startStatusBar();
    /* reimp */ void readProperties( const KConfigGroup &config );
    /* reimp */ void saveProperties( KConfigGroup &config );
    void saveGeometry();
    void loadGeometry();
    bool queryClose();
};

#endif // KARM_MAIN_WINDOW_H
