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

/*
 * TrayIcon.
 *
 * This implements the functionality of the little icon in the kpanel
 * tray. Among which are tool tips and the running clock animated icon
 */

#include "tray.h"

#include <QToolTip>
#include <QMenu>
#include <QApplication>
#include <QDesktopWidget>
#include <QMovie>

#include <KLocalizedString>

#include "mainwindow.h"
#include "model/task.h"
#include "timetrackerwidget.h"
#include "ktt_debug.h"

TrayIcon::TrayIcon(MainWindow* parent)
    : KStatusNotifierItem(parent)
{
    Q_INIT_RESOURCE(pics);

    setObjectName("Ktimetracker Tray");

    m_animation = new QMovie(":/pics/active-icon.gif", QByteArray("GIF"), this);
    connect(m_animation, &QMovie::frameChanged, this, &TrayIcon::setActiveIcon);
    m_animation->jumpToFrame(0);

    auto* widget = dynamic_cast<TimeTrackerWidget*>(parent->centralWidget());
    if (widget) {
        QAction* action = widget->action("configure_ktimetracker");
        if (action) {
            contextMenu()->addAction(action);
        }

        action = widget->action("stopAll");
        if (action) {
            contextMenu()->addAction(action);
        }
    }

    updateToolTip(QList<Task*>());
}

void TrayIcon::startClock()
{
    m_animation->start();
}

void TrayIcon::stopClock()
{
    m_animation->stop();
}

void TrayIcon::setActiveIcon(int)
{
    setIconByPixmap(QIcon(m_animation->currentPixmap()));
}

void TrayIcon::updateToolTip(const QList<Task*> &activeTasks)
{
    if (activeTasks.isEmpty()) {
        this->setToolTip( "ktimetracker", "ktimetracker", i18n("No active tasks") );
        return;
    }

    QFontMetrics fm(QToolTip::font());
    const QString continued = i18nc("ellipsis to truncate long list of tasks", ", ...");
    const int buffer = fm.boundingRect(continued).width();
    const int desktopWidth = QApplication::desktop()->screenGeometry(associatedWidget()).width();
    const int maxWidth = desktopWidth - buffer;

    QString qTip;
    QString s;

    // Build the tool tip with all of the names of the active tasks.
    // If at any time the width of the tool tip is larger than the desktop,
    // stop building it.

    for (int i = 0; i < activeTasks.count(); ++i) {
        Task* task = activeTasks.at(i);
        if (i > 0) {
            s += i18nc("separator between task names", ", ") + task->name();
        } else {
            s += task->name();
        }

        int width = fm.boundingRect(s).width();
        if (width > maxWidth) {
            qTip += continued;
            break;
        }
        qTip = s;
    }
    this->setToolTip("ktimetracker", "ktimetracker", qTip);
}
