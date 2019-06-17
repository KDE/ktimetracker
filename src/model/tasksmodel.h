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
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    TasksModelItem *item(const QModelIndex &index) const;

    QList<TasksModelItem *> getAllItems();
    QList<Task *> getAllTasks();

    void addChild(TasksModelItem *item);

    Qt::DropActions supportedDropActions() const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /** return the task with the given UID */
    Task *taskByUID(const QString &uid);

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
