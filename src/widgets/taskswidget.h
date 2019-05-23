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

    Task* taskAtViewIndex(QModelIndex viewIndex);

    /**  Return the current item in the view, cast to a Task pointer.  */
    Task* currentItem();

    /**
     * Restores the item state of every item. An item is a task in the list.
     * Its state is whether it is expanded or not. If a task shall be expanded
     * is stored in the _preferences object.
     */
    void restoreItemState();

public Q_SLOTS:
    /** item state stores if a task is expanded so you can see the subtasks */
    void itemStateChanged(const QModelIndex &index);

    void slotCustomContextMenuRequested(const QPoint&);
    void slotSetPercentage(QAction*);
    void slotSetPriority(QAction*);

Q_SIGNALS:
    void updateButtons();
    void contextMenuRequested(const QPoint&);
    void taskDropped();
    void taskDoubleClicked(Task*);

private:
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void dropEvent(QDropEvent*) override;

    QSortFilterProxyModel* m_filterProxyModel;
    TasksModel *m_tasksModel;

    QMenu *m_popupPercentageMenu;
    QMap<QAction*, int> m_percentage;
    QMenu *m_popupPriorityMenu;
    QMap<QAction*, int> m_priority;
};

#endif // KTIMETRACKER_TASKSWIDGET_H
