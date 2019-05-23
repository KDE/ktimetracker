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

#include "taskview.h"

#include <QClipboard>
#include <QProgressDialog>
#include <QHeaderView>

#include <KMessageBox>
#include <KJobWidgets>
#include <KIO/StoredTransferJob>

#include "model/task.h"
#include "model/tasksmodel.h"
#include "model/eventsmodel.h"
#include "widgets/taskswidget.h"
#include "csvexportdialog.h"
#include "desktoptracker.h"
#include "edittaskdialog.h"
#include "idletimedetector.h"
#include "plannerparser.h"
#include "ktimetracker.h"
#include "export/totalsastext.h"
#include "treeviewheadercontextmenu.h"
#include "focusdetector.h"
#include "ktimetrackerutility.h"
#include "ktt_debug.h"

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

    connect(m_focusDetector, &FocusDetector::newFocus, this, &TaskView::newFocusWindowDetected);

    // set up the minuteTimer
    m_minuteTimer = new QTimer(this);
    connect( m_minuteTimer, SIGNAL(timeout()), this, SLOT(minuteUpdate()));
    m_minuteTimer->start(1000 * secsPerMinute);

    // Set up the idle detection.
    m_idleTimeDetector = new IdleTimeDetector(KTimeTrackerSettings::period());
    connect(m_idleTimeDetector, &IdleTimeDetector::subtractTime, this, &TaskView::subtractTime);
    connect(m_idleTimeDetector, &IdleTimeDetector::stopAllTimers, this, &TaskView::stopAllTimers);
    if (!m_idleTimeDetector->isIdleDetectionPossible()) {
        KTimeTrackerSettings::setEnabled(false);
    }

    // Setup auto save timer
    m_autoSaveTimer = new QTimer(this);
    connect( m_autoSaveTimer, SIGNAL(timeout()), this, SLOT(save()));

    // Setup manual save timer (to save changes a little while after they happen)
    m_manualSaveTimer = new QTimer(this);
    m_manualSaveTimer->setSingleShot( true );
    connect( m_manualSaveTimer, SIGNAL(timeout()), this, SLOT(save()));

    // Connect desktop tracker events to task starting/stopping
    m_desktopTracker = new DesktopTracker();
    connect( m_desktopTracker, SIGNAL(reachedActiveDesktop(Task*)),
           this, SLOT(startTimerFor(Task*)));
    connect( m_desktopTracker, SIGNAL(leftActiveDesktop(Task*)),
           this, SLOT(stopTimerFor(Task*)));

    // Header context menu
    TreeViewHeaderContextMenu *headerContextMenu = new TreeViewHeaderContextMenu(this, m_tasksWidget, QVector<int>{0});
    connect(headerContextMenu, &TreeViewHeaderContextMenu::columnToggled, this, &TaskView::slotColumnToggled);
}

void TaskView::newFocusWindowDetected(const QString &taskName)
{
    QString newTaskName = taskName;
    newTaskName.remove( '\n' );

    if ( m_focusTrackingActive )
    {
        bool found = false;  // has taskName been found in our tasks
        stopTimerFor( m_lastTaskWithFocus );
        for (Task *task : getAllTasks()) {
            if (task->name() == newTaskName) {
                found = true;
                startTimerFor(task);
                m_lastTaskWithFocus = task;
            }
        }
        if (!found) {
            QString taskuid = addTask(newTaskName);
            if (taskuid.isNull()) {
                KMessageBox::error(
                    nullptr,
                    i18n("Error storing new task. Your changes were not saved. "
                         "Make sure you can edit your iCalendar file. "
                         "Also quit all applications using this file and remove "
                         "any lock file related to its name from ~/.kde/share/apps/kabc/lock/ "));
            }
            for (Task *task : getAllTasks()) {
                if (task->name() == newTaskName) {
                    startTimerFor(task);
                    m_lastTaskWithFocus = task;
                }
            }
        }
        emit updateButtons();
    } // focustrackingactive
}

TimeTrackerStorage* TaskView::storage()
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
    assert( !( url.isEmpty() ) );

    if (m_tasksWidget) {
        delete m_tasksWidget;
        m_tasksWidget = nullptr;
    }

    // if the program is used as an embedded plugin for konqueror, there may be a need
    // to load from a file without touching the preferences.
    qCDebug(KTT_LOG) << "Entering function";
//    m_isLoading = true;
    QString err = m_storage->load(this, url);

    m_tasksWidget = new TasksWidget(dynamic_cast<QWidget*>(parent()), m_filterProxyModel, nullptr);
    connect(m_tasksWidget, &TasksWidget::updateButtons, this, &TaskView::updateButtons);
    connect(m_tasksWidget, &TasksWidget::contextMenuRequested, this, &TaskView::contextMenuRequested);
    connect(m_tasksWidget, &TasksWidget::taskDropped, this, &TaskView::reFreshTimes);
    connect(m_tasksWidget, &TasksWidget::taskDoubleClicked, this, &TaskView::onTaskDoubleClicked);
    m_tasksWidget->setRootIsDecorated(true);

    reconfigure();

    // Connect to the new model created by TimeTrackerStorage::load()
    m_filterProxyModel->setSourceModel(tasksModel());
    m_tasksWidget->setSourceModel(tasksModel());
    for (int i = 0; i <= tasksModel()->columnCount(QModelIndex()); ++i) {
        m_tasksWidget->resizeColumnToContents(i);
    }

    if (!err.isEmpty()) {
        KMessageBox::error(m_tasksWidget, err);
//        m_isLoading = false;
        qCDebug(KTT_LOG) << "Leaving TaskView::load";
        return;
    }

    // Register tasks with desktop tracker
    for (Task *task : getAllTasks()) {
        m_desktopTracker->registerForDesktops(task, task->desktops());
    }

    // Start all tasks that have an event without endtime
    for (Task *task : getAllTasks()) {
        if (!m_storage->allEventsHaveEndTiMe(task)) {
            task->resumeRunning();
            m_activeTasks.append(task);
            emit updateButtons();
            if (m_activeTasks.count() == 1) {
                emit timersActive();
            }
            emit tasksChanged(m_activeTasks);
        }
    }

    if (tasksModel()->topLevelItemCount() > 0) {
        m_tasksWidget->restoreItemState();
        m_tasksWidget->setCurrentIndex(m_filterProxyModel->mapFromSource(
            tasksModel()->index(tasksModel()->topLevelItem(0), 0)));

        if (!m_desktopTracker->startTracking().isEmpty()) {
            KMessageBox::error(nullptr, i18n("Your virtual desktop number is too high, desktop tracking will not work"));
        }
//        m_isLoading = false;
        refresh();
    }
    for (int i = 0; i <= tasksModel()->columnCount(QModelIndex()); ++i) {
        m_tasksWidget->resizeColumnToContents(i);
    }
    qCDebug(KTT_LOG) << "Leaving function";
}

void TaskView::closeStorage()
{
    m_storage->closeStorage();
}

bool TaskView::allEventsHaveEndTiMe()
{
    return m_storage->allEventsHaveEndTiMe();
}

void TaskView::refresh()
{
    if (!m_tasksWidget) {
        return;
    }

    qCDebug(KTT_LOG) << "entering function";
    for (Task *task : getAllTasks()) {
        task->invalidateCompletedState();
        task->update();  // maybe there was a change in the times's format
    }

    // remove root decoration if there is no more child.
//    int i = 0;
//    while (itemAt(++i) && itemAt(i)->depth() == 0){};
    //setRootIsDecorated( itemAt( i ) && ( itemAt( i )->depth() != 0 ) );
    // FIXME workaround? seems that the QItemDelegate for the procent column only
    // works properly if rootIsDecorated == true.
    m_tasksWidget->setRootIsDecorated(true);

    emit updateButtons();
    qCDebug(KTT_LOG) << "exiting TaskView::refresh()";
}

QString TaskView::reFreshTimes()
/** Refresh the times of the tasks, e.g. when the history has been changed by the user */
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    // re-calculate the time for every task based on events in the history
    resetDisplayTimeForAllTasks();

    for (Task *task : getAllTasks()) {
        // get all events for task
        for (const auto *event : storage()->eventsModel()->eventsForTask(task)) {
            QDateTime kdatetimestart = event->dtStart();
            QDateTime kdatetimeend = event->dtEnd();
            QDateTime eventstart = QDateTime::fromString(kdatetimestart.toString().remove("Z"));
            QDateTime eventend = QDateTime::fromString(kdatetimeend.toString().remove("Z"));
            int duration = eventstart.secsTo(eventend) / 60;
            task->addTime(duration);
            qCDebug(KTT_LOG) << "duration is" << duration;

            if (task->sessionStartTiMe().isValid()) {
                // if there is a session
                if (task->sessionStartTiMe().secsTo(eventstart) > 0 &&
                    task->sessionStartTiMe().secsTo(eventend) > 0) {
                    // if the event is after the session start
                    int sessionTime = eventstart.secsTo(eventend) / 60;
                    task->setSessionTime(task->sessionTime() + sessionTime);
                }
            } else {
                // so there is no session at all
                task->addSessionTime(duration);
            }
        }
    }

    for (Task *task : getAllTasks()) {
        task->recalculatetotaltime();
        task->recalculatetotalsessiontime();
    }

    refresh();
    qCDebug(KTT_LOG) << "Leaving TaskView::reFreshTimes()";
    return err;
}

void TaskView::importPlanner(const QString& fileName)
{
    qCDebug(KTT_LOG) << "entering importPlanner";
    PlannerParser *handler = new PlannerParser(this, storage()->projectModel(), m_tasksWidget->currentItem());
    QString lFileName = fileName;
    if (lFileName.isEmpty()) {
        lFileName = QFileDialog::getOpenFileName();
    }
    QFile xmlFile(lFileName);
    QXmlInputSource source(&xmlFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(handler);
    reader.parse(source);
    refresh();
}

QString TaskView::report(const ReportCriteria& rc)
{
    QString err;
    if (rc.reportType == ReportCriteria::CSVHistoryExport) {
        err = m_storage->exportCSVHistory(this, rc.from, rc.to, rc);
    } else { // rc.reportType == ReportCriteria::CSVTotalsExport
        if (!rc.bExPortToClipBoard) {
            err = exportcsvFile(rc);
        } else {
            err = clipTotals(rc);
        }
    }
    return err;
}

void TaskView::exportCSVFileDialog()
{
    qCDebug(KTT_LOG) << "TaskView::exportCSVFileDialog()";

    CSVExportDialog dialog(ReportCriteria::CSVTotalsExport, m_tasksWidget);
    if (m_tasksWidget->currentItem() && m_tasksWidget->currentItem()->isRoot()) {
        dialog.enableTasksToExportQuestion();
    }
    if (dialog.exec()) {
        QString err = report(dialog.reportCriteria());
        if (!err.isEmpty()) {
            KMessageBox::error(m_tasksWidget, i18n(err.toLatin1()));
        }
    }
}

QString TaskView::exportCSVHistoryDialog()
{
    qCDebug(KTT_LOG) << "TaskView::exportCSVHistoryDialog()";
    QString err;

    CSVExportDialog dialog(ReportCriteria::CSVHistoryExport, m_tasksWidget);
    if (m_tasksWidget->currentItem() && m_tasksWidget->currentItem()->isRoot()) {
        dialog.enableTasksToExportQuestion();
    }
    if (dialog.exec()) {
        err = report(dialog.reportCriteria());
    }

    return err;
}

long TaskView::count()
{
    long n = 0;
    for (Task *task : getAllTasks()) {
        ++n;
    }

    return n;
}

QStringList TaskView::tasks()
{
    QStringList result;
    for (Task *task : getAllTasks()) {
        result << task->name();
    }

    return result;
}

Task* TaskView::task(const QString& taskId)
{
    Task* result = nullptr;
    for (Task *task : getAllTasks()) {
        if (task->uid() == taskId) {
            result = task;
        }
    }

    return result;
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
        QString errMsg = m_storage->fileUrl().toString() + ":\n";

        if (err == QString("Could not save. Could not lock file.")) {
            errMsg += i18n("Could not save. Disk full?");
        } else {
            errMsg += i18n("Could not save.");
        }

        KMessageBox::error(m_tasksWidget, errMsg);
    }
}

void TaskView::startCurrentTimer()
{
    startTimerFor(m_tasksWidget->currentItem());
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
            task->setRunning(true, m_storage, startTime);
            m_activeTasks.append(task);
            emit updateButtons();
            if (m_activeTasks.count() == 1) {
                emit timersActive();
            }
            emit tasksChanged(m_activeTasks);
        }
    }
}

void TaskView::clearActiveTasks()
{
    m_activeTasks.clear();
}

void TaskView::stopAllTimers(const QDateTime& when)
{
    qCDebug(KTT_LOG) << "Entering function";
    QProgressDialog dialog(i18n("Stopping timers..."), i18n("Cancel"), 0, m_activeTasks.count(), m_tasksWidget);
    if (m_activeTasks.count() > 1) {
        dialog.show();
    }

    for (Task *task : m_activeTasks) {
        QApplication::processEvents();
        task->setRunning(false, m_storage, when);
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
    for (Task *task : getAllTasks()) {
        task->startNewSession();
    }
    qCDebug(KTT_LOG) << "Leaving TaskView::startNewSession";
}

void TaskView::resetTimeForAllTasks()
/* This procedure resets all times (session and overall) for all tasks and subtasks. */
{
    qCDebug(KTT_LOG) << "Entering function";
    for (Task *task : getAllTasks()) {
        task->resetTimes();
    }
    storage()->deleteAllEvents();
    qCDebug(KTT_LOG) << "Leaving function";
}

void TaskView::resetDisplayTimeForAllTasks()
/* This procedure resets all times (session and overall) for all tasks and subtasks. */
{
    qCDebug(KTT_LOG) << "Entering function";
    for (Task *task : getAllTasks()) {
        task->resetTimes();
    }
    qCDebug(KTT_LOG) << "Leaving function";
}

void TaskView::stopTimerFor(Task* task)
{
    qCDebug(KTT_LOG) << "Entering function";
    if ( task != 0 && m_activeTasks.indexOf(task) != -1 )
    {
        m_activeTasks.removeAll(task);
        task->setRunning(false, m_storage);
        if ( m_activeTasks.count() == 0 )
        {
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
    addTimeToActiveTasks(1, false);
}

void TaskView::addTimeToActiveTasks(int minutes, bool save_data)
{
    for (Task *task : m_activeTasks) {
        task->changeTime(minutes, save_data ? m_storage : nullptr);
    }
}

void TaskView::newTask()
{
    newTask(i18n("New Task"), nullptr);
}

void TaskView::newTask(const QString& caption, Task* parent)
{
    auto *dialog = new EditTaskDialog(this, caption, nullptr);
    long total, totalDiff, session, sessionDiff;
    DesktopList desktopList;

    int result = dialog->exec();
    if (result == QDialog::Accepted) {
        QString taskName = i18n("Unnamed Task");
        if (!dialog->taskName().isEmpty()) {
            taskName = dialog->taskName();
        }
        QString taskDescription = dialog->taskDescription();

        total = totalDiff = session = sessionDiff = 0;
        dialog->status(&desktopList);

        // If all available desktops are checked, disable auto tracking,
        // since it makes no sense to track for every desktop.
        if (desktopList.size() == m_desktopTracker->desktopCount()) {
            desktopList.clear();
        }

        QString uid = addTask(taskName, taskDescription, total, session, desktopList, parent);
        if (uid.isNull()) {
            KMessageBox::error(nullptr, i18n(
                "Error storing new task. Your changes were not saved. "
                "Make sure you can edit your iCalendar file. Also quit "
                "all applications using this file and remove any lock "
                "file related to its name from ~/.kde/share/apps/kabc/lock/"));
        }
    }
    emit updateButtons();
}

QString TaskView::addTask(
    const QString& taskname, const QString& taskdescription, long total, long session,
    const DesktopList& desktops, Task* parent)
{
    qCDebug(KTT_LOG) << "Entering function; taskname =" << taskname;
    m_tasksWidget->setSortingEnabled(false);

    Task *task = new Task(
        taskname, taskdescription, total, session, desktops, this, storage()->projectModel(), parent);
    if (task->uid().isNull()) {
        qFatal("failed to generate UID");
    }

    m_desktopTracker->registerForDesktops(task, desktops);
    m_tasksWidget->setCurrentIndex(m_filterProxyModel->mapFromSource(tasksModel()->index(task, 0)));
    task->invalidateCompletedState();
    save();

    m_tasksWidget->setSortingEnabled(true);
    return task->uid();
}

void TaskView::newSubTask()
{
    Task* task = m_tasksWidget->currentItem();
    if (!task) {
        return;
    }

    newTask(i18n("New Sub Task"), task);

    m_tasksWidget->setExpanded(m_filterProxyModel->mapFromSource(tasksModel()->index(task, 0)), true);

    refresh();
}

void TaskView::editTask()
{
    qCDebug(KTT_LOG) <<"Entering editTask";
    Task* task = m_tasksWidget->currentItem();
    if (!task) {
        return;
    }

    DesktopList desktopList = task->desktops();
    DesktopList oldDeskTopList = desktopList;
    EditTaskDialog* dialog = new EditTaskDialog(this, i18n("Edit Task"), &desktopList);
    dialog->setTask(task->name());
    dialog->setDescription(task->description());
    int result = dialog->exec();
    if (result == QDialog::Accepted) {
        QString taskName = i18n("Unnamed Task");
        if (!dialog->taskName().isEmpty()) {
            taskName = dialog->taskName();
        }
        // setName only does something if the new name is different
        task->setName(taskName, m_storage);
        task->setDescription(dialog->taskDescription());
        // update session time as well if the time was changed
        if (!dialog->timeChange().isEmpty()) {
            task->changeTime(dialog->timeChange().toInt(), m_storage);
        }
        dialog->status(&desktopList);
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
        save();
        emit updateButtons();
    }
}

void TaskView::deleteTaskBatch(Task* task)
{
    QString uid=task->uid();
    task->remove(m_storage);
    deleteEntry(uid); // forget if the item was expanded or collapsed
    save();

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
            response = KMessageBox::warningContinueCancel( 0,
                i18n( "Are you sure you want to delete the selected"
                " task and its entire history?\n"
                "NOTE: all subtasks and their history will also "
                "be deleted."),
                i18n( "Deleting Task"), KStandardGuiItem::del());
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
    save();
    emit updateButtons();
}

void TaskView::subtractTime(int minutes)
{
    addTimeToActiveTasks(-minutes, false); // subtract time in memory, but do not store it
}

void TaskView::deletingTask(Task* deletedTask)
{
    qCDebug(KTT_LOG) << "Entering function";
    DesktopList desktopList;

    m_desktopTracker->registerForDesktops(deletedTask, desktopList);
    m_activeTasks.removeAll(deletedTask);

    emit tasksChanged(m_activeTasks);
}

void TaskView::markTaskAsIncomplete()
{
    setPerCentComplete(50); // if it has been reopened, assume half-done
}

QString TaskView::clipTotals( const ReportCriteria &rc )
// This function stores the user's tasks into the clipboard.
// rc tells how the user wants his report, e.g. all times or session times
{
    QApplication::clipboard()->setText(totalsAsText(tasksModel(), m_tasksWidget->currentItem(), rc));
    return QString();
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

void TaskView::reconfigure()
{
    /* Adapt columns */
    m_tasksWidget->setColumnHidden(1, !KTimeTrackerSettings::displaySessionTime());
    m_tasksWidget->setColumnHidden(2, !KTimeTrackerSettings::displayTime());
    m_tasksWidget->setColumnHidden(3, !KTimeTrackerSettings::displayTotalSessionTime());
    m_tasksWidget->setColumnHidden(4, !KTimeTrackerSettings::displayTotalTime());
    m_tasksWidget->setColumnHidden(5, !KTimeTrackerSettings::displayPriority());
    m_tasksWidget->setColumnHidden(6, !KTimeTrackerSettings::displayPercentComplete());

    /* idleness */
    m_idleTimeDetector->setMaxIdle(KTimeTrackerSettings::period());
    m_idleTimeDetector->toggleOverAllIdleDetection(KTimeTrackerSettings::enabled());

    /* auto save */
    if (KTimeTrackerSettings::autoSave()) {
        m_autoSaveTimer->start(KTimeTrackerSettings::autoSavePeriod() * 1000 * secsPerMinute);
    } else if (m_autoSaveTimer->isActive()) {
        m_autoSaveTimer->stop();
    }

    refresh();
}

QList<Task*> TaskView::getAllTasks()
{
    QList<Task*> tasks;
    if (m_storage->isLoaded()) {
        for (TasksModelItem *item : tasksModel()->getAllItems()) {
            Task *task = dynamic_cast<Task*>(item);
            if (task) {
                tasks.append(task);
            } else {
                qFatal("dynamic_cast to Task failed");
            }
        }
    }

    return tasks;
}

int TaskView::sortColumn() const
{
    return m_tasksWidget->header()->sortIndicatorSection();
}

//----------------------------------------------------------------------------
// Routines that handle Comma-Separated Values export file format.
//
QString TaskView::exportcsvFile(const ReportCriteria &rc)
{
    QString delim = rc.delimiter;
    QString dquote = rc.quote;
    QString double_dquote = dquote + dquote;
    QString err;
    QProgressDialog dialog(
        i18n("Exporting to CSV..."), i18n("Cancel"),
        0, static_cast<int>(2 * count()), m_tasksWidget, nullptr);
    dialog.setAutoClose(true);
    dialog.setWindowTitle(i18nc("@title:window", "Export Progress"));

    if (count() > 1) {
        dialog.show();
    }

    QString retval;

    // Find max task depth
    int maxdepth = 0;
    for (Task *task : getAllTasks()) {
        if (dialog.wasCanceled()) {
            break;
        }

        dialog.setValue(dialog.value() + 1);

//        if (tasknr % 15 == 0) {
//            QApplication::processEvents(); // repainting is slow
//        }
        QApplication::processEvents();

        if (task->depth() > maxdepth) {
            maxdepth = task->depth();
        }
    }

    // Export to file
    for (Task *task : getAllTasks()) {
        if (dialog.wasCanceled()) {
            break;
        }

        dialog.setValue(dialog.value() + 1);

//        if (tasknr % 15 == 0) {
//            QApplication::processEvents(); // repainting is slow
//        }
        QApplication::processEvents();

        // indent the task in the csv-file:
        for (int i = 0; i < task->depth(); ++i) {
            retval += delim;
        }

        /*
        // CSV compliance
        // Surround the field with quotes if the field contains
        // a comma (delim) or a double quote
        if (task->name().contains(delim) || task->name().contains(dquote))
        to_quote = true;
        else
        to_quote = false;
        */
        bool to_quote = true;

        if (to_quote) {
            retval += dquote;
        }

        // Double quotes replaced by a pair of consecutive double quotes
        retval += task->name().replace(dquote, double_dquote);

        if (to_quote) {
            retval += dquote;
        }

        // maybe other tasks are more indented, so to align the columns:
        for (int i = 0; i < maxdepth - task->depth(); ++i) {
            retval += delim;
        }

        retval += delim + formatTime(task->sessionTime(), rc.decimalMinutes)
                  + delim + formatTime(task->time(), rc.decimalMinutes)
                  + delim + formatTime(task->totalSessionTime(), rc.decimalMinutes)
                  + delim + formatTime(task->totalTime(), rc.decimalMinutes)
                  + '\n';
    }

    // save, either locally or remote
    if (rc.url.isLocalFile()) {
        QString filename = rc.url.toLocalFile();
        if (filename.isEmpty()) {
            filename = rc.url.url();
        }
        QFile f(filename);
        if (!f.open(QIODevice::WriteOnly)) {
            err = i18n("Could not open \"%1\".", filename);
        }
        if (err.length() == 0) {
            QTextStream stream(&f);
            // Export to file
            stream << retval;
            f.close();
        }
    } else {
        // use remote file
        auto* const job = KIO::storedPut(retval.toUtf8(), rc.url, -1);
        KJobWidgets::setWindow(job, &dialog);
        if (!job->exec()) {
            err = QString::fromLatin1("Could not upload");
        }
    }

    return err;
}

inline TasksModel *TaskView::tasksModel()
{
    return m_storage->tasksModel();
}

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
