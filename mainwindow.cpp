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

#include "mainwindow.h"

#include <numeric>

#include <QMenu>
#include <QString>
#include <QTimer>
#include <QAction>
#include <QDebug>
#include <QStatusBar>

#include <KMessageBox>
#include <KLocalizedString>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KXMLGUIFactory>
#include <KActionCollection>
#include <KPluginLoader>
#include <KPluginFactory>
#include <KSharedConfig>

#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "task.h"
#include "taskview.h"
#include "timekard.h"
#include "tray.h"
#include "timetrackerwidget.h"
#include "ktt_debug.h"

MainWindow::MainWindow(const QString& path)
    : KParts::MainWindow()
{
    qCDebug(KTT_LOG) << "Entering function, icsfile is " << path;
    // Setup our actions
    setupActions();

    // this routine will find and load our Part.
    KPluginLoader loader( "ktimetrackerpart" );
    KPluginFactory *factory = loader.factory();
    if (!factory) {
        // if we couldn't find our Part, we exit since the Shell by
        // itself can't do anything useful
        qCritical() << "Could not find the KTimeTracker part: factory is 0";
        KMessageBox::error(this, i18n( "Could not find the KTimeTracker part." ));
        QTimer::singleShot(0, qApp, SLOT(quit()));
        // we return here, cause qApp->quit() only means "exit the
        // next time we enter the event loop...
        return;
    }

    // now that the Part is loaded, we cast it to a Part to get our hands on it

    //NOTE: Use the dynamic_cast below. Without it, KPluginLoader will use a qobject_cast
    // that fails, because ktimetrackerpart is defined twice, once in ktimetracker's binary
    // and another one in the plugin. The build system should be fixed.
    //m_part = factory->create<ktimetrackerpart>( this );

    m_part = dynamic_cast<KTimeTrackerPart*>(factory->create<KParts::ReadWritePart>(this));

    if (!m_part) {
        qCritical() << "Could not find the KTimeTracker part: m_part is 0";
        KMessageBox::error(this, i18n( "Could not create the KTimeTracker part." ));
        QTimer::singleShot(0, qApp, SLOT(quit()));
        return;
    }

    auto* widget = dynamic_cast<TimetrackerWidget*>(m_part->widget());
    if (!widget) {
        qCritical() << "KPart widget has wrong type";
        KMessageBox::error(this, i18n("Could not find the KTimeTracker widget."));
        QTimer::singleShot(0, qApp, SLOT(quit()));
        return;
    }

    // tell the KParts::MainWindow that this is indeed
    // the main widget
    setCentralWidget(widget);
    m_part->openFile(path);
    slotSetCaption(path);  // set the window title to our iCal file
    connect(configureAction, SIGNAL(triggered(bool)),
        widget, SLOT(showSettingsDialog()));
    widget->setupActions(actionCollection());
    setupGUI();

    setWindowFlags(windowFlags() | Qt::WindowContextHelpButtonHint);

    // connections
    connect(widget, &TimetrackerWidget::statusBarTextChangeRequested, this, &MainWindow::setStatusBar);
    connect(widget, &TimetrackerWidget::setCaption, this, &MainWindow::slotSetCaption);

    // Setup context menu request handling
    connect(widget, &TimetrackerWidget::contextMenuRequested, this, &MainWindow::taskViewCustomContextMenuRequested);

    if (KTimeTrackerSettings::trayIcon()) {
        _tray = new TrayIcon(this);
        connect(widget, &TimetrackerWidget::timersActive, _tray, &TrayIcon::startClock);
        connect(widget, &TimetrackerWidget::timersInactive, _tray, &TrayIcon::stopClock);
        connect( widget, SIGNAL(tasksChanged(QList<Task*>)), _tray, SLOT(updateToolTip(QList<Task*>)));
    }
}

void MainWindow::setupActions()
{
    configureAction = new QAction(this);
    configureAction->setText(i18n("Configure KTimeTracker..."));
    actionCollection()->addAction("configure_ktimetracker", configureAction);
}

void MainWindow::slotSetCaption(const QString& qs)
{
    setCaption(qs);
}

void MainWindow::setStatusBar(const QString& qs)
{
    statusBar()->showMessage(i18n(qs.toUtf8()));
}

MainWindow::~MainWindow()
{
    qCDebug(KTT_LOG) << "MainWindow::~MainWindows: Quitting ktimetracker.";
    saveGeometry();
}

void MainWindow::keyBindings()
{
    KShortcutsDialog::configure(actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, this);
}

void MainWindow::makeMenus()
{
    mainWidget->setupActions(actionCollection());
    actionKeyBindings = KStandardAction::keyBindings(this, SLOT(keyBindings()), actionCollection());
    setupGUI();
    actionKeyBindings->setToolTip(i18n("Configure key bindings"));
    actionKeyBindings->setWhatsThis(i18n("This will let you configure key bindings which are specific to ktimetracker."));
}

bool MainWindow::queryClose()
{
//    if ( !kapp->sessionSaving() )
//    {
//        hide();
//        return false;
//    }
    return KMainWindow::queryClose();
}

void MainWindow::taskViewCustomContextMenuRequested( const QPoint& point )
{
    QMenu* pop = dynamic_cast<QMenu*>(factory()->container("task_popup", this));
    if (pop) {
        pop->popup(point);
    }
}
