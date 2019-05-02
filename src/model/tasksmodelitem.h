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

    virtual bool operator<(const TasksModelItem &other) const;

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
    void sortChildren(int column, Qt::SortOrder order, bool climb);
    void sortChildrenImpl(int column, Qt::SortOrder order, bool climb);

    TasksModel *m_model;
    TasksModelItem *m_parent; // =nullptr for top-level items
    QList<TasksModelItem *> m_children;
};

#endif // KTIMETRACKER_TASKSMODELITEM_H
