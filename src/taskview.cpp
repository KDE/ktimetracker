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

#include "taskview.h"

#include <QMouseEvent>
#include <QPointer>
#include <QProgressDialog>
#include <QSortFilterProxyModel>
#include <QTimer>

#include <KMessageBox>

#include "dialogs/exportdialog.h"
#include "desktoptracker.h"
#include "dialogs/edittimedialog.h"
#include "dialogs/taskpropertiesdialog.h"
#include "export/export.h"
#include "focusdetector.h"
#include "dialogs/historydialog.h"
#include "idletimedetector.h"
#include "import/plannerparser.h"
#include "ktimetracker.h"
#include "ktimetrackerutility.h"
#include "ktt_debug.h"
#include "model/eventsmodel.h"
#include "model/task.h"
#include "model/tasksmodel.h"
#include "treeviewheadercontextmenu.h"
#include "widgets/taskswidget.h"

void deleteEntry(const QString& key)
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.deleteEntry(key);
    config.sync();
}

TaskView::TaskView(QWidget *parent)
    : QObject(parent)
    , m_filterProxyModel(new QSortFilterProxyModel(this))
    , m_storage(new TimeTrackerStorage())
    , m_focusTrackingActive(false)
    , m_lastTaskWithFocus(nullptr)
    , m_focusDetector(new FocusDetector())
    , m_tasksWidget(nullptr)
{
    m_filterProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterProxyModel->setRecursiveFilteringEnabled(true);
    m_filterProxyModel->setSortRole(Task::SortRole);

    connect(m_focusDetector, &FocusDetector::newFocus, this, &TaskView::newFocusWindowDetected);

    // set up the minuteTimer
    m_minuteTimer = new QTimer(this);
    connect(m_minuteTimer, &QTimer::timeout, this, &TaskView::minuteUpdate);
    m_minuteTimer->start(1000 * secsPerMinute);

    // Set up the idle detection.
    m_idleTimeDetector = new IdleTimeDetector(KTimeTrackerSettings::period());
    connect(m_idleTimeDetector, &IdleTimeDetector::subtractTime, this, &TaskView::subtractTime);
    connect(m_idleTimeDetector, &IdleTimeDetector::stopAllTimers, this, &TaskView::stopAllTimers);
    if (!IdleTimeDetector::isIdleDetectionPossible()) {
        KTimeTrackerSettings::setEnabled(false);
    }

    // Setup auto save timer
    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &TaskView::save);

    // Setup manual save timer (to save changes a little while after they happen)
    m_manualSaveTimer = new QTimer(this);
    m_manualSaveTimer->setSingleShot( true );
    connect(m_manualSaveTimer, &QTimer::timeout, this, &TaskView::save);

    // Connect desktop tracker events to task starting/stopping
    m_desktopTracker = new DesktopTracker();
    connect(m_desktopTracker, &DesktopTracker::reachedActiveDesktop, this, &TaskView::startTimerForNow);
    connect(m_desktopTracker, &DesktopTracker::leftActiveDesktop, this, &TaskView::stopTimerFor);
}

void TaskView::newFocusWindowDetected(const QString &taskName)
{
    QString newTaskName = taskName;
    newTaskName.remove('\n');

    if (!m_focusTrackingActive) {
        return;
    }

    bool found = false;  // has taskName been found in our tasks
    stopTimerFor(m_lastTaskWithFocus);
    for (Task *task : storage()->tasksModel()->getAllTasks()) {
        if (task->name() == newTaskName) {
            found = true;
            startTimerForNow(task);
            m_lastTaskWithFocus = task;
        }
    }
    if (!found) {
        if (!addTask(newTaskName)) {
            KMessageBox::error(
                nullptr,
                i18n("Error storing new task. Your changes were not saved. "
                     "Make sure you can edit your iCalendar file. "
                     "Also quit all applications using this file and remove "
                     "any lock file related to its name from ~/.kde/share/apps/kabc/lock/ "));
        }
        save();
        for (Task *task : storage()->tasksModel()->getAllTasks()) {
            if (task->name() == newTaskName) {
                startTimerForNow(task);
                m_lastTaskWithFocus = task;
            }
        }
    }
    emit updateButtons();
}

TimeTrackerStorage *TaskView::storage()
{
    return m_storage;
}

TaskView::~TaskView()
{
    delete m_storage;
    KTimeTrackerSettings::self()->save();
}

void TaskView::load(const QUrl &url)
{
    if (m_tasksWidget) {
        delete m_tasksWidget;
        m_tasksWidget = nullptr;
    }

    // if the program is used as an embedded plugin for konqueror, there may be a need
    // to load from a file without touching the preferences.
    QString err = m_storage->load(this, url);
    if (!err.isEmpty()) {
        KMessageBox::error(m_tasksWidget, err);
        qCDebug(KTT_LOG) << "Leaving TaskView::load";
        return;
    }

    m_tasksWidget = new TasksWidget(dynamic_cast<QWidget*>(parent()), m_filterProxyModel, nullptr);
    connect(m_tasksWidget, &TasksWidget::updateButtons, this, &TaskView::updateButtons);
    connect(m_tasksWidget, &TasksWidget::contextMenuRequested, this, &TaskView::contextMenuRequested);
    connect(m_tasksWidget, &TasksWidget::taskDoubleClicked, this, &TaskView::onTaskDoubleClicked);
    m_tasksWidget->setRootIsDecorated(true);

    reconfigureModel();
    m_tasksWidget->reconfigure();

    // Connect to the new model created by TimeTrackerStorage::load()
    auto *tasksModel = m_storage->tasksModel();
    m_filterProxyModel->setSourceModel(tasksModel);
    m_tasksWidget->setSourceModel(tasksModel);
    for (int i = 0; i <= tasksModel->columnCount(QModelIndex()); ++i) {
        m_tasksWidget->resizeColumnToContents(i);
    }

    // Table header context menu
    auto *headerContextMenu = new TreeViewHeaderContextMenu(this, m_tasksWidget, QVector<int>{0});
    connect(headerContextMenu, &TreeViewHeaderContextMenu::columnToggled, this, &TaskView::slotColumnToggled);

    connect(tasksModel, &TasksModel::taskCompleted, this, &TaskView::stopTimerFor);
    connect(tasksModel, &TasksModel::taskDropped, this, &TaskView::reFreshTimes);
    connect(tasksModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &TaskView::taskAboutToBeRemoved);
    connect(storage()->eventsModel(), &EventsModel::timesChanged, this, &TaskView::reFreshTimes);

    // Register tasks with desktop tracker
    for (Task *task : storage()->tasksModel()->getAllTasks()) {
        m_desktopTracker->registerForDesktops(task, task->desktops());
    }

    // Start all tasks that have an event without endtime
    for (Task *task : storage()->tasksModel()->getAllTasks()) {
        if (!m_storage->allEventsHaveEndTime(task)) {
            task->resumeRunning();
            m_activeTasks.append(task);
            emit updateButtons();
            if (m_activeTasks.count() == 1) {
                emit timersActive();
            }
            emit tasksChanged(m_activeTasks);
        }
    }

    if (tasksModel->topLevelItemCount() > 0) {
        m_tasksWidget->restoreItemState();
        m_tasksWidget->setCurrentIndex(m_filterProxyModel->mapFromSource(
            tasksModel->index(tasksModel->topLevelItem(0), 0)));

        if (!m_desktopTracker->startTracking().isEmpty()) {
            KMessageBox::error(nullptr, i18n("Your virtual desktop number is too high, desktop tracking will not work."));
        }
    }

    refreshModel();
    refreshView();

    for (int i = 0; i <= tasksModel->columnCount(QModelIndex()); ++i) {
        m_tasksWidget->resizeColumnToContents(i);
    }
}

void TaskView::closeStorage()
{
    m_storage->closeStorage();
}

void TaskView::refreshModel()
{
    for (Task *task : storage()->tasksModel()->getAllTasks()) {
        task->invalidateCompletedState();
        task->update();  // maybe there was a change in the times's format
    }
}

void TaskView::refreshView()
{
    m_tasksWidget->refresh();
}

/**
 * Refresh the times of the tasks, e.g. when the history has been changed by the user.
 * Re-calculate the time for every task based on events in the history.
 */
QString TaskView::reFreshTimes()
{
    QString err;

    // This procedure resets all times (session and overall) for all tasks and subtasks.
    // Reset session and total time for all tasks - do not touch the storage.
    for (Task *task : storage()->tasksModel()->getAllTasks()) {
        task->resetTimes();
    }

    for (Task *task : storage()->tasksModel()->getAllTasks()) {
        // get all events for task
        for (const auto *event : storage()->eventsModel()->eventsForTask(task)) {
            QDateTime eventStart = event->dtStart();
            QDateTime eventEnd = event->dtEnd();

            const int64_t duration = event->duration() / 60;
            task->addTime(duration);
            qCDebug(KTT_LOG) << "duration is" << duration;

            if (task->sessionStartTiMe().isValid()) {
                // if there is a session
                if (task->sessionStartTiMe().secsTo(eventStart) > 0 &&
                    task->sessionStartTiMe().secsTo(eventEnd) > 0) {
                    // if the event is after the session start
                    task->addSessionTime(duration);
                }
            } else {
                // so there is no session at all
                task->addSessionTime(duration);
            }
        }
    }

    // Recalculate total times after changing hierarchy by drag&drop
    for (Task *task : storage()->tasksModel()->getAllTasks()) {
        // Start recursive method recalculateTotalTimesSubtree() for each top-level task.
        if (task->isRoot()) {
            task->recalculateTotalTimesSubtree();
        }
    }

    refreshModel();
    refreshView();
    qCDebug(KTT_LOG) << "Leaving TaskView::reFreshTimes()";
    return err;
}

void TaskView::importPlanner(const QString& fileName)
{
    qCDebug(KTT_LOG) << "entering importPlanner";
    auto *handler = new PlannerParser(storage()->projectModel(), m_tasksWidget->currentItem());
    QFile xmlFile(fileName);
    QXmlInputSource source(&xmlFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(handler);
    reader.parse(source);
    refreshModel();
    refreshView();
}

void TaskView::scheduleSave()
{
    m_manualSaveTimer->start(10);
}

void TaskView::save()
{
    qCDebug(KTT_LOG) << "Entering TaskView::save()";
    QString err = m_storage->save();

    if (!err.isNull()) {
        KMessageBox::error(m_tasksWidget, err);
    }
}

void TaskView::startCurrentTimer()
{
    startTimerForNow(m_tasksWidget->currentItem());
}

void TaskView::startTimerFor(Task *task, const QDateTime &startTime)
{
    qCDebug(KTT_LOG) << "Entering function";
    if (task != nullptr && m_activeTasks.indexOf(task) == -1) {
        if (!task->isComplete()) {
            if (KTimeTrackerSettings::uniTasking()) {
                stopAllTimers();
            }
            m_idleTimeDetector->startIdleDetection();

            task->setRunning(true, startTime);
            save();

            m_activeTasks.append(task);
            emit updateButtons();
            if (m_activeTasks.count() == 1) {
                emit timersActive();
            }
            emit tasksChanged(m_activeTasks);
        }
    }
}

void TaskView::startTimerForNow(Task *task)
{
    startTimerFor(task, QDateTime::currentDateTime());
}

void TaskView::clearActiveTasks()
{
    m_activeTasks.clear();
}

void TaskView::stopAllTimers(const QDateTime& when)
{
    qCDebug(KTT_LOG) << "Entering function";
    QProgressDialog dialog(
        i18nc("@info:progress", "Stopping timers..."), i18n("Cancel"), 0, m_activeTasks.count(), m_tasksWidget);
    if (m_activeTasks.count() > 1) {
        dialog.show();
    }

    for (Task *task : m_activeTasks) {
        QApplication::processEvents();

        task->setRunning(false, when);
        save();

        dialog.setValue(dialog.value() + 1);
    }

    m_idleTimeDetector->stopIdleDetection();
    m_activeTasks.clear();
    emit updateButtons();
    emit timersInactive();
    emit tasksChanged(m_activeTasks);
}

void TaskView::toggleFocusTracking()
{
    m_focusTrackingActive = !m_focusTrackingActive;

    if (m_focusTrackingActive) {
        // FIXME: should get the currently active window and start tracking it?
    } else {
        stopTimerFor(m_lastTaskWithFocus);
    }

    emit updateButtons();
}

void TaskView::startNewSession()
/* This procedure starts a new session. We speak of session times,
overalltimes (comprising all sessions) and total times (comprising all subtasks).
That is why there is also a total session time. */
{
    qCDebug(KTT_LOG) <<"Entering TaskView::startNewSession";
    for (Task *task : storage()->tasksModel()->getAllTasks()) {
        task->startNewSession();
    }
    qCDebug(KTT_LOG) << "Leaving TaskView::startNewSession";
}

void TaskView::resetTimeForAllTasks()
/* This procedure resets all times (session and overall) for all tasks and subtasks. */
{
    qCDebug(KTT_LOG) << "Entering function";
    for (Task *task : storage()->tasksModel()->getAllTasks()) {
        task->resetTimes();
    }
    storage()->deleteAllEvents();
    qCDebug(KTT_LOG) << "Leaving function";
}

void TaskView::stopTimerFor(Task* task)
{
    qCDebug(KTT_LOG) << "Entering function";
    if (task != nullptr && m_activeTasks.indexOf(task) != -1) {
        m_activeTasks.removeAll(task);

        task->setRunning(false);
        save();

        if (m_activeTasks.count() == 0) {
            m_idleTimeDetector->stopIdleDetection();
            emit timersInactive();
        }
        emit updateButtons();
    }
    emit tasksChanged(m_activeTasks);
}

void TaskView::stopCurrentTimer()
{
    stopTimerFor(m_tasksWidget->currentItem());
    if (m_focusTrackingActive && m_lastTaskWithFocus == m_tasksWidget->currentItem()) {
        toggleFocusTracking();
    }
}

void TaskView::minuteUpdate()
{
    addTimeToActiveTasks(1);
}

void TaskView::addTimeToActiveTasks(int64_t minutes)
{
    for (Task *task : m_activeTasks) {
        task->changeTime(minutes, nullptr);
    }
    scheduleSave();
}

void TaskView::newTask()
{
    newTask(i18nc("@title:window", "New Task"), nullptr);
}

void TaskView::newTask(const QString &caption, Task *parent)
{
    QPointer<TaskPropertiesDialog> dialog = new TaskPropertiesDialog(
        m_tasksWidget->parentWidget(), caption, QString(), QString(), DesktopList());

    if (dialog->exec() == QDialog::Accepted) {
        QString taskName = i18n("Unnamed Task");
        if (!dialog->name().isEmpty()) {
            taskName = dialog->name();
        }
        QString taskDescription = dialog->description();

        auto desktopList = dialog->desktops();

        // If all available desktops are checked, disable auto tracking,
        // since it makes no sense to track for every desktop.
        if (desktopList.size() == m_desktopTracker->desktopCount()) {
            desktopList.clear();
        }

        int64_t total = 0;
        int64_t session = 0;
        auto *task = addTask(taskName, taskDescription, total, session, desktopList, parent);
        save();
        if (!task) {
            KMessageBox::error(nullptr, i18n(
                "Error storing new task. Your changes were not saved. "
                "Make sure you can edit your iCalendar file. Also quit "
                "all applications using this file and remove any lock "
                "file related to its name from ~/.kde/share/apps/kabc/lock/"));
        }
    }
    delete dialog;

    emit updateButtons();
}

Task *TaskView::addTask(
    const QString& taskname, const QString& taskdescription, int64_t total, int64_t session,
    const DesktopList& desktops, Task* parent)
{
    qCDebug(KTT_LOG) << "Entering function; taskname =" << taskname;
    m_tasksWidget->setSortingEnabled(false);

    Task *task = new Task(
        taskname, taskdescription, total, session, desktops, storage()->projectModel(), parent);
    if (task->uid().isNull()) {
        qFatal("failed to generate UID");
    }

    m_desktopTracker->registerForDesktops(task, desktops);
    m_tasksWidget->setCurrentIndex(m_filterProxyModel->mapFromSource(storage()->tasksModel()->index(task, 0)));
    task->invalidateCompletedState();

    m_tasksWidget->setSortingEnabled(true);
    return task;
}

void TaskView::newSubTask()
{
    Task* task = m_tasksWidget->currentItem();
    if (!task) {
        return;
    }

    newTask(i18nc("@title:window", "New Sub Task"), task);

    m_tasksWidget->setExpanded(m_filterProxyModel->mapFromSource(storage()->tasksModel()->index(task, 0)), true);

    refreshModel();
    refreshView();
}

void TaskView::editTask()
{
    qCDebug(KTT_LOG) <<"Entering editTask";
    Task* task = m_tasksWidget->currentItem();
    if (!task) {
        return;
    }

    auto oldDeskTopList = task->desktops();
    QPointer<TaskPropertiesDialog> dialog = new TaskPropertiesDialog(
        m_tasksWidget->parentWidget(), i18nc("@title:window", "Edit Task"),
        task->name(), task->description(), oldDeskTopList);
    if (dialog->exec() == QDialog::Accepted) {
        QString name = i18n("Unnamed Task");
        if (!dialog->name().isEmpty()) {
            name = dialog->name();
        }

        // setName only does something if the new name is different
        task->setName(name);
        task->setDescription(dialog->description());
        auto desktopList = dialog->desktops();
        // If all available desktops are checked, disable auto tracking,
        // since it makes no sense to track for every desktop.
        if (desktopList.size() == m_desktopTracker->desktopCount()) {
            desktopList.clear();
        }
        // only do something for autotracking if the new setting is different
        if (oldDeskTopList != desktopList) {
            task->setDesktopList(desktopList);
            m_desktopTracker->registerForDesktops(task, desktopList);
        }
        emit updateButtons();
    }

    delete dialog;
}

void TaskView::editTaskTime()
{
    qCDebug(KTT_LOG) <<"Entering editTask";
    Task* task = m_tasksWidget->currentItem();
    if (!task) {
        return;
    }

    QPointer<EditTimeDialog> editTimeDialog = new EditTimeDialog(
        m_tasksWidget->parentWidget(),
        task->name(), task->description(),
        static_cast<int>(task->time()));

    if (editTimeDialog->exec() == QDialog::Accepted) {
        if (editTimeDialog->editHistoryRequested()) {
            editHistory();
        } else {
            editTaskTime(task->uid(), editTimeDialog->changeMinutes());
        }
    }

    delete editTimeDialog;
}

void TaskView::editHistory()
{
    QPointer<HistoryDialog> dialog = new HistoryDialog(m_tasksWidget->parentWidget(), storage()->projectModel());
    dialog->exec();
}

void TaskView::setPerCentComplete(int completion)
{
    Task* task = m_tasksWidget->currentItem();
    if (!task) {
        KMessageBox::information(nullptr, i18n("No task selected."));
        return;
    }

    if (completion < 0) {
        completion = 0;
    }
    if (completion < 100) {
        task->setPercentComplete(completion);
        task->invalidateCompletedState();
        emit updateButtons();
    }
}

void TaskView::deleteTaskBatch(Task* task)
{
    QString uid = task->uid();
    task->remove(m_storage);
    deleteEntry(uid); // forget if the item was expanded or collapsed

    // Stop idle detection if no more counters are running
    if (m_activeTasks.count() == 0) {
        m_idleTimeDetector->stopIdleDetection();
        emit timersInactive();
    }

    task->delete_recursive();
    emit tasksChanged(m_activeTasks);
}

void TaskView::deleteTask(Task* task)
/* Attention when popping up a window asking for confirmation.
If you have "Track active applications" on, this window will create a new task and
make this task running and selected. */
{
    if (!task) {
        task = m_tasksWidget->currentItem();
    }

    if (!m_tasksWidget->currentItem()) {
        KMessageBox::information(nullptr, i18n("No task selected."));
    } else {
        int response = KMessageBox::Continue;
        if (KTimeTrackerSettings::promptDelete()) {
            response = KMessageBox::warningContinueCancel(nullptr,
                i18n("Are you sure you want to delete the selected task and its entire history?\n"
                     "Note: All subtasks and their history will also be deleted."),
                i18nc("@title:window", "Deleting Task"), KStandardGuiItem::del());
        }

        if (response == KMessageBox::Continue) {
            deleteTaskBatch(task);
        }
    }
}

void TaskView::markTaskAsComplete()
{
    if (!m_tasksWidget->currentItem()) {
        KMessageBox::information(nullptr, i18n("No task selected."));
        return;
    }

    m_tasksWidget->currentItem()->setPercentComplete(100);
    m_tasksWidget->currentItem()->invalidateCompletedState();
    emit updateButtons();
}

void TaskView::subtractTime(int64_t minutes)
{
    addTimeToActiveTasks(-minutes);
}

void TaskView::markTaskAsIncomplete()
{
    setPerCentComplete(50); // if it has been reopened, assume half-done
}

void TaskView::slotColumnToggled(int column)
{
    switch (column) {
    case 1:
        KTimeTrackerSettings::setDisplaySessionTime(!m_tasksWidget->isColumnHidden(1));
        break;
    case 2:
        KTimeTrackerSettings::setDisplayTime(!m_tasksWidget->isColumnHidden(2));
        break;
    case 3:
        KTimeTrackerSettings::setDisplayTotalSessionTime(!m_tasksWidget->isColumnHidden(3));
        break;
    case 4:
        KTimeTrackerSettings::setDisplayTotalTime(!m_tasksWidget->isColumnHidden(4));
        break;
    case 5:
        KTimeTrackerSettings::setDisplayPriority(!m_tasksWidget->isColumnHidden(5));
        break;
    case 6:
        KTimeTrackerSettings::setDisplayPercentComplete(!m_tasksWidget->isColumnHidden(6));
        break;
    }
    KTimeTrackerSettings::self()->save();
}

bool TaskView::isFocusTrackingActive() const
{
    return m_focusTrackingActive;
}

void TaskView::reconfigureModel()
{
    /* idleness */
    m_idleTimeDetector->setMaxIdle(KTimeTrackerSettings::period());
    m_idleTimeDetector->toggleOverAllIdleDetection(KTimeTrackerSettings::enabled());

    /* auto save */
    if (KTimeTrackerSettings::autoSave()) {
        m_autoSaveTimer->start(KTimeTrackerSettings::autoSavePeriod() * 1000 * secsPerMinute);
    } else if (m_autoSaveTimer->isActive()) {
        m_autoSaveTimer->stop();
    }

    refreshModel();
}

//----------------------------------------------------------------------------

void TaskView::onTaskDoubleClicked(Task *task)
{
    if (task->isRunning()) {
        // if task is running, stop it
        stopCurrentTimer();
    } else if (!task->isComplete()) {
        // if task is not running, start it
        stopAllTimers();
        startCurrentTimer();
    }
}

void TaskView::editTaskTime(const QString& taskUid, int64_t minutes)
{
    // update session time if the time was changed
    auto* task = m_storage->tasksModel()->taskByUID(taskUid);
    if (task) {
        task->changeTime(minutes, m_storage->eventsModel());
        scheduleSave();
    }
}

void TaskView::taskAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    if (first != last) {
        qFatal("taskAboutToBeRemoved: unexpected removal of multiple items at once");
    }

    TasksModelItem *item = nullptr;
    if (parent.isValid()) {
        // Nested task
        auto *parentItem = storage()->tasksModel()->item(parent);
        if (!parentItem) {
            qFatal("taskAboutToBeRemoved: parentItem is nullptr");
        }

        item = parentItem->child(first);
    } else {
        // Top-level task
        item = storage()->tasksModel()->topLevelItem(first);
    }

    if (!item) {
        qFatal("taskAboutToBeRemoved: item is nullptr");
    }

    // We use static_cast here instead of dynamic_cast because this
    // taskAboutToBeRemoved() slot is called from TasksModelItem's destructor
    // when the Task object is already destructed, thus dynamic_cast would
    // return nullptr.
    auto *task = dynamic_cast<Task*>(item);
    if (!task) {
        qFatal("taskAboutToBeRemoved: task is nullptr");
    }

    // Handle task deletion
    DesktopList desktopList;
    m_desktopTracker->registerForDesktops(task, desktopList);
    m_activeTasks.removeAll(task);
    emit tasksChanged(m_activeTasks);
}
