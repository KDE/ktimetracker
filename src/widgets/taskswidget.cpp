/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "taskswidget.h"

#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include "ktimetracker.h"
#include "ktt_debug.h"
#include "model/task.h"
#include "model/tasksmodel.h"

bool readBoolEntry(const QString &key)
{
    return KSharedConfig::openConfig()->group(QString()).readEntry(key, true);
}

void writeEntry(const QString &key, bool value)
{
    KConfigGroup config = KSharedConfig::openConfig()->group(QString());
    config.writeEntry(key, value);
    config.sync();
}

//BEGIN ProgressColumnDelegate (custom painting of the progress column)
class ProgressColumnDelegate : public QStyledItemDelegate {
public:
    explicit ProgressColumnDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (index.column() == 6) {
            QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &option, painter);
            int rX = option.rect.x() + 2;
            int rY = option.rect.y() + 2;
            int rWidth = option.rect.width() - 4;
            int rHeight = option.rect.height() - 4;
            int value = index.model()->data(index).toInt();
            int newWidth = (int)(rWidth * (value / 100.));

            if (QApplication::isLeftToRight()) {
                int mid = rY + rHeight / 2;
                int width = rWidth / 2;
                QLinearGradient gradient1(rX, mid, rX + width, mid);
                gradient1.setColorAt(0, Qt::red);
                gradient1.setColorAt(1, Qt::yellow);
                painter->fillRect(rX, rY, (newWidth < width) ? newWidth : width, rHeight, gradient1);

                if (newWidth > width) {
                    QLinearGradient gradient2(rX + width, mid, rX + 2 * width, mid);
                    gradient2.setColorAt(0, Qt::yellow);
                    gradient2.setColorAt(1, Qt::green);
                    painter->fillRect(rX + width, rY, newWidth - width, rHeight, gradient2);
                }

                painter->setPen(option.state & QStyle::State_Selected ? option.palette.highlight().color()
                                                                      : option.palette.window().color());
                for (int x = rHeight; x < newWidth; x += rHeight) {
                    painter->drawLine(rX + x, rY, rX + x, rY + rHeight - 1);
                }
            } else {
                int mid = option.rect.height() - rHeight / 2;
                int width = rWidth / 2;
                QLinearGradient gradient1(rX, mid, rX + width, mid);
                gradient1.setColorAt(0, Qt::red);
                gradient1.setColorAt(1, Qt::yellow);
                painter->fillRect(option.rect.height(), rY, (newWidth < width) ? newWidth : width, rHeight, gradient1);

                if (newWidth > width) {
                    QLinearGradient gradient2(rX + width, mid, rX + 2 * width, mid);
                    gradient2.setColorAt(0, Qt::yellow);
                    gradient2.setColorAt(1, Qt::green);
                    painter->fillRect(rX + width, rY, newWidth - width, rHeight, gradient2);
                }

                painter->setPen(option.state & QStyle::State_Selected ? option.palette.highlight().color()
                                                                      : option.palette.window().color());
                for (int x = rWidth - rHeight; x > newWidth; x -= rHeight) {
                    painter->drawLine(rWidth - x, rY, rWidth - x, rY + rHeight - 1);
                }
            }
            painter->setPen(Qt::black);
            painter->drawText(option.rect, Qt::AlignCenter | Qt::AlignVCenter, QString::number(value) + QString::fromLatin1(" %"));
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }
};
//END

TasksWidget::TasksWidget(QWidget *parent, QSortFilterProxyModel *filterProxyModel, TasksModel *tasksModel)
    : QTreeView(parent)
    , m_filterProxyModel(filterProxyModel)
    , m_tasksModel(tasksModel)
    , m_popupPercentageMenu(nullptr)
    , m_popupPriorityMenu(nullptr)
{
    setModel(filterProxyModel);

    connect(this, &QTreeView::expanded, this, &TasksWidget::itemStateChanged);
    connect(this, &QTreeView::collapsed, this, &TasksWidget::itemStateChanged);

    setWindowFlags(windowFlags() | Qt::WindowContextHelpButtonHint);

    setAllColumnsShowFocus(true);
    setSortingEnabled(true);
    setAlternatingRowColors(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setItemDelegateForColumn(6, new ProgressColumnDelegate(this));

    // Context menu for task progress percentage
    m_popupPercentageMenu = new QMenu(this);
    for (int i = 0; i <= 100; i += 5) {
        QString label = i18nc("@item:inmenu Task progress", "%1 %", i);
        m_percentage[m_popupPercentageMenu->addAction(label)] = i;
    }
    connect(m_popupPercentageMenu, &QMenu::triggered, this, &TasksWidget::slotSetPercentage);

    // Context menu for task priority
    m_popupPriorityMenu = new QMenu(this);
    for (int i = 0; i <= 9; ++i) {
        QString label;
        switch (i) {
        case 0:
            label = i18nc("@item:inmenu Task priority", "unspecified");
            break;
        case 1:
            label = i18nc("@item:inmenu Task priority", "1 (highest)");
            break;
        case 5:
            label = i18nc("@item:inmenu Task priority", "5 (medium)");
            break;
        case 9:
            label = i18nc("@item:inmenu Task priority", "9 (lowest)");
            break;
        default:
            label = QStringLiteral("%1").arg(i);
            break;
        }
        m_priority[m_popupPriorityMenu->addAction(label)] = i;
    }
    connect(m_popupPriorityMenu, &QMenu::triggered, this, &TasksWidget::slotSetPriority);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &TasksWidget::customContextMenuRequested, this, &TasksWidget::slotCustomContextMenuRequested);

    sortByColumn(0, Qt::AscendingOrder);
}

void TasksWidget::itemStateChanged(const QModelIndex &index)
{
    Task *task = taskAtViewIndex(index);
    if (!task) {
        return;
    }

    qCDebug(KTT_LOG) << "TaskView::itemStateChanged()"
                     << " uid=" << task->uid() << " state=" << isExpanded(index);
    writeEntry(task->uid(), isExpanded(index));
}

void TasksWidget::slotCustomContextMenuRequested(const QPoint &pos)
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
        Q_EMIT contextMenuRequested(newPos);
        break;
    }
}

void TasksWidget::slotSetPercentage(QAction *action)
{
    if (currentItem()) {
        currentItem()->setPercentComplete(m_percentage[action]);
        Q_EMIT updateButtons();
    }
}

void TasksWidget::slotSetPriority(QAction *action)
{
    if (currentItem()) {
        currentItem()->setPriority(m_priority[action]);
    }
}

void TasksWidget::mouseMoveEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid() && index.column() == 6 && event->buttons() == Qt::LeftButton) {
        int newValue = (int)((event->pos().x() - visualRect(index).x()) / (double)(visualRect(index).width()) * 101);
        if (newValue > 100) {
            newValue = 100;
        }

        if (event->modifiers() & Qt::ShiftModifier) {
            int delta = newValue % 10;
            if (delta >= 5) {
                newValue += (10 - delta);
            } else {
                newValue -= delta;
            }
        }
        if (selectionModel()->isSelected(index)) {
            Task *task = taskAtViewIndex(index);
            if (task) {
                task->setPercentComplete(newValue);
                Q_EMIT updateButtons();
            }
        }
    } else {
        QTreeView::mouseMoveEvent(event);
    }
}

bool TasksWidget::mousePositionInsideCheckbox(QMouseEvent *event) const
{
    QModelIndex index = indexAt(event->pos());
    return index.isValid() && index.column() == 0 && visualRect(index).x() <= event->pos().x()
        && event->pos().x() < visualRect(index).x() + 19;
}

void TasksWidget::mousePressEvent(QMouseEvent *event)
{
    qCDebug(KTT_LOG) << "Entering function, event->button()=" << event->button();

    if (mousePositionInsideCheckbox(event) && event->buttons() == Qt::LeftButton) {
        // if the user toggles a task as complete/incomplete
        QModelIndex index = indexAt(event->pos());
        Task *task = taskAtViewIndex(index);
        if (task) {
            if (task->isComplete()) {
                task->setPercentComplete(0);
            } else {
                task->setPercentComplete(100);
            }
            Q_EMIT updateButtons();
        }
    } else {
        // the user did not mark a task as complete/incomplete
        if (KTimeTrackerSettings::configPDA()) {
            // if you have a touchscreen, you cannot right-click. So, display context menu on any click.
            QPoint newPos = viewport()->mapToGlobal(event->pos());
            Q_EMIT contextMenuRequested(newPos);
        }

        QTreeView::mousePressEvent(event);
    }
}

void TasksWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    qCDebug(KTT_LOG) << "Entering function, event->button()=" << event->button();
    QModelIndex index = indexAt(event->pos());

    // if the user toggles a task as complete/incomplete
    if (index.isValid() && !mousePositionInsideCheckbox(event)) {
        Task *task = taskAtViewIndex(index);
        if (task) {
            Q_EMIT taskDoubleClicked(task);
        }
    } else {
        QTreeView::mouseDoubleClickEvent(event);
    }
}

void TasksWidget::currentChanged(const QModelIndex & /*current*/, const QModelIndex & /*previous*/)
{
    Q_EMIT updateButtons();
}

void TasksWidget::restoreItemState()
{
    qCDebug(KTT_LOG) << "Entering function";

    if (m_tasksModel->topLevelItemCount() > 0) {
        for (auto *item : m_tasksModel->getAllItems()) {
            auto *task = dynamic_cast<Task *>(item);
            setExpanded(m_filterProxyModel->mapFromSource(m_tasksModel->index(task, 0)), readBoolEntry(task->uid()));
        }
    }
    qCDebug(KTT_LOG) << "Leaving function";
}

Task *TasksWidget::taskAtViewIndex(QModelIndex viewIndex)
{
//    if (!m_storage->isLoaded()) {
//        return nullptr;
//    }
    if (!m_tasksModel) {
        return nullptr;
    }

    QModelIndex index = m_filterProxyModel->mapToSource(viewIndex);
    return dynamic_cast<Task *>(m_tasksModel->item(index));
}

void TasksWidget::setSourceModel(TasksModel *tasksModel)
{
    m_tasksModel = tasksModel;
}

Task *TasksWidget::currentItem()
{
    return taskAtViewIndex(QTreeView::currentIndex());
}

void TasksWidget::setFilterText(const QString &text)
{
    m_filterProxyModel->setFilterFixedString(text);
}

void TasksWidget::refresh()
{
    // remove root decoration if there is no more child.
//    int i = 0;
//    while (itemAt(++i) && itemAt(i)->depth() == 0){};
    //setRootIsDecorated( itemAt( i ) && ( itemAt( i )->depth() != 0 ) );
    // FIXME workaround? seems that the QItemDelegate for the percent column only
    // works properly if rootIsDecorated == true.
    setRootIsDecorated(true);

    Q_EMIT updateButtons();
    qCDebug(KTT_LOG) << "exiting TaskView::refresh()";
}

void TasksWidget::reconfigure()
{
    /* Adapt columns */
    setColumnHidden(1, !KTimeTrackerSettings::displaySessionTime());
    setColumnHidden(2, !KTimeTrackerSettings::displayTime());
    setColumnHidden(3, !KTimeTrackerSettings::displayTotalSessionTime());
    setColumnHidden(4, !KTimeTrackerSettings::displayTotalTime());
    setColumnHidden(5, !KTimeTrackerSettings::displayPriority());
    setColumnHidden(6, !KTimeTrackerSettings::displayPercentComplete());

    refresh();
}

#include "moc_taskswidget.cpp"
