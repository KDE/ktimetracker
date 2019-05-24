/*
 * Copyright (C) 2003 by Scott Monachello <smonach@cox.net>
 * Copyright (C) 2019  Alexander Potashev <aspotashev@gmail.com>
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

class TrayIcon;
class TimeTrackerWidget;

/**
 * Main window to tie the application together.
 */
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& path);
    ~MainWindow() override;

public Q_SLOTS:
    void slotSetCaption(const QString&);
    void setStatusBar(const QString&);
    void quit();

protected Q_SLOTS:
    void taskViewCustomContextMenuRequested(const QPoint&);

protected:
    bool queryClose() override;

private:
    bool openFile(const QString& path);

    TrayIcon* m_tray;
    TimeTrackerWidget* m_mainWidget;
    bool m_quitRequested;
};

#endif // KTIMETRACKER_MAIN_WINDOW_H
