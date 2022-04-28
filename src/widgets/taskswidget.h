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

#ifndef KTIMETRACKER_TASKSWIDGET_H
#define KTIMETRACKER_TASKSWIDGET_H

#include <QTreeView>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
QT_END_NAMESPACE

class TasksModel;
class Task;

class TasksWidget : public QTreeView
{
    Q_OBJECT

public:
    TasksWidget(QWidget *parent, QSortFilterProxyModel *filterProxyModel, TasksModel *tasksModel);
    ~TasksWidget() override = default;

    void setSourceModel(TasksModel *tasksModel);

    Task *taskAtViewIndex(QModelIndex viewIndex);

    /**  Return the current item in the view, cast to a Task pointer.  */
    Task *currentItem();

    /**
     * Restores the item state of every item. An item is a task in the list.
     * Its state is whether it is expanded or not. If a task shall be expanded
     * is stored in the _preferences object.
     */
    void restoreItemState();

    /** Used to refresh (e.g. after import) */
    void refresh();

    void reconfigure();

public Q_SLOTS:
    /** item state stores if a task is expanded so you can see the subtasks */
    void itemStateChanged(const QModelIndex &index);

    void slotCustomContextMenuRequested(const QPoint &);
    void slotSetPercentage(QAction *);
    void slotSetPriority(QAction *);
    void setFilterText(const QString &text);

Q_SIGNALS:
    void updateButtons();
    void contextMenuRequested(const QPoint &);
    void taskDoubleClicked(Task *);

private:
    bool mousePositionInsideCheckbox(QMouseEvent *event) const;

    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

    QSortFilterProxyModel *m_filterProxyModel;
    TasksModel *m_tasksModel;

    QMenu *m_popupPercentageMenu;
    QMap<QAction *, int> m_percentage;
    QMenu *m_popupPriorityMenu;
    QMap<QAction *, int> m_priority;
};

#endif // KTIMETRACKER_TASKSWIDGET_H
