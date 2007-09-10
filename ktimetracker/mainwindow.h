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
class TrayIcon;
class QPoint;
class QString;

class Task;
class TaskView;
class TimetrackerWidget;

/**
 * Main window to tie the application together.
 */

class MainWindow : public KParts::MainWindow
{
  Q_OBJECT

  private:
    void             makeMenus();
    QString          _hasTask( Task* task, const QString &taskname ) const;
    Task*            _hasUid( Task* task, const QString &uid ) const;

    KAccel*          _accel;
    KAccelMenuWatch* _watcher;
    long             _totalSum;
    long             _sessionSum;
    TrayIcon*        _tray;
    KAction*         actionPreferences;
    KAction*         actionKeyBindings;

    TimetrackerWidget *mainWidget;

    friend class TrayIcon;

  public:
    MainWindow( const QString &icsfile = "" );
    virtual ~MainWindow();

  public Q_SLOTS:
    void setStatusBar( const QString& );
    /** Quit ktimetracker (what else...) */
    void quit();
  protected Q_SLOTS:
    void keyBindings();
    void startNewSession();
    void updateTime( long, long );
    void updateStatusBar();
    void taskViewCustomContextMenuRequested( const QPoint& );
    void showSettingsDialog();
//    void timeLoggingChanged( bool on );

  protected:
    void startStatusBar();
    virtual void saveProperties( KConfigGroup& );
    virtual void readProperties( const KConfigGroup& );
    void saveGeometry();
    void loadGeometry();
    bool queryClose();

};

#endif // KARM_MAIN_WINDOW_H
