/*
 * Copyright (c) 2019 Alexander Potashev <aspotashev@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tasksmodel.h"

#include <QStack>
#include <QPixmap>
#include <QMovie>

#include <KLocalizedString>

#include "task.h"

TasksModel::TasksModel()
    : m_rootItem(new TasksModelItem(this, nullptr))
    , m_headerLabels{
        i18n("Task Name"),
        i18n("Session Time"),
        i18n("Time"),
        i18n("Total Session Time"),
        i18n("Total Time"),
        i18n("Priority"),
        i18n("Percent Complete")}
    , m_clockAnimation(nullptr)
    , m_dragCutTask(nullptr)
{
    Q_INIT_RESOURCE(pics);

    m_clockAnimation = new QMovie(":/pics/watch.gif", QByteArray(), this);

    // Prepare animated icon
    connect(m_clockAnimation, &QMovie::frameChanged, this, &TasksModel::setActiveIcon);

    // TODO: stop animation when no tasks are active
    m_clockAnimation->start();
}

void TasksModel::clear()
{
    beginResetModel();

    // Empty "m_children", move it to "children".
    QList<TasksModelItem*> children;
    children.swap(m_rootItem->m_children);

    for (int i = 0; i < children.count(); ++i) {
        TasksModelItem *item = children.at(i);
        item->m_parent = nullptr;
        delete item;
    }

    endResetModel();
}

int TasksModel::topLevelItemCount() const
{
    return m_rootItem->childCount();
}

int TasksModel::indexOfTopLevelItem(TasksModelItem *item) const
{
    return m_rootItem->m_children.indexOf(item);
}

TasksModelItem *TasksModel::topLevelItem(int index) const
{
    return m_rootItem->child(index);
}

TasksModelItem *TasksModel::takeTopLevelItem(int index)
{
    return m_rootItem->takeChild(index);
}

TasksModelItem *TasksModel::item(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    return static_cast<TasksModelItem*>(index.internalPointer());
}

QModelIndex TasksModel::index(TasksModelItem *item, int column) const
{
    if (!item || item == m_rootItem) {
        return {};
    }

    const TasksModelItem *par = item->parent();
    auto *itm = const_cast<TasksModelItem*>(item);
    if (!par) {
        par = m_rootItem;
    }
    int row = par->m_children.lastIndexOf(itm);
    return createIndex(row, column, itm);
}

QModelIndex TasksModel::index(int row, int column, const QModelIndex &parent) const
{
    int c = columnCount(parent);
    if (row < 0 || column < 0 || column >= c) {
        return {};
    }

    TasksModelItem *parentItem = parent.isValid() ? item(parent) : m_rootItem;
    if (parentItem && row < parentItem->childCount()) {
        TasksModelItem *itm = parentItem->child(row);
        if (itm) {
            return createIndex(row, column, itm);
        }

        return {};
    }

    return {};
}

QModelIndex TasksModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return {};
    }

    auto *item = static_cast<TasksModelItem *>(child.internalPointer());
    if (!item || item == m_rootItem) {
        return {};
    }

    TasksModelItem *parent = item->parent();
    return index(parent, 0);
}

int TasksModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_rootItem->childCount();
    }

    TasksModelItem *parentItem = item(parent);
    if (parentItem) {
        return parentItem->childCount();
    }

    return 0;
}

int TasksModel::columnCount(const QModelIndex &parent) const
{
    return m_headerLabels.size();
}

QVariant TasksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    switch (role) {
        case Qt::TextAlignmentRole:
            // Align task name: left
            // Align priority: center
            // Align HH:MM: right
            if (index.column() == 5) {
                return Qt::AlignCenter;
            } else if (index.column() >= 1) {
                return {Qt::AlignRight | Qt::AlignVCenter};
            }
            break;
        case Qt::WhatsThisRole:
            if (index.column() == 0) {
                return i18n("The task name is what you call the task, it can be chosen freely.");
            } else if (index.column() == 1) {
                return i18n("The session time is the time since you last chose \"start new session.\"");
            }
            break;
        case Qt::DecorationRole: {
            auto *task = dynamic_cast<Task *>(item(index));

            if (index.column() == 0) {
                return QPixmap(task->isComplete() ? ":/pics/task-complete.xpm" : ":/pics/task-incomplete.xpm");
            } else if (index.column() == 1) {
                return task->isRunning() ? m_clockAnimation->currentPixmap() : QPixmap(":/pics/empty-watch.xpm");
            }
            break;
        }
        default: {
            TasksModelItem *itm = item(index);
            if (itm) {
                return itm->data(index.column(), role);
            }
            break;
        }
    }

    return {};
}

QList<TasksModelItem *> TasksModel::getAllItems()
{
    QList<TasksModelItem*> res;

    QStack<TasksModelItem*> stack;
    stack.push(m_rootItem);
    while (!stack.isEmpty()) {
        TasksModelItem *item = stack.pop();
        if (item != m_rootItem) {
            res.append(item);
        }

        for (int c = 0; c < item->m_children.count(); ++c)
            stack.push(item->m_children.at(c));
    }

    return res;
}

QList<Task*> TasksModel::getAllTasks()
{
    QList<Task*> tasks;
    for (TasksModelItem *item : getAllItems()) {
        Task *task = dynamic_cast<Task*>(item);
        if (task) {
            tasks.append(task);
        } else {
            qFatal("dynamic_cast to Task failed");
        }
    }

    return tasks;
}

QVariant TasksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || orientation != Qt::Horizontal || section >= m_headerLabels.size()) {
        return {};
    }

    switch (role) {
        case Qt::DisplayRole:
            return m_headerLabels[section];
        case Qt::WhatsThisRole:
            switch (section) {
                case 0:
                    return i18n("The task name is what you call the task, it can be chosen freely.");
                case 1:
                    return i18n("The session time is the time since you last chose \"start new session.\"");
                case 3:
                    return i18n("The total session time is the session time of this task and all its subtasks.");
                case 4:
                    return i18n("The total time is the time of this task and all its subtasks.");
                default:
                    break;
            }
            break;
        default:
            break;
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

void TasksModel::setActiveIcon(int frameNumber)
{
    for (auto *task : getAllItems()) {
        task->invalidateRunningState();
    }
}

void TasksModel::addChild(TasksModelItem *item)
{
    m_rootItem->addChild(item);
}

Qt::ItemFlags TasksModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        // Allow move to top-level
        return Qt::ItemIsDropEnabled;
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

Qt::DropActions TasksModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

bool TasksModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                                 const QModelIndex &parent) const
{
    if (!m_dragCutTask) {
        return false;
    }

    return QAbstractItemModel::canDropMimeData(data, action, row, column, parent);
}

bool TasksModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent)) {
        return false;
    }

    if (action == Qt::IgnoreAction) {
        return true;
    }

    Task *task = dynamic_cast<Task *>(m_dragCutTask);
    if (!task) {
        return false;
    }

    // If have parent, move to subtasks of the parent task.
    // If don't have parent, move to top-level tasks.
    TasksModelItem *newParent = parent.isValid() ? item(parent) : m_rootItem;
    if (!newParent) {
        return false;
    }

    task->move(newParent);
    return true;
}

QMimeData *TasksModel::mimeData(const QModelIndexList &indexes) const
{
    m_dragCutTask = item(indexes[0]);
    return QAbstractItemModel::mimeData(indexes);
}

TasksModelItem *TasksModel::taskByUID(const QString &uid)
{
    for (auto *item : getAllItems()) {
        if (dynamic_cast<Task*>(item)->uid() == uid) {
            return item;
        }
    }

    return nullptr;
}
