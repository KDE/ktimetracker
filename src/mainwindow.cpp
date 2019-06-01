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

#include "mainwindow.h"

#include <QMenu>
#include <QStatusBar>
#include <QGuiApplication>

#include <KLocalizedString>
#include <KXMLGUIFactory>
#include <KActionCollection>

#include "ktimetracker.h"
#include "tray.h"
#include "model/task.h"
#include "timetrackerwidget.h"
#include "ktt_debug.h"

MainWindow::MainWindow(const QUrl &url)
    : KXmlGuiWindow(nullptr)
    , m_tray(nullptr)
    , m_mainWidget(nullptr)
    , m_quitRequested(false)
{
    qCDebug(KTT_LOG) << "Entering function, url is " << url;

    // we need an instance
    m_mainWidget = new TimeTrackerWidget(this);
    setCentralWidget(m_mainWidget);

    // Setup our actions
    auto* configureAction = new QAction(this);
    configureAction->setText(i18n("Configure KTimeTracker..."));
    actionCollection()->addAction("configure_ktimetracker", configureAction);

    // Load the specified .ics file in the tasks widget
    m_mainWidget->openFile(url);
    setCaption(url.url());  // set the window title to our iCal file

    connect(configureAction, &QAction::triggered, m_mainWidget, &TimeTrackerWidget::showSettingsDialog);
    m_mainWidget->setupActions(actionCollection());

    KStandardAction::quit(this, &MainWindow::quit, actionCollection());

    setupGUI();

    setWindowFlags(windowFlags() | Qt::WindowContextHelpButtonHint);

    // connections
    connect(m_mainWidget, &TimeTrackerWidget::statusBarTextChangeRequested, this, &MainWindow::setStatusBar);
    connect(m_mainWidget, &TimeTrackerWidget::setCaption, this, QOverload<const QString&>::of(&KMainWindow::setCaption));

    // Setup context menu request handling
    connect(m_mainWidget, &TimeTrackerWidget::contextMenuRequested, this, &MainWindow::taskViewCustomContextMenuRequested);

    if (KTimeTrackerSettings::trayIcon()) {
        m_tray = new TrayIcon(this);
        connect(m_mainWidget, &TimeTrackerWidget::timersActive, m_tray, &TrayIcon::startClock);
        connect(m_mainWidget, &TimeTrackerWidget::timersInactive, m_tray, &TrayIcon::stopClock);
        connect(m_mainWidget, &TimeTrackerWidget::tasksChanged, m_tray, &TrayIcon::updateToolTip);
    }
}

//bool KTimeTrackerPart::openFile()
//{
//    return openFile(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + QStringLiteral("ktimetracker/ktimetracker.ics"));
//}

//bool KTimeTrackerPart::saveFile()
//{
//    m_mainWidget->saveFile();
//    return true;
//}

void MainWindow::setStatusBar(const QString& qs)
{
    statusBar()->showMessage(i18n(qs.toUtf8()));
}

MainWindow::~MainWindow()
{
    qCDebug(KTT_LOG) << "MainWindow::~MainWindows: Quitting ktimetracker.";
    saveGeometry();
}

bool MainWindow::queryClose()
{
    auto* app = dynamic_cast<QGuiApplication*>(QGuiApplication::instance());
    if (!m_quitRequested && app && !app->isSavingSession()) {
        hide();
        return false;
    }

    return KMainWindow::queryClose();
}

void MainWindow::taskViewCustomContextMenuRequested( const QPoint& point )
{
    QMenu* pop = dynamic_cast<QMenu*>(guiFactory()->container("task_popup", this));
    if (pop) {
        pop->popup(point);
    }
}

void MainWindow::quit()
{
    if (m_mainWidget->closeAllFiles()) {
        m_quitRequested = true;
        close();
    }
}
