#include "tasksmodel.h"

#include <QStack>
#include <QDebug>
#include <QPixmap>
#include <QMovie>
#include <QMimeData>

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
    , m_clockAnimation(new QMovie(":/pics/watch.gif", QByteArray(), this))
    , m_dragCutTask(nullptr)
{
    // Prepare animated icon
    connect(m_clockAnimation, &QMovie::frameChanged, this, &TasksModel::setActiveIcon);

    // TODO: stop animation when no tasks are active
    m_clockAnimation->start();
}

void TasksModel::clear()
{
    beginResetModel();

    for (int i = 0; i < m_rootItem->childCount(); ++i) {
        TasksModelItem *item = m_rootItem->m_children.at(i);
        item->m_parent = nullptr;
        delete item;
    }
    m_rootItem->m_children.clear();

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
            if (index.column() == 5) {
                // Align priority left
                return Qt::AlignCenter;
            } else if (index.column() >= 1) {
                // Align HH:MM right
                return Qt::AlignRight;
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

void TasksModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= columnCount(QModelIndex())) {
        return;
    }

    m_rootItem->sortChildren(column, order, true);
}

// static
bool TasksModel::itemLessThan(const QPair<TasksModelItem*,int> &left,
                              const QPair<TasksModelItem*,int> &right)
{
    return *(left.first) < *(right.first);
}

// static
bool TasksModel::itemGreaterThan(const QPair<TasksModelItem*,int> &left,
                                 const QPair<TasksModelItem*,int> &right)
{
    return *(right.first) < *(left.first);
}

void TasksModel::sortItems(QList<TasksModelItem*> *items, int column, Qt::SortOrder order)
{
    Q_UNUSED(column);

    // store the original order of indexes
    QVector<QPair<TasksModelItem*,int>> sorting(items->count());
    for (int i = 0; i < sorting.count(); ++i) {
        sorting[i].first = items->at(i);
        sorting[i].second = i;
    }

    // do the sorting
    typedef bool(*LessThan)(const QPair<TasksModelItem*,int>&,const QPair<TasksModelItem*,int>&);
    LessThan compare = (order == Qt::AscendingOrder ? &itemLessThan : &itemGreaterThan);
    std::stable_sort(sorting.begin(), sorting.end(), compare);

    QModelIndexList fromList;
    QModelIndexList toList;
    int colCount = columnCount(QModelIndex());
    for (int r = 0; r < sorting.count(); ++r) {
        int oldRow = sorting.at(r).second;
        if (oldRow == r) {
            continue;
        }

        TasksModelItem *item = sorting.at(r).first;
        items->replace(r, item);
        for (int c = 0; c < colCount; ++c) {
            QModelIndex from = createIndex(oldRow, c, item);
            QModelIndex to = createIndex(r, c, item);
            fromList << from;
            toList << to;
        }
    }
    changePersistentIndexList(fromList, toList);
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
}

QMimeData *TasksModel::mimeData(const QModelIndexList &indexes) const
{
    m_dragCutTask = item(indexes[0]);
    return QAbstractItemModel::mimeData(indexes);
}
