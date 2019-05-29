/*
 * Copyright (C) 2007 by Mathias Soeken <msoeken@tzi.de>
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

#include "timetrackerwidget.h"

#include <QFileDialog>

#include <KActionCollection>
#include <KConfigDialog>
#include <KMessageBox>
#include <KStandardAction>

#include "model/task.h"
#include "model/eventsmodel.h"
#include "model/tasksmodel.h"
#include "model/projectmodel.h"
#include "settings/ktimetrackerconfigdialog.h"
#include "widgets/searchline.h"
#include "historydialog.h"
#include "idletimedetector.h"
#include "ktimetrackerutility.h"
#include "ktimetracker.h"
#include "mainadaptor.h"
#include "reportcriteria.h"
#include "taskview.h"
#include "widgets/taskswidget.h"
#include "ktimetracker-version.h"
#include "mainwindow.h"
#include "ktt_debug.h"
#include "csvexportdialog.h"

TimeTrackerWidget::TimeTrackerWidget(QWidget *parent)
    : QWidget(parent)
    , m_searchLine(nullptr)
    , m_taskView(new TaskView(this))
    , m_actionCollection(nullptr)
{
    qCDebug(KTT_LOG) << "Entering function";
    new MainAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/KTimeTracker", this);

    QLayout* layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);

    fillLayout(nullptr);
}

void TimeTrackerWidget::fillLayout(TasksWidget *tasksWidget)
{
    // Remove all items from layout
    QLayoutItem *child;
    while ((child = layout()->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    if (tasksWidget) {
        m_searchLine = new SearchLine(this);
        connect(m_searchLine, &SearchLine::textSubmitted, this, &TimeTrackerWidget::slotAddTask);
        connect(m_searchLine, &SearchLine::searchUpdated, tasksWidget, &TasksWidget::setFilterText);
        layout()->addWidget(m_searchLine);

        layout()->addWidget(tasksWidget);

        showSearchBar(!KTimeTrackerSettings::configPDA() && KTimeTrackerSettings::showSearchBar());
    } else {
        auto *placeholderWidget = new QWidget(this);
        placeholderWidget->setBackgroundRole(QPalette::Dark);
        placeholderWidget->setAutoFillBackground(true);
        layout()->addWidget(placeholderWidget);
    }
}

bool TimeTrackerWidget::allEventsHaveEndTiMe()
{
    return currentTaskView()->allEventsHaveEndTiMe();
}

int TimeTrackerWidget::focusSearchBar()
{
    if (m_searchLine->isEnabled()) {
        m_searchLine->setFocus();
    }
    return 0;
}

void TimeTrackerWidget::addTaskView(const QUrl &url)
{
    qCDebug(KTT_LOG) << "Entering function (url=" << url << ")";
    bool isNew = url.isEmpty();
    QUrl lFileName = url;

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

    TaskView *taskView = m_taskView;

    connect(taskView, &TaskView::contextMenuRequested, this, &TimeTrackerWidget::contextMenuRequested);
    connect(taskView, &TaskView::timersActive, this, &TimeTrackerWidget::timersActive);
    connect(taskView, &TaskView::timersInactive, this, &TimeTrackerWidget::timersInactive);
    connect(taskView, &TaskView::tasksChanged, this, &TimeTrackerWidget::tasksChanged);

    emit setCaption(url.toString());
    taskView->load(lFileName);

    fillLayout(m_taskView->tasksWidget());

    // When adding the first tab currentChanged is not emitted, so...
    if (!m_taskView) {
        emit currentTaskViewChanged();
        slotCurrentChanged();
    }
}

TaskView* TimeTrackerWidget::currentTaskView() const
{
    return m_taskView;
}

Task* TimeTrackerWidget::currentTask()
{
    TaskView* taskView = currentTaskView();
    return taskView ? taskView->tasksWidget()->currentItem() : nullptr;
}

void TimeTrackerWidget::setupActions(KActionCollection* actionCollection)
{
    Q_INIT_RESOURCE(pics);

    m_actionCollection = actionCollection;

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
    connect(deleteTask, &QAction::triggered, this, QOverload<>::of(&TimeTrackerWidget::deleteTask));

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
    actionCollection->setDefaultShortcut(markTaskAsIncomplete, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));
    connect(markTaskAsIncomplete, &QAction::triggered, this, &TimeTrackerWidget::markTaskAsIncomplete);

    QAction* exportAction = actionCollection->addAction(QStringLiteral("export_dialog"));
    exportAction->setText(i18n("&Export..."));
    exportAction->setIcon(QIcon::fromTheme("document-export"));
    connect(exportAction, &QAction::triggered, this, &TimeTrackerWidget::exportDialog);

    QAction* importPlanner = actionCollection->addAction(QStringLiteral("import_planner"));
    importPlanner->setText(i18n("Import Tasks From &Planner..."));
    importPlanner->setIcon(QIcon::fromTheme("document-import"));
    connect(importPlanner, &QAction::triggered, [=]() {
        const QString &fileName = QFileDialog::getOpenFileName();
        importPlannerFile(fileName);
    });

    QAction* showSearchBar = actionCollection->addAction(QStringLiteral("searchbar"));
    showSearchBar->setCheckable(true);
    showSearchBar->setChecked(KTimeTrackerSettings::showSearchBar());
    showSearchBar->setText(i18n("Show Searchbar"));
    connect(showSearchBar, &QAction::triggered, this, &TimeTrackerWidget::slotSearchBar);

    connect(this, &TimeTrackerWidget::currentTaskChanged, this, &TimeTrackerWidget::slotUpdateButtons);
    connect(this, &TimeTrackerWidget::currentTaskViewChanged, this, &TimeTrackerWidget::slotUpdateButtons);
    connect(this, &TimeTrackerWidget::updateButtons, this, &TimeTrackerWidget::slotUpdateButtons);
}

QAction *TimeTrackerWidget::action(const QString &name) const
{
    return m_actionCollection->action(name);
}

void TimeTrackerWidget::openFile(const QUrl &url)
{
    qCDebug(KTT_LOG) << "Entering function, url is " << url;
    QUrl newUrl = url;
    if (newUrl.isEmpty()) {
        const QString &path = QFileDialog::getOpenFileName(this);
        if (path.isEmpty()) {
            return;
        } else {
            newUrl = QUrl::fromLocalFile(path);
        }
    }
    addTaskView(newUrl);
}

bool TimeTrackerWidget::closeFile()
{
    qCDebug(KTT_LOG) << "Entering TimeTrackerWidget::closeFile";
    TaskView* taskView = currentTaskView();

    if (taskView) {
        taskView->save();
        taskView->closeStorage();
    }

    emit currentTaskViewChanged();
    emit setCaption(QString());
    slotCurrentChanged();

    delete taskView; // removeTab does not delete its widget.
    m_taskView = nullptr;
    return true;
}

void TimeTrackerWidget::saveFile()
{
    currentTaskView()->save();
}

void TimeTrackerWidget::showSearchBar(bool visible)
{
    m_searchLine->setVisible(visible);
}

bool TimeTrackerWidget::closeAllFiles()
{
    qCDebug(KTT_LOG) << "Entering TimeTrackerWidget::closeAllFiles";
    bool err = true;
    if (m_taskView) {
        m_taskView->stopAllTimers();
        err = closeFile();
    }
    return err;
}

void TimeTrackerWidget::slotCurrentChanged()
{
    qDebug() << "entering KTimeTrackerWidget::slotCurrentChanged";

    if (m_taskView) {
        connect(m_taskView, &TaskView::updateButtons, this, &TimeTrackerWidget::updateButtons, Qt::UniqueConnection);
        connect(m_taskView, &TaskView::setStatusBarText, this, &TimeTrackerWidget::statusBarTextChangeRequested, Qt::UniqueConnection);
        connect(m_taskView, &TaskView::timersActive, this, &TimeTrackerWidget::timersActive, Qt::UniqueConnection);
        connect(m_taskView, &TaskView::timersInactive, this, &TimeTrackerWidget::timersInactive, Qt::UniqueConnection);
        connect(m_taskView, &TaskView::tasksChanged, this, &TimeTrackerWidget::tasksChanged, Qt::UniqueConnection);

        emit setCaption(m_taskView->storage()->fileUrl().toString());
    }
    m_searchLine->setEnabled(m_taskView);
}

void TimeTrackerWidget::slotAddTask(const QString &taskName)
{
    TaskView *taskView = currentTaskView();
    taskView->addTask(taskName, QString(), 0, 0, DesktopList(), nullptr);
}

void TimeTrackerWidget::slotUpdateButtons()
{
    Task *item = currentTask();

    action(QStringLiteral("start"))->setEnabled(item && !item->isRunning() && !item->isComplete());
    action(QStringLiteral("stop"))->setEnabled(item && item->isRunning());
    action(QStringLiteral("delete_task"))->setEnabled(item);
    action(QStringLiteral("edit_task"))->setEnabled(item);
    action(QStringLiteral("mark_as_complete"))->setEnabled(item && !item->isComplete());
    action(QStringLiteral("mark_as_incomplete"))->setEnabled(item && item->isComplete());

    action(QStringLiteral("new_task"))->setEnabled(currentTaskView());
    action(QStringLiteral("new_sub_task"))->setEnabled(currentTaskView() && currentTaskView()->storage()->isLoaded() && currentTaskView()->storage()->tasksModel()->getAllTasks().size());
    action(QStringLiteral("focustracking"))->setEnabled(currentTaskView());
    action(QStringLiteral("focustracking"))->setChecked(currentTaskView() && currentTaskView()->isFocusTrackingActive());
    action(QStringLiteral("start_new_session"))->setEnabled(currentTaskView());
    action(QStringLiteral("edit_history"))->setEnabled(currentTaskView());
    action(QStringLiteral("reset_all_times"))->setEnabled(currentTaskView());
    action(QStringLiteral("export_dialog"))->setEnabled(currentTaskView());
    action(QStringLiteral("import_planner"))->setEnabled(currentTaskView());
    action(QStringLiteral("file_save"))->setEnabled(currentTaskView());
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
    KTimeTrackerSettings::self()->load();

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

void TimeTrackerWidget::exportDialog()
{
    qCDebug(KTT_LOG) << "TimeTrackerWidget::exportDialog()";

    auto *taskView = currentTaskView();
    CSVExportDialog dialog(taskView->tasksWidget(), taskView);
    if (taskView->tasksWidget()->currentItem() && taskView->tasksWidget()->currentItem()->isRoot()) {
        dialog.enableTasksToExportQuestion();
    }
    dialog.exec();
}

void TimeTrackerWidget::startNewSession()
{
    currentTaskView()->startNewSession();
}

void TimeTrackerWidget::editHistory()
{
    // HistoryDialog is the new HistoryDialog, but the EditHiStoryDiaLog exists as well.
    // HistoryDialog can be edited with qtcreator and qtdesigner, EditHiStoryDiaLog cannot.
    if (currentTaskView()) {
        auto *dialog = new HistoryDialog(currentTaskView()->tasksWidget(), currentTaskView()->storage()->projectModel());
        if (currentTaskView()->storage()->eventsModel()->events().count() != 0) {
            dialog->exec();
        } else {
            KMessageBox::information(nullptr, i18nc("@info in message box", "There is no history yet. Start and stop a task and you will have an entry in your history."));
        }
    }
}

void TimeTrackerWidget::resetAllTimes()
{
    if (currentTaskView()) {
        if (KMessageBox::warningContinueCancel(
            this,
            i18n("Do you really want to reset the time to zero for all tasks? This will delete the entire history."),
            i18n("Confirmation Required"),
            KGuiItem(i18n("Reset All Times"))) == KMessageBox::Continue) {
            currentTaskView()->resetTimeForAllTasks();
        }
    }
}

void TimeTrackerWidget::focusTracking()
{
    currentTaskView()->toggleFocusTracking();
    action(QStringLiteral("focustracking"))->setChecked(currentTaskView()->isFocusTrackingActive());
}

void TimeTrackerWidget::slotSearchBar()
{
    bool currentVisible = KTimeTrackerSettings::showSearchBar();
    KTimeTrackerSettings::setShowSearchBar(!currentVisible);
    action(QStringLiteral("searchbar"))->setChecked(!currentVisible);
    showSearchBar(!currentVisible);
}
//END

/** \defgroup dbus slots ‘‘dbus slots’’ */
/* @{ */
QString TimeTrackerWidget::version() const
{
    return KTIMETRACKER_VERSION_STRING;
}

QStringList TimeTrackerWidget::taskIdsFromName( const QString &taskName ) const
{
    QStringList result;

    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return result;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->name() == taskName) {
            result << task->uid();
        }
    }

    return result;
}

void TimeTrackerWidget::addTask( const QString &taskName )
{
    TaskView *taskView = currentTaskView();

    if (taskView) {
        taskView->addTask(taskName, QString(), 0, 0, DesktopList(), nullptr);
    }
}

void TimeTrackerWidget::addSubTask(const QString &taskName, const QString &taskId)
{
    TaskView *taskView = currentTaskView();

    if (taskView) {
        taskView->addTask(taskName, QString(), 0, 0, DesktopList(), taskView->storage()->tasksModel()->taskByUID(taskId));
        taskView->refresh();
    }
}

void TimeTrackerWidget::deleteTask(const QString &taskId)
{
    TaskView *taskView = currentTaskView();

    if (!taskView) {
        return;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->uid() == taskId) {
            taskView->deleteTaskBatch(task);
        }
    }
}

void TimeTrackerWidget::setPercentComplete(const QString &taskId, int percent)
{
    TaskView *taskView = currentTaskView();
    
    if (!taskView) {
        return;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->uid() == taskId) {
            task->setPercentComplete(percent);
        }
    }
}

int TimeTrackerWidget::bookTime(const QString &taskId, const QString &dateTime, int minutes)
{
    QDate startDate;
    QTime startTime;
    QDateTime startDateTime;

    if (minutes <= 0) {
        return KTIMETRACKER_ERR_INVALID_DURATION;
    }

    Task *task = nullptr;
    TaskView *taskView = currentTaskView();
    if (taskView) {
        for (Task *t : taskView->storage()->tasksModel()->getAllTasks()) {
            if (t->uid() == taskId) {
                task = t;
                break;
            }
        }
    }

    if (!task) {
        return KTIMETRACKER_ERR_UID_NOT_FOUND;
    }

    // Parse datetime
    startDate = QDate::fromString(dateTime, Qt::ISODate);

    if (dateTime.length() > 10) { // "YYYY-MM-DD".length() = 10
        startTime = QTime::fromString(dateTime, Qt::ISODate);
    } else {
        startTime = QTime(12, 0);
    }

    if (startDate.isValid() && startTime.isValid()) {
        startDateTime = QDateTime(startDate, startTime);
    } else {
        return KTIMETRACKER_ERR_INVALID_DATE;
    }

    // Update task totals (session and total) and save to disk
    task->changeTotalTimes(task->sessionTime() + minutes, task->totalTime() + minutes);
    if (!taskView->storage()->bookTime(task, startDateTime, minutes * 60)) {
        return KTIMETRACKER_ERR_GENERIC_SAVE_FAILED;
    }

    return 0;
}

int TimeTrackerWidget::changeTime(const QString &taskId, int minutes)
{
    if (minutes <= 0) {
        return KTIMETRACKER_ERR_INVALID_DURATION;
    }

    // Find task
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        //FIXME: it mimics the behaviour with the for loop, but I am not sure semantics were right. Maybe a new error code must be defined?
        return KTIMETRACKER_ERR_UID_NOT_FOUND;
    }

    Task *task = nullptr;
    for (Task *t : taskView->storage()->tasksModel()->getAllTasks()) {
        if (t->uid() == taskId) {
            task = t;
            break;
        }
    }

    if (!task) {
        return KTIMETRACKER_ERR_UID_NOT_FOUND;
    }

    task->changeTime(minutes, taskView->storage()->eventsModel());
    taskView->scheduleSave();
    return 0;
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
    return IdleTimeDetector::isIdleDetectionPossible();
}

int TimeTrackerWidget::totalMinutesForTaskId(const QString &taskId) const
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return -1;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->uid() == taskId) {
            return task->totalTime();
        }
    }

    return -1;
}

void TimeTrackerWidget::startTimerFor(const QString &taskId)
{
    qDebug();

    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->uid() == taskId) {
            taskView->startTimerFor(task);
            return;
        }
    }
}

bool TimeTrackerWidget::startTimerForTaskName( const QString &taskName )
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return false;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->name() == taskName ) {
            taskView->startTimerFor(task);
            return true;
        }
    }

    return false;
}

bool TimeTrackerWidget::stopTimerForTaskName(const QString &taskName)
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return false;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->name() == taskName) {
            taskView->stopTimerFor(task);
            return true;
        }
    }

    return false;
}

void TimeTrackerWidget::stopTimerFor(const QString &taskId)
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->uid() == taskId) {
            taskView->stopTimerFor(task);
            return;
        }
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

    return taskView->report(rc, filename);
}

void TimeTrackerWidget::importPlannerFile(const QString &filename)
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return;
    }

    taskView->importPlanner(filename);
}

bool TimeTrackerWidget::isActive(const QString &taskId) const
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return false;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->uid() == taskId) {
            return task->isRunning();
        }
    }

    return false;
}

bool TimeTrackerWidget::isTaskNameActive(const QString &taskName) const
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return false;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->name() == taskName) {
            return task->isRunning();
        }
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

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        result << task->name();
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

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->isRunning()) {
            result << task->name();
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
        if (m_taskView->storage()->tasksModel()->getAllTasks().empty()) {
            setWhatsThis(i18n("This is ktimetracker, KDE's program to help you track your time. Best, start with creating your first task - enter it into the field where you see \"search or add task\"."));
        } else {
            setWhatsThis(i18n("You have already created a task. You can now start and stop timing"));
        }
    }

    return QWidget::event(event);
}
// END of dbus slots group
/* @} */
