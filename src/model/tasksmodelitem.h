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

#ifndef KTIMETRACKER_TASKSMODELITEM_H
#define KTIMETRACKER_TASKSMODELITEM_H

#include <QList>

class TasksModel;

class TasksModelItem
{
    friend class TasksModel;

public:
    explicit TasksModelItem(TasksModel *model, TasksModelItem *parent);
    virtual ~TasksModelItem();

    TasksModelItem *parent() const;
    TasksModelItem *child(int index) const;
    int childCount() const;
    TasksModelItem *takeChild(int index);
    int indexOfChild(TasksModelItem *child) const;
    void insertChild(int index, TasksModelItem *child);

    virtual QVariant data(int column, int role) const;

    void addChild(TasksModelItem *child);

    void invalidateRunningState();
    void invalidateCompletedState();

private:
    TasksModel *m_model;
    TasksModelItem *m_parent; // =nullptr for top-level items
    QList<TasksModelItem *> m_children;
};

#endif // KTIMETRACKER_TASKSMODELITEM_H
