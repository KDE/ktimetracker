/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KTIMETRACKER_TASKSMODELITEM_H
#define KTIMETRACKER_TASKSMODELITEM_H

#include <QList>
#include <QVariant>

class TasksModel;

class TasksModelItem
{
    friend class TasksModel;

public:
    explicit TasksModelItem(TasksModel *model, TasksModelItem *parent);
    virtual ~TasksModelItem() = default;

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

protected:
    void disconnectFromParent();

private:
    TasksModel *m_model;
    TasksModelItem *m_parent; // =nullptr for top-level items
    QList<TasksModelItem *> m_children;
};

#endif // KTIMETRACKER_TASKSMODELITEM_H
