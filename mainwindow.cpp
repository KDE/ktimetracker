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

MainWindow::MainWindow(const QString &icsfile)
  :  KParts::MainWindow( )
{
    qCDebug(KTT_LOG) << "Entering function, icsfile is " << icsfile;
    // Setup our actions
    setupActions();

    // this routine will find and load our Part.
    KPluginLoader loader( "ktimetrackerpart" );
    KPluginFactory *factory = loader.factory();
    if (factory)
    {
        // now that the Part is loaded, we cast it to a Part to get our hands on it

        //NOTE: Use the dynamic_cast below. Without it, KPluginLoader will use a qobject_cast
        // that fails, because ktimetrackerpart is defined twice, once in ktimetracker's binary
        // and another one in the plugin. The build system should be fixed.
        //m_part = factory->create<ktimetrackerpart>( this );

        m_part = dynamic_cast<KTimeTrackerPart*>(factory->create<KParts::ReadWritePart>(this));

        if (m_part)
        {
            // tell the KParts::MainWindow that this is indeed
            // the main widget
            setCentralWidget(m_part->widget());
            m_part->openFile(icsfile);
            slotSetCaption( icsfile );  // set the window title to our iCal file
            connect(configureAction, SIGNAL(triggered(bool)),
                m_part->widget(), SLOT(showSettingsDialog()));
            ((TimetrackerWidget *) (m_part->widget()))->setupActions( actionCollection() );
            setupGUI();
        }
        else
        {
          qCritical() << "Could not find the KTimeTracker part: m_part is 0";
          KMessageBox::error(this, i18n( "Could not create the KTimeTracker part." ));
          QTimer::singleShot(0, qApp, SLOT(quit()));
          return;
        }
    }
    else
    {
        // if we couldn't find our Part, we exit since the Shell by
        // itself can't do anything useful
        qCritical() << "Could not find the KTimeTracker part: factory is 0";
        KMessageBox::error(this, i18n( "Could not find the KTimeTracker part." ));
        QTimer::singleShot(0, qApp, SLOT(quit()));
        // we return here, cause qApp->quit() only means "exit the
        // next time we enter the event loop...
        return;
    }
    setWindowFlags( windowFlags() | Qt::WindowContextHelpButtonHint );

    // connections
    connect( m_part->widget(), SIGNAL(statusBarTextChangeRequested(QString)),
                 this, SLOT(setStatusBar(QString)) );
    connect( m_part->widget(), SIGNAL(setCaption(QString)),
                 this, SLOT(slotSetCaption(QString)) );
    loadGeometry();

    // Setup context menu request handling
    connect( m_part->widget(),
           SIGNAL(contextMenuRequested(QPoint)),
           this,
           SLOT(taskViewCustomContextMenuRequested(QPoint)) );
    if (KTimeTrackerSettings::trayIcon())
    {
        _tray = new TrayIcon( this );
        connect( m_part->widget(), SIGNAL(timersActive()), _tray, SLOT(startClock()) );
        connect( m_part->widget(), SIGNAL(timersInactive()), _tray, SLOT(stopClock()) );
        connect( m_part->widget(), SIGNAL(tasksChanged(QList<Task*>)), _tray, SLOT(updateToolTip(QList<Task*>)));
    }
}

void MainWindow::setupActions()
{
    configureAction = new QAction(this);
    configureAction->setText(i18n("Configure KTimeTracker..."));
    actionCollection()->addAction("configure_ktimetracker", configureAction);
}

void MainWindow::readProperties( const KConfigGroup &cfg )
{
    if( cfg.readEntry( "WindowShown", true ))
        show();
}

void MainWindow::saveProperties( KConfigGroup &cfg )
{
    cfg.writeEntry( "WindowShown", isVisible());
}

void MainWindow::slotSetCaption( const QString& qs )
{
    setCaption( qs );
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
    KShortcutsDialog::configure( actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, this );
}

void MainWindow::makeMenus()
{
    mainWidget->setupActions( actionCollection() );
    actionKeyBindings = KStandardAction::keyBindings( this, SLOT(keyBindings()),
        actionCollection() );
    setupGUI();
    actionKeyBindings->setToolTip( i18n( "Configure key bindings" ) );
    actionKeyBindings->setWhatsThis( i18n( "This will let you configure key"
                                           "bindings which are specific to ktimetracker" ) );
}

void MainWindow::loadGeometry()
{
    if (initialGeometrySet()) setAutoSaveSettings();
    else
    {
        KConfigGroup config = KSharedConfig::openConfig()->group( QString::fromLatin1("Main Window Geometry") );
        int w = config.readEntry( QString::fromLatin1("Width"), 100 );
        int h = config.readEntry( QString::fromLatin1("Height"), 100 );
        w = qMax( w, sizeHint().width() );
        h = qMax( h, sizeHint().height() );
        resize(w, h);
    }
}


void MainWindow::saveGeometry()
{
    KConfigGroup config = KSharedConfig::openConfig()->group( QString::fromLatin1("Main Window Geometry") );
    config.writeEntry( QString::fromLatin1("Width"), width());
    config.writeEntry( QString::fromLatin1("Height"), height());
    config.sync();
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
    QMenu* pop = dynamic_cast<QMenu*>( factory()->container( i18n( "task_popup" ), this ) );
    if ( pop )
        pop->popup( point );
}

