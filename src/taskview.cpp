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

#include <cassert>

#include <QFile>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QPainter>
#include <QString>
#include <QTimer>
#include <QMouseEvent>
#include <QList>
#include <QClipboard>
#include <QApplication>
#include <QDebug>
#include <QProgressDialog>

#include <KMessageBox>
#include <KUrlRequester>
#include <KLocalizedString>

#include "csvexportdialog.h"
#include "desktoptracker.h"
#include "edittaskdialog.h"
#include "idletimedetector.h"
#include "plannerparser.h"
#include "ktimetracker.h"
#include "task.h"
#include "timekard.h"
#include "treeviewheadercontextmenu.h"
#include "focusdetector.h"
#include "ktt_debug.h"

#define T_LINESIZE 1023

class DesktopTracker;

bool readBoolEntry(const QString& key)
{
    return KSharedConfig::openConfig()->group(QString()).readEntry(key, true);
}

void writeEntry(const QString& key, bool value)
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.writeEntry(key, value);
    config.sync();
}

void deleteEntry(const QString& key)
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.deleteEntry(key);
    config.sync();
}

//BEGIN TaskViewDelegate (custom painting of the progress column)
class TaskViewDelegate : public QStyledItemDelegate {
public:
    explicit TaskViewDelegate(QObject *parent)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if (index.column () == 6) {
            QApplication::style()->drawControl( QStyle::CE_ItemViewItem, &option, painter );
            int rX = option.rect.x() + 2;
            int rY = option.rect.y() + 2;
            int rWidth = option.rect.width() - 4;
            int rHeight = option.rect.height() - 4;
            int value = index.model()->data( index ).toInt();
            int newWidth = (int)(rWidth * (value / 100.));

            if(QApplication::isLeftToRight())
            {
                int mid = rY + rHeight / 2;
                int width = rWidth / 2;
                QLinearGradient gradient1( rX, mid, rX + width, mid);
                gradient1.setColorAt( 0, Qt::red );
                gradient1.setColorAt( 1, Qt::yellow );
                painter->fillRect( rX, rY, (newWidth < width) ? newWidth : width, rHeight, gradient1 );

                if (newWidth > width)
                {
                    QLinearGradient gradient2( rX + width, mid, rX + 2 * width, mid);
                    gradient2.setColorAt( 0, Qt::yellow );
                    gradient2.setColorAt( 1, Qt::green );
                    painter->fillRect( rX + width, rY, newWidth - width, rHeight, gradient2 );
                }

                painter->setPen( option.state & QStyle::State_Selected ? option.palette.highlight().color() : option.palette.background().color() );
                for (int x = rHeight; x < newWidth; x += rHeight)
                {
                    painter->drawLine( rX + x, rY, rX + x, rY + rHeight - 1 );
                }
            }
            else
            {
                int mid = option.rect.height() - rHeight / 2;
                int width = rWidth / 2;
                QLinearGradient gradient1( rX, mid, rX + width, mid);
                gradient1.setColorAt( 0, Qt::red );
                gradient1.setColorAt( 1, Qt::yellow );
                painter->fillRect( option.rect.height(), rY, (newWidth < width) ? newWidth : width, rHeight, gradient1 );

                if (newWidth > width)
                {
                    QLinearGradient gradient2( rX + width, mid, rX + 2 * width, mid);
                    gradient2.setColorAt( 0, Qt::yellow );
                    gradient2.setColorAt( 1, Qt::green );
                    painter->fillRect( rX + width, rY, newWidth - width, rHeight, gradient2 );
                }

                painter->setPen( option.state & QStyle::State_Selected ? option.palette.highlight().color() : option.palette.background().color() );
                for (int x = rWidth- rHeight; x > newWidth; x -= rHeight)
                {
                    painter->drawLine( rWidth - x, rY, rWidth - x, rY + rHeight - 1 );
                }

            }
            painter->setPen( Qt::black );
            painter->drawText( option.rect, Qt::AlignCenter | Qt::AlignVCenter, QString::number(value) + " %" );
        }
        else
        {
            QStyledItemDelegate::paint( painter, option, index );
        }
    }
};
//END

TaskView::TaskView(QWidget* parent)
    : QTreeWidget(parent)
    , m_isLoading(false)
    , m_storage(new TimeTrackerStorage())
    , m_focusTrackingActive(false)
    , m_lastTaskWithFocus(nullptr)
    , m_popupPercentageMenu(nullptr)
    , m_popupPriorityMenu(nullptr)
    , m_focusDetector(new FocusDetector())
{
    connect( this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
           this, SLOT(itemStateChanged(QTreeWidgetItem*)) );
    connect( this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
           this, SLOT(itemStateChanged(QTreeWidgetItem*)) );
    connect( this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
           this, SLOT(slotItemDoubleClicked(QTreeWidgetItem*,int)) );
    connect(m_focusDetector, &FocusDetector::newFocus, this, &TaskView::newFocusWindowDetected);

    QStringList labels;
    setWindowFlags( windowFlags() | Qt::WindowContextHelpButtonHint );
    labels << i18n( "Task Name" ) << i18n( "Session Time" ) << i18n( "Time" )
         << i18n( "Total Session Time" ) << i18n( "Total Time" )
         << i18n( "Priority" ) << i18n( "Percent Complete" );
    setHeaderLabels( labels );
    headerItem()->setWhatsThis(0,i18n("The task name is what you call the task, it can be chosen freely."));
    headerItem()->setWhatsThis(1,i18n("The session time is the time since you last chose \"start new session.\""));
    headerItem()->setWhatsThis(3,i18n("The total session time is the session time of this task and all its subtasks."));
    headerItem()->setWhatsThis(4,i18n("The total time is the time of this task and all its subtasks."));
    setAllColumnsShowFocus( true );
    setSortingEnabled( true );
    setAlternatingRowColors( true );
    setDragDropMode( QAbstractItemView::InternalMove );
    setItemDelegateForColumn( 6, new TaskViewDelegate(this) );

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
    TreeViewHeaderContextMenu *headerContextMenu = new TreeViewHeaderContextMenu( this, this, TreeViewHeaderContextMenu::AlwaysCheckBox, QVector<int>{0});
    connect(headerContextMenu, &TreeViewHeaderContextMenu::columnToggled, this, &TaskView::slotColumnToggled);

    // Context Menu
    m_popupPercentageMenu = new QMenu(this);
    for (int i = 0; i <= 100; i += 10) {
        QString label = i18n("%1 %" , i);
        m_percentage[m_popupPercentageMenu->addAction(label)] = i;
    }
    connect(m_popupPercentageMenu, &QMenu::triggered, this, &TaskView::slotSetPercentage);

    m_popupPriorityMenu = new QMenu(this);
    for (int i = 0; i <= 9; ++i) {
        QString label;
        switch (i) {
        case 0:
            label = i18n("unspecified");
            break;
        case 1:
            label = i18nc("combox entry for highest priority", "1 (highest)");
            break;
        case 5:
            label = i18nc("combox entry for medium priority", "5 (medium)");
            break;
        case 9:
            label = i18nc("combox entry for lowest priority", "9 (lowest)");
            break;
        default:
            label = QString("%1").arg(i);
            break;
        }
        m_priority[m_popupPriorityMenu->addAction(label)] = i;
    }
    connect(m_popupPriorityMenu, &QMenu::triggered, this, &TaskView::slotSetPriority);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &TaskView::customContextMenuRequested, this, &TaskView::slotCustomContextMenuRequested);

    reconfigure();
    sortByColumn(0, Qt::AscendingOrder);
    for (int i = 0; i <= columnCount(); ++i) {
        resizeColumnToContents(i);
    }
}

void TaskView::newFocusWindowDetected(const QString &taskName)
{
    QString newTaskName = taskName;
    newTaskName.remove( '\n' );

    if ( m_focusTrackingActive )
    {
        bool found = false;  // has taskName been found in our tasks
        stopTimerFor( m_lastTaskWithFocus );
        int i = 0;
        for ( Task* task = itemAt( i ); task; task = itemAt( ++i ) )
        {
            if ( task->name() == newTaskName )
            {
                found = true;
                startTimerFor( task );
                m_lastTaskWithFocus = task;
            }
        }
        if ( !found )
        {
            QString taskuid = addTask( newTaskName );
            if ( taskuid.isNull() )
            {
                KMessageBox::error( 0, i18n(
                "Error storing new task. Your changes were not saved. Make sure you can edit your iCalendar file. Also quit all applications using this file and remove any lock file related to its name from ~/.kde/share/apps/kabc/lock/ " ) );
            }
            i = 0;
            for ( Task* task = itemAt( i ); task; task = itemAt( ++i ) )
            {
                if (task->name() == newTaskName)
                {
                    startTimerFor( task );
                    m_lastTaskWithFocus = task;
                }
            }
        }
        emit updateButtons();
    } // focustrackingactive
}

void TaskView::mouseMoveEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );

    if (index.isValid() && index.column() == 6)
    {
        int newValue = (int)((event->pos().x() - visualRect(index).x()) / (double)(visualRect(index).width()) * 100);
        if ( event->modifiers() & Qt::ShiftModifier )
        {
            int delta = newValue % 10;
            if ( delta >= 5 )
            {
                newValue += (10 - delta);
            } else
            {
                newValue -= delta;
            }
        }
        QTreeWidgetItem *item = itemFromIndex( index );
        if (item && item->isSelected())
        {
            Task *task = static_cast<Task*>(item);
            if (task)
            {
                task->setPercentComplete( newValue, m_storage );
                emit updateButtons();
            }
        }
    }
    else
    {
        QTreeWidget::mouseMoveEvent( event );
    }
}

void TaskView::mousePressEvent( QMouseEvent *event )
{
    qCDebug(KTT_LOG) << "Entering function, event->button()=" << event->button();
    QModelIndex index = indexAt( event->pos() );

    // if the user toggles a task as complete/incomplete
    if ( index.isValid() && index.column() == 0 && visualRect( index ).x() <= event->pos().x()
      && event->pos().x() < visualRect( index ).x() + 19)
    {
        QTreeWidgetItem *item = itemFromIndex( index );
        if (item)
        {
            Task *task = static_cast<Task*>(item);
            if (task)
            {
                if (task->isComplete()) task->setPercentComplete( 0, m_storage );
                else
                {
                    task->setPercentComplete( 100, m_storage );
                }
                emit updateButtons();
            }
        }
    }
    else // the user did not mark a task as complete/incomplete
    {
        if ( KTimeTrackerSettings::configPDA() )
        // if you have a touchscreen, you cannot right-click. So, display context menu on any click.
        {
            QPoint newPos = viewport()->mapToGlobal( event->pos() );
            emit contextMenuRequested( newPos );
        }
        QTreeWidget::mousePressEvent( event );
    }
}

TimeTrackerStorage* TaskView::storage()
{
    return m_storage;
}

TaskView::~TaskView()
{
    delete m_storage;
    KTimeTrackerSettings::self()->writeConfig();
}

Task* TaskView::currentItem() const
{
    qCDebug(KTT_LOG) << "Entering function";
    return static_cast< Task* >( QTreeWidget::currentItem() );
}

Task* TaskView::itemAt(int i)
/* This procedure delivers the item at the position i in the KTreeWidget.
Every item is a task. The items are counted linearily. The uppermost item
has the number i=0. */
{
    if ( topLevelItemCount() == 0 ) return 0;

    QTreeWidgetItemIterator item( this );
    while( *item && i-- ) ++item;

    qCDebug(KTT_LOG) << "Leaving TaskView::itemAt" << "returning " << (*item==0);
    if ( !( *item ) )
        return 0;
    else
        return (Task*)*item;
}

void TaskView::load( const QString &fileName )
{
    assert( !( fileName.isEmpty() ) );

    // if the program is used as an embedded plugin for konqueror, there may be a need
    // to load from a file without touching the preferences.
    qCDebug(KTT_LOG) << "Entering function";
    m_isLoading = true;
    QString err = m_storage->load(this, fileName);

    if (!err.isEmpty())
    {
        KMessageBox::error(this, err);
        m_isLoading = false;
        qCDebug(KTT_LOG) << "Leaving TaskView::load";
        return;
    }

    // Register tasks with desktop tracker
    int i = 0;
    for ( Task* t = itemAt(i); t; t = itemAt(++i) )
        m_desktopTracker->registerForDesktops( t, t->desktops() );
    // till here
    // Start all tasks that have an event without endtime
    i = 0;
    for (Task* t = itemAt(i); t; t = itemAt(++i)) {
        if (!m_storage->allEventsHaveEndTiMe(t)) {
            t->resumeRunning();
            m_activeTasks.append(t);
            emit updateButtons();
            if (m_activeTasks.count() == 1) {
                emit timersActive();
            }
            emit tasksChanged(m_activeTasks);
        }
    }
    // till here

    if (topLevelItemCount() > 0) {
        restoreItemState();
        setCurrentItem(topLevelItem( 0 ));
        if ( !m_desktopTracker->startTracking().isEmpty() )
            KMessageBox::error( 0, i18n( "Your virtual desktop number is too high, desktop tracking will not work" ) );
        m_isLoading = false;
        refresh();
    }
    for (int i = 0; i <= columnCount(); ++i) {
        resizeColumnToContents(i);
    }
    qCDebug(KTT_LOG) << "Leaving function";
}

void TaskView::restoreItemState()
/* Restores the item state of every item. An item is a task in the list.
Its state is whether it is expanded or not. If a task shall be expanded
is stored in the _preferences object. */
{
    qCDebug(KTT_LOG) << "Entering function";

    if (topLevelItemCount() > 0) {
        for (Task *task : getAllTasks()) {
            task->setExpanded(readBoolEntry(task->uid()));
        }
    }
    qCDebug(KTT_LOG) << "Leaving function";
}

void TaskView::itemStateChanged(QTreeWidgetItem* item)
{
    qDebug() << "Entering function";
    if (!item || m_isLoading) {
        return;
    }

    Task *t = (Task *)item;
    qCDebug(KTT_LOG) <<"TaskView::itemStateChanged()" <<" uid=" << t->uid() <<" state=" << t->isExpanded();
    writeEntry(t->uid(), t->isExpanded());
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
    qCDebug(KTT_LOG) << "entering function";
    int i = 0;
    for (Task* t = itemAt(i); t; t = itemAt(++i)) {
        t->setPixmapProgress();
        t->update();  // maybe there was a change in the times's format
    }

    // remove root decoration if there is no more child.
    i = 0;
    while (itemAt(++i) && itemAt(i)->depth() == 0){};
    //setRootIsDecorated( itemAt( i ) && ( itemAt( i )->depth() != 0 ) );
    // FIXME workaround? seems that the QItemDelegate for the procent column only
    // works properly if rootIsDecorated == true.
    setRootIsDecorated(true);

    emit updateButtons();
    qCDebug(KTT_LOG) << "exiting TaskView::refresh()";
}

QString TaskView::reFreshTimes()
/** Refresh the times of the tasks, e.g. when the history has been changed by the user */
{
    qCDebug(KTT_LOG) << "Entering function";
    QString err;
    // re-calculate the time for every task based on events in the history
    KCalCore::Event::List eventList = storage()->rawevents(); // get all events (!= tasks)
    int n = -1;
    resetDisplayTimeForAllTasks();
    emit reSetTimes();
    while (itemAt(++n)) // loop over all tasks
    {
        for( KCalCore::Event::List::iterator i = eventList.begin(); i != eventList.end(); ++i ) // loop over all events
        {
            if ( (*i)->relatedTo() == itemAt(n)->uid() ) // if event i belongs to task n
            {
                QDateTime kdatetimestart = (*i)->dtStart();
                QDateTime kdatetimeend = (*i)->dtEnd();
                QDateTime eventstart = QDateTime::fromString(kdatetimestart.toString().remove("Z"));
                QDateTime eventend = QDateTime::fromString(kdatetimeend.toString().remove("Z"));
                int duration=eventstart.secsTo( eventend )/60;
                itemAt(n)->addTime( duration );
                emit totalTimesChanged( 0, duration );
                qCDebug(KTT_LOG) << "duration is " << duration;

                if ( itemAt(n)->sessionStartTiMe().isValid() )
                {
                    // if there is a session
                    if ((itemAt(n)->sessionStartTiMe().secsTo( eventstart )>0) &&
                        (itemAt(n)->sessionStartTiMe().secsTo( eventend )>0))
                    // if the event is after the session start
                    {
                        int sessionTime=eventstart.secsTo( eventend )/60;
                        itemAt(n)->setSessionTime( itemAt(n)->sessionTime()+sessionTime );
                    }
                }
                else
                // so there is no session at all
                {
                    itemAt(n)->addSessionTime( duration );
                    emit totalTimesChanged( duration, 0 );
                };
            }
        }
    }

    for (int i=0; i<count(); ++i) itemAt(i)->recalculatetotaltime();
    for (int i=0; i<count(); ++i) itemAt(i)->recalculatetotalsessiontime();

    refresh();
    qCDebug(KTT_LOG) << "Leaving TaskView::reFreshTimes()";
    return err;
}

void TaskView::importPlanner(const QString& fileName)
{
    qCDebug(KTT_LOG) << "entering importPlanner";
    PlannerParser *handler = new PlannerParser(this);
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
    return m_storage->report(this, rc);
}

void TaskView::exportcsvFile()
{
    qCDebug(KTT_LOG) << "TaskView::exportcsvFile()";

    CSVExportDialog dialog(ReportCriteria::CSVTotalsExport, this);
    if (currentItem() && currentItem()->isRoot()) {
        dialog.enableTasksToExportQuestion();
    }
    if (dialog.exec()) {
        QString err = m_storage->report(this, dialog.reportCriteria());
        if (!err.isEmpty()) {
            KMessageBox::error(this, i18n(err.toLatin1()));
        }
    }
}

QString TaskView::exportcsvHistory()
{
    qCDebug(KTT_LOG) << "TaskView::exportcsvHistory()";
    QString err;

    CSVExportDialog dialog( ReportCriteria::CSVHistoryExport, this );
    if ( currentItem() && currentItem()->isRoot() )
        dialog.enableTasksToExportQuestion();
    if ( dialog.exec() )
    {
        err = m_storage->report( this, dialog.reportCriteria() );
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
    int i = 0;
    while (itemAt(i)) {
        result << itemAt(i)->name();
        ++i;
    }
    return result;
}

Task* TaskView::task(const QString& taskId)
{
    Task* result = nullptr;
    int i = -1;
    while (itemAt(++i)) {
        if (itemAt(i)) {
            if (itemAt(i)->uid() == taskId) {
                result = itemAt(i);
            }
        }
    }

    return result;
}

void TaskView::dropEvent (QDropEvent* event)
{
    QTreeWidget::dropEvent(event);
    reFreshTimes();
}

void TaskView::scheduleSave()
{
    m_manualSaveTimer->start(10);
}

void TaskView::save()
{
    qCDebug(KTT_LOG) << "Entering TaskView::save()";
    QString err = m_storage->save(this);

    if (!err.isNull()) {
        QString errMsg = m_storage->icalfile() + ":\n";

        if (err == QString("Could not save. Could not lock file.")) {
            errMsg += i18n("Could not save. Disk full?");
        } else {
            errMsg += i18n("Could not save.");
        }

        KMessageBox::error(this, errMsg);
    }
}

void TaskView::startCurrentTimer()
{
    startTimerFor( currentItem() );
}

void TaskView::startTimerFor( Task* task, const QDateTime &startTime )
{
    qCDebug(KTT_LOG) << "Entering function";
    if (task != 0 && m_activeTasks.indexOf(task) == -1)
    {
        if (!task->isComplete())
        {
            if ( KTimeTrackerSettings::uniTasking() ) stopAllTimers();
            m_idleTimeDetector->startIdleDetection();
            task->setRunning(true, m_storage, startTime);
            m_activeTasks.append(task);
            emit updateButtons();
            if ( m_activeTasks.count() == 1 )
                emit timersActive();
            emit tasksChanged( m_activeTasks );
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
    QProgressDialog dialog(i18n("Stopping timers..."), i18n("Cancel"), 0, m_activeTasks.count(), this);
    if (m_activeTasks.count() > 1) {
        dialog.show();
    }

    for (Task *task : m_activeTasks) {
//        kapp->processEvents();
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
    emit tasksChanged( m_activeTasks );
}

void TaskView::stopCurrentTimer()
{
    stopTimerFor( currentItem() );
    if ( m_focusTrackingActive && m_lastTaskWithFocus == currentItem() )
    {
        toggleFocusTracking();
    }
}

void TaskView::minuteUpdate()
{
    addTimeToActiveTasks(1, false);
}

void TaskView::addTimeToActiveTasks(int minutes, bool save_data)
{
    foreach ( Task *task, m_activeTasks )
        task->changeTime( minutes, ( save_data ? m_storage : 0 ) );
}

void TaskView::newTask()
{
    newTask(i18n("New Task"), 0);
}

void TaskView::newTask(const QString& caption, Task* parent)
{
    EditTaskDialog *dialog = new EditTaskDialog(this, caption, 0);
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
    setSortingEnabled(false);

    Task* task;
    if (parent) {
        task = new Task(taskname, taskdescription, total, session, desktops, parent);
    } else {
        task = new Task(taskname, taskdescription, total, session, desktops, this);
    }

    task->setUid(m_storage->addTask(task, parent));
    QString taskuid = task->uid();
    if (!taskuid.isNull()) {
        m_desktopTracker->registerForDesktops(task, desktops);
        setCurrentItem(task);
        task->setSelected(true);
        task->setPixmapProgress();
        save();
    } else {
        delete task;
    }

    setSortingEnabled(true);
    return taskuid;
}

void TaskView::newSubTask()
{
    Task* task = currentItem();
    if (!task) {
        return;
    }

    newTask(i18n("New Sub Task"), task);
    task->setExpanded(true);
    refresh();
}

void TaskView::editTask()
{
    qCDebug(KTT_LOG) <<"Entering editTask";
    Task* task = currentItem();
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
    Task* task = currentItem();
    if (task == 0) {
        KMessageBox::information(0, i18n("No task selected."));
        return;
    }

    if (completion < 0) {
        completion = 0;
    }
    if (completion < 100) {
        task->setPercentComplete(completion, m_storage);
        task->setPixmapProgress();
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
    qCDebug(KTT_LOG) << "Entering function";
    if (task == 0) {
        task = currentItem();
    }

    if (currentItem() == 0) {
        KMessageBox::information(0, i18n("No task selected."));
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
    if (!currentItem()) {
        KMessageBox::information(nullptr, i18n("No task selected."));
        return;
    }

    currentItem()->setPercentComplete(100, m_storage);
    currentItem()->setPixmapProgress();
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
    qCDebug(KTT_LOG) << "Entering function";
    TimeKard t;
    QApplication::clipboard()->setText(t.totalsAsText(this, rc));
    return QString();
}

QString TaskView::setClipBoardText(const QString& s)
{
    QString err; // maybe we find possible errors later
//    KApplication::clipboard()->setText(s);
    return err;
}

void TaskView::slotItemDoubleClicked(QTreeWidgetItem* item, int)
{
    if (item) {
        Task *task = static_cast<Task*>( item );
        if ( task )
        {
            if ( task->isRunning() )
            {
                stopCurrentTimer();
            }
            else if ( !task->isComplete() )
            {
                stopAllTimers();
                startCurrentTimer();
            }
        }
    }
}

void TaskView::slotColumnToggled( int column )
{
    switch ( column )
    {
    case 1:
        KTimeTrackerSettings::setDisplaySessionTime( !isColumnHidden( 1 ) );
        break;
    case 2:
        KTimeTrackerSettings::setDisplayTime( !isColumnHidden( 2 ) );
        break;
    case 3:
        KTimeTrackerSettings::setDisplayTotalSessionTime( !isColumnHidden( 3 ) );
        break;
    case 4:
        KTimeTrackerSettings::setDisplayTotalTime( !isColumnHidden( 4 ) );
        break;
    case 5:
        KTimeTrackerSettings::setDisplayPriority( !isColumnHidden( 5 ) );
        break;
    case 6:
        KTimeTrackerSettings::setDisplayPercentComplete( !isColumnHidden( 6 ) );
        break;
    }
    KTimeTrackerSettings::self()->writeConfig();
}

void TaskView::slotCustomContextMenuRequested(const QPoint& pos)
{
    QPoint newPos = viewport()->mapToGlobal(pos);
    int column = columnAt(pos.x());

    switch (column) {
    case 6: /* percentage */
        m_popupPercentageMenu->popup(newPos);
        break;

    case 5: /* priority */
        m_popupPriorityMenu->popup(newPos);
        break;

    default:
        emit contextMenuRequested(newPos);
        break;
    }
}

void TaskView::slotSetPercentage(QAction* action)
{
    if (currentItem()) {
        currentItem()->setPercentComplete(m_percentage[action], storage());
        emit updateButtons();
    }
}

void TaskView::slotSetPriority(QAction* action)
{
    if (currentItem()) {
        currentItem()->setPriority(m_priority[action]);
    }
}

bool TaskView::isFocusTrackingActive() const
{
    return m_focusTrackingActive;
}

QList<Task*> TaskView::activeTasks() const
{
    return m_activeTasks;
}

void TaskView::reconfigure()
{
    qCDebug(KTT_LOG) << "Entering function";
    /* Adapt columns */
    setColumnHidden( 1, !KTimeTrackerSettings::displaySessionTime() );
    setColumnHidden( 2, !KTimeTrackerSettings::displayTime() );
    setColumnHidden( 3, !KTimeTrackerSettings::displayTotalSessionTime() );
    setColumnHidden( 4, !KTimeTrackerSettings::displayTotalTime() );
    setColumnHidden( 5, !KTimeTrackerSettings::displayPriority() );
    setColumnHidden( 6, !KTimeTrackerSettings::displayPercentComplete() );

    /* idleness */
    m_idleTimeDetector->setMaxIdle( KTimeTrackerSettings::period() );
    m_idleTimeDetector->toggleOverAllIdleDetection( KTimeTrackerSettings::enabled() );

    /* auto save */
    if ( KTimeTrackerSettings::autoSave() )
    {
        m_autoSaveTimer->start(
            KTimeTrackerSettings::autoSavePeriod() * 1000 * secsPerMinute
        );
    }
    else if ( m_autoSaveTimer->isActive() )
    {
        m_autoSaveTimer->stop();
    }

    refresh();
}

QList<Task*> TaskView::getAllTasks()
{
    QList<Task*> tasks;
    for (QTreeWidgetItemIterator it(this); *it; ++it) {
        Task *task = dynamic_cast<Task*>(*it);
        tasks.append(task);
    }

    return tasks;
}
