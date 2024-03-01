/*
    SPDX-FileCopyrightText: 2003 Scott Monachello <smonach@cox.net>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTIMETRACKER_MAIN_WINDOW_H
#define KTIMETRACKER_MAIN_WINDOW_H

#include <KXmlGuiWindow>
#include <model/task.h>

class TrayIcon;
class TimeTrackerWidget;

/**
 * Main window to tie the application together.
 */
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QUrl &url);
    ~MainWindow() override;

public Q_SLOTS:
    void setStatusBar(const QString &);
    void quit();

protected Q_SLOTS:
    void taskViewCustomContextMenuRequested(const QPoint &);
    void updateWindowCaptionTasks(const QList<Task *> &activeTasks);
    void updateWindowCaptionFile(const QUrl &url);

protected:
    bool queryClose() override;

private:
    TrayIcon *m_tray;
    TimeTrackerWidget *m_mainWidget;
    bool m_quitRequested;
};

#endif // KTIMETRACKER_MAIN_WINDOW_H
