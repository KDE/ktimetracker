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
#include "ui_mainwindow.h"
#include <QMenu>
#include <QString>

#include <KAction>
#include <KApplication>       // kapp
#include <QDebug>
#include "ktt_debug.h"
#include <KGlobal>
#include <KIcon>
#include <KLocale>            // i18n
#include <KMessageBox>
#include <KPushButton>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KStatusBar>         // statusBar()
#include <KXMLGUIFactory>
#include <KActionCollection>

#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "task.h"
#include "taskview.h"
#include "timekard.h"
#include "tray.h"

#include "timetrackerwidget.h"


/*

  Missing:
      - reaction on click on icons in the toolbar
      - context menu

  */
MainWindow::MainWindow( const QString &icsfile )
  :  QMainWindow( ),
    m_ui(new Ui::MainWindow)
{
      m_ui->setupUi(this);
      delete m_ui->treeWidget;
      m_ui->treeWidget=new TaskView(m_ui->centralwidget);
      ((TaskView *) m_ui->treeWidget)->load(icsfile);
      this->setWindowTitle(QString(icsfile));
      m_ui->gridLayout->addWidget(m_ui->treeWidget, 1, 0, 1, 1);
      m_ui->treeWidget->hide();
      m_ui->treeWidget->show();
      m_ui->ktreewidgetsearchline->show();
      m_ui->toolBar->addAction(KIcon("document-new"),"New Task",this,SLOT(newtask()));
      m_ui->toolBar->addAction(KIcon("subtask-new-ktimetracker"),"New Subtask");
      m_ui->toolBar->addAction(KIcon("media-playback-start"),"Start");
      m_ui->toolBar->addAction(KIcon("media-playback-stop"),"Stop");
      m_ui->toolBar->addAction(KIcon("edit-delete"), "Delete");
      m_ui->toolBar->addAction(KIcon("document-properties"),"Edit");
      setWindowFlags( windowFlags() | Qt::WindowContextHelpButtonHint );
/*

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

    _tray = new TrayIcon( this );

    connect( _tray, SIGNAL(quitSelected()), m_part->widget(), SLOT(quit()) );

    connect( m_part->widget(), SIGNAL(timersActive()), _tray, SLOT(startClock()) );
    connect( m_part->widget(), SIGNAL(timersInactive()), _tray, SLOT(stopClock()) );
    connect( m_part->widget(), SIGNAL(tasksChanged(QList<Task*>)),
                      _tray, SLOT(updateToolTip(QList<Task*>)));
                      */
}

void MainWindow::newtask()
{
    ((TaskView*) m_ui->treeWidget)->newTask();
}

void MainWindow::showSettingsDialog()
{
    qCDebug(KTT_LOG) << "Entering function";
    /* show main window b/c if this method was started from tray icon and the window
        is not visible the application quits after accepting the settings dialog.
    */
    window()->show();
    KTimeTrackerConfigDialog *dialog = new KTimeTrackerConfigDialog( i18n( "Settings" ), this);
    dialog->exec();
    m_ui->ktreewidgetsearchline->setHidden(KTimeTrackerSettings::configPDA());
    ((TaskView*) m_ui->treeWidget)->reconfigure();
    delete dialog;
}

void MainWindow::setupActions()
{
    configureAction = new KAction(this);
    configureAction->setText(i18n("Configure KTimeTracker..."));
    //actionCollection()->addAction("configure_ktimetracker", configureAction);
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
    //KShortcutsDialog::configure( actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, this );
}

void MainWindow::makeMenus()
{
    //mainWidget->setupActions( actionCollection() );
    //actionKeyBindings = KStandardAction::keyBindings( this, SLOT(keyBindings()),
      //  actionCollection() );
    //setupGUI();
    actionKeyBindings->setToolTip( i18n( "Configure key bindings" ) );
    actionKeyBindings->setWhatsThis( i18n( "This will let you configure key"
                                           "bindings which are specific to ktimetracker" ) );
}

void MainWindow::loadGeometry()
{
   // if (initialGeometrySet()) setAutoSaveSettings();
    //else
    {
        KConfigGroup config = KGlobal::config()->group( QString::fromLatin1("Main Window Geometry") );
        int w = config.readEntry( QString::fromLatin1("Width"), 100 );
        int h = config.readEntry( QString::fromLatin1("Height"), 100 );
        w = qMax( w, sizeHint().width() );
        h = qMax( h, sizeHint().height() );
        resize(w, h);
    }
}


void MainWindow::saveGeometry()
{
    KConfigGroup config = KGlobal::config()->group( QString::fromLatin1("Main Window Geometry") );
    config.writeEntry( QString::fromLatin1("Width"), width());
    config.writeEntry( QString::fromLatin1("Height"), height());
    config.sync();
}

void MainWindow::on_actionConfigure_ktimetracker_triggered()
{
    showSettingsDialog();
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

