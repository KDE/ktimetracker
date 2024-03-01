/*
    SPDX-FileCopyrightText: 2003 Scott Monachello <smonach@cox.net>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/*
 * TrayIcon.
 *
 * This implements the functionality of the little icon in the kpanel
 * tray. Among which are tool tips and the running clock animated icon
 */

#include "tray.h"

#include <QApplication>
#include <QMenu>
#include <QMovie>
#include <QToolTip>
#include <QScreen>

#include <KLocalizedString>

#include "ktt_debug.h"
#include "mainwindow.h"
#include "model/task.h"
#include "timetrackerwidget.h"

TrayIcon::TrayIcon(MainWindow *parent)
    : KStatusNotifierItem(parent)
{
    Q_INIT_RESOURCE(pics);

    setObjectName(QStringLiteral("Ktimetracker Tray"));

    m_animation = new QMovie(QStringLiteral(":/pics/active-icon.gif"), QByteArray("GIF"), this);
    connect(m_animation, &QMovie::frameChanged, this, &TrayIcon::setActiveIcon);
    m_animation->jumpToFrame(0);

    auto *widget = dynamic_cast<TimeTrackerWidget *>(parent->centralWidget());
    if (widget) {
        QAction *action = widget->action(QStringLiteral("configure_ktimetracker"));
        if (action) {
            contextMenu()->addAction(action);
        }

        action = widget->action(QStringLiteral("stopAll"));
        if (action) {
            contextMenu()->addAction(action);
        }
    }

    updateToolTip(QList<Task *>());
}

void TrayIcon::startClock()
{
    m_animation->start();
}

void TrayIcon::stopClock()
{
    m_animation->stop();
}

void TrayIcon::setActiveIcon(int /*unused*/)
{
    setIconByPixmap(QIcon(m_animation->currentPixmap()));
}

void TrayIcon::updateToolTip(const QList<Task *> &activeTasks)
{
    if (activeTasks.isEmpty()) {
        this->setToolTip(QStringLiteral("ktimetracker"), QStringLiteral("ktimetracker"), i18n("No active tasks"));
        return;
    }

    QFontMetrics fm(QToolTip::font());
    const QString continued = i18nc("ellipsis to truncate long list of tasks", ", ...");
    const int buffer = fm.boundingRect(continued).width();
    const int desktopWidth = associatedWindow()->screen()->geometry().width();
    const int maxWidth = desktopWidth - buffer;

    QString qTip;
    QString s;

    // Build the tool tip with all of the names of the active tasks.
    // If at any time the width of the tool tip is larger than the desktop,
    // stop building it.

    for (int i = 0; i < activeTasks.count(); ++i) {
        Task *task = activeTasks.at(i);
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
    this->setToolTip(QStringLiteral("ktimetracker"), QStringLiteral("ktimetracker"), qTip);
}

#include "moc_tray.cpp"
