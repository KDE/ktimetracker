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
#include <QPointer>

#include <KActionCollection>
#include <KConfigDialog>
#include <KMessageBox>
#include <KStandardAction>

#include "dialogs/edittimedialog.h"
#include "dialogs/exportdialog.h"
#include "dialogs/historydialog.h"
#include "export/export.h"
#include "idletimedetector.h"
#include "ktimetracker-version.h"
#include "ktimetracker.h"
#include "ktimetrackerutility.h"
#include "ktt_debug.h"
#include "mainwindow.h"
#include "model/eventsmodel.h"
#include "model/projectmodel.h"
#include "model/task.h"
#include "model/tasksmodel.h"
#include "reportcriteria.h"
#include "settings/ktimetrackerconfigdialog.h"
#include "taskview.h"
#include "widgets/searchline.h"
#include "widgets/taskswidget.h"

TimeTrackerWidget::TimeTrackerWidget(QWidget *parent)
    : QWidget(parent)
    , m_searchLine(nullptr)
    , m_taskView(nullptr)
    , m_actionCollection(nullptr)
{
    registerDBus();

    QLayout *layout = new QVBoxLayout;
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

    closeFile();

    m_taskView = new TaskView(this);
    connect(m_taskView, &TaskView::contextMenuRequested, this, &TimeTrackerWidget::contextMenuRequested);
    connect(m_taskView, &TaskView::timersActive, this, &TimeTrackerWidget::timersActive);
    connect(m_taskView, &TaskView::timersInactive, this, &TimeTrackerWidget::timersInactive);
    connect(m_taskView, &TaskView::tasksChanged, this, &TimeTrackerWidget::tasksChanged);

    emit currentFileChanged(url.toString());
    m_taskView->load(url);

    fillLayout(m_taskView->tasksWidget());

    emit currentTaskViewChanged();
    slotCurrentChanged();
}

TaskView *TimeTrackerWidget::currentTaskView() const
{
    return m_taskView;
}

Task *TimeTrackerWidget::currentTask()
{
    auto *taskView = currentTaskView();
    if (taskView == nullptr) {
        return nullptr;
    }

    auto *tasksWidget = taskView->tasksWidget();
    if (tasksWidget == nullptr) {
        return nullptr;
    }

    return tasksWidget->currentItem();
}

void TimeTrackerWidget::setupActions(KActionCollection *actionCollection)
{
    Q_INIT_RESOURCE(pics);

    KStandardAction::open(this, &TimeTrackerWidget::openFileDialog, actionCollection);
    KStandardAction::save(this, &TimeTrackerWidget::saveFile, actionCollection);
    KStandardAction::preferences(this, &TimeTrackerWidget::showSettingsDialog, actionCollection);

    {
        QAction *action = actionCollection->addAction(QStringLiteral("ktimetracker_start_new_session"));
        action->setText(i18nc("@action:inmenu", "Start &New Session"));
        action->setToolTip(i18nc("@info:tooltip", "Starts a new session"));
        action->setWhatsThis(i18nc("@info:whatsthis",
                                   "This will reset the "
                                   "session time to 0 for all tasks, to start a new session, without "
                                   "affecting the totals."));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::startNewSession);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("edit_history"));
        action->setText(i18nc("@action:inmenu", "Edit History..."));
        action->setToolTip(i18nc("@info:tooltip", "Edits history of all tasks of the current document"));
        action->setWhatsThis(i18nc("@info:whatsthis",
                                   "A window will "
                                   "be opened where you can change start and "
                                   "stop times of tasks or add a "
                                   "comment to them."));
        action->setIcon(QIcon::fromTheme("view-history"));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::editHistory);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("reset_all_times"));
        action->setText(i18nc("@action:inmenu", "&Reset All Times"));
        action->setToolTip(i18nc("@info:tooltip", "Resets all times"));
        action->setWhatsThis(i18nc("@info:whatsthis",
                                   "This will reset the session "
                                   "and total time to 0 for all tasks, to restart from scratch."));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::resetAllTimes);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("start"));
        action->setText(i18nc("@action:inmenu", "&Start"));
        action->setToolTip(i18nc("@info:tooltip", "Starts timing for selected task"));
        action->setWhatsThis(i18nc("@info:whatsthis",
                                   "This will start timing for the "
                                   "selected task.\nIt is even possible to time several tasks "
                                   "simultaneously.\n\nYou may also start timing of tasks by "
                                   "double clicking "
                                   "the left mouse button on a given task. This will, however, "
                                   "stop timing "
                                   "of other tasks."));
        action->setIcon(QIcon::fromTheme("media-playback-start"));
        actionCollection->setDefaultShortcut(action, QKeySequence(Qt::Key_G));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::startCurrentTimer);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("stop"));
        action->setText(i18nc("@action:inmenu", "S&top"));
        action->setToolTip(i18nc("@info:tooltip", "Stops timing of the selected task"));
        action->setWhatsThis(i18nc("@info:whatsthis", "Stops timing of the selected task"));
        action->setIcon(QIcon::fromTheme("media-playback-stop"));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::stopCurrentTimer);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("focusSearchBar"));
        action->setText(i18nc("@action:inmenu", "Focus on Searchbar"));
        action->setToolTip(i18nc("@info:tooltip", "Sets the focus on the searchbar"));
        action->setWhatsThis(i18nc("@info:whatsthis", "Sets the focus on the searchbar"));
        actionCollection->setDefaultShortcut(action, QKeySequence(Qt::Key_S));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::focusSearchBar);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("stopAll"));
        action->setText(i18nc("@action:inmenu", "Stop &All Timers"));
        action->setToolTip(i18nc("@info:tooltip", "Stops all of the active timers"));
        action->setWhatsThis(i18nc("@info:whatsthis", "Stops all of the active timers"));
        actionCollection->setDefaultShortcut(action, QKeySequence(Qt::Key_Escape));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::stopAllTimers);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("focustracking"));
        action->setCheckable(true);
        action->setText(i18nc("@action:inmenu", "Track Active Applications"));
        action->setToolTip(i18nc("@info:tooltip",
                                 "Auto-creates and updates tasks when the "
                                 "focus of the current window has changed"));
        action->setWhatsThis(i18nc("@info:whatsthis",
                                   "If the focus of a window changes for the "
                                   "first time when this action is enabled, a new task will be "
                                   "created "
                                   "with the title of the window as its name and will be "
                                   "started. If there "
                                   "already exists such an task it will be started."));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::focusTracking);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("new_task"));
        action->setText(i18nc("@action:inmenu", "&New Task..."));
        action->setToolTip(i18nc("@info:tooltip", "Creates new top level task"));
        action->setWhatsThis(i18nc("@info:whatsthis", "This will create a new top level task."));
        action->setIcon(QIcon::fromTheme("task-new"));
        actionCollection->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_T));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::newTask);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("new_sub_task"));
        action->setText(i18nc("@action:inmenu", "New &Subtask..."));
        action->setToolTip(i18nc("@info:tooltip", "Creates a new subtask to the current selected task"));
        action->setWhatsThis(i18nc("@info:whatsthis", "This will create a new subtask to the current selected task."));
        action->setIcon(QIcon::fromTheme("view-task-child"));
        actionCollection->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_B));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::newSubTask);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("delete_task"));
        action->setText(i18nc("@action:inmenu", "&Delete"));
        action->setToolTip(i18nc("@info:tooltip", "Deletes selected task"));
        action->setWhatsThis(i18nc("@info:whatsthis", "This will delete the selected task(s) and all subtasks."));
        action->setIcon(QIcon::fromTheme("edit-delete"));
        actionCollection->setDefaultShortcut(action, QKeySequence(Qt::Key_Delete));
        connect(action, &QAction::triggered, this, QOverload<>::of(&TimeTrackerWidget::deleteTask));
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("edit_task"));
        action->setText(i18nc("@action:inmenu", "&Properties"));
        action->setToolTip(i18nc("@info:tooltip", "Edit name or description for selected task"));
        action->setWhatsThis(i18nc("@info:whatsthis",
                                   "This will bring up a dialog "
                                   "box where you may edit the parameters for the selected task."));
        action->setIcon(QIcon::fromTheme("document-properties"));
        actionCollection->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_E));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::editTask);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("edit_task_time"));
        action->setText(i18nc("@action:inmenu", "Edit &Time..."));
        action->setToolTip(i18nc("@info:tooltip", "Edit time for selected task"));
        action->setWhatsThis(i18nc("@info:whatsthis",
                                   "This will bring up a dialog "
                                   "box where you may edit the times for the selected task."));
        action->setIcon(QIcon::fromTheme("document-edit"));
        actionCollection->setDefaultShortcut(action, QKeySequence(Qt::Key_E));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::editTaskTime);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("mark_as_complete"));
        action->setText(i18nc("@action:inmenu", "&Mark as Complete"));
        action->setIcon(QPixmap(":/pics/task-complete.xpm"));
        actionCollection->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::Key_M));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::markTaskAsComplete);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("mark_as_incomplete"));
        action->setText(i18nc("@action:inmenu", "&Mark as Incomplete"));
        action->setIcon(QPixmap(":/pics/task-incomplete.xpm"));
        actionCollection->setDefaultShortcut(action, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::markTaskAsIncomplete);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("export_dialog"));
        action->setText(i18nc("@action:inmenu", "&Export..."));
        action->setIcon(QIcon::fromTheme("document-export"));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::exportDialog);
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("import_planner"));
        action->setText(i18nc("@action:inmenu", "Import Tasks From &Planner..."));
        action->setIcon(QIcon::fromTheme("document-import"));
        connect(action, &QAction::triggered, [=]() {
            const QString &fileName = QFileDialog::getOpenFileName();
            importPlannerFile(fileName);
        });
    }

    {
        QAction *action = actionCollection->addAction(QStringLiteral("searchbar"));
        action->setCheckable(true);
        action->setChecked(KTimeTrackerSettings::showSearchBar());
        action->setText(i18nc("@action:inmenu", "Show Searchbar"));
        connect(action, &QAction::triggered, this, &TimeTrackerWidget::slotSearchBar);
    }

    m_actionCollection = actionCollection;

    connect(this, &TimeTrackerWidget::currentTaskChanged, this, &TimeTrackerWidget::slotUpdateButtons);
    connect(this, &TimeTrackerWidget::currentTaskViewChanged, this, &TimeTrackerWidget::slotUpdateButtons);
    connect(this, &TimeTrackerWidget::updateButtons, this, &TimeTrackerWidget::slotUpdateButtons);
    slotUpdateButtons();
}

QAction *TimeTrackerWidget::action(const QString &name) const
{
    return m_actionCollection->action(name);
}

void TimeTrackerWidget::openFile(const QUrl &url)
{
    qCDebug(KTT_LOG) << "Entering function, url is " << url;
    addTaskView(url);
}

void TimeTrackerWidget::openFileDialog()
{
    const QString &path = QFileDialog::getOpenFileName(this);
    if (!path.isEmpty()) {
        openFile(QUrl::fromLocalFile(path));
    }
}

bool TimeTrackerWidget::closeFile()
{
    qCDebug(KTT_LOG) << "Entering TimeTrackerWidget::closeFile";
    TaskView *taskView = currentTaskView();

    if (taskView) {
        taskView->save();
        taskView->closeStorage();
    }

    emit currentTaskViewChanged();
    emit currentFileChanged(QString());
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
        connect(m_taskView,
                &TaskView::setStatusBarText,
                this,
                &TimeTrackerWidget::statusBarTextChangeRequested,
                Qt::UniqueConnection);
        connect(m_taskView, &TaskView::timersActive, this, &TimeTrackerWidget::timersActive, Qt::UniqueConnection);
        connect(m_taskView, &TaskView::timersInactive, this, &TimeTrackerWidget::timersInactive, Qt::UniqueConnection);
        connect(m_taskView, &TaskView::tasksChanged, this, &TimeTrackerWidget::tasksChanged, Qt::UniqueConnection);
        connect(m_taskView, &TaskView::minutesUpdated, this, &TimeTrackerWidget::minutesUpdated, Qt::UniqueConnection);

        emit currentFileChanged(m_taskView->storage()->fileUrl().toString());
    }
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
    action(QStringLiteral("edit_task_time"))->setEnabled(item);
    action(QStringLiteral("edit_task"))->setEnabled(item);
    action(QStringLiteral("mark_as_complete"))->setEnabled(item && !item->isComplete());
    action(QStringLiteral("mark_as_incomplete"))->setEnabled(item && item->isComplete());

    action(QStringLiteral("new_task"))->setEnabled(currentTaskView());
    action(QStringLiteral("new_sub_task"))
        ->setEnabled(currentTaskView() && currentTaskView()->storage()->isLoaded()
                     && currentTaskView()->storage()->tasksModel()->getAllTasks().size());
    action(QStringLiteral("focustracking"))->setEnabled(currentTaskView());
    action(QStringLiteral("focustracking"))
        ->setChecked(currentTaskView() && currentTaskView()->isFocusTrackingActive());
    action(QStringLiteral("ktimetracker_start_new_session"))->setEnabled(currentTaskView());
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

    auto *dialog = new KConfigDialog(this, "settings", KTimeTrackerSettings::self());
    dialog->setFaceType(KPageDialog::List);
    dialog->addPage(new KTimeTrackerBehaviorConfig(dialog),
                    i18nc("@title:tab", "Behavior"),
                    QStringLiteral("preferences-other"));
    dialog->addPage(new KTimeTrackerDisplayConfig(dialog),
                    i18nc("@title:tab", "Appearance"),
                    QStringLiteral("preferences-desktop-theme"));
    dialog->addPage(new KTimeTrackerStorageConfig(dialog),
                    i18nc("@title:tab", "Storage"),
                    QStringLiteral("system-file-manager"));
    connect(dialog, &KConfigDialog::settingsChanged, this, &TimeTrackerWidget::loadSettings);
    dialog->show();
}

void TimeTrackerWidget::loadSettings()
{
    KTimeTrackerSettings::self()->load();

    showSearchBar(!KTimeTrackerSettings::configPDA() && KTimeTrackerSettings::showSearchBar());
    currentTaskView()->reconfigureModel();
    currentTaskView()->tasksWidget()->reconfigure();

    //  for updating window caption
    emit currentFileChanged(m_taskView->storage()->fileUrl().toString());
    emit tasksChanged(currentTaskView()->storage()->tasksModel()->getActiveTasks());
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

void TimeTrackerWidget::stopAllTimers()
{
    currentTaskView()->stopAllTimers(QDateTime::currentDateTime());
}

void TimeTrackerWidget::newTask()
{
    currentTaskView()->newTask(i18nc("@title:window", "New Task"), nullptr);
}

void TimeTrackerWidget::newSubTask()
{
    currentTaskView()->newSubTask();
}

void TimeTrackerWidget::editTask()
{
    currentTaskView()->editTask();
}

void TimeTrackerWidget::editTaskTime()
{
    qCDebug(KTT_LOG) << "Entering editTask";
    Task *task = currentTask();
    if (!task) {
        return;
    }

    QPointer<EditTimeDialog> editTimeDialog =
        new EditTimeDialog(this, task->name(), task->description(), static_cast<int>(task->time()));

    if (editTimeDialog->exec() == QDialog::Accepted) {
        if (editTimeDialog->editHistoryRequested()) {
            editHistory();
        } else {
            currentTaskView()->editTaskTime(task->uid(), editTimeDialog->changeMinutes());
        }
    }

    delete editTimeDialog;
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
    ExportDialog dialog(taskView->tasksWidget(), taskView);
    if (taskView->tasksWidget()->currentItem() && taskView->tasksWidget()->currentItem()->isRoot()) {
        dialog.enableTasksToExportQuestion();
    }
    dialog.exec();
}

void TimeTrackerWidget::startNewSession()
{
    currentTaskView()->storage()->tasksModel()->startNewSession();
}

void TimeTrackerWidget::editHistory()
{
    // HistoryDialog is the new HistoryDialog, but the EditHiStoryDiaLog exists as well.
    // HistoryDialog can be edited with qtcreator and qtdesigner, EditHiStoryDiaLog cannot.
    if (currentTaskView()) {
        QPointer<HistoryDialog> dialog =
            new HistoryDialog(currentTaskView()->tasksWidget(), currentTaskView()->storage()->projectModel());
        if (currentTaskView()->storage()->eventsModel()->events().count() != 0) {
            dialog->exec();
        } else {
            KMessageBox::information(nullptr,
                                     i18nc("@info in message box",
                                           "There is no history yet. Start and stop a task and you "
                                           "will have an entry in your history."));
        }
    }
}

void TimeTrackerWidget::resetAllTimes()
{
    if (currentTaskView()) {
        if (KMessageBox::warningContinueCancel(this,
                                               i18n("Do you really want to reset the time to zero for all "
                                                    "tasks? This will delete the entire history."),
                                               i18nc("@title:window", "Confirmation Required"),
                                               KGuiItem(i18nc("@action:button", "Reset All Times")))
            == KMessageBox::Continue) {
            currentTaskView()->storage()->projectModel()->resetTimeForAllTasks();
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
#include "mainadaptor.h"

void TimeTrackerWidget::registerDBus()
{
    new MainAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/KTimeTracker", this);
}

QString TimeTrackerWidget::version() const
{
    return KTIMETRACKER_VERSION_STRING;
}

QStringList TimeTrackerWidget::taskIdsFromName(const QString &taskName) const
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

void TimeTrackerWidget::addTask(const QString &taskName)
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
        taskView
            ->addTask(taskName, QString(), 0, 0, DesktopList(), taskView->storage()->tasksModel()->taskByUID(taskId));
        taskView->storage()->projectModel()->refresh();
        taskView->tasksWidget()->refresh();
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
            break;
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

int TimeTrackerWidget::bookTime(const QString &taskId, const QString &dateTime, int64_t minutes)
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

int TimeTrackerWidget::changeTime(const QString &taskId, int64_t minutes)
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
    return 0;
}

QString TimeTrackerWidget::error(int errorCode) const
{
    switch (errorCode) {
    case KTIMETRACKER_ERR_GENERIC_SAVE_FAILED:
        return i18n("Save failed, most likely because the file could not be locked.");
    case KTIMETRACKER_ERR_COULD_NOT_MODIFY_RESOURCE:
        return i18n("Could not modify calendar resource.");
    case KTIMETRACKER_ERR_MEMORY_EXHAUSTED:
        return i18n("Out of memory--could not create object.");
    case KTIMETRACKER_ERR_UID_NOT_FOUND:
        return i18n("UID not found.");
    case KTIMETRACKER_ERR_INVALID_DATE:
        return i18n("Invalidate date--format is YYYY-MM-DD.");
    case KTIMETRACKER_ERR_INVALID_TIME:
        return i18n("Invalid time--format is YYYY-MM-DDTHH:MM:SS.");
    case KTIMETRACKER_ERR_INVALID_DURATION:
        return i18n("Invalid task duration--must be greater than zero.");
    default:
        return i18n("Invalid error number: %1", errorCode);
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
            taskView->startTimerForNow(task);
            return;
        }
    }
}

bool TimeTrackerWidget::startTimerForTaskName(const QString &taskName)
{
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return false;
    }

    for (Task *task : taskView->storage()->tasksModel()->getAllTasks()) {
        if (task->name() == taskName) {
            taskView->startTimerForNow(task);
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
    if (taskView) {
        taskView->stopAllTimers();
    }
}

QString TimeTrackerWidget::exportCSVFile(const QString &filename,
                                         const QString &from,
                                         const QString &to,
                                         int type,
                                         bool decimalMinutes,
                                         bool allTasks,
                                         const QString &delimiter,
                                         const QString &quote)
{
    TaskView *taskView = currentTaskView();

    if (!taskView) {
        return "";
    }

    ReportCriteria rc;

    rc.from = QDate::fromString(from);
    if (rc.from.isNull()) {
        rc.from = QDate::fromString(from, Qt::ISODate);
    }

    rc.to = QDate::fromString(to);
    if (rc.to.isNull()) {
        rc.to = QDate::fromString(to, Qt::ISODate);
    }

    rc.reportType = static_cast<ReportCriteria::REPORTTYPE>(type);
    rc.decimalMinutes = decimalMinutes;
    rc.allTasks = allTasks;
    rc.delimiter = delimiter;
    rc.quote = quote;

    QString output = exportToString(taskView->storage()->projectModel(), taskView->tasksWidget()->currentItem(), rc);
    return writeExport(output, QUrl::fromLocalFile(filename));
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
    TaskView *taskView = currentTaskView();
    if (!taskView) {
        return result;
    }

    for (Task *task : taskView->storage()->tasksModel()->getActiveTasks()) {
        result << task->name();
    }

    return result;
}

void TimeTrackerWidget::saveAll()
{
    currentTaskView()->save();
}

void TimeTrackerWidget::quit()
{
    auto *mainWindow = dynamic_cast<MainWindow *>(parent()->parent());
    if (mainWindow) {
        mainWindow->quit();
    } else {
        qCWarning(KTT_LOG) << "Cast to MainWindow failed";
    }
}

bool TimeTrackerWidget::event(QEvent *event) // inherited from QWidget
{
    if (event->type() == QEvent::QueryWhatsThis) {
        if (m_taskView->storage()->tasksModel()->getAllTasks().empty()) {
            setWhatsThis(i18nc("@info:whatsthis",
                               "This is ktimetracker, KDE's program to help "
                               "you track your time. "
                               "Best, start with creating your first task - "
                               "enter it into the field "
                               "where you see \"Search or add task\"."));
        } else {
            setWhatsThis(i18nc("@info:whatsthis",
                               "You have already created a task. You can now "
                               "start and stop timing."));
        }
    }

    return QWidget::event(event);
}
// END of dbus slots group
/* @} */
