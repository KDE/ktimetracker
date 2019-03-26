/*
 *     Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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

// TODO: what is the sense of tasksChanged()?

#include "timetrackerwidget.h"
#include "ktimetrackerconfigdialog.h"

#include <QDBusConnection>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMap>
#include <QVBoxLayout>
#include <QVector>
#include <QDebug>
#include <QFileDialog>
#include <QAction>
#include <QUrl>

#include <KLocalizedString>
#include <KActionCollection>
#include <KConfig>
#include <KConfigDialog>
#include "ktt_debug.h"
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KStandardAction>
#include <QTemporaryFile>
#include <KTreeWidgetSearchLine>
#include <KIO/Job>

#include "historydialog.h"
#include "idletimedetector.h"
#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "mainadaptor.h"
#include "reportcriteria.h"
#include "task.h"
#include "taskview.h"
#include "ktimetracker-version.h"
#include "mainwindow.h"


//@cond PRIVATE
class TimetrackerWidget::Private
{
  public:
    Private() :
      mTaskView( 0 ) {}

    QWidget *mSearchLine;
    KTreeWidgetSearchLine *mSearchWidget;
    TaskView *mTaskView;
    QMap<QString, QAction *> mActions;
    KActionCollection *m_actionCollection;
};
//@endcond

TimetrackerWidget::TimetrackerWidget( QWidget *parent ) : QWidget( parent ),
  d( new TimetrackerWidget::Private() )
{
    qCDebug(KTT_LOG) << "Entering function";
    new MainAdaptor( this );
    QDBusConnection::sessionBus().registerObject( "/KTimeTracker", this );

    QLayout *layout = new QVBoxLayout;
    layout->setMargin( 0 );
    layout->setSpacing( 0 );

    QLayout *innerLayout = new QHBoxLayout;
    d->mSearchLine = new QWidget(this);
    d->mSearchWidget = new KTreeWidgetSearchLine(d->mSearchLine);
    d->mSearchWidget->setClickMessage(i18n("Search or add task"));
    d->mSearchWidget->setWhatsThis(i18n("This is a combined field. As long as you do not type ENTER, it acts as a filter. Then, only tasks that match your input are shown. As soon as you type ENTER, your input is used as name to create a new task."));
    d->mSearchWidget->installEventFilter( this );
    innerLayout->addWidget(d->mSearchWidget);
    d->mSearchLine->setLayout(innerLayout);

    d->mTaskView = new TaskView(this);
    layout->addWidget(d->mSearchLine);
    layout->addWidget(d->mTaskView);
    setLayout(layout);

    showSearchBar(!KTimeTrackerSettings::configPDA() && KTimeTrackerSettings::showSearchBar());
}

TimetrackerWidget::~TimetrackerWidget()
{
    delete d;
}

bool TimetrackerWidget::allEventsHaveEndTiMe()
{
    return currentTaskView()->allEventsHaveEndTiMe();
}

int TimetrackerWidget::focusSearchBar()
{
    qCDebug(KTT_LOG) << "Entering function";
    if ( d->mSearchWidget->isVisible() ) d->mSearchWidget->setFocus();
    return 0;
}

void TimetrackerWidget::addTaskView(const QString &fileName)
{
    qCDebug(KTT_LOG) << "Entering function (fileName=" << fileName << ")";
    bool isNew = fileName.isEmpty();
    QString lFileName = fileName;

    if (isNew) {
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(false);
        if (tempFile.open()) {
            lFileName = tempFile.fileName();
            tempFile.close();
        } else {
            KMessageBox::error(this, i18n("Cannot create new file."));
            return;
        }
    }

    TaskView *taskView = d->mTaskView;

    connect( taskView, SIGNAL(contextMenuRequested(QPoint)),
           this, SIGNAL(contextMenuRequested(QPoint)) );
    connect( taskView, SIGNAL(timersActive()),
           this, SIGNAL(timersActive()) );
    connect( taskView, SIGNAL(timersInactive()),
           this, SIGNAL(timersInactive()) );
    connect( taskView, SIGNAL(tasksChanged(QList<Task*>)),
           this, SIGNAL(tasksChanged(QList<Task*>)));

    emit setCaption( fileName );
    taskView->load( lFileName );
    d->mSearchWidget->addTreeWidget( taskView );

    // When adding the first tab currentChanged is not emitted, so...
    if ( !d->mTaskView )
    {
        emit currentTaskViewChanged();
        slotCurrentChanged();
    }
}

TaskView* TimetrackerWidget::currentTaskView() const
{
    return d->mTaskView;
}

Task* TimetrackerWidget::currentTask()
{
    TaskView *taskView = 0;
    if ( ( taskView = currentTaskView() ) )
    {
        return taskView->currentItem();
    }
    else
    {
        return 0;
    }
}

void TimetrackerWidget::setupActions(KActionCollection* actionCollection)
{
    d->m_actionCollection = actionCollection;

    KStandardAction::open(this, SLOT(openFile()), actionCollection);
    KStandardAction::save(this, &TimetrackerWidget::saveFile, actionCollection);
    KStandardAction::preferences(this, &TimetrackerWidget::showSettingsDialog, actionCollection);

    QAction* startNewSession = actionCollection->addAction(QStringLiteral("start_new_session"));
    startNewSession->setText(i18n("Start &New Session"));
    startNewSession->setToolTip(i18n("Starts a new session"));
    startNewSession->setWhatsThis(i18n("This will reset the "
                                       "session time to 0 for all tasks, to start a new session, without "
                                       "affecting the totals."));
    connect(startNewSession, &QAction::triggered, this, &TimetrackerWidget::startNewSession);

    QAction* editHistory = actionCollection->addAction(QStringLiteral("edit_history"));
    editHistory->setText(i18n("Edit History..."));
    editHistory->setToolTip(i18n("Edits history of all tasks of the current document"));
    editHistory->setWhatsThis(i18n("A window will "
                                   "be opened where you can change start and stop times of tasks or add a "
                                   "comment to them."));
    editHistory->setIcon(QIcon::fromTheme("view-history"));
    connect(editHistory, &QAction::triggered, this, &TimetrackerWidget::editHistory);

    QAction* resetAllTimes = actionCollection->addAction(QStringLiteral("reset_all_times"));
    resetAllTimes->setText(i18n("&Reset All Times"));
    resetAllTimes->setToolTip(i18n("Resets all times"));
    resetAllTimes->setWhatsThis(i18n("This will reset the session "
                                     "and total time to 0 for all tasks, to restart from scratch."));
    connect(resetAllTimes, &QAction::triggered, this, &TimetrackerWidget::resetAllTimes);

    QAction* startCurrentTimer = actionCollection->addAction(QStringLiteral("start"));
    startCurrentTimer->setText(i18n("&Start"));
    startCurrentTimer->setToolTip(i18n("Starts timing for selected task"));
    startCurrentTimer->setWhatsThis(i18n("This will start timing for the "
                                         "selected task.\nIt is even possible to time several tasks "
                                         "simultanously.\n\nYou may also start timing of tasks by double clicking "
                                         "the left mouse button on a given task. This will, however, stop timing "
                                         "of other tasks."));
    startCurrentTimer->setIcon(QIcon::fromTheme("media-playback-start"));
    actionCollection->setDefaultShortcut(startCurrentTimer, QKeySequence(Qt::Key_G));
    connect(startCurrentTimer, &QAction::triggered, this, &TimetrackerWidget::startCurrentTimer);

    QAction* stopCurrentTimer = actionCollection->addAction(QStringLiteral("stop"));
    stopCurrentTimer->setText(i18n("S&top"));
    stopCurrentTimer->setToolTip(i18n("Stops timing of the selected task"));
    stopCurrentTimer->setWhatsThis(i18n("Stops timing of the selected task"));
    stopCurrentTimer->setIcon(QIcon::fromTheme("media-playback-stop"));
    connect(stopCurrentTimer, &QAction::triggered, this, &TimetrackerWidget::stopCurrentTimer);

    QAction* focusSearchBar = actionCollection->addAction(QStringLiteral("focusSearchBar"));
    focusSearchBar->setText(i18n("Focus on Searchbar"));
    focusSearchBar->setToolTip(i18n("Sets the focus on the searchbar"));
    focusSearchBar->setWhatsThis(i18n("Sets the focus on the searchbar"));
    actionCollection->setDefaultShortcut(focusSearchBar, QKeySequence(Qt::Key_S));
    connect(focusSearchBar, &QAction::triggered, this, &TimetrackerWidget::focusSearchBar);

    QAction* stopAllTimers = actionCollection->addAction(QStringLiteral("stopAll"));
    stopAllTimers->setText(i18n("Stop &All Timers"));
    stopAllTimers->setToolTip(i18n("Stops all of the active timers"));
    stopAllTimers->setWhatsThis(i18n("Stops all of the active timers"));
    actionCollection->setDefaultShortcut(stopAllTimers, QKeySequence(Qt::Key_Escape));
    connect(stopAllTimers, SIGNAL(triggered()), this, SLOT(stopAllTimers()));

    QAction* focusTracking = actionCollection->addAction(QStringLiteral("focustracking"));
    focusTracking->setCheckable(true);
    focusTracking->setText(i18n("Track Active Applications"));
    focusTracking->setToolTip(i18n("Auto-creates and updates tasks when the focus of the "
                                   "current window has changed"));
    focusTracking->setWhatsThis(i18n("If the focus of a window changes for the "
                                     "first time when this action is enabled, a new task will be created "
                                     "with the title of the window as its name and will be started. If there "
                                     "already exists such an task it will be started."));
    connect(focusTracking, &QAction::triggered, this, &TimetrackerWidget::focusTracking);

    QAction* newTask = actionCollection->addAction(QStringLiteral("new_task"));
    newTask->setText(i18n("&New Task..."));
    newTask->setToolTip(i18n("Creates new top level task"));
    newTask->setWhatsThis(i18n("This will create a new top level task."));
    newTask->setIcon(QIcon::fromTheme("document-new"));
    actionCollection->setDefaultShortcut(newTask, QKeySequence(Qt::CTRL + Qt::Key_T));
    connect(newTask, &QAction::triggered, this, &TimetrackerWidget::newTask);

    QAction* newSubTask = actionCollection->addAction(QStringLiteral("new_sub_task"));
    newSubTask->setText(i18n("New &Subtask..."));
    newSubTask->setToolTip(i18n("Creates a new subtask to the current selected task"));
    newSubTask->setWhatsThis(i18n("This will create a new subtask to the current selected task."));
    newSubTask->setIcon(QIcon::fromTheme("subtask-new-ktimetracker"));
    actionCollection->setDefaultShortcut(newSubTask, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_N));
    connect(newSubTask, &QAction::triggered, this, &TimetrackerWidget::newSubTask);

    QAction* deleteTask = actionCollection->addAction(QStringLiteral("delete_task"));
    deleteTask->setText(i18n("&Delete"));
    deleteTask->setToolTip(i18n("Deletes selected task"));
    deleteTask->setWhatsThis(i18n("This will delete the selected task(s) and all subtasks."));
    deleteTask->setIcon(QIcon::fromTheme("edit-delete"));
    actionCollection->setDefaultShortcut(deleteTask, QKeySequence(Qt::Key_Delete));
    connect(deleteTask, &QAction::triggered, this, static_cast<void(TimetrackerWidget::*)()>(&TimetrackerWidget::deleteTask));

    QAction* editTask = actionCollection->addAction(QStringLiteral("edit_task"));
    editTask->setText(i18n("&Edit..."));
    editTask->setToolTip(i18n("Edits name or times for selected task"));
    editTask->setWhatsThis(i18n("This will bring up a dialog "
                                "box where you may edit the parameters for the selected task."));
    editTask->setIcon(QIcon::fromTheme("document-properties"));
    actionCollection->setDefaultShortcut(editTask, QKeySequence(Qt::CTRL + Qt::Key_E));
    connect(editTask, &QAction::triggered, this, &TimetrackerWidget::editTask);

    QAction* markTaskAsComplete = actionCollection->addAction(QStringLiteral("mark_as_complete"));
    markTaskAsComplete->setText(i18n("&Mark as Complete"));
    markTaskAsComplete->setIcon(QPixmap(":/pics/task-complete.xpm"));
    actionCollection->setDefaultShortcut(markTaskAsComplete, QKeySequence(Qt::CTRL + Qt::Key_M));
    connect(markTaskAsComplete, &QAction::triggered, this, &TimetrackerWidget::markTaskAsComplete);

    QAction* markTaskAsIncomplete = actionCollection->addAction(QStringLiteral("mark_as_incomplete"));
    markTaskAsIncomplete->setText(i18n("&Mark as Incomplete"));
    markTaskAsIncomplete->setIcon(QPixmap(":/pics/task-incomplete.xpm"));
    actionCollection->setDefaultShortcut(markTaskAsIncomplete, QKeySequence(Qt::CTRL + Qt::Key_M));
    connect(markTaskAsIncomplete, &QAction::triggered, this, &TimetrackerWidget::markTaskAsIncomplete);

    QAction* exportcsvFile = actionCollection->addAction(QStringLiteral("export_times"));
    exportcsvFile->setText(i18n("&Export Times..."));
    connect(exportcsvFile, &QAction::triggered, this, &TimetrackerWidget::exportcsvFile);

    QAction* exportcsvHistory = actionCollection->addAction(QStringLiteral("export_history"));
    exportcsvHistory->setText(i18n("Export &History..."));
    connect(exportcsvHistory, &QAction::triggered, this, &TimetrackerWidget::exportcsvHistory);

    QAction* importPlanner = actionCollection->addAction(QStringLiteral("import_planner"));
    importPlanner->setText(i18n("Import Tasks From &Planner..."));
    connect(importPlanner, SIGNAL(triggered(bool)), this, SLOT(importPlanner()));

    QAction* showSearchBar = actionCollection->addAction(QStringLiteral("searchbar"));
    showSearchBar->setCheckable(true);
    showSearchBar->setChecked(KTimeTrackerSettings::self()->showSearchBar());
    showSearchBar->setText(i18n("Show Searchbar"));
    connect(showSearchBar, &QAction::triggered, this, &TimetrackerWidget::slotSearchBar);

    connect(this, &TimetrackerWidget::currentTaskChanged, this, &TimetrackerWidget::slotUpdateButtons);
    connect(this, &TimetrackerWidget::currentTaskViewChanged, this, &TimetrackerWidget::slotUpdateButtons);
    connect(this, &TimetrackerWidget::updateButtons, this, &TimetrackerWidget::slotUpdateButtons);
}

QAction * TimetrackerWidget::action( const QString &name ) const
{
    return d->m_actionCollection->action(name);
}

void TimetrackerWidget::openFile( const QString &fileName )
{
    qCDebug(KTT_LOG) << "Entering function, fileName is " << fileName;
    QString newFileName = fileName;
    if (newFileName.isEmpty()) {
        newFileName = QFileDialog::getOpenFileName(this);
        if (newFileName.isEmpty()) {
            return;
        }
    }
    addTaskView(newFileName);
}

void TimetrackerWidget::openFile(const QUrl& fileName)
{
    openFile(fileName.toLocalFile());
}

bool TimetrackerWidget::closeFile()
{
    qCDebug(KTT_LOG) << "Entering TimetrackerWidget::closeFile";
    TaskView* taskView = currentTaskView();

    if (taskView) {
        taskView->save();
        taskView->closeStorage();
    }

    d->mSearchWidget->removeTreeWidget(taskView);

    emit currentTaskViewChanged();
    emit setCaption(QString());
    slotCurrentChanged();

    delete taskView; // removeTab does not delete its widget.
    d->mTaskView = nullptr;
    return true;
}

void TimetrackerWidget::saveFile()
{
    currentTaskView()->save();
}

void TimetrackerWidget::showSearchBar(bool visible)
{
    d->mSearchLine->setVisible(visible);
}

bool TimetrackerWidget::closeAllFiles()
{
    qCDebug(KTT_LOG) << "Entering TimetrackerWidget::closeAllFiles";
    bool err = true;
    if (d->mTaskView) {
        d->mTaskView->stopAllTimers();
        err = closeFile();
    }
    return err;
}

void TimetrackerWidget::slotCurrentChanged()
{
    qDebug() << "entering KTimetrackerWidget::slotCurrentChanged";

    if (d->mTaskView) {
        disconnect( d->mTaskView, SIGNAL(totalTimesChanged(long,long)) );
        disconnect( d->mTaskView, SIGNAL(reSetTimes()) );
        disconnect( d->mTaskView, SIGNAL(itemSelectionChanged()) );
        disconnect( d->mTaskView, SIGNAL(updateButtons()) );
        disconnect( d->mTaskView, SIGNAL(setStatusBarText(QString)) );
        disconnect( d->mTaskView, SIGNAL(timersActive()) );
        disconnect( d->mTaskView, SIGNAL(timersInactive()) );
        disconnect( d->mTaskView, SIGNAL(tasksChanged(QList<Task*>)),
                this, SIGNAL(tasksChanged(QList<Task*>)) );
        
        connect( d->mTaskView, SIGNAL(totalTimesChanged(long,long)),
            this, SIGNAL(totalTimesChanged(long,long)) );
        connect( d->mTaskView, SIGNAL(reSetTimes()),
            this, SIGNAL(reSetTimes()) );
        connect( d->mTaskView, SIGNAL(itemSelectionChanged()),
            this, SIGNAL(currentTaskChanged()) );
        connect( d->mTaskView, SIGNAL(updateButtons()),
            this, SIGNAL(updateButtons()) );
        connect( d->mTaskView, SIGNAL(setStatusBarText(QString)), // FIXME signature
            this, SIGNAL(statusBarTextChangeRequested(QString)) );
        connect( d->mTaskView, SIGNAL(timersActive()),
            this, SIGNAL(timersActive()) );
        connect( d->mTaskView, SIGNAL(timersInactive()),
            this, SIGNAL(timersInactive()) );
        connect( d->mTaskView, SIGNAL(tasksChanged(QList<Task*>)), // FIXME signature
            this, SIGNAL(tasksChanged(QList<Task*>)) );
        emit setCaption( d->mTaskView->storage()->icalfile() );
    }
    d->mSearchWidget->setEnabled( d->mTaskView );
}

bool TimetrackerWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == d->mSearchWidget) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast< QKeyEvent* >(event);
            if ( keyEvent->key() == Qt::Key_Enter ||
                keyEvent->key() == Qt::Key_Return )
            {
                if ( !d->mSearchWidget->displayText().isEmpty() ) slotAddTask( d->mSearchWidget->displayText() );
                    return true;
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

void TimetrackerWidget::slotAddTask(const QString &taskName)
{
    TaskView *taskView = currentTaskView();
    taskView->addTask(taskName, QString(), 0, 0, DesktopList(), 0);

    d->mSearchWidget->clear();
}

void TimetrackerWidget::slotUpdateButtons()
{
    qCDebug(KTT_LOG) << "Entering function";
    Task *item = currentTask();

    action(QStringLiteral("start"))->setEnabled(item && !item->isRunning() && !item->isComplete());
    action(QStringLiteral("stop"))->setEnabled(item && item->isRunning());
    action(QStringLiteral("delete_task"))->setEnabled(item);
    action(QStringLiteral("edit_task"))->setEnabled(item);
    action(QStringLiteral("mark_as_complete"))->setEnabled(item && !item->isComplete());
    action(QStringLiteral("mark_as_incomplete"))->setEnabled(item && item->isComplete());

    action(QStringLiteral("new_task"))->setEnabled(currentTaskView());
    action(QStringLiteral("new_sub_task"))->setEnabled(currentTaskView() && currentTaskView()->count());
    action(QStringLiteral("focustracking"))->setEnabled(currentTaskView());
    action(QStringLiteral("focustracking"))->setChecked(currentTaskView() && currentTaskView()->isFocusTrackingActive());
    action(QStringLiteral("start_new_session"))->setEnabled(currentTaskView());
    action(QStringLiteral("edit_history"))->setEnabled(currentTaskView());
    action(QStringLiteral("reset_all_times"))->setEnabled(currentTaskView());
    action(QStringLiteral("export_times"))->setEnabled(currentTaskView());
    action(QStringLiteral("export_history"))->setEnabled(currentTaskView());
    action(QStringLiteral("import_planner"))->setEnabled(currentTaskView());
    action(QStringLiteral("file_save"))->setEnabled(currentTaskView());
    qCDebug(KTT_LOG) << "Leaving function";
}

void TimetrackerWidget::showSettingsDialog()
{
    qCDebug(KTT_LOG) << "Entering function";
    /* show main window b/c if this method was started from tray icon and the window
        is not visible the application quits after accepting the settings dialog.
    */
    window()->show();
    KTimeTrackerConfigDialog *dialog=new KTimeTrackerConfigDialog(i18n( "Settings" ), this);
    dialog->exec();
    delete dialog;
    KTimeTrackerSettings::self()->readConfig();

    showSearchBar(!KTimeTrackerSettings::configPDA() && KTimeTrackerSettings::showSearchBar());
    currentTaskView()->reconfigure();
}

//BEGIN wrapper slots
void TimetrackerWidget::startCurrentTimer()
{
    currentTaskView()->startCurrentTimer();
}

void TimetrackerWidget::stopCurrentTimer()
{
    currentTaskView()->stopCurrentTimer();
}

void TimetrackerWidget::stopAllTimers( const QDateTime &when )
{
    currentTaskView()->stopAllTimers(when);
}

void TimetrackerWidget::newTask()
{
    currentTaskView()->newTask();
}

void TimetrackerWidget::newSubTask()
{
    currentTaskView()->newSubTask();
}

void TimetrackerWidget::editTask()
{
    currentTaskView()->editTask();
}

void TimetrackerWidget::deleteTask()
{
    currentTaskView()->deleteTask();
}

void TimetrackerWidget::markTaskAsComplete()
{
    currentTaskView()->markTaskAsComplete();
}

void TimetrackerWidget::markTaskAsIncomplete()
{
    currentTaskView()->markTaskAsIncomplete();
}

void TimetrackerWidget::exportcsvFile()
{
    currentTaskView()->exportcsvFile();
}

void TimetrackerWidget::exportcsvHistory()
{
//    currentTaskView()->exportcsvHistory();
}

void TimetrackerWidget::importPlanner(const QString &fileName)
{
    currentTaskView()->importPlanner(fileName);
}

void TimetrackerWidget::startNewSession()
{
    currentTaskView()->startNewSession();
}

void TimetrackerWidget::editHistory()
{
    // HistoryDialog is the new HistoryDialog, but the EditHiStoryDiaLog exists as well.
    // HistoryDialog can be edited with qtcreator and qtdesigner, EditHiStoryDiaLog cannot.
    if ( currentTaskView() )
    {
        HistoryDialog *dlg = new HistoryDialog( currentTaskView() );
        if (currentTaskView()->storage()->rawevents().count()!=0) dlg->exec();
        else KMessageBox::information(0, i18nc("@info in message box", "There is no history yet. Start and stop a task and you will have an entry in your history."));
    }
}

void TimetrackerWidget::resetAllTimes()
{
    if ( currentTaskView() )
    {
        if ( KMessageBox::warningContinueCancel( this,
            i18n( "Do you really want to reset the time to zero for all tasks? This will delete the entire history." ),
            i18n( "Confirmation Required" ), KGuiItem( i18n( "Reset All Times" ) ) ) == KMessageBox::Continue )
        currentTaskView()->resetTimeForAllTasks();
    }
}

void TimetrackerWidget::focusTracking()
{
    currentTaskView()->toggleFocusTracking();
    action(QStringLiteral("focustracking"))->setChecked(currentTaskView()->isFocusTrackingActive());
}

void TimetrackerWidget::slotSearchBar()
{
    bool currentVisible = KTimeTrackerSettings::self()->showSearchBar();
    KTimeTrackerSettings::self()->setShowSearchBar(!currentVisible);
    action(QStringLiteral("searchbar"))->setChecked(!currentVisible);
    showSearchBar(!currentVisible);
}
//END

/** \defgroup dbus slots ‘‘dbus slots’’ */
/* @{ */
QString TimetrackerWidget::version() const
{
    return KTIMETRACKER_VERSION;
}

QStringList TimetrackerWidget::taskIdsFromName( const QString &taskName ) const
{
    QStringList result;

    TaskView *taskView = currentTaskView();
    if ( !taskView ) return result;
    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        Task *task = static_cast< Task* >( *it );
        if ( task && task->name() == taskName )
        {
            result << task->uid();
        }
        ++it;
    }

    return result;
}

void TimetrackerWidget::addTask( const QString &taskName )
{
    TaskView *taskView = currentTaskView();

    if ( taskView )
    {
        taskView->addTask( taskName, QString(), 0, 0, DesktopList(), 0 );
    }
}

void TimetrackerWidget::addSubTask( const QString& taskName, const QString &taskId )
{
    TaskView *taskView = currentTaskView();

    if ( taskView )
    {
        taskView->addTask( taskName, QString(), 0, 0, DesktopList(), taskView->task( taskId) );
        taskView->refresh();
    }
}

void TimetrackerWidget::deleteTask( const QString &taskId )
{
    TaskView *taskView = currentTaskView();

    if ( !taskView ) return;

    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        Task *task = static_cast< Task* >( *it );
        if ( task && task->uid() == taskId )
        {
            taskView->deleteTaskBatch( task );
        }
        ++it;
    }
}

void TimetrackerWidget::setPercentComplete( const QString &taskId, int percent )
{
    TaskView *taskView = currentTaskView();
    
    if ( !taskView ) return;

    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        Task *task = static_cast< Task* >( *it );
        if ( task && task->uid() == taskId )
        {
            task->setPercentComplete( percent, taskView->storage() );
        }
        ++it;
    }
}

int TimetrackerWidget::changeTime( const QString &taskId, int minutes )
{
    int result=0;
    QDate startDate;
    QTime startTime;
    QDateTime startDateTime;
    Task *task = 0, *t = 0;

    TaskView *taskView;

    if ( minutes <= 0 ) return KTIMETRACKER_ERR_INVALID_DURATION;

    // Find task
    taskView = currentTaskView();
    if ( taskView ) return KTIMETRACKER_ERR_UID_NOT_FOUND; //FIXME: it mimics the behaviour with the for loop, but I am not sure semantics were right. Maybe a new error code must be defined?

    QTreeWidgetItemIterator it( taskView );
    while ( *it ) {
        t = static_cast< Task* >( *it );
        if ( t && t->uid() == taskId ) {
            task = t;
            break;
        }
        ++it;
    }

    if ( !task ) result=KTIMETRACKER_ERR_UID_NOT_FOUND;
    else task->changeTime(minutes, task->taskView()->storage());
    return result;
}

QString TimetrackerWidget::error( int errorCode ) const
{
    switch ( errorCode )
    {
        case KTIMETRACKER_ERR_GENERIC_SAVE_FAILED:
            return i18n( "Save failed, most likely because the file could not be locked." );
        case KTIMETRACKER_ERR_COULD_NOT_MODIFY_RESOURCE:
            return i18n( "Could not modify calendar resource." );
        case KTIMETRACKER_ERR_MEMORY_EXHAUSTED:
            return i18n( "Out of memory--could not create object." );
        case KTIMETRACKER_ERR_UID_NOT_FOUND:
            return i18n( "UID not found." );
        case KTIMETRACKER_ERR_INVALID_DATE:
            return i18n( "Invalidate date--format is YYYY-MM-DD." );
        case KTIMETRACKER_ERR_INVALID_TIME:
            return i18n( "Invalid time--format is YYYY-MM-DDTHH:MM:SS." );
        case KTIMETRACKER_ERR_INVALID_DURATION:
            return i18n( "Invalid task duration--must be greater than zero." );
        default:
            return i18n( "Invalid error number: %1", errorCode );
    }
}

bool TimetrackerWidget::isIdleDetectionPossible() const
{
    bool result;
    IdleTimeDetector *idletimedetector1=new IdleTimeDetector(50);
    result=idletimedetector1->isIdleDetectionPossible();
    delete idletimedetector1;
    return result;
}

int TimetrackerWidget::totalMinutesForTaskId( const QString &taskId ) const
{
    TaskView *taskView = currentTaskView();
    if ( !taskView ) return -1;
    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        Task *task = static_cast< Task* >( *it );
        if ( task && task->uid() == taskId )
        {
            return task->totalTime();
        }
        ++it;
    }
    return -1;
}

void TimetrackerWidget::startTimerFor( const QString &taskId )
{
    qDebug();
        
    TaskView *taskView = currentTaskView();
    if ( !taskView ) return;
    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        Task *task = static_cast< Task* >( *it );
        if ( task && task->uid() == taskId )
        {
            taskView->startTimerFor( task );
            return;
        }
        ++it;
    }
}

bool TimetrackerWidget::startTimerForTaskName( const QString &taskName )
{
    TaskView *taskView = currentTaskView();
    if ( !taskView ) return false;
    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        Task *task = static_cast< Task* >( *it );
        if ( task && task->name() == taskName )
        {
            taskView->startTimerFor( task );
            return true;
        }
        ++it;
    }
    return false;
}


bool TimetrackerWidget::stopTimerForTaskName( const QString &taskName )
{
    TaskView *taskView = currentTaskView();
    if ( !taskView ) return false;

    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        Task *task = static_cast< Task* >( *it );

        if ( task && task->name() == taskName )
        {
            taskView->stopTimerFor( task );
            return true;
        }
        ++it;
    }
    return false;
}


void TimetrackerWidget::stopTimerFor( const QString &taskId )
{
    TaskView *taskView = currentTaskView();
    if ( !taskView ) return;
    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        Task *task = static_cast< Task* >( *it );
        if ( task && task->uid() == taskId )
        {
            taskView->stopTimerFor( task );
            return;
        }
        ++it;
    }
}

void TimetrackerWidget::stopAllTimersDBUS()
{
    TaskView *taskView = currentTaskView();
    if (taskView) taskView->stopAllTimers();
}

QString TimetrackerWidget::exportCSVFile( const QString &filename,
                                          const QString &from,
                                          const QString &to, int type,
                                          bool decimalMinutes,
                                          bool allTasks,
                                          const QString &delimiter,
                                          const QString &quote )
{
    TaskView *taskView = currentTaskView();

    if ( !taskView ) return "";
    ReportCriteria rc;
    rc.url = filename;
    rc.from = QDate::fromString( from );
    if ( rc.from.isNull() )
        rc.from = QDate::fromString( from, Qt::ISODate );
    rc.to = QDate::fromString( to );
    if ( rc.to.isNull() )
        rc.to = QDate::fromString( to, Qt::ISODate );
    rc.reportType = ( ReportCriteria::REPORTTYPE )type;
    rc.decimalMinutes = decimalMinutes;
    rc.allTasks = allTasks;
    rc.delimiter = delimiter;
    rc.quote = quote;

    return taskView->report( rc );
}

void TimetrackerWidget::importPlannerFile( const QString &filename )
{
    TaskView *taskView = currentTaskView();
    if ( !taskView ) return;
    taskView->importPlanner( filename );
}

bool TimetrackerWidget::isActive( const QString &taskId ) const
{
    TaskView *taskView = currentTaskView();
    if ( !taskView ) return false;
    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        Task *task = static_cast< Task* >( *it );

        if ( task && task->uid() == taskId )
        {
            return task->isRunning();
        }
        ++it;
    }
    return false;
}

bool TimetrackerWidget::isTaskNameActive( const QString &taskName ) const
{
    TaskView *taskView = currentTaskView();
    if ( !taskView ) return false;
    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        Task *task = static_cast< Task* >( *it );
        if ( task && task->name() == taskName )
        {
            return task->isRunning();
        }
        ++it;
    }
    return false;
}

QStringList TimetrackerWidget::tasks() const
{
    QStringList result;

    TaskView *taskView = currentTaskView();
    if ( !taskView ) return result;
    QTreeWidgetItemIterator it( taskView );
    while ( *it )
    {
        result << static_cast< Task* >( *it )->name();
        ++it;
    }
    return result;
}

QStringList TimetrackerWidget::activeTasks() const
{
    QStringList result;
    TaskView *taskView = currentTaskView();
    if ( !taskView ) return result;
    for ( int j = 0; j < taskView->count(); ++j )
    {
        if ( taskView->itemAt( j )->isRunning() )
        {
            result << taskView->itemAt( j )->name();
        }
    }
    return result;
}

void TimetrackerWidget::saveAll()
{
    currentTaskView()->save();
}

void TimetrackerWidget::quit()
{
    auto* mainWindow = dynamic_cast<MainWindow*>(parent()->parent());
    if (mainWindow) {
        mainWindow->quit();
    } else {
        qCWarning(KTT_LOG) << "Cast to MainWindow failed";
    }
}

bool TimetrackerWidget::event ( QEvent * event ) // inherited from QWidget
{
    if (event->type()==QEvent::QueryWhatsThis)
    {
        if ( d->mTaskView->count() == 0 )
            setWhatsThis( i18n("This is ktimetracker, KDE's program to help you track your time. Best, start with creating your first task - enter it into the field where you see \"search or add task\".") );
        else setWhatsThis( i18n("You have already created a task. You can now start and stop timing") );
    }
    return QWidget::event(event);
}
// END of dbus slots group
/* @} */
