/*
    SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KTIMETRACKER_TASKSMODEL_H
#define KTIMETRACKER_TASKSMODEL_H

#include <QAbstractItemModel>

QT_BEGIN_NAMESPACE
class QMovie;
QT_END_NAMESPACE

class TasksModelItem;
class Task;

class TasksModel : public QAbstractItemModel
{
    Q_OBJECT

    friend class TasksModelItem;

public:
    TasksModel();
    ~TasksModel() override = default;

    // Clears the tree widget by removing all of its items and selections.
    void clear();

    int topLevelItemCount() const;
    int indexOfTopLevelItem(TasksModelItem *item) const;
    TasksModelItem *topLevelItem(int index) const;

    TasksModelItem *takeTopLevelItem(int index);

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex index(TasksModelItem *item, int column) const;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    TasksModelItem *item(const QModelIndex &index) const;

    QList<TasksModelItem *> getAllItems();
    QList<Task *> getAllTasks();

    QList<Task *> getActiveTasks();

    void addChild(TasksModelItem *item);

    Qt::DropActions supportedDropActions() const override;
    bool canDropMimeData(const QMimeData *data,
                         Qt::DropAction action,
                         int row,
                         int column,
                         const QModelIndex &parent) const override;
    bool
    dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /** return the task with the given UID */
    Task *taskByUID(const QString &uid);

    /*
     * Reset session time to zero for all tasks.
     *
     * This procedure starts a new session. We speak of session times,
     * overalltimes (comprising all sessions) and total times (comprising all subtasks).
     * That is why there is also a total session time.
     */
    void startNewSession();

    /**
     * Add time delta to all active tasks.
     * Does not modify events model.
     */
    void addTimeToActiveTasks(int64_t minutes);

public Q_SLOTS:
    void setActiveIcon(int frameNumber);

Q_SIGNALS:
    void taskCompleted(Task *task);
    void taskDropped();

private:
    TasksModelItem *m_rootItem;
    QStringList m_headerLabels;
    QMovie *m_clockAnimation;
    mutable QString m_dragCutTaskId;
};

#endif // KTIMETRACKER_TASKSMODEL_H
