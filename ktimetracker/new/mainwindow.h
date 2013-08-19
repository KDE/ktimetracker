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

#ifndef KTIMETRACKER_MAIN_WINDOW_H
#define KTIMETRACKER_MAIN_WINDOW_H

#include <KParts/MainWindow>
#include "ktimetrackerpart.h"
#include "reportcriteria.h"
#include "ktimetrackerconfigdialog.h"

class KAccel;
class KAccelMenuWatch;
class KAction;
class TrayIcon;
class QPoint;
class QString;

class Task;
class TimetrackerWidget;

namespace Ui {
    class MainWindow;
}
/**
 * Main window to tie the application together.
 */

class MainWindow : public QMainWindow
{
  Q_OBJECT

  private:
    void             makeMenus();
    void setupActions();
    Ui::MainWindow *m_ui;
    KAccel*          _accel;
    KAccelMenuWatch* _watcher;
    TrayIcon*        _tray;
    KAction*         actionKeyBindings;
    KAction* configureAction;

    TimetrackerWidget *mainWidget;

    friend class TrayIcon;
    ktimetrackerpart *m_part;

  public:
    explicit MainWindow(QWidget *parent = 0);
    explicit MainWindow( const QString &icsfile = "" );
    virtual ~MainWindow();

  public Q_SLOTS:
    void setStatusBar( const QString& );
    /* this is duplicated from timetrackerwidget */
    void showSettingsDialog();
    /* quit() has been offloaded to timetrackerwidget */
  protected Q_SLOTS:
    void keyBindings();
    void on_actionConfigure_ktimetracker_triggered();
    void on_actionQuit_triggered();
    void newtask();

  protected:
    void saveGeometry();
    void loadGeometry();
};

#endif // KTIMETRACKER_MAIN_WINDOW_H
