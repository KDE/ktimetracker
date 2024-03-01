/*
    SPDX-FileCopyrightText: 2003 Scott Monachello <smonach@cox.net>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#include <QList>

#include <KStatusNotifierItem>

QT_BEGIN_NAMESPACE
class QMovie;
QT_END_NAMESPACE

class MainWindow;
class Task;

class TrayIcon : public KStatusNotifierItem
{
    Q_OBJECT

public:
    explicit TrayIcon(MainWindow *parent);
    ~TrayIcon() override = default;

private:
    QMovie *m_animation;

public Q_SLOTS:
    void startClock();
    void stopClock();
    void updateToolTip(const QList<Task *> &activeTasks);

protected Q_SLOTS:
    void setActiveIcon(int frame);
};

#endif // TRAY_ICON_H
