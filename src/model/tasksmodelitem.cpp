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

#include "tasksmodelitem.h"

#include "tasksmodel.h"

TasksModelItem *TasksModelItem::takeChild(int index)
{
    if (index < 0 || index >= m_children.count()) {
        return nullptr;
    }

    m_model->beginRemoveRows(m_model->index(this, 0), index, index);

    TasksModelItem *item = m_children.takeAt(index);
    item->m_parent = nullptr;

    m_model->endRemoveRows();
    return item;
}

int TasksModelItem::indexOfChild(TasksModelItem *child) const
{
    return m_children.indexOf(child);
}

void TasksModelItem::insertChild(int index, TasksModelItem *child) {
    if (index < 0 || index > m_children.count() || child == nullptr) {
        return;
    }

    if (m_model) {
        if (m_model->m_rootItem == this) {
            child->m_parent = nullptr;
        } else {
            child->m_parent = this;
        }

        m_model->beginInsertRows(child->m_parent ? m_model->index(child->m_parent, 0) : QModelIndex(), index, index);
        m_children.insert(index, child);
        m_model->endInsertRows();
    } else {
        child->m_parent = this;
        m_children.insert(index, child);
    }
}

QVariant TasksModelItem::data(int /*unused*/, int /*unused*/) const
{
    return {};
}

TasksModelItem *TasksModelItem::parent() const
{
    return m_parent;
}

TasksModelItem *TasksModelItem::child(int index) const
{
    if (index < 0 || index >= m_children.size()) {
        return nullptr;
    }
    return m_children.at(index);
}

int TasksModelItem::childCount() const
{
    return m_children.count();
}

TasksModelItem::TasksModelItem(TasksModel *model, TasksModelItem *parent)
    : m_model(model)
    , m_parent(parent)
    , m_children()
{
}

void TasksModelItem::addChild(TasksModelItem *child)
{
    if (child) {
        insertChild(m_children.count(), child);
    }
}

TasksModelItem::~TasksModelItem()
{
    TasksModelItem *parent = m_parent ? m_parent : m_model->m_rootItem;

    int i = parent->m_children.indexOf(this);
    if (i >= 0) {
        m_model->beginRemoveRows(m_model->index(parent, 0), i, i);

        // users _could_ do changes when connected to rowsAboutToBeRemoved,
        // so we check again to make sure 'i' is valid
        if (!parent->m_children.isEmpty() && parent->m_children.at(i) == this) {
            parent->m_children.takeAt(i);
        }

        m_model->endRemoveRows();
    }
}

void TasksModelItem::invalidateRunningState()
{
    // invalidate icon in column "Session Time"
    QModelIndex index = m_model->index(this, 1);
    emit m_model->dataChanged(index, index, QVector<int>{Qt::DecorationRole});
}

void TasksModelItem::invalidateCompletedState() {
    // invalidate icon in column "Task Name"
    QModelIndex index = m_model->index(this, 0);
    emit m_model->dataChanged(index, index, QVector<int>{Qt::DecorationRole});
}
