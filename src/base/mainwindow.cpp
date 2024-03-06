/*
    SPDX-FileCopyrightText: 2003 Scott Monachello <smonach@cox.net>
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mainwindow.h"

#include <QGuiApplication>
#include <QMenu>
#include <QStatusBar>

#include <KActionCollection>
#include <KLocalizedString>
#include <KXMLGUIFactory>

#include "ktimetracker.h"
#include "ktimetrackerutility.h"
#include "ktt_debug.h"
#include "model/task.h"
#include "timetrackerwidget.h"
#include "tray.h"

MainWindow::MainWindow(const QUrl &url)
    : KXmlGuiWindow(nullptr)
    , m_tray(nullptr)
    , m_mainWidget(nullptr)
    , m_quitRequested(false)
{
    qCDebug(KTT_LOG) << "Entering function, url is " << url;

    m_mainWidget = new TimeTrackerWidget(this);
    setCentralWidget(m_mainWidget);

    auto *configureAction = new QAction(this);
    configureAction->setText(i18nc("@action:inmenu", "Configure KTimeTracker..."));
    actionCollection()->addAction(QStringLiteral("configure_ktimetracker"), configureAction);
    connect(configureAction, &QAction::triggered, m_mainWidget, &TimeTrackerWidget::showSettingsDialog);
    m_mainWidget->setupActions(actionCollection());

    KStandardAction::quit(this, &MainWindow::quit, actionCollection());

    setupGUI();

    setWindowFlags(windowFlags() | Qt::WindowContextHelpButtonHint);

    // connections
    connect(m_mainWidget, &TimeTrackerWidget::statusBarTextChangeRequested, this, &MainWindow::setStatusBar);
    connect(m_mainWidget, &TimeTrackerWidget::currentFileChanged, this, &MainWindow::updateWindowCaptionFile);
    connect(m_mainWidget, &TimeTrackerWidget::tasksChanged, this, &MainWindow::updateWindowCaptionTasks);
    connect(m_mainWidget, &TimeTrackerWidget::minutesUpdated, this, &MainWindow::updateWindowCaptionTasks);

    // Setup context menu request handling
    connect(m_mainWidget,
            &TimeTrackerWidget::contextMenuRequested,
            this,
            &MainWindow::taskViewCustomContextMenuRequested);

    if (KTimeTrackerSettings::trayIcon()) {
        m_tray = new TrayIcon(this);
        connect(m_mainWidget, &TimeTrackerWidget::timersActive, m_tray, &TrayIcon::startClock);
        connect(m_mainWidget, &TimeTrackerWidget::timersInactive, m_tray, &TrayIcon::stopClock);
        connect(m_mainWidget, &TimeTrackerWidget::tasksChanged, m_tray, &TrayIcon::updateToolTip);
    }

    // Load the specified .ics file in the tasks widget
    m_mainWidget->openFile(url);
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

void MainWindow::setStatusBar(const QString &qs)
{
    statusBar()->showMessage(i18n(qs.toUtf8().constData()));
}

MainWindow::~MainWindow()
{
    qCDebug(KTT_LOG) << "MainWindow::~MainWindows: Quitting ktimetracker.";
    saveGeometry();
    quit();
}

bool MainWindow::queryClose()
{
    if (!m_quitRequested && qApp && !qApp->isSavingSession()) {
        this->windowHandle()->hide();
        return false;
    }

    return KMainWindow::queryClose();
}

void MainWindow::taskViewCustomContextMenuRequested(const QPoint &point)
{
    QMenu *pop = dynamic_cast<QMenu *>(guiFactory()->container(QStringLiteral("task_popup"), this));
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

void MainWindow::updateWindowCaptionFile(const QUrl &url)
{
    if (!KTimeTrackerSettings::windowTitleCurrentFile()) {
        return;
    }

    this->setCaption(url.toDisplayString(QUrl::PreferLocalFile));
}

void MainWindow::updateWindowCaptionTasks(const QList<Task *> &activeTasks)
{
    if (!KTimeTrackerSettings::windowTitleCurrentTask()) {
        return;
    }

    if (activeTasks.isEmpty()) {
        this->setCaption(i18n("No active tasks"));
        return;
    }

    QString qCaption;

    // Build the caption with all of the names of the active tasks.
    for (int i = 0; i < activeTasks.count(); ++i) {
        Task *task = activeTasks.at(i);
        if (i > 0) {
            qCaption += i18nc("separator between task names", ", ");
        }
        qCaption += formatTime(task->sessionTime()) + QStringLiteral(" ") + task->name();
    }

    this->setCaption(qCaption);
}

#include "moc_mainwindow.cpp"
