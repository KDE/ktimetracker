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
#include <QTemporaryFile>

#include <KLocalizedString>
#include <KActionCollection>
#include <KConfig>
#include <KConfigDialog>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KStandardAction>
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
#include "settings/ktimetrackerconfigdialog.h"
#include "ktt_debug.h"


//@cond PRIVATE
class TimeTrackerWidget::Private
{
public:
    Private()
        : mSearchLine(nullptr)
        , mSearchWidget(nullptr)
        , mTaskView(nullptr)
        , m_actionCollection(nullptr)
    {
    }

    QWidget *mSearchLine;
    KTreeWidgetSearchLine *mSearchWidget;
    TaskView *mTaskView;
    KActionCollection *m_actionCollection;
};
//@endcond

TimeTrackerWidget::TimeTrackerWidget(QWidget* parent)
    : QWidget(parent)
    , d(new TimeTrackerWidget::Private())
{
    qCDebug(KTT_LOG) << "Entering function";
    new MainAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/KTimeTracker", this);

    QLayout* layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    QLayout* innerLayout = new QHBoxLayout;
    d->mSearchLine = new QWidget(this);
    d->mSearchWidget = new KTreeWidgetSearchLine(d->mSearchLine);
    d->mSearchWidget->setPlaceholderText(i18n("Search or add task"));
    d->mSearchWidget->setWhatsThis(i18n("This is a combined field. As long as you do not type ENTER, it acts as a filter. Then, only tasks that match your input are shown. As soon as you type ENTER, your input is used as name to create a new task."));
    d->mSearchWidget->installEventFilter(this);
    innerLayout->addWidget(d->mSearchWidget);
    d->mSearchLine->setLayout(innerLayout);

    d->mTaskView = new TaskView(this);
    layout->addWidget(d->mSearchLine);
    layout->addWidget(d->mTaskView);
    setLayout(layout);

    showSearchBar(!KTimeTrackerSettings::configPDA() && KTimeTrackerSettings::showSearchBar());
}

TimeTrackerWidget::~TimeTrackerWidget()
{
    delete d;
}

bool TimeTrackerWidget::allEventsHaveEndTiMe()
{
    return currentTaskView()->allEventsHaveEndTiMe();
}

int TimeTrackerWidget::focusSearchBar()
{
    qCDebug(KTT_LOG) << "Entering function";
    if (d->mSearchWidget->isVisible()) {
        d->mSearchWidget->setFocus();
    }
    return 0;
}

void TimeTrackerWidget::addTaskView(const QString &fileName)
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

    connect(taskView, &TaskView::contextMenuRequested, this, &TimeTrackerWidget::contextMenuRequested);
    connect(taskView, &TaskView::timersActive, this, &TimeTrackerWidget::timersActive);
    connect(taskView, &TaskView::timersInactive, this, &TimeTrackerWidget::timersInactive);
    connect(taskView, &TaskView::tasksChanged, this, &TimeTrackerWidget::tasksChanged);

    emit setCaption(fileName);
    taskView->load(lFileName);
    d->mSearchWidget->addTreeWidget(taskView);

    // When adding the first tab currentChanged is not emitted, so...
    if (!d->mTaskView) {
        emit currentTaskViewChanged();
        slotCurrentChanged();
    }
}

TaskView* TimeTrackerWidget::currentTaskView() const
{
    return d->mTaskView;
}

Task* TimeTrackerWidget::currentTask()
{
    TaskView* taskView = currentTaskView();
    return taskView ? taskView->currentItem() : nullptr;
}

void TimeTrackerWidget::setupActions(KActionCollection* actionCollection)
{
    d->m_actionCollection = actionCollection;

    KStandardAction::open(this, SLOT(openFile()), actionCollection);
    KStandardAction::save(this, &TimeTrackerWidget::saveFile, actionCollection);
    KStandardAction::preferences(this, &TimeTrackerWidget::showSettingsDialog, actionCollection);

    QAction* startNewSession = actionCollection->addAction(QStringLiteral("start_new_session"));
    startNewSession->setText(i18n("Start &New Session"));
    startNewSession->setToolTip(i18n("Starts a new session"));
    startNewSession->setWhatsThis(i18n("This will reset the "
                                       "session time to 0 for all tasks, to start a new session, without "
                                       "affecting the totals."));
    connect(startNewSession, &QAction::triggered, this, &TimeTrackerWidget::startNewSession);

    QAction* editHistory = actionCollection->addAction(QStringLiteral("edit_history"));
    editHistory->setText(i18n("Edit History..."));
    editHistory->setToolTip(i18n("Edits history of all tasks of the current document"));
    editHistory->setWhatsThis(i18n("A window will "
                                   "be opened where you can change start and stop times of tasks or add a "
                                   "comment to them."));
    editHistory->setIcon(QIcon::fromTheme("view-history"));
    connect(editHistory, &QAction::triggered, this, &TimeTrackerWidget::editHistory);

    QAction* resetAllTimes = actionCollection->addAction(QStringLiteral("reset_all_times"));
    resetAllTimes->setText(i18n("&Reset All Times"));
    resetAllTimes->setToolTip(i18n("Resets all times"));
    resetAllTimes->setWhatsThis(i18n("This will reset the session "
                                     "and total time to 0 for all tasks, to restart from scratch."));
    connect(resetAllTimes, &QAction::triggered, this, &TimeTrackerWidget::resetAllTimes);

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
    connect(startCurrentTimer, &QAction::triggered, this, &TimeTrackerWidget::startCurrentTimer);

    QAction* stopCurrentTimer = actionCollection->addAction(QStringLiteral("stop"));
    stopCurrentTimer->setText(i18n("S&top"));
    stopCurrentTimer->setToolTip(i18n("Stops timing of the selected task"));
    stopCurrentTimer->setWhatsThis(i18n("Stops timing of the selected task"));
    stopCurrentTimer->setIcon(QIcon::fromTheme("media-playback-stop"));
    connect(stopCurrentTimer, &QAction::triggered, this, &TimeTrackerWidget::stopCurrentTimer);

    QAction* focusSearchBar = actionCollection->addAction(QStringLiteral("focusSearchBar"));
    focusSearchBar->setText(i18n("Focus on Searchbar"));
    focusSearchBar->setToolTip(i18n("Sets the focus on the searchbar"));
    focusSearchBar->setWhatsThis(i18n("Sets the focus on the searchbar"));
    actionCollection->setDefaultShortcut(focusSearchBar, QKeySequence(Qt::Key_S));
    connect(focusSearchBar, &QAction::triggered, this, &TimeTrackerWidget::focusSearchBar);

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
    connect(focusTracking, &QAction::triggered, this, &TimeTrackerWidget::focusTracking);

    QAction* newTask = actionCollection->addAction(QStringLiteral("new_task"));
    newTask->setText(i18n("&New Task..."));
    newTask->setToolTip(i18n("Creates new top level task"));
    newTask->setWhatsThis(i18n("This will create a new top level task."));
    newTask->setIcon(QIcon::fromTheme("document-new"));
    actionCollection->setDefaultShortcut(newTask, QKeySequence(Qt::CTRL + Qt::Key_T));
    connect(newTask, &QAction::triggered, this, &TimeTrackerWidget::newTask);

    QAction* newSubTask = actionCollection->addAction(QStringLiteral("new_sub_task"));
    newSubTask->setText(i18n("New &Subtask..."));
    newSubTask->setToolTip(i18n("Creates a new subtask to the current selected task"));
    newSubTask->setWhatsThis(i18n("This will create a new subtask to the current selected task."));
    newSubTask->setIcon(QIcon::fromTheme("subtask-new-ktimetracker"));
    actionCollection->setDefaultShortcut(newSubTask, QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_N));
    connect(newSubTask, &QAction::triggered, this, &TimeTrackerWidget::newSubTask);

    QAction* deleteTask = actionCollection->addAction(QStringLiteral("delete_task"));
    deleteTask->setText(i18n("&Delete"));
    deleteTask->setToolTip(i18n("Deletes selected task"));
    deleteTask->setWhatsThis(i18n("This will delete the selected task(s) and all subtasks."));
    deleteTask->setIcon(QIcon::fromTheme("edit-delete"));
    actionCollection->setDefaultShortcut(deleteTask, QKeySequence(Qt::Key_Delete));
    connect(deleteTask, &QAction::triggered, this, static_cast<void(TimeTrackerWidget::*)()>(&TimeTrackerWidget::deleteTask));

    QAction* editTask = actionCollection->addAction(QStringLiteral("edit_task"));
    editTask->setText(i18n("&Edit..."));
    editTask->setToolTip(i18n("Edits name or times for selected task"));
    editTask->setWhatsThis(i18n("This will bring up a dialog "
                                "box where you may edit the parameters for the selected task."));
    editTask->setIcon(QIcon::fromTheme("document-properties"));
    actionCollection->setDefaultShortcut(editTask, QKeySequence(Qt::CTRL + Qt::Key_E));
    connect(editTask, &QAction::triggered, this, &TimeTrackerWidget::editTask);

    QAction* markTaskAsComplete = actionCollection->addAction(QStringLiteral("mark_as_complete"));
    markTaskAsComplete->setText(i18n("&Mark as Complete"));
    markTaskAsComplete->setIcon(QPixmap(":/pics/task-complete.xpm"));
    actionCollection->setDefaultShortcut(markTaskAsComplete, QKeySequence(Qt::CTRL + Qt::Key_M));
    connect(markTaskAsComplete, &QAction::triggered, this, &TimeTrackerWidget::markTaskAsComplete);

    QAction* markTaskAsIncomplete = actionCollection->addAction(QStringLiteral("mark_as_incomplete"));
    markTaskAsIncomplete->setText(i18n("&Mark as Incomplete"));
    markTaskAsIncomplete->setIcon(QPixmap(":/pics/task-incomplete.xpm"));
    actionCollection->setDefaultShortcut(markTaskAsIncomplete, QKeySequence(Qt::CTRL + Qt::Key_M));
    connect(markTaskAsIncomplete, &QAction::triggered, this, &TimeTrackerWidget::markTaskAsIncomplete);

    QAction* exportcsvFile = actionCollection->addAction(QStringLiteral("export_times"));
    exportcsvFile->setText(i18n("&Export Times..."));
    connect(exportcsvFile, &QAction::triggered, this, &TimeTrackerWidget::exportcsvFile);

    QAction* exportcsvHistory = actionCollection->addAction(QStringLiteral("export_history"));
    exportcsvHistory->setText(i18n("Export &History..."));
    connect(exportcsvHistory, &QAction::triggered, this, &TimeTrackerWidget::exportcsvHistory);

    QAction* importPlanner = actionCollection->addAction(QStringLiteral("import_planner"));
    importPlanner->setText(i18n("Import Tasks From &Planner..."));
    connect(importPlanner, SIGNAL(triggered(bool)), this, SLOT(importPlanner()));

    QAction* showSearchBar = actionCollection->addAction(QStringLiteral("searchbar"));
    showSearchBar->setCheckable(true);
    showSearchBar->setChecked(KTimeTrackerSettings::self()->showSearchBar());
    showSearchBar->setText(i18n("Show Searchbar"));
    connect(showSearchBar, &QAction::triggered, this, &TimeTrackerWidget::slotSearchBar);

    connect(this, &TimeTrackerWidget::currentTaskChanged, this, &TimeTrackerWidget::slotUpdateButtons);
    connect(this, &TimeTrackerWidget::currentTaskViewChanged, this, &TimeTrackerWidget::slotUpdateButtons);
    connect(this, &TimeTrackerWidget::updateButtons, this, &TimeTrackerWidget::slotUpdateButtons);
}

QAction * TimeTrackerWidget::action(const QString& name) const
{
    return d->m_actionCollection->action(name);
}

void TimeTrackerWidget::openFile(const QString& fileName)
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

void TimeTrackerWidget::openFile(const QUrl& fileName)
{
    openFile(fileName.toLocalFile());
}

bool TimeTrackerWidget::closeFile()
{
    qCDebug(KTT_LOG) << "Entering TimeTrackerWidget::closeFile";
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

void TimeTrackerWidget::saveFile()
{
    currentTaskView()->save();
}

void TimeTrackerWidget::showSearchBar(bool visible)
{
    d->mSearchLine->setVisible(visible);
}

bool TimeTrackerWidget::closeAllFiles()
{
    qCDebug(KTT_LOG) << "Entering TimeTrackerWidget::closeAllFiles";
    bool err = true;
    if (d->mTaskView) {
        d->mTaskView->stopAllTimers();
        err = closeFile();
    }
    return err;
}

void TimeTrackerWidget::slotCurrentChanged()
{
    qDebug() << "entering KTimeTrackerWidget::slotCurrentChanged";

    if (d->mTaskView) {
        disconnect(d->mTaskView, SLOT(totalTimesChanged(long, long)));
        disconnect(d->mTaskView, SLOT(reSetTimes()));
        disconnect(d->mTaskView, SLOT(itemSelectionChanged()));
        disconnect(d->mTaskView, SLOT(updateButtons()));
        disconnect(d->mTaskView, SLOT(setStatusBarText(QString)));
        disconnect(d->mTaskView, SLOT(timersActive()));
        disconnect(d->mTaskView, SLOT(timersInactive()));
        disconnect(d->mTaskView, &TaskView::tasksChanged, this, &TimeTrackerWidget::tasksChanged);

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
        emit setCaption(d->mTaskView->storage()->icalfile());
    }
    d->mSearchWidget->setEnabled(d->mTaskView);
}

bool TimeTrackerWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == d->mSearchWidget && event->type() == QEvent::KeyPress) {
        auto* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if (keyEvent && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)) {
            if (!d->mSearchWidget->displayText().isEmpty()) {
                slotAddTask( d->mSearchWidget->displayText());
            }

            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

void TimeTrackerWidget::slotAddTask(const QString &taskName)
{
    TaskView *taskView = currentTaskView();
    taskView->addTask(taskName, QString(), 0, 0, DesktopList(), 0);

    d->mSearchWidget->clear();
}

void TimeTrackerWidget::slotUpdateButtons()
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

void TimeTrackerWidget::showSettingsDialog()
{
    if (KConfigDialog::showDialog("settings")) {
        return;
    }

    KConfigDialog *dialog = new KConfigDialog(this, "settings", KTimeTrackerSettings::self());
    dialog->setFaceType(KPageDialog::List);
    dialog->addPage(new KTimeTrackerBehaviorConfig(dialog), i18nc("@title:tab", "Behavior"), QStringLiteral("preferences-other"));
    dialog->addPage(new KTimeTrackerDisplayConfig(dialog), i18nc("@title:tab", "Appearance"), QStringLiteral("preferences-desktop-theme"));
    dialog->addPage(new KTimeTrackerStorageConfig(dialog), i18nc("@title:tab", "Storage"), QStringLiteral("system-file-manager"));
    connect(dialog, SIGNAL(settingsChanged(const QString&)), this, SLOT(loadSettings()));
    dialog->show();
}

void TimeTrackerWidget::loadSettings()
{
    KTimeTrackerSettings::self()->readConfig();

    showSearchBar(!KTimeTrackerSettings::configPDA() && KTimeTrackerSettings::showSearchBar());
    currentTaskView()->reconfigure();
}

//BEGIN wrapper slots
void TimeTrackerWidget::startCurrentTimer()
{
    currentTaskView()->startCurrentTimer();
}

void TimeTrackerWidget::stopCurrentTimer()
{
    currentTaskView()->stopCurrentTimer();
}

void TimeTrackerWidget::stopAllTimers(const QDateTime& when)
{
    currentTaskView()->stopAllTimers(when);
}

void TimeTrackerWidget::newTask()
{
    currentTaskView()->newTask();
}

void TimeTrackerWidget::newSubTask()
{
    currentTaskView()->newSubTask();
}

void TimeTrackerWidget::editTask()
{
    currentTaskView()->editTask();
}

void TimeTrackerWidget::deleteTask()
{
    currentTaskView()->deleteTask();
}

void TimeTrackerWidget::markTaskAsComplete()
{
    currentTaskView()->markTaskAsComplete();
}

void TimeTrackerWidget::markTaskAsIncomplete()
{
    currentTaskView()->markTaskAsIncomplete();
}

void TimeTrackerWidget::exportcsvFile()
{
    currentTaskView()->exportcsvFile();
}

void TimeTrackerWidget::exportcsvHistory()
{
//    currentTaskView()->exportcsvHistory();
}

void TimeTrackerWidget::importPlanner(const QString &fileName)
{
    currentTaskView()->importPlanner(fileName);
}

void TimeTrackerWidget::startNewSession()
{
    currentTaskView()->startNewSession();
}

void TimeTrackerWidget::editHistory()
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

void TimeTrackerWidget::resetAllTimes()
{
    if ( currentTaskView() )
    {
        if ( KMessageBox::warningContinueCancel( this,
            i18n( "Do you really want to reset the time to zero for all tasks? This will delete the entire history." ),
            i18n( "Confirmation Required" ), KGuiItem( i18n( "Reset All Times" ) ) ) == KMessageBox::Continue )
        currentTaskView()->resetTimeForAllTasks();
    }
}

void TimeTrackerWidget::focusTracking()
{
    currentTaskView()->toggleFocusTracking();
    action(QStringLiteral("focustracking"))->setChecked(currentTaskView()->isFocusTrackingActive());
}

void TimeTrackerWidget::slotSearchBar()
{
    bool currentVisible = KTimeTrackerSettings::self()->showSearchBar();
    KTimeTrackerSettings::self()->setShowSearchBar(!currentVisible);
    action(QStringLiteral("searchbar"))->setChecked(!currentVisible);
    showSearchBar(!currentVisible);
}
//END

/** \defgroup dbus slots ‘‘dbus slots’’ */
/* @{ */
QString TimeTrackerWidget::version() const
{
    return KTIMETRACKER_VERSION;
}

QStringList TimeTrackerWidget::taskIdsFromName( const QString &taskName ) const
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

void TimeTrackerWidget::addTask( const QString &taskName )
{
    TaskView *taskView = currentTaskView();

    if ( taskView )
    {
        taskView->addTask( taskName, QString(), 0, 0, DesktopList(), 0 );
    }
}

void TimeTrackerWidget::addSubTask( const QString& taskName, const QString &taskId )
{
    TaskView *taskView = currentTaskView();

    if ( taskView )
    {
        taskView->addTask( taskName, QString(), 0, 0, DesktopList(), taskView->task( taskId) );
        taskView->refresh();
    }
}

void TimeTrackerWidget::deleteTask( const QString &taskId )
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

void TimeTrackerWidget::setPercentComplete( const QString &taskId, int percent )
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

int TimeTrackerWidget::changeTime( const QString &taskId, int minutes )
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

QString TimeTrackerWidget::error( int errorCode ) const
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

bool TimeTrackerWidget::isIdleDetectionPossible() const
{
    bool result;
    IdleTimeDetector *idletimedetector1=new IdleTimeDetector(50);
    result=idletimedetector1->isIdleDetectionPossible();
    delete idletimedetector1;
    return result;
}

int TimeTrackerWidget::totalMinutesForTaskId( const QString &taskId ) const
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

void TimeTrackerWidget::startTimerFor( const QString &taskId )
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

bool TimeTrackerWidget::startTimerForTaskName( const QString &taskName )
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


bool TimeTrackerWidget::stopTimerForTaskName( const QString &taskName )
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


void TimeTrackerWidget::stopTimerFor( const QString &taskId )
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

void TimeTrackerWidget::stopAllTimersDBUS()
{
    TaskView *taskView = currentTaskView();
    if (taskView) taskView->stopAllTimers();
}

QString TimeTrackerWidget::exportCSVFile(
    const QString &filename, const QString &from, const QString &to, int type,
    bool decimalMinutes, bool allTasks, const QString &delimiter, const QString &quote)
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

void TimeTrackerWidget::importPlannerFile(const QString& filename)
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return;
    }

    taskView->importPlanner(filename);
}

bool TimeTrackerWidget::isActive(const QString& taskId) const
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return false;
    }

    QTreeWidgetItemIterator it(taskView);
    while (*it) {
        Task* task = static_cast<Task*>(*it);

        if (task && task->uid() == taskId) {
            return task->isRunning();
        }
        ++it;
    }
    return false;
}

bool TimeTrackerWidget::isTaskNameActive( const QString &taskName ) const
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return false;
    }

    QTreeWidgetItemIterator it(taskView);
    while (*it) {
        Task* task = static_cast<Task*>(*it);
        if (task && task->name() == taskName) {
            return task->isRunning();
        }
        ++it;
    }
    return false;
}

QStringList TimeTrackerWidget::tasks() const
{
    QStringList result;

    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return result;
    }

    QTreeWidgetItemIterator it(taskView);
    while (*it) {
        result << static_cast<Task*>(*it)->name();
        ++it;
    }
    return result;
}

QStringList TimeTrackerWidget::activeTasks() const
{
    QStringList result;
    TaskView* taskView = currentTaskView();
    if (!taskView) {
        return result;
    }

    for (int j = 0; j < taskView->count(); ++j) {
        if (taskView->itemAt(j)->isRunning()) {
            result << taskView->itemAt(j)->name();
        }
    }
    return result;
}

void TimeTrackerWidget::saveAll()
{
    currentTaskView()->save();
}

void TimeTrackerWidget::quit()
{
    auto* mainWindow = dynamic_cast<MainWindow*>(parent()->parent());
    if (mainWindow) {
        mainWindow->quit();
    } else {
        qCWarning(KTT_LOG) << "Cast to MainWindow failed";
    }
}

bool TimeTrackerWidget::event(QEvent* event) // inherited from QWidget
{
    if (event->type() == QEvent::QueryWhatsThis) {
        if ( d->mTaskView->count() == 0 ) {
            setWhatsThis(i18n("This is ktimetracker, KDE's program to help you track your time. Best, start with creating your first task - enter it into the field where you see \"search or add task\"."));
        } else {
            setWhatsThis(i18n("You have already created a task. You can now start and stop timing"));
        }
    }

    return QWidget::event(event);
}
// END of dbus slots group
/* @} */
