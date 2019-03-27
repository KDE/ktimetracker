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

#include <KXmlGuiWindow>

class KAccel;
class KAccelMenuWatch;
class QAction;

class TrayIcon;
class Task;
class TimeTrackerWidget;

/**
 * Main window to tie the application together.
 */
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

private:
    void setupActions();
    bool openFile(const QString& path);

    KAccel*          _accel;
    KAccelMenuWatch* _watcher;
    TrayIcon*        _tray;
    QAction *         actionKeyBindings;
    QAction * configureAction;

    friend class TrayIcon;
    TimeTrackerWidget *m_mainWidget;

public:
    explicit MainWindow(const QString& path = QString());
    ~MainWindow() override;

public Q_SLOTS:
    void slotSetCaption(const QString&);
    void setStatusBar(const QString&);
    void quit();

protected Q_SLOTS:
    void taskViewCustomContextMenuRequested(const QPoint&);

protected:
    bool queryClose();
};

#endif // KTIMETRACKER_MAIN_WINDOW_H
