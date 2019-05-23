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

    void sort(int column, Qt::SortOrder order) override;

    QList<TasksModelItem *> getAllItems();

    void addChild(TasksModelItem *item);

    Qt::DropActions supportedDropActions() const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    TasksModelItem *taskByUID(const QString &uid);

public Q_SLOTS:
    void setActiveIcon(int frameNumber);

Q_SIGNALS:
    void taskCompleted(Task *task);

private:
    void sortItems(QList<TasksModelItem*> *items, int column, Qt::SortOrder order);

    static bool itemLessThan(const QPair<TasksModelItem*,int> &left,
                             const QPair<TasksModelItem*,int> &right);
    static bool itemGreaterThan(const QPair<TasksModelItem*,int> &left,
                                const QPair<TasksModelItem*,int> &right);


    TasksModelItem *m_rootItem;
    QStringList m_headerLabels;
    QMovie *m_clockAnimation;
    mutable TasksModelItem *m_dragCutTask;
};

#endif // KTIMETRACKER_TASKSMODEL_H
